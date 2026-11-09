#!/bin/sh
# Build one static archive.
# Darwin: xcrun libtool -static, which merges nested archives into object members.
# Other hosts: GNU ar stores a nested .a as one member, so flatten those objects first.
# Usage: static_archive.sh OUTPUT INPUT...
set -eu

if [ "$#" -lt 1 ]; then
    echo "usage: static_archive.sh OUTPUT INPUT..." >&2
    exit 2
fi

output=$1
shift

if [ "$(uname -s)" = Darwin ]; then
    xcrun libtool -static -o "$output" "$@"
    exit 0
fi

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
index=0
for input in "$@"; do
    case $input in
        *.a)
            members=$work/a$index
            mkdir "$members"
            archive=$(CDPATH= cd -- "$(dirname -- "$input")" && pwd)/$(basename -- "$input")
            (cd "$members" && ar x "$archive")
            ;;
        *.o)
            cp "$input" "$work/o$index.o"
            ;;
        *)
            echo "static_archive: unsupported input: $input" >&2
            exit 2
            ;;
    esac
    index=$((index + 1))
done

rm -f "$output"
set --
for object in "$work"/*.o "$work"/*/*.o; do
    if [ -f "$object" ]; then
        set -- "$@" "$object"
    fi
done
if [ "$#" -eq 0 ]; then
    echo "static_archive: no objects for $output" >&2
    exit 2
fi
ar rcs "$output" "$@"
