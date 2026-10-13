#!/bin/sh
set -eu

vendor_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
output_dir=${1:?Provide an ignored output directory}
mkdir -p "$output_dir"
output_dir=$(CDPATH= cd -- "$output_dir" && pwd)
cxx=${CXX:-clang++}
sdk_path=$(xcrun --sdk macosx --show-sdk-path)
objects_file="$output_dir/objects.txt"
: > "$objects_file"
while IFS= read -r source_file; do
    test -n "$source_file" || continue
    object_name=$(printf '%s' "$source_file" | tr '/' '_')
    wrapper="$output_dir/$object_name.cpp"
    object="$output_dir/$object_name.o"
    cat > "$wrapper" <<EOF
#include "$vendor_dir/math.hpp"
namespace sadx::bochs_fpatan {
#include "$vendor_dir/source/$source_file"
}
EOF
    "$cxx" -std=c++20 -O2 -arch arm64 -mmacosx-version-min=14.0 -isysroot "$sdk_path" -fvisibility=hidden \
        -I"$vendor_dir" -I"$vendor_dir/source" \
        -I"$vendor_dir/source/softfloat3e/include" \
        -c "$wrapper" -o "$object"
    printf '%s\n' "$object" >> "$objects_file"
done < "$vendor_dir/sources.txt"
# This generated list contains our output directory and manifest-derived file
# names. Read each line without word splitting so spaces in paths are preserved.
set --
while IFS= read -r object; do set -- "$@" "$object"; done < "$objects_file"
# Rewrite the archive from the exact manifest; removed sources cannot survive
# as stale members when this dependency closure changes.
xcrun libtool -static -o "$output_dir/libbochs_fpatan.a" "$@"
