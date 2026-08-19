# Haven Setup Guide

This guide covers local development: prerequisites, builds, configuration, and testing. For the
project overview and architecture, see [`README.md`](README.md). For the browser client, see
**[Haven UI](../haven_ui)**.

## Prerequisites

Git, CMake ≥ 3.28, Ninja, pkg-config, curl, a C++20 compiler, and Docker Engine/Desktop with
Compose.

**macOS**

```bash
xcode-select --install
brew tap couchbaselabs/homebrew-couchbase
brew install cmake ninja pkgconf couchbase-cxx-client llvm   # llvm is optional (clang-format/tidy)
export CMAKE_PREFIX_PATH="$(brew --prefix couchbase-cxx-client)"
```

**Ubuntu / Debian**

```bash
sudo apt update && sudo apt install -y build-essential clang cmake curl git gpg \
  ninja-build pkg-config zip unzip tar

DIST_ARCH="$(. /etc/os-release; echo "${VERSION_CODENAME}/$(uname -m)")"
curl -L "https://packages.couchbase.com/clients/cxx/repos/deb/${DIST_ARCH}/DEB-GPG-KEY.txt" |
  sudo gpg --yes --dearmor -o /usr/share/keyrings/couchbase-archive-keyring.gpg
sudo curl -L -o /etc/apt/sources.list.d/couchbase-cxx-client.sources \
  "https://packages.couchbase.com/clients/cxx/repos/deb/${DIST_ARCH}/couchbase-cxx-client.sources"
sudo apt update && sudo apt install -y couchbase-cxx-client couchbase-cxx-client-dev
```

**Fedora**

```bash
sudo dnf install -y clang cmake curl gcc-c++ git ninja-build pkgconf-pkg-config tar unzip zip
DIST_ARCH="$(rpm -E '%dist/%_arch' | sed 's/^\.//')"
sudo curl -L -o /etc/yum.repos.d/couchbase-cxx-client.repo \
  "https://packages.couchbase.com/clients/cxx/repos/rpm/${DIST_ARCH}/couchbase-cxx-client.repo"
sudo dnf install -y couchbase-cxx-client couchbase-cxx-client-devel
```

**Windows** — use WSL 2 (Ubuntu) and follow the Ubuntu instructions above inside it; Haven's
scripts assume a POSIX shell. Clone the repo inside the WSL filesystem (e.g. `~/projects`), not
under `/mnt/c`, for build performance. Install Docker Desktop with the WSL 2 backend and enable
integration for your distro.

Verify the toolchain:

```bash
cmake --version && ninja --version && pkg-config --version && clang++ --version && docker version
```

Keep the repo path free of semicolons — CMake treats them as list separators.

## Bootstrap and Build

Haven pins a repository-local vcpkg install:

```bash
./scripts/bootstrap-vcpkg.sh   # safe to re-run
```

Configure, build, and test (native debug):

```bash
cmake --preset dev
cmake --build --preset dev
ctest --preset dev --output-on-failure
```

Other presets:

```bash
cmake --preset dev-asan && cmake --build --preset dev-asan && ctest --preset dev-asan   # ASan + UBSan, warnings as errors
cmake --preset release && cmake --build --preset release && ctest --preset release      # release
```

Run the server:

```bash
./build/dev/apps/server/haven-server   # listens on 0.0.0.0:8080 by default
```

## Docker Compose (Couchbase)

```bash
cp .env.example .env
docker compose up --build --detach
docker compose ps --all
docker compose logs couchbase-init
```

The one-shot `couchbase-init` service creates the `haven` bucket, `reservation` scope, the
`resources`/`reservations`/`idempotency` collections, all indexes in
[`deploy/couchbase/indexes.sql`](deploy/couchbase/indexes.sql), and seed resource data
(`HVN_SEED_ORGANIZATION_ID` overrides the default seed organization).

Load the environment and run natively:

```bash
set -a; source .env; set +a
./build/dev/apps/server/haven-server
```

Verify and browse:

```bash
curl --fail http://localhost:8080/health/live
open http://localhost:8091   # Couchbase Web Console — bucket haven, scope reservation
```

Optional Redis-backed resource cache: `docker compose up --detach redis` and set
`HVN_REDIS_ENABLED=true`. Redis failures never block startup or authoritative Couchbase reads.

Stop Couchbase (keeps data volume): `docker compose down`. Use `--volumes` only to reset local
data intentionally.

## Runtime Configuration

Haven reads process configuration from environment variables via the bootstrap layer; application
and domain code never read the environment directly.

```bash
cp .env.example .env
```

| Variable | Default | Description |
|---|---:|---|
| `HVN_HTTP_ADDRESS` | `0.0.0.0` | HTTP bind address |
| `HVN_HTTP_PORT` | `8080` | HTTP port |
| `HVN_HTTP_THREADS` | `1` | Drogon worker threads |
| `HVN_LOG_LEVEL` | `info` | `trace`\|`debug`\|`info`\|`warn`\|`error`\|`critical` |
| `HVN_COUCHBASE_CONNECTION_STRING` | — | Required: Couchbase cluster endpoint |
| `HVN_COUCHBASE_USERNAME` | — | Required |
| `HVN_COUCHBASE_PASSWORD` | — | Required |
| `HVN_COUCHBASE_BUCKET` | — | Required |
| `HVN_COUCHBASE_SCOPE` | — | Required |
| `HVN_IDEMPOTENCY_RETENTION_SECONDS` | `86400` | Idempotency record retention |
| `HVN_REDIS_ENABLED` | `false` | Enables the optional resource detail cache |

Couchbase variables have no production defaults — all five are required by the running process.
Never commit secrets; use a dedicated secret manager in production.

Load `.env` into a native shell session (Docker Compose reads it automatically):

```bash
set -a; source .env; set +a
```

## Testing

```bash
ctest --preset dev --exclude-regex 'Couchbase.*IntegrationTest'   # no external services

set -a; source .env; set +a
ctest --preset dev --label-regex couchbase --output-on-failure    # after `docker compose up`
ctest --preset dev --label-regex redis --output-on-failure        # requires redis service

ctest --preset dev -R <TestName>                                  # run a specific test
```

Unit tests avoid external services entirely; integration tests require the corresponding Docker
service and skip automatically when its environment variables are absent.

## Formatting and Static Analysis

```bash
find apps include src tests \( -name '*.cpp' -o -name '*.hpp' \) -print0 | xargs -0 clang-format -i
find apps include src tests \( -name '*.cpp' -o -name '*.hpp' \) -print0 | xargs -0 clang-format --dry-run --Werror

find apps src tests -name '*.cpp' -print0 | xargs -0 -n1 clang-tidy -p build/dev
```

On Homebrew LLVM installs, tools live under `$(brew --prefix llvm)/bin`.

## Troubleshooting

| Symptom | Fix |
|---|---|
| `CMAKE_MAKE_PROGRAM is not set` | `brew install ninja` (Apple Silicon: `eval "$(/opt/homebrew/bin/brew shellenv)"`) |
| vcpkg cannot find pkg-config | `brew install pkgconf` |
| vcpkg HTTP/2 framing error | Presets already force HTTP/1.1 for asset downloads; check `mkdir -p .build-tools/vcpkg-downloads` and unset stray proxy variables (`env \| grep -i proxy`) |
| Docker daemon unreachable | `open -a Docker && docker info` |
| Stale CMake paths after moving the repo | `rm -rf build && cmake --preset dev` |
