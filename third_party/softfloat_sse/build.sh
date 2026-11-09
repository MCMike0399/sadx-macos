#!/bin/sh
set -eu
if [ "$#" -ne 1 ]; then echo 'usage: build.sh OUTPUT_DIRECTORY' >&2; exit 2; fi
sadx_sse_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
# shellcheck disable=SC1091
. "$sadx_sse_root/../host-toolchain.sh"
sadx_sse_shared="$sadx_sse_root/../softfloat"
mkdir -p "$1/objects"
sadx_sse_output=$(CDPATH= cd -- "$1" && pwd)
set --
while IFS= read -r sadx_sse_source; do
    sadx_sse_name=${sadx_sse_source##*/}
    # shellcheck disable=SC2086
    sadx_compile_c -std=c11 -O2 -fPIC $sadx_arch_flags -Werror=implicit-function-declaration \
        -I"$sadx_sse_shared" -I"$sadx_sse_shared/source/include" -I"$sadx_sse_root/source/8086-SSE" \
        -c "$sadx_sse_root/source/$sadx_sse_source" -o "$sadx_sse_output/objects/${sadx_sse_name%.c}.o"
    set -- "$@" "$sadx_sse_output/objects/${sadx_sse_name%.c}.o"
done < "$sadx_sse_root/sources.txt"
sadx_static_archive "$sadx_sse_output/softfloat-sse.a" "$@"
echo "$sadx_sse_output/softfloat-sse.a"
