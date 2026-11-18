#!/bin/sh
# Builds the player inside the tools/docker image.
#
#   tools/docker/build.sh [linux|windows|all|release VERSION|shell]
#
# linux    -> build-docker-linux/dk94
# windows  -> build-docker-win/dk94.exe (+ libtcc.dll, SDL2.dll)
# all      -> both (default)
# release  -> tools/make_release.sh VERSION, packages in dist/
# shell    -> interactive shell in the build container
set -eu

ROOT=$(cd "$(dirname "$0")/../.." && pwd)
IMAGE=dk94ap-build
TARGET=${1:-all}

docker build -q -t "$IMAGE" "$ROOT/tools/docker" >/dev/null

# Build as the host user on Linux so the output isn't owned by root.
USER_ARGS=""
if [ "$(uname -s)" = Linux ]; then
    USER_ARGS="--user $(id -u):$(id -g)"
fi
TTY_ARGS=""
[ -t 0 ] && TTY_ARGS="-it"

run() {
    # shellcheck disable=SC2086
    docker run --rm $TTY_ARGS $USER_ARGS -e HOME=/tmp -v "$ROOT:/src" "$IMAGE" sh -c "$1"
}

LINUX='cmake -S . -B build-docker-linux -G Ninja -DCMAKE_BUILD_TYPE=Release -DDK94_PLAYER=ON -DDK94_STATIC_SDL=ON && ninja -C build-docker-linux dk94'
WINDOWS='cmake -S . -B build-docker-win -G Ninja -DCMAKE_BUILD_TYPE=Release -DDK94_PLAYER=ON \
    -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64.cmake \
    -DSDL2_DIR=$SDL2_MINGW_DIR/x86_64-w64-mingw32/lib/cmake/SDL2 && ninja -C build-docker-win dk94'

case "$TARGET" in
    linux)   run "$LINUX" ;;
    windows) run "$WINDOWS" ;;
    all)     run "$LINUX && $WINDOWS" ;;
    release) run "tools/make_release.sh ${2:?usage: build.sh release VERSION}" ;;
    shell)   run "bash" ;;
    *)       echo "usage: $0 [linux|windows|all|release VERSION|shell]" >&2; exit 2 ;;
esac
