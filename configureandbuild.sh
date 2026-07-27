#!/bin/bash
set -euo pipefail

DOCKER_IMAGE="rafradek/ubuntu2004dev:latest"

REPO_DIR="${REPO_DIR:-$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)}"
BUILD_ROOT="${BUILD_ROOT:-$(cd "$REPO_DIR/.." && pwd)}"

GAMESERVER_DIR="${GAMESERVER_DIR:-${2:-"/var/tf2server/tf"}}"
MAX_AMBUILD_JOBS="${MAX_AMBUILD_JOBS:-${3:-$(nproc 2>/dev/null || echo 4)}}"

MODE="${MODE:-full}"
FULL_REBUILD="${FULL_REBUILD:-n}"

if [ "$#" -gt 0 ]; then
    case "$1" in
        release)
            MODE="release"
            ;;
        full_rebuild)
            FULL_REBUILD="y"
            ;;
    esac
fi

in_container()
{
    [ -f /.dockerenv ] || [ -n "${SIGMOD_DOCKER_BUILD:-}" ]
}

log()
{
    echo "[configureandbuild] $*"
}

dos2unix_files()
{
    if ! command -v dos2unix >/dev/null 2>&1; then
        log "dos2unix not found, falling back to sed"
        local f
        for f in "$@"; do
            [ -f "$f" ] || continue
            sed -i 's/\r$//' "$f"
        done
        return
    fi

    local f
    for f in "$@"; do
        [ -f "$f" ] || continue
        dos2unix "$f"
    done
}

normalize_repo_scripts()
{
    log "Normalizing line endings in build scripts"
    local -a files=()
    local f

    for f in "${REPO_DIR}"/*.sh "${REPO_DIR}/configure.py"; do
        [ -f "$f" ] && files+=("$f")
    done

    if [ -d "${REPO_DIR}/libs/udis86" ]; then
        while IFS= read -r -d '' f; do
            files+=("$f")
        done < <(find "${REPO_DIR}/libs/udis86" -type f \( -name '*.sh' -o -name '*.ac' -o -name '*.am' \) \
            ! -path '*/.git/*' -print0)
    fi

    if [ "${#files[@]}" -gt 0 ]; then
        dos2unix_files "${files[@]}"
    fi
}

run_on_host()
{
    if ! command -v docker >/dev/null 2>&1; then
        echo "docker is required but not installed." >&2
        exit 1
    fi

    if ! docker info >/dev/null 2>&1; then
        echo "docker daemon is not running or not accessible." >&2
        exit 1
    fi

    log "Pulling ${DOCKER_IMAGE}..."
    docker pull "$DOCKER_IMAGE"

    log "Starting build container (mode=${MODE}, full_rebuild=${FULL_REBUILD})..."
    dos2unix_files "${REPO_DIR}/configureandbuild.sh"

    local docker_rc=0
    docker run --rm \
        --entrypoint bash \
        -v "${BUILD_ROOT}:${BUILD_ROOT}" \
        -w "${REPO_DIR}" \
        -e SIGMOD_DOCKER_BUILD=1 \
        -e MODE="${MODE}" \
        -e FULL_REBUILD="${FULL_REBUILD}" \
        -e GAMESERVER_DIR="${GAMESERVER_DIR}" \
        -e MAX_AMBUILD_JOBS="${MAX_AMBUILD_JOBS}" \
        -e REPO_DIR="${REPO_DIR}" \
        -e BUILD_ROOT="${BUILD_ROOT}" \
        "$DOCKER_IMAGE" \
        -c "command -v dos2unix >/dev/null && dos2unix '${REPO_DIR}/configureandbuild.sh' || sed -i 's/\\r\$//' '${REPO_DIR}/configureandbuild.sh'; SIGMOD_DOCKER_BUILD=1 bash '${REPO_DIR}/configureandbuild.sh'" \
        || docker_rc=$?

    if [ "$docker_rc" -ne 0 ]; then
        echo "Docker build failed (exit ${docker_rc})." >&2
        exit "$docker_rc"
    fi

    log "Build finished successfully."
    deploy_to_gameserver
}

clone_alliedmodders()
{
    mkdir -p "${BUILD_ROOT}/alliedmodders"
    cd "${BUILD_ROOT}/alliedmodders"

    if [ ! -d ambuild/.git ]; then
        git clone https://github.com/alliedmodders/ambuild --depth 1
    fi
    if [ ! -d sourcemod/.git ]; then
        git clone --recursive https://github.com/alliedmodders/sourcemod --depth 1 -b 1.11-dev
    fi
    if [ ! -d hl2sdk-sdk2013/.git ]; then
        git clone https://github.com/alliedmodders/hl2sdk --depth 1 -b sdk2013 hl2sdk-sdk2013
    fi
    if [ ! -d hl2sdk-tf2/.git ]; then
        git clone https://github.com/alliedmodders/hl2sdk --depth 1 -b tf2 hl2sdk-tf2
    fi
    if [ ! -d hl2sdk-css/.git ]; then
        git clone https://github.com/alliedmodders/hl2sdk --depth 1 -b css hl2sdk-css
    fi
    if [ ! -d metamod-source/.git ]; then
        git clone https://github.com/alliedmodders/metamod-source --depth 1 -b 1.11-dev
    fi
}

install_ambuild()
{
    cd "${BUILD_ROOT}/alliedmodders/ambuild"
    pip install .
    export PATH="${HOME}/.local/bin:${PATH}"
}

build_udis86()
{
    local udis="${REPO_DIR}/libs/udis86"
    local python="${PYTHON:-python3}"

    cd "$udis"
    ./autogen.sh

    if [ -f Makefile ]; then
        make distclean >/dev/null 2>&1 || make clean >/dev/null 2>&1 || true
    fi
    rm -rf libudis86/.libs libudis86/.deps libudis86/*.o

    "$python" scripts/ud_itab.py docs/x86/optable.xml libudis86

    ./configure --disable-shared --enable-static --with-python="$python"

    log "Building udis86 (x86)"
    make -C libudis86 CFLAGS="-m32" LDFLAGS="-m32"
    mv libudis86/.libs/libudis86.a ../libudis86.a

    log "Building udis86 (x64)"
    make -C libudis86 clean
    rm -rf libudis86/.libs libudis86/.deps libudis86/*.o
    make -C libudis86 CFLAGS="-fPIC" LDFLAGS=""
    mv libudis86/.libs/libudis86.a ../libudis86x64.a
}

build_lua()
{
    cd "${REPO_DIR}/libs"
    if [ ! -d lua ]; then
        wget -q https://www.lua.org/ftp/lua-5.4.4.tar.gz
        tar -xf lua-*.tar.gz
        rm lua-*.tar.gz
        mv lua-* lua
    fi
    cd lua

	log "Building Lua (x86)"
    make clean
    make CC=g++ MYCFLAGS='-m32' MYLDFLAGS='-m32'
    mv src/liblua.a ../liblua.a

	log "Building Lua (x64)"
    make clean
    make CC=g++ MYCFLAGS='-fPIC'
    mv src/liblua.a ../libluax64.a
}

deploy_to_gameserver()
{
    log "Deploying package to ${GAMESERVER_DIR}"
    cp -rf "${REPO_DIR}/build/release/package/"* "${GAMESERVER_DIR}/"
}

run_in_container()
{
    export PATH="${HOME}/.local/bin:${PATH}"
    git config --global --add safe.directory '*'

    normalize_repo_scripts

    if [ "$FULL_REBUILD" = "y" ]; then
        log "Full rebuild: clearing alliedmodders and native libs"
        rm -rf "${BUILD_ROOT}/alliedmodders"
        rm -rf "${REPO_DIR}/libs/lua"
        rm -rf "${REPO_DIR}/libs/udis86/libudis86/.libs"
        rm -f "${REPO_DIR}/libs/libudis86.a" "${REPO_DIR}/libs/libudis86x64.a"
        rm -f "${REPO_DIR}/libs/liblua.a" "${REPO_DIR}/libs/libluax64.a"
    fi

    if [ "$MODE" = "release" ]; then
        log "Release build"
        install_ambuild
        cd "${REPO_DIR}"
        ./autoconfig.sh
        ./multibuild.sh
        return
    fi

    log "Setting up alliedmodders dependencies"
    clone_alliedmodders
    install_ambuild

    cd "${REPO_DIR}"
    git submodule init
    git submodule update --depth 1

    log "Building udis86"
    build_udis86

    log "Building Lua"
    build_lua

    log "Configuring and building"
    cd "${REPO_DIR}"
    ./autoconfig.sh
    ./multibuild.sh
}

if in_container; then
    run_in_container
else
    run_on_host
fi
