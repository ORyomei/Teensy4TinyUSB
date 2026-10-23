#!/usr/bin/env sh
# Re-vendor TinyUSB into src/tinyusb from a release tag: the device stack, the class drivers and the ChipIdea
# High Speed port used by the i.MX RT1062. Host stack, Type-C and the other ports are left out.
#   tools/update_tinyusb.sh 0.18.0
set -eu
TAG="${1:?usage: update_tinyusb.sh <tinyusb tag, e.g. 0.18.0>}"
LIB=$(cd "$(dirname "$0")/.." && pwd)
DST="$LIB/src/tinyusb"
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

git clone --quiet --depth 1 --branch "$TAG" https://github.com/hathach/tinyusb.git "$TMP/tinyusb"
SRC="$TMP/tinyusb/src"

rm -rf "$DST"
mkdir -p "$DST/portable/chipidea"
cp "$TMP/tinyusb/LICENSE" "$DST/"
cp "$SRC/tusb.c" "$SRC/tusb.h" "$SRC/tusb_option.h" "$DST/"
cp -r "$SRC/common" "$SRC/device" "$SRC/osal" "$SRC/class" "$DST/"
cp -r "$SRC/portable/chipidea/ci_hs" "$DST/portable/chipidea/"
echo "$TAG" > "$DST/VERSION"
echo "TinyUSB $TAG -> $DST"
