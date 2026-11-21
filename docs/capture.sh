#!/usr/bin/env bash
# Regenerates the README images in docs/images/ from the two demo programs in test/.
# Requires: Xvfb, xdotool, ImageMagick (import/convert), plus SDL2 and SDL2_image.
# Usage (from the repository root): bash docs/capture.sh [display-number]
#
# The 2D images are taken from the demos' own windows (run with --2d). With
# STRATA_LIBRARY set to libstrata.so.0.9 (tools/fetch-strata.sh linux <dir>),
# the 3D images are drawn too, headlessly, by test/view3dShots.cpp.
set -euo pipefail
N="${1:-73}"
OUT="$PWD/docs/images"
TMP="$(mktemp -d)"
mkdir -p "$OUT"

cd test
g++ -std=c++11 -I../src ../src/GridClass.cpp testingGridClass.cpp $(sdl2-config --cflags --libs) -lSDL2_image -ldl -o testingGridClass
g++ -std=c++11 -I../src ../src/GridClass.cpp simpleLevel.cpp $(sdl2-config --cflags --libs) -lSDL2_image -ldl -o simpleLevel

# software GL keeps Xvfb from loading a host GPU driver
export LIBGL_ALWAYS_SOFTWARE=1 GALLIUM_DRIVER=llvmpipe
export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-$TMP}"
Xvfb ":$N" -screen 0 1280x800x24 >/dev/null 2>&1 &
XPID=$!
trap 'kill $XPID 2>/dev/null; rm -rf "$TMP"' EXIT
export DISPLAY=":$N"
for _ in $(seq 50); do xdotool getdisplaygeometry >/dev/null 2>&1 && break; sleep 0.2; done

# testingGridClass: a still of the 20x20 outlined grid
./testingGridClass --2d &
PID=$!
sleep 1.5
import -window "$(xdotool search --pid "$PID" | tail -1)" "$OUT/testingGridClass.png"
kill "$PID"; wait "$PID" 2>/dev/null || true

# simpleLevel: the textured level after the sprite has come to rest on the ground row
./simpleLevel --2d &
PID=$!
sleep 1.5
import -window "$(xdotool search --pid "$PID" | tail -1)" "$OUT/simpleLevel.png"
kill "$PID"; wait "$PID" 2>/dev/null || true

# the 3D view of both levels, headlessly, when Strata is at hand
if [ -n "${STRATA_LIBRARY:-}" ]; then
  g++ -std=c++11 -I../src ../src/GridClass.cpp view3dShots.cpp $(sdl2-config --cflags --libs) -lSDL2_image -ldl -o view3dShots
  ./view3dShots "$TMP" >/dev/null
  cp "$TMP/sdlgrid-3d-simpleLevel-500x500.png" "$OUT/simpleLevel-3d.png"
  cp "$TMP/sdlgrid-3d-testingGridClass-500x500.png" "$OUT/testingGridClass-3d.png"
fi
echo "wrote $(ls "$OUT" | tr '\n' ' ')"
