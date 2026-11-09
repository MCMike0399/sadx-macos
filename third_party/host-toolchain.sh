# Host compiler and static-archive commands for the vendored arithmetic libraries.
# Darwin keeps the Apple ARM64 contract: xcrun, arm64, macOS 14, and libtool.
# Any other host compiles for the machine that runs the portable tests.
# Source this file; do not execute it.

sadx_host_os=$(uname -s)

if [ "$sadx_host_os" = Darwin ]; then
    sadx_cxx=$(xcrun --find clang++)
    sadx_arch_flags='-arch arm64 -mmacosx-version-min=14.0'
    sadx_sdk=$(xcrun --sdk macosx --show-sdk-path)
else
    sadx_cxx=${CXX:-clang++}
    # The macOS product build targets Apple ARM64, where plain char is unsigned.
    # Linux x86_64 defaults to signed char. Match the product ABI in these libraries.
    sadx_arch_flags='-funsigned-char'
    sadx_sdk=
fi

sadx_compile_c() {
    if [ "$sadx_host_os" = Darwin ]; then
        xcrun clang "$@"
    else
        ${CC:-clang} "$@"
    fi
}

sadx_static_archive() {
    sadx_archive_output=$1
    shift
    if [ "$sadx_host_os" = Darwin ]; then
        xcrun libtool -static -o "$sadx_archive_output" "$@"
    else
        rm -f "$sadx_archive_output"
        ar rcs "$sadx_archive_output" "$@"
    fi
}
