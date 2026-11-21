#!/usr/bin/env bash
# Fetches one of Strata's release downloads (the version in
# third_party/strata/VERSION, whose strata.h src/GridView3DStrata.h is built against),
# checks it against the release's SHA256SUMS, unpacks it under <dest> and
# prints the directory it unpacked to:
#
#   tools/fetch-strata.sh linux <dest>     libstrata.so.0.9 under lib/
#   tools/fetch-strata.sh windows <dest>   strata.dll under bin/
#
# A program using the 3D view finds the library through STRATA_LIBRARY, beside
# its executable, or on the library path:
#
#   STRATA_LIBRARY="$(tools/fetch-strata.sh linux /tmp/strata)/lib/libstrata.so.0.9" ./simpleLevel
#
# STRATA_DOWNLOADS=<dir> takes the download and SHA256SUMS from <dir> instead
# of GitHub (for building offline).
set -euo pipefail
cd "$(dirname "$0")/.."

kind=${1:?usage: tools/fetch-strata.sh <linux|windows> <dest>}
dest=${2:?usage: tools/fetch-strata.sh <linux|windows> <dest>}
version=$(tr -d '[:space:]' < third_party/strata/VERSION)
case "$kind" in
  linux) asset="strata-$version-linux-x64.tar.gz" ;;
  windows) asset="strata-$version-win-x64.zip" ;;
  *) echo "unknown kind '$kind' (linux or windows)" >&2; exit 2 ;;
esac

mkdir -p "$dest"
dest=$(cd "$dest" && pwd)
if [ -n "${STRATA_DOWNLOADS:-}" ]; then
  cp "$STRATA_DOWNLOADS/$asset" "$STRATA_DOWNLOADS/SHA256SUMS" "$dest/"
else
  base="https://github.com/Preponderous-Software/strata/releases/download/v$version"
  curl -fsSL --retry 3 -o "$dest/$asset" "$base/$asset"
  curl -fsSL --retry 3 -o "$dest/SHA256SUMS" "$base/SHA256SUMS"
fi
(cd "$dest" && grep " $asset\$" SHA256SUMS | sha256sum -c - >&2)

out="$dest/strata-$version-$kind"
rm -rf "$out"
mkdir -p "$out"
case "$asset" in
  *.zip) unzip -q "$dest/$asset" -d "$out" ;;
  *) tar -xzf "$dest/$asset" -C "$out" ;;
esac
echo "$out"
