#!/bin/sh
set -eu

if [ "$#" -ne 1 ]; then
    echo "usage: sh third_party/softfloat/build.sh OUTPUT_DIRECTORY" >&2
    exit 2
fi
sadx_sf_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
sadx_sf_output=$1
mkdir -p "$sadx_sf_output/objects"
sadx_sf_output=$(CDPATH= cd -- "$sadx_sf_output" && pwd)
set --
while IFS= read -r sadx_sf_source; do
    sadx_sf_name=${sadx_sf_source##*/}
    xcrun clang -std=c11 -O2 -fPIC -arch arm64 -mmacosx-version-min=14.0 -Werror=implicit-function-declaration \
        -I"$sadx_sf_root" -I"$sadx_sf_root/source/include" -I"$sadx_sf_root/source/8086" \
        -c "$sadx_sf_root/source/$sadx_sf_source" \
        -o "$sadx_sf_output/objects/${sadx_sf_name%.c}.o"
    set -- "$@" "$sadx_sf_output/objects/${sadx_sf_name%.c}.o"
done < "$sadx_sf_root/sources.txt"
# Rewrite from the exact manifest; stale objects/archive members cannot enter.
xcrun libtool -static -o "$sadx_sf_output/softfloat.a" "$@"
echo "$sadx_sf_output/softfloat.a"
