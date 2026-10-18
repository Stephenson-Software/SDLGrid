#!/usr/bin/env bash
# Regenerates the README images in docs/images/ from the two demo programs in test/.
# Requires: Xvfb, xdotool, ImageMagick (import/convert), plus SDL2 and SDL2_image.
# Usage (from the repository root): bash docs/capture.sh [display-number]
set -euo pipefail
N="${1:-73}"
OUT="$PWD/docs/images"
TMP="$(mktemp -d)"
mkdir -p "$OUT"

cd test
g++ -std=c++11 -I../src ../src/GridClass.cpp testingGridClass.cpp $(sdl2-config --cflags --libs) -lSDL2_image -o testingGridClass
g++ -std=c++11 -I../src ../src/GridClass.cpp simpleLevel.cpp $(sdl2-config --cflags --libs) -lSDL2_image -o simpleLevel

# software GL keeps Xvfb from loading a host GPU driver
export LIBGL_ALWAYS_SOFTWARE=1 GALLIUM_DRIVER=llvmpipe
export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-$TMP}"
Xvfb ":$N" -screen 0 1280x800x24 >/dev/null 2>&1 &
XPID=$!
trap 'kill $XPID 2>/dev/null; rm -rf "$TMP"' EXIT
export DISPLAY=":$N"
for _ in $(seq 50); do xdotool getdisplaygeometry >/dev/null 2>&1 && break; sleep 0.2; done

# testingGridClass: a still of the 20x20 outlined grid
./testingGridClass &
PID=$!
sleep 1.5
import -window "$(xdotool search --pid "$PID" | tail -1)" "$OUT/testingGridClass.png"
kill "$PID"; wait "$PID" 2>/dev/null || true

# simpleLevel: the textured level after the sprite has come to rest on the ground row
./simpleLevel &
PID=$!
sleep 1.5
import -window "$(xdotool search --pid "$PID" | tail -1)" "$OUT/simpleLevel.png"
kill "$PID"; wait "$PID" 2>/dev/null || true
echo "wrote $(ls "$OUT" | tr '\n' ' ')"
