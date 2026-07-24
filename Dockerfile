# syntax=docker/dockerfile:1.7

# -----------------------------------------------------------------------------
# Haven Container Build
#
# Builds and tests Haven in a reproducible Ubuntu environment, then copies only
# the installed application and required runtime libraries into the final image.
#
# Couchbase, Redis, and Kafka remain outside this image and will be introduced
# as separate Docker Compose services.
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

# vcpkg requires its configured download-cache path to exist before bootstrap.
RUN mkdir -p \
        /workspace/.build-tools \
        "${VCPKG_DOWNLOADS}"

# Clone the project-pinned vcpkg release.
#
# HTTP/1.1 is used to reduce GitHub HTTP/2 framing failures observed on some
# development and corporate networks.
RUN git \
        -c http.version=HTTP/1.1 \
        clone \
        --branch "${VCPKG_VERSION}" \
        --depth 1 \
        --single-branch \
        https://github.com/microsoft/vcpkg.git \
        "${VCPKG_ROOT}" \
    && "${VCPKG_ROOT}/bootstrap-vcpkg.sh" -disableMetrics

# Local build output and the host vcpkg installation are excluded through
# .dockerignore, so only repository-owned source files enter the image.
COPY . .

# Configure, compile, test, and install Haven.
RUN cmake --preset release \
    && cmake --build --preset release \
    && ctest --preset release \
    && cmake --install build/release --prefix /opt/haven

# Collect dynamically linked libraries produced by vcpkg.
#
# This directory may remain empty when dependencies are linked statically.
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

# Default Haven runtime configuration.
#
# Docker Compose or the deployment environment may override these values.
ENV HVN_HTTP_ADDRESS=0.0.0.0
ENV HVN_HTTP_PORT=8080
ENV HVN_HTTP_THREADS=1
ENV HVN_LOG_LEVEL=info

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

COPY --from=builder /opt/haven/bin/haven-server /usr/local/bin/haven-server
COPY --from=builder /opt/haven/lib/ /usr/local/lib/

RUN ldconfig

USER haven:haven

EXPOSE 8080

STOPSIGNAL SIGTERM

ENTRYPOINT ["/usr/local/bin/haven-server"]