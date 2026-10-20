#include "hle/hle_compare_string.hpp"
#include "hle/hle_process.hpp"
#include <algorithm>
#include <limits>

namespace sadx {
namespace {
// [platform:locale] CompareStringA counts bytes, and any negative count denotes
// a null-terminated string. Microsoft: /windows/win32/api/winnls/nf-winnls-comparestringa.
std::optional<std::string> comparison_text(X86Guest &guest, std::uint32_t address,
                                         std::int32_t count) {
    if (!address)
        return std::nullopt;
    std::string text;
    for (std::uint64_t index = 0; count < 0 || index < static_cast<unsigned>(count); ++index) {
        if (index > std::numeric_limits<std::uint32_t>::max() - address)
            return std::nullopt;
        unsigned char byte = 0;
        if (!guest.read(address + static_cast<std::uint32_t>(index), &byte, 1))
            return std::nullopt;
        if (byte == 0 && count < 0)
            break;
        // Embedded null collation has not been captured. Retain it so the
        // supported-domain check reports that case instead of guessing.
        text.push_back(static_cast<char>(byte));
    }
    return text;
}
bool ascii_alphanumeric(unsigned char byte) {
    return (byte >= 'a' && byte <= 'z') || (byte >= '0' && byte <= '9');
}
// The observed caller compares SonicDX%02d.snc with directory entries. Restrict
// this implementation to English, ignore-case ASCII identifiers. Punctuation
// ordering and non-ASCII collation require Windows NLS evidence, not strcmp.
std::optional<std::uint32_t> compare_identifiers(std::string left, std::string right) {
    for (auto *text : {&left, &right}) {
        for (auto &byte : *text) {
            if (byte >= 'A' && byte <= 'Z')
                byte = static_cast<char>(byte + ('a' - 'A'));
            if (!ascii_alphanumeric(static_cast<unsigned char>(byte)) && byte != '.' && byte != '_')
                return std::nullopt;
        }
    }
    const auto length = std::min(left.size(), right.size());
    for (std::size_t index = 0; index < length; ++index) {
        if (left[index] == right[index])
            continue;
        if (!ascii_alphanumeric(left[index]) || !ascii_alphanumeric(right[index]))
            return std::nullopt;
        return left[index] < right[index] ? 1U : 3U;
    }
    // Prefix comparisons are supported only where the remaining suffix is
    // alphanumeric; do not invent the weight of punctuation against end-of-text.
    for (const auto *text : {&left, &right})
        for (std::size_t index = length; index < text->size(); ++index)
            if (!ascii_alphanumeric((*text)[index]))
                return std::nullopt;
    return left.size() == right.size() ? 2U : left.size() < right.size() ? 1U : 3U;
}
}
void register_hle_compare_string(HleProcess &process) {
    process.hle().add("KERNEL32.dll", "CompareStringA", {24, 6, [&process](HleCall &call) {
        std::uint32_t args[6]{};
        for (unsigned index = 0; index < 6; ++index) {
            const auto argument = call.argument(index);
            if (!argument) {
                call.guest.stop("CompareStringA: unreadable argument");
                return 0U;
            }
            args[index] = *argument;
        }
        if ((args[0] != 9 && args[0] != 0x409) || args[1] != 1) {
            call.guest.stop("CompareStringA: unsupported locale " + std::to_string(args[0]) +
                            " or comparison flags " + std::to_string(args[1]));
            return 0U;
        }
        const auto left = comparison_text(call.guest, args[2], static_cast<std::int32_t>(args[3]));
        const auto right = comparison_text(call.guest, args[4], static_cast<std::int32_t>(args[5]));
        if (!left || !right) {
            call.guest.stop("CompareStringA: unreadable string span");
            return 0U;
        }
        const auto result = compare_identifiers(*left, *right);
        if (!result) {
            call.guest.stop("CompareStringA: unsupported NLS collation for supplied strings");
            return 0U;
        }
        process.assume("CompareStringA: English NORM_IGNORECASE ASCII save identifiers; "
                       "non-ASCII and differing punctuation collation stop explicitly");
        return *result;
    }});
}
}
