#!/bin/sh
set -eu

if [ "$#" -ne 1 ]; then
    echo "usage: sh third_party/softfloat/build.sh OUTPUT_DIRECTORY" >&2
    exit 2
fi
sadx_sf_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
# shellcheck disable=SC1091
. "$sadx_sf_root/../host-toolchain.sh"
sadx_sf_output=$1
mkdir -p "$sadx_sf_output/objects"
sadx_sf_output=$(CDPATH= cd -- "$sadx_sf_output" && pwd)
set --
while IFS= read -r sadx_sf_source; do
    sadx_sf_name=${sadx_sf_source##*/}
    # shellcheck disable=SC2086
    sadx_compile_c -std=c11 -O2 -fPIC $sadx_arch_flags -Werror=implicit-function-declaration \
        -I"$sadx_sf_root" -I"$sadx_sf_root/source/include" -I"$sadx_sf_root/source/8086" \
        -c "$sadx_sf_root/source/$sadx_sf_source" \
        -o "$sadx_sf_output/objects/${sadx_sf_name%.c}.o"
    set -- "$@" "$sadx_sf_output/objects/${sadx_sf_name%.c}.o"
done < "$sadx_sf_root/sources.txt"
# Rewrite from the exact manifest; stale objects/archive members cannot enter.
sadx_static_archive "$sadx_sf_output/softfloat.a" "$@"
echo "$sadx_sf_output/softfloat.a"
