# Haven Setup Guide

This guide covers local development, builds, configuration, testing, and code
quality tooling. For the project overview and architecture, see
[`README.md`](README.md).

## Prerequisites

Haven requires:

- Git
- CMake 3.28 or newer
- Ninja
- pkg-config
- curl
- a C++20-compatible compiler
- Docker Engine with Compose or Docker Desktop for container-based development

### macOS

Install Apple's command-line tools and the required build utilities:

```bash
xcode-select --install
brew tap couchbaselabs/homebrew-couchbase
brew install cmake ninja pkgconf couchbase-cxx-client
export CMAKE_PREFIX_PATH="$(brew --prefix couchbase-cxx-client)"
```

LLVM is optional, but provides `clang-format` and `clang-tidy`:

```bash
brew install llvm
```

Verify the environment:

```bash
cmake --version
ninja --version
pkg-config --version
clang++ --version
docker version
```

### Windows

The recommended Windows environment is WSL 2 with Ubuntu. Haven's bootstrap
and development commands are shell scripts, so using WSL keeps the workflow
consistent with Linux and macOS.

Open PowerShell as Administrator and install WSL:

```powershell
wsl --install -d Ubuntu
```

Restart Windows if prompted, open Ubuntu, and install the build tools and
Couchbase C++ SDK:

```bash
sudo apt update
sudo apt install -y \
  build-essential \
  clang \
  cmake \
  curl \
  git \
  gpg \
  ninja-build \
  pkg-config \
  zip \
  unzip \
  tar

DIST_ARCH="$(. /etc/os-release; echo "${VERSION_CODENAME}/$(uname -m)")"
curl -L \
  "https://packages.couchbase.com/clients/cxx/repos/deb/${DIST_ARCH}/DEB-GPG-KEY.txt" |
  sudo gpg --yes --dearmor \
    -o /usr/share/keyrings/couchbase-archive-keyring.gpg
sudo curl -L \
  -o /etc/apt/sources.list.d/couchbase-cxx-client.sources \
  "https://packages.couchbase.com/clients/cxx/repos/deb/${DIST_ARCH}/couchbase-cxx-client.sources"
sudo apt update
sudo apt install -y couchbase-cxx-client couchbase-cxx-client-dev
```

Verify the environment from the WSL terminal:

```bash
cmake --version
ninja --version
pkg-config --version
clang++ --version
git --version
curl --version
```

Ensure the installed CMake version is 3.28 or newer. If the Ubuntu package is
older, install a current CMake release before continuing.

For Docker-based development, install Docker Desktop for Windows, enable its
WSL 2 backend, and turn on integration for the Ubuntu distribution. Then
verify Docker from WSL:

```bash
docker version
```

Clone Haven inside the WSL filesystem (for example, under `~/projects`) rather
than under `/mnt/c` for better build and filesystem performance.

Native Windows builds are not currently part of Haven's supported developer
workflow. The Windows instructions above build and run Haven inside WSL 2.

### Linux

#### Ubuntu and Debian

Install the required compiler, build tools, and Couchbase C++ SDK:

```bash
sudo apt update
sudo apt install -y \
  build-essential \
  clang \
  cmake \
  curl \
  git \
  gpg \
  ninja-build \
  pkg-config \
  zip \
  unzip \
  tar

DIST_ARCH="$(. /etc/os-release; echo "${VERSION_CODENAME}/$(uname -m)")"
curl -L \
  "https://packages.couchbase.com/clients/cxx/repos/deb/${DIST_ARCH}/DEB-GPG-KEY.txt" |
  sudo gpg --yes --dearmor \
    -o /usr/share/keyrings/couchbase-archive-keyring.gpg
sudo curl -L \
  -o /etc/apt/sources.list.d/couchbase-cxx-client.sources \
  "https://packages.couchbase.com/clients/cxx/repos/deb/${DIST_ARCH}/couchbase-cxx-client.sources"
sudo apt update
sudo apt install -y couchbase-cxx-client couchbase-cxx-client-dev
```

#### Fedora

Install the required compiler, build tools, and Couchbase C++ SDK:

```bash
sudo dnf install -y \
  clang \
  cmake \
  curl \
  gcc-c++ \
  git \
  ninja-build \
  pkgconf-pkg-config \
  tar \
  unzip \
  zip

DIST_ARCH="$(rpm -E '%dist/%_arch' | sed 's/^\.//')"
sudo curl -L \
  -o /etc/yum.repos.d/couchbase-cxx-client.repo \
  "https://packages.couchbase.com/clients/cxx/repos/rpm/${DIST_ARCH}/couchbase-cxx-client.repo"
sudo dnf install -y couchbase-cxx-client couchbase-cxx-client-devel
```

Verify the Linux environment:

```bash
cmake --version
ninja --version
pkg-config --version
clang++ --version
git --version
curl --version
```

Haven requires CMake 3.28 or newer. Distribution repositories can provide an
older release, particularly on long-term-support distributions; install a
current CMake release if the version check fails.

For container-based development, install Docker Engine with the Compose
plugin (or Docker Desktop for Linux) using the instructions for your
distribution. Verify both commands:

```bash
docker version
docker compose version
```

Keep the repository in a tooling-safe path without semicolons or unusual
punctuation. CMake treats semicolons as list separators.

## Bootstrap vcpkg

Haven uses a repository-local, pinned vcpkg installation:

```bash
chmod +x scripts/bootstrap-vcpkg.sh
./scripts/bootstrap-vcpkg.sh
```

The script is safe to run more than once and creates local development
artifacts that must not be committed:

```text
.build-tools/
├── vcpkg/
└── vcpkg-downloads/
```

## Native Debug Build

Configure and build Haven:

```bash
cmake --preset dev
cmake --build --preset dev
```

Run all tests:

```bash
ctest --preset dev
```

Display detailed output when a test fails:

```bash
ctest --preset dev --output-on-failure
```

Start Haven:

```bash
./build/dev/apps/server/haven-server
```

The process listens on `0.0.0.0:8080` unless environment variables override
the address or port.

## Sanitizer Build

Configure, build, and test with AddressSanitizer and
UndefinedBehaviorSanitizer:

```bash
cmake --preset dev-asan
cmake --build --preset dev-asan
ctest --preset dev-asan
```

The sanitizer build also treats Haven compiler warnings as errors.

## Release Build

Configure, build, and test the release preset:

```bash
cmake --preset release
cmake --build --preset release
ctest --preset release
```

## Docker Compose

Copy the example environment file, then start and initialize Couchbase Server:

```bash
cp .env.example .env
docker compose up --build --detach
```

The one-shot `couchbase-init` service creates the configured `haven` bucket,
`reservation` scope, `resources` and `reservations` collections, and all
secondary indexes from [`deploy/couchbase/indexes.sql`](deploy/couchbase/indexes.sql).
Check the database and initialization result:

```bash
docker compose ps --all
docker compose logs couchbase-init
```

Load the same environment into the shell, then run the native Haven server:

```bash
set -a
source .env
set +a
./build/dev/apps/server/haven-server
```

Verify liveness and open the Couchbase Web Console:

```bash
curl --fail http://localhost:8080/health/live
open http://localhost:8091
```

Stop Couchbase without deleting its named data volume:

```bash
docker compose down
```

Use `docker compose down --volumes` only when intentionally resetting all
local Couchbase data.

## Runtime Configuration

Haven loads process-level configuration from environment variables through the
bootstrap layer. Application and domain code must not read environment
variables directly.

Copy the example configuration:

```bash
cp .env.example .env
```

### Supported Variables

| Variable | Default | Valid values | Description |
|---|---:|---|---|
| `HVN_HTTP_ADDRESS` | `0.0.0.0` | Non-blank address or hostname | Address on which the HTTP server listens |
| `HVN_HTTP_PORT` | `8080` | Integer from `1` to `65535` | TCP port exposed by the HTTP server |
| `HVN_HTTP_THREADS` | `1` | Positive integer | Number of Drogon event-loop worker threads |
| `HVN_LOG_LEVEL` | `info` | `trace`, `debug`, `info`, `warn`, `warning`, `error`, `critical` | Minimum application log severity |
| `HVN_COUCHBASE_CONNECTION_STRING` | Required | Couchbase connection string | Couchbase cluster endpoint |
| `HVN_COUCHBASE_USERNAME` | Required | Non-empty string | Couchbase user |
| `HVN_COUCHBASE_PASSWORD` | Required | Non-empty secret | Couchbase password |
| `HVN_COUCHBASE_BUCKET` | Required | Non-empty bucket name | Bucket containing Haven data |
| `HVN_COUCHBASE_SCOPE` | Required | Non-empty scope name | Scope containing Haven collections |

Log-level values are case-insensitive, and `warning` is accepted as an alias
for `warn`.

### Default Configuration

```dotenv
HVN_HTTP_ADDRESS=0.0.0.0
HVN_HTTP_PORT=8080
HVN_HTTP_THREADS=1
HVN_LOG_LEVEL=info
HVN_COUCHBASE_CONNECTION_STRING=couchbase://127.0.0.1
HVN_COUCHBASE_USERNAME=Administrator
HVN_COUCHBASE_PASSWORD=password
HVN_COUCHBASE_BUCKET=haven
HVN_COUCHBASE_SCOPE=reservation
```

One HTTP worker thread is used as a predictable local-development default.
Production thread-count selection will later be reviewed against container CPU
limits and workload characteristics. Couchbase has no production defaults:
all five variables are required by the native process. The values above are
local-only placeholders matching Docker Compose.

### Native Environment Loading

The native Haven executable does not automatically parse `.env` files. Load
the file into the current shell:

```bash
set -a
source .env
set +a
```

Then start Haven:

```bash
./build/dev/apps/server/haven-server
```

You may also configure individual values directly:

```bash
HVN_HTTP_ADDRESS=127.0.0.1 \
HVN_HTTP_PORT=9090 \
HVN_HTTP_THREADS=4 \
HVN_LOG_LEVEL=debug \
./build/dev/apps/server/haven-server
```

Docker Compose automatically reads the root `.env` file. The Docker Compose
workflow is described in the preceding section. Never commit secrets;
production deployments should use a dedicated secret-management system.

### Configuration Ownership

The bootstrap layer translates external string values into typed
configuration:

```text
Environment variables
        ↓
Bootstrap validation
        ↓
ApplicationConfiguration
        ↓
Process composition
```

The resulting configuration includes:

```text
ApplicationConfiguration
├── HttpConfiguration
│   ├── address
│   ├── port
│   └── worker_threads
├── LoggingConfiguration
│   └── level
└── CouchbaseConfiguration
    ├── connection_string
    ├── username
    ├── password
    ├── bucket_name
    └── scope_name
```

The Haven-owned `LogLevel` type remains independent of Drogon. The server
composition root maps it to Haven's logging system before constructing
Couchbase infrastructure.

## Docker

On macOS, start Docker Desktop and verify that its daemon is available:

```bash
open -a Docker
docker info
```

Use Docker Compose to run the local Couchbase Server:

```bash
docker compose up --build --detach
docker compose ps --all
docker compose logs couchbase-init
```

Run Haven natively as described above, verify it, then stop Couchbase when
finished:

```bash
curl --fail http://localhost:8080/health/live
docker compose down
```

The current Compose topology contains Couchbase Server and its one-shot
initializer. Redis and Kafka are not part of the implemented runtime.

## Resource Detail Endpoint

After Couchbase initialization has seeded or an administrator has inserted a
Resource document, start Haven and request that Resource within its owning
organization:

```bash
curl \
  --request GET \
  http://localhost:8080/api/v1/organizations/<organizationId>/resources/<resourceId>
```

A matching tenant-scoped Resource returns `200 OK` and its current public
metadata. A missing Resource or the same Resource ID under a different
organization returns `404 Not Found`. Malformed path identifiers return
`400 Bad Request`.

This endpoint reads directly through `GetResourceHandler` and
`CouchbaseResourceRepository`. Redis caching is not implemented and remains
planned for Phase 10.2.

## Health Endpoint

Check process liveness:

```bash
curl --fail http://localhost:8080/health/live
```

Expected response:

```json
{
  "service": "haven-api",
  "status": "alive"
}
```

Liveness represents only the health of the Haven process. It does not depend
on Couchbase, Redis, Kafka, or external network services. Dependency health
will be exposed separately through the readiness endpoint.

## Testing

Run all tests that do not require Couchbase:

```bash
ctest --preset dev --exclude-regex 'Couchbase.*IntegrationTest'
```

Run Couchbase integration tests after Compose initialization and environment
loading:

```bash
set -a
source .env
set +a
ctest --preset dev --label-regex couchbase --output-on-failure
```

The integration suite uses unique tenant/entity identifiers, cleans up its
documents, and covers resource and reservation round trips, wrong-tenant
reads, same-ID tenant isolation, and half-open overlap boundaries.

Run either integration executable directly:

```bash
./build/dev/tests/integration/infrastructure/haven_resource_repository_integration_test
./build/dev/tests/integration/infrastructure/haven_reservation_repository_integration_test
```

Run tests matching a CTest name:

```bash
ctest --preset dev -R LiveResponse
```

Unit tests intentionally avoid external services. Couchbase tests under
`tests/integration` require the configured live Docker service and skip only
when the five Couchbase environment variables are absent.

## Formatting

Format one file:

```bash
clang-format -i apps/server/main.cpp
```

Format all C++ files:

```bash
find apps include src tests \
  \( -name '*.cpp' -o -name '*.hpp' \) \
  -print0 |
xargs -0 clang-format -i
```

Check formatting without changing files:

```bash
find apps include src tests \
  \( -name '*.cpp' -o -name '*.hpp' \) \
  -print0 |
xargs -0 clang-format --dry-run --Werror
```

When LLVM is installed through Homebrew, `clang-format` may be located at
`/opt/homebrew/opt/llvm/bin/clang-format`.

## Static Analysis

A successful development configuration generates
`build/dev/compile_commands.json`.

Analyze one file:

```bash
clang-tidy apps/server/main.cpp -p build/dev
```

Analyze all Haven source and test files:

```bash
find apps src tests \
  -name '*.cpp' \
  -print0 |
xargs -0 -n1 clang-tidy -p build/dev
```

When LLVM is installed through Homebrew, `clang-tidy` may be located at
`/opt/homebrew/opt/llvm/bin/clang-tidy`.

## Troubleshooting

### CMake cannot find Ninja

If CMake reports `CMAKE_MAKE_PROGRAM is not set`, install and verify Ninja:

```bash
brew install ninja
ninja --version
```

For Apple Silicon Homebrew:

```bash
eval "$(/opt/homebrew/bin/brew shellenv)"
```

### vcpkg cannot find pkg-config

Install and verify `pkg-config`:

```bash
brew install pkgconf
pkg-config --version
```

### vcpkg reports an HTTP/2 framing error

An error such as `curl operation failed with error code 16` is a network
transport failure while downloading dependency source archives. Verify the
download cache exists:

```bash
mkdir -p .build-tools/vcpkg-downloads
```

Check for unexpected proxy variables:

```bash
env | grep -i proxy
```

When no proxy is intentionally configured:

```bash
unset HTTP_PROXY HTTPS_PROXY ALL_PROXY
unset http_proxy https_proxy all_proxy
```

### Docker cannot connect to the daemon

If Docker reports `failed to connect to the docker API`, start Docker Desktop
and verify the daemon:

```bash
open -a Docker
docker info
```

### Build directory contains stale paths

CMake caches absolute paths. After renaming or moving the repository, recreate
the build directory:

```bash
rm -rf build
cmake --preset dev
```
# Optional Redis Resource detail cache

Run `docker compose up --detach redis` and set `HVN_REDIS_ENABLED=true` to enable the optional
Resource detail cache. Redis failure never prevents startup or authoritative Couchbase reads.
Run its live tests with `ctest --preset dev --label-regex redis --output-on-failure`. Set
`HVN_REDIS_ENABLED=false` to bypass Redis completely.
