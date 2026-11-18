# Windows wrapper for tools/docker/build.sh (needs Docker Desktop).
#
#   tools\docker\build.ps1 [linux|windows|all|release VERSION|shell]
param([string]$Target = "all", [string]$Version = "")

$ErrorActionPreference = "Stop"
$root = (Resolve-Path "$PSScriptRoot\..\..").Path
$image = "dk94ap-build"

docker build -q -t $image "$root\tools\docker" | Out-Null
if ($LASTEXITCODE -ne 0) { throw "docker build failed" }

$linux = 'cmake -S . -B build-docker-linux -G Ninja -DCMAKE_BUILD_TYPE=Release -DDK94_PLAYER=ON -DDK94_STATIC_SDL=ON && ninja -C build-docker-linux dk94'
$windows = 'cmake -S . -B build-docker-win -G Ninja -DCMAKE_BUILD_TYPE=Release -DDK94_PLAYER=ON -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64.cmake -DSDL2_DIR=$SDL2_MINGW_DIR/x86_64-w64-mingw32/lib/cmake/SDL2 && ninja -C build-docker-win dk94'

switch ($Target) {
    "linux"   { $cmd = $linux }
    "windows" { $cmd = $windows }
    "all"     { $cmd = "$linux && $windows" }
    "release" { if (-not $Version) { throw "usage: build.ps1 release VERSION" }; $cmd = "tools/make_release.sh $Version" }
    "shell"   { $cmd = "bash" }
    default   { throw "usage: build.ps1 [linux|windows|all|release VERSION|shell]" }
}

$tty = @()
if ($Target -eq "shell") { $tty = @("-it") }
# Compiler/CMake warnings go to stderr; with "Stop", Windows PowerShell would
# turn them into a terminating error when the output is redirected.
$ErrorActionPreference = "Continue"
docker run --rm @tty -e HOME=/tmp -v "${root}:/src" $image sh -c $cmd
exit $LASTEXITCODE
