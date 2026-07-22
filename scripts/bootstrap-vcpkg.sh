#!/usr/bin/env bash

#------------------------------------------------------------------------------
# Haven vcpkg Bootstrap
#
# Downloads and bootstraps the project-pinned vcpkg release under:
#
#     .build-tools/vcpkg/
#
# The script keeps vcpkg repository-local without using a Git submodule or
# depending on a developer's globally installed vcpkg version.
#
# Usage:
#
#     ./scripts/bootstrap-vcpkg.sh
#------------------------------------------------------------------------------

set -euo pipefail

readonly VCPKG_VERSION="2026.06.24"
readonly VCPKG_REPOSITORY="https://github.com/microsoft/vcpkg.git"

readonly SCRIPT_DIRECTORY="$(
    cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1
    pwd
)"

readonly PROJECT_ROOT="$(
    cd -- "${SCRIPT_DIRECTORY}/.." >/dev/null 2>&1
    pwd
)"

readonly BUILD_TOOLS_DIRECTORY="${PROJECT_ROOT}/.build-tools"
readonly VCPKG_ROOT="${BUILD_TOOLS_DIRECTORY}/vcpkg"
readonly VERSION_FILE="${VCPKG_ROOT}/.haven-vcpkg-version"

log() {
    printf '[haven-bootstrap] %s\n' "$1"
}

fail() {
    printf '[haven-bootstrap] error: %s\n' "$1" >&2
    exit 1
}

command_exists() {
    command -v "$1" >/dev/null 2>&1
}

validate_prerequisites() {
    if ! command_exists git; then
        fail "Git is required but was not found in PATH."
    fi

    if ! command_exists cmake; then
        fail "CMake is required but was not found in PATH.

On macOS, install it with:

    brew install cmake

Then verify:

    cmake --version"
    fi

    if ! command_exists ninja; then
        fail "Ninja is required but was not found in PATH.

On macOS, install it with:

    brew install ninja

Then verify:

    ninja --version"
    fi

    if ! command_exists curl; then
        fail "curl is required but was not found in PATH."
    fi
}

installed_version() {
    if [[ -f "${VERSION_FILE}" ]]; then
        cat "${VERSION_FILE}"
    fi
}

remove_incomplete_installation() {
    if [[ -d "${VCPKG_ROOT}" && ! -x "${VCPKG_ROOT}/vcpkg" ]]; then
        log "Removing incomplete vcpkg installation."
        rm -rf "${VCPKG_ROOT}"
    fi
}

clone_vcpkg() {
    log "Cloning vcpkg ${VCPKG_VERSION}."

    mkdir -p "${BUILD_TOOLS_DIRECTORY}"

    # HTTP/1.1 avoids intermittent Git HTTP/2 framing failures seen on some
    # networks and macOS Git installations.
    #
    # --depth 1 and --single-branch limit the clone to the selected release.
    # --filter=blob:none reduces the initial amount of downloaded data.
    git \
        -c http.version=HTTP/1.1 \
        clone \
        --branch "${VCPKG_VERSION}" \
        --depth 1 \
        --single-branch \
        --filter=blob:none \
        "${VCPKG_REPOSITORY}" \
        "${VCPKG_ROOT}"
}

bootstrap_vcpkg() {
    log "Bootstrapping vcpkg."

    "${VCPKG_ROOT}/bootstrap-vcpkg.sh" -disableMetrics
}

record_version() {
    printf '%s\n' "${VCPKG_VERSION}" >"${VERSION_FILE}"
}

verify_installation() {
    if [[ ! -x "${VCPKG_ROOT}/vcpkg" ]]; then
        fail "vcpkg bootstrap completed without producing an executable."
    fi

    if [[ ! -f "${VERSION_FILE}" ]]; then
        fail "vcpkg version marker was not created."
    fi
}

main() {
    validate_prerequisites
    remove_incomplete_installation

    local current_version
    current_version="$(installed_version || true)"

    if [[ -x "${VCPKG_ROOT}/vcpkg" &&
          "${current_version}" == "${VCPKG_VERSION}" ]]; then
        log "vcpkg ${VCPKG_VERSION} is already installed."
        log "Location: ${VCPKG_ROOT}"
        exit 0
    fi

    if [[ -d "${VCPKG_ROOT}" ]]; then
        log "Removing an outdated or unrecognized vcpkg installation."
        rm -rf "${VCPKG_ROOT}"
    fi

    clone_vcpkg
    bootstrap_vcpkg
    record_version
    verify_installation

    log "vcpkg ${VCPKG_VERSION} installed successfully."
    log "Location: ${VCPKG_ROOT}"
    log "You can now run: cmake --preset dev"
}

main "$@"