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
brew install cmake ninja pkgconf
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

Restart Windows if prompted, open Ubuntu, and install the build tools:

```bash
sudo apt update
sudo apt install -y \
  build-essential \
  clang \
  cmake \
  curl \
  git \
  ninja-build \
  pkg-config \
  zip \
  unzip \
  tar
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

### Linux

#### Ubuntu and Debian

Install the required compiler and build tools:

```bash
sudo apt update
sudo apt install -y \
  build-essential \
  clang \
  cmake \
  curl \
  git \
  ninja-build \
  pkg-config \
  zip \
  unzip \
  tar
```

#### Fedora

Install the required compiler and build tools:

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

Copy the example environment file, then build and start the environment:

```bash
cp .env.example .env
docker compose up --build -d
```

Check the service and follow its logs:

```bash
curl --fail http://localhost:8080/health/live
docker compose logs -f haven-api
```

Stop the environment:

```bash
docker compose down
```

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

Log-level values are case-insensitive, and `warning` is accepted as an alias
for `warn`.

### Default Configuration

```dotenv
HVN_HTTP_ADDRESS=0.0.0.0
HVN_HTTP_PORT=8080
HVN_HTTP_THREADS=1
HVN_LOG_LEVEL=info
```

One HTTP worker thread is used as a predictable local-development default.
Production thread-count selection will later be reviewed against container CPU
limits and workload characteristics.

### Validation Behavior

Configuration is parsed and validated before the HTTP server starts. Invalid
configuration causes Haven to terminate with a non-zero exit status.

Examples of invalid values:

```dotenv
HVN_HTTP_ADDRESS="   "
HVN_HTTP_PORT=0
HVN_HTTP_PORT=65536
HVN_HTTP_PORT=8080abc
HVN_HTTP_THREADS=0
HVN_HTTP_THREADS=-1
HVN_HTTP_THREADS=4abc
HVN_LOG_LEVEL=verbose
```

Example configuration error:

```text
Haven configuration error: HVN_HTTP_PORT must be an integer between 1 and 65535
```

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
└── LoggingConfiguration
    └── level
```

The Haven-owned `LogLevel` type remains independent of Drogon or any future
logging implementation. The observability/logger module will later map this
configuration to Haven's logging system.

## Docker

On macOS, start Docker Desktop and verify that its daemon is available:

```bash
open -a Docker
docker info
```

Build the image:

```bash
docker buildx build \
  --load \
  --tag haven-api:foundation \
  .
```

Run the image directly:

```bash
docker run \
  --rm \
  --name haven-api \
  --publish 8080:8080 \
  haven-api:foundation
```

Or use Docker Compose:

```bash
docker compose up --build --detach
docker compose ps
docker compose logs --follow haven-api
```

Verify the running service, then stop it when finished:

```bash
curl --fail http://localhost:8080/health/live
docker compose down
```

The current Compose topology contains only `haven-api`. Couchbase, Redis, and
Kafka will be added when their infrastructure adapters exist.

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

Run all unit tests through CTest:

```bash
ctest --preset dev
```

Run the GoogleTest executable directly:

```bash
./build/dev/tests/unit/haven_unit_tests
```

Run a GoogleTest subset:

```bash
./build/dev/tests/unit/haven_unit_tests \
  --gtest_filter="EnvironmentConfigurationTest.*"
```

Run tests matching a CTest name:

```bash
ctest --preset dev -R LiveResponse
```

Current unit tests intentionally avoid external services, Docker dependencies,
network ports, Couchbase, Redis, and Kafka. Infrastructure integration tests
will be introduced separately.

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
