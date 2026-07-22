# syntax=docker/dockerfile:1.7

# -----------------------------------------------------------------------------
# Haven Container Build
#
# Builds and tests Haven in a reproducible Ubuntu environment, then copies only
# the installed application and its required libraries into a smaller runtime
# image.
#
# The image intentionally does not include Couchbase, Redis, or Kafka. Those
# services will be introduced through Docker Compose in later milestones.
# -----------------------------------------------------------------------------

# =============================================================================
# Builder stage
# =============================================================================

FROM ubuntu:24.04 AS builder

ARG DEBIAN_FRONTEND=noninteractive
ARG VCPKG_VERSION=2026.06.24

ENV VCPKG_ROOT=/workspace/.build-tools/vcpkg
ENV VCPKG_DOWNLOADS=/workspace/.build-tools/vcpkg-downloads
ENV VCPKG_DISABLE_METRICS=1

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

# Install the compiler, build system, and tools required by vcpkg ports.
RUN apt-get update \
    && apt-get install --yes --no-install-recommends \
        build-essential \
        ca-certificates \
        cmake \
        curl \
        git \
        ninja-build \
        perl \
        pkg-config \
        tar \
        unzip \
        zip \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

# Create the download directory before bootstrapping vcpkg.
#
# vcpkg requires VCPKG_DOWNLOADS to reference an existing directory.
RUN mkdir -p \
        /workspace/.build-tools \
        "${VCPKG_DOWNLOADS}"

# Use HTTP/1.1 because some networks exhibit intermittent HTTP/2 framing
# failures while downloading from GitHub.
RUN git \
        -c http.version=HTTP/1.1 \
        clone \
        --branch "${VCPKG_VERSION}" \
        --depth 1 \
        --single-branch \
        https://github.com/microsoft/vcpkg.git \
        "${VCPKG_ROOT}" \
    && "${VCPKG_ROOT}/bootstrap-vcpkg.sh" -disableMetrics

# .dockerignore prevents local build output and the host vcpkg installation
# from entering the Linux build context.
COPY . .

# Configure, compile, test, and install Haven.
RUN cmake --preset release \
    && cmake --build --preset release \
    && ctest --preset release \
    && cmake --install build/release --prefix /opt/haven

# Collect any dynamically linked libraries produced by vcpkg.
#
# Default Linux vcpkg builds are commonly static, so this directory may remain
# empty. Keeping this step makes the runtime stage resilient if a dependency is
# later built dynamically.
RUN mkdir -p /opt/haven/lib \
    && find build/release/vcpkg_installed \
        -type f \
        \( -name '*.so' -o -name '*.so.*' \) \
        -exec cp -L '{}' /opt/haven/lib/ ';'


# =============================================================================
# Runtime stage
# =============================================================================

FROM ubuntu:24.04 AS runtime

ARG DEBIAN_FRONTEND=noninteractive

ENV HAVEN_HTTP_ADDRESS=0.0.0.0
ENV HAVEN_HTTP_PORT=8080

# Install only the operating-system libraries required to run and inspect the
# service. curl will also be used by the Docker Compose health check.
RUN apt-get update \
    && apt-get install --yes --no-install-recommends \
        ca-certificates \
        curl \
        libstdc++6 \
    && rm -rf /var/lib/apt/lists/* \
    && groupadd \
        --system \
        --gid 10001 \
        haven \
    && useradd \
        --system \
        --uid 10001 \
        --gid haven \
        --home-dir /nonexistent \
        --shell /usr/sbin/nologin \
        haven

COPY --from=builder \
    /opt/haven/bin/haven-server \
    /usr/local/bin/haven-server

COPY --from=builder \
    /opt/haven/lib/ \
    /usr/local/lib/

RUN ldconfig

USER haven:haven

EXPOSE 8080

STOPSIGNAL SIGTERM

ENTRYPOINT ["/usr/local/bin/haven-server"]