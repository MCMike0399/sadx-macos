#pragma once
#include "apps/native_assets.hpp"
#include "runtime/message_font_assets.hpp"
#include <CommonCrypto/CommonDigest.h>
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace sadx::native::app {
// NOTSA: this imported format has integer fields, never JSON booleans.
inline std::int64_t message_font_integer(id value, std::int64_t low, std::int64_t high,
                                         const char *name) {
    if (value && CFGetTypeID((__bridge CFTypeRef)value) == CFBooleanGetTypeID())
        throw std::runtime_error(std::string("invalid message font integer: ") + name);
    return integer(value, low, high, name);
}
// NOTSA: resource paths, hexadecimal bytes and source text cannot contain a hidden NUL suffix.
inline std::string message_font_string(id value, const char *name) {
    const auto result = string(value, name);
    if ([value lengthOfBytesUsingEncoding:NSUTF8StringEncoding] != result.size())
        throw std::runtime_error(std::string("embedded NUL in message font string: ") + name);
    return result;
}
// NOTSA: fixed-size hexadecimal source metadata, including zero bytes in glyph records.
inline std::vector<std::uint8_t> message_font_bytes(id value, std::size_t size) {
    const auto text = message_font_string(value, "message font bytes");
    if (text.size() != size * 2)
        throw std::runtime_error("invalid message font byte count");
    const auto digit = [](char c) -> unsigned {
        if (c >= '0' && c <= '9') return unsigned(c - '0');
        if (c >= 'a' && c <= 'f') return unsigned(c - 'a' + 10);
        throw std::runtime_error("invalid message font hexadecimal byte");
    };
    std::vector<std::uint8_t> result(size);
    for (std::size_t index = 0; index < size; ++index)
        result[index] = static_cast<std::uint8_t>((digit(text[index * 2]) << 4) |
                                                 digit(text[index * 2 + 1]));
    return result;
}
inline MessageFontAssets load_message_font_assets(const std::filesystem::path &directory) {
    const auto metadata = read_metadata(directory / "message-fonts.json");
    (void)message_font_integer(metadata[@"version"], 1, 1, "version");
    const auto declarations = dictionary(metadata[@"fonts"], "message fonts");
    const auto load = [&](NSString *family, std::size_t expected_count) {
        const auto declaration = dictionary(declarations[family], "message font declaration");
        const auto count = static_cast<std::size_t>(message_font_integer(declaration[@"glyph_count"],
            expected_count, expected_count, "message font glyph count"));
        const std::filesystem::path relative(message_font_string(declaration[@"glyph_file"], "message font file"));
        if (relative.is_absolute())
            throw std::runtime_error("message font filename must be relative");
        const auto root = std::filesystem::canonical(directory);
        const auto file = std::filesystem::canonical(root / relative);
        const auto contained = file.lexically_relative(root);
        if (contained.empty() || *contained.begin() == "..")
            throw std::runtime_error("message font resolves outside imported assets");
        NSData *data = [NSData dataWithContentsOfFile:[NSString stringWithUTF8String:file.c_str()]];
        if (!data || data.length != count * 72)
            throw std::runtime_error("message font payload has the wrong glyph length");
        std::array<unsigned char, CC_SHA256_DIGEST_LENGTH> digest{};
        CC_SHA256(data.bytes, static_cast<CC_LONG>(data.length), digest.data());
        std::ostringstream actual;
        for (auto byte : digest)
            actual << std::hex << std::setw(2) << std::setfill('0') << unsigned(byte);
        if (actual.str() != message_font_string(declaration[@"glyph_sha256"], "message font identity"))
            throw std::runtime_error("message font payload identity mismatch");
        auto font = std::make_shared<message_text::FontData>();
        font->glyphs.resize(count);
        const auto *bytes = static_cast<const std::uint8_t *>(data.bytes);
        for (std::size_t index = 0; index < count; ++index)
            std::memcpy(font->glyphs[index].bitmap.data(), bytes + index * 72, 72);
        const auto fallback = message_font_bytes(declaration[@"fallback_bytes"], 72);
        std::copy(fallback.begin(), fallback.end(), font->fallback.bitmap.begin());
        return font;
    };
    MessageFontAssets result{};
    result.wide = load(@"wide", 8836);
    result.single = load(@"single", 256);
    const auto languages = array(metadata[@"languages"], result.languages.size(), "message languages");
    for (std::size_t index = 0; index < result.languages.size(); ++index) {
        const auto rule = dictionary(languages[index], "message language");
        result.languages[index].lead_first = static_cast<std::uint8_t>(
            message_font_integer(rule[@"lead_first"], 0, 255, "message lead byte"));
        result.languages[index].lead_last = static_cast<std::uint8_t>(
            message_font_integer(rule[@"lead_last"], 0, 255, "message final lead byte"));
        // The pinned source directory has two 128..255 rows, then three 0..0 rows.
        const std::uint8_t expected_first = index < 2 ? 128 : 0;
        const std::uint8_t expected_last = index < 2 ? 255 : 0;
        if (result.languages[index].lead_first != expected_first ||
            result.languages[index].lead_last != expected_last)
            throw std::runtime_error("message font language rule differs from the recovered directory");
        if (rule[@"remapping"] != NSNull.null)
            throw std::runtime_error("message font language remapping is not imported");
    }
    const auto widths = array(metadata[@"widths"], result.expected_widths.size(), "message font widths");
    for (std::size_t index = 0; index < result.expected_widths.size(); ++index)
        result.expected_widths[index] = static_cast<std::int8_t>(
            message_font_integer(widths[index], 1, 24, "message font width"));
    result.saving_text = message_font_string(metadata[@"saving_text"], "source saving message");
    if (result.saving_text != "Now Saving...")
        throw std::runtime_error("saving text differs from the recovered source message");
    return result;
}
} // namespace sadx::native::app
