# One image that builds both the Linux and the Windows (MinGW-w64 cross) player.
# Used by tools/docker/build.sh / build.ps1 locally, and by CI.
#
# Ubuntu 22.04 is deliberately old: the Linux binary links against its glibc
# (2.35), so it runs on any distro at least that new.  Its SDL (2.0.20) is too
# old for the runtime (needs >= 2.24), so SDL is built from source below and
# linked statically into the Linux binary (-DDK94_STATIC_SDL=ON); SDL still
# loads X11 / Wayland / ALSA / PulseAudio / PipeWire at runtime, so the binary
# does not depend on the distro's SDL at all.
FROM ubuntu:22.04

ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential cmake ninja-build pkg-config git curl ca-certificates \
        bzip2 file python3.11 \
        gcc-mingw-w64-x86-64 binutils-mingw-w64-x86-64 \
        libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxfixes-dev \
        libxi-dev libxss-dev libxkbcommon-dev libwayland-dev wayland-protocols \
        libdecor-0-dev libegl-dev libgl-dev libgles-dev libdrm-dev libgbm-dev \
        libasound2-dev libpulse-dev libpipewire-0.3-dev libudev-dev libdbus-1-dev \
    && rm -rf /var/lib/apt/lists/*

ARG SDL2_VERSION=2.32.10

# SDL2 for the Linux build, static, into /usr/local.
RUN curl -fsSL -o /tmp/sdl2-src.tgz \
        "https://github.com/libsdl-org/SDL/releases/download/release-${SDL2_VERSION}/SDL2-${SDL2_VERSION}.tar.gz" \
    && tar -xzf /tmp/sdl2-src.tgz -C /tmp \
    && cmake -S /tmp/SDL2-${SDL2_VERSION} -B /tmp/sdl2-build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
        -DSDL_SHARED=OFF -DSDL_STATIC=ON -DSDL_TEST=OFF \
    && ninja -C /tmp/sdl2-build install \
    && rm -rf /tmp/sdl2-src.tgz /tmp/SDL2-${SDL2_VERSION} /tmp/sdl2-build

# The table generator needs Python >= 3.11 (tomllib); 22.04's python3 is 3.10.
# Putting 3.11 first on PATH makes CMake's FindPython3 pick it, without
# replacing the python3 that apt depends on.
RUN ln -s /usr/bin/python3.11 /usr/local/bin/python3

# SDL2 development package for MinGW (the Windows build links against it and
# ships its SDL2.dll).
RUN curl -fsSL -o /tmp/sdl2.tgz \
        "https://github.com/libsdl-org/SDL/releases/download/release-${SDL2_VERSION}/SDL2-devel-${SDL2_VERSION}-mingw.tar.gz" \
    && mkdir -p /opt/sdl2-mingw \
    && tar -xzf /tmp/sdl2.tgz -C /opt/sdl2-mingw \
    && rm /tmp/sdl2.tgz
ENV SDL2_MINGW_DIR=/opt/sdl2-mingw/SDL2-${SDL2_VERSION}

# Let any host user write the build tree; git refuses bind-mounted repos owned
# by someone else unless they are marked safe.
RUN git config --system --add safe.directory '*'
WORKDIR /src
