#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
namespace sadx {
// Text conversions the Win32/CRT HLE needs: code page 1252 (the guest's ANSI code page), UTF-8 and
// UTF-16, and CT_CTYPE1 classification. 1252 follows Windows' table (bytes 81/8D/8F/90/9D map to the
// C1 controls, as MultiByteToWideChar does). Classification above U+007F is derived from Unicode
// categories; Windows' own NLS tables for those characters were not captured.
std::uint16_t cp1252_to_utf16(std::uint8_t byte);
std::optional<std::uint8_t> utf16_to_cp1252(std::uint16_t unit); // nullopt: no exact mapping
std::optional<std::vector<std::uint16_t>> utf8_to_utf16(const std::vector<std::uint8_t>&); // nullopt: invalid UTF-8
std::vector<std::uint8_t> utf16_to_utf8(const std::vector<std::uint16_t>&);
std::uint16_t ctype1(std::uint16_t unit);   // C1_UPPER 1, LOWER 2, DIGIT 4, SPACE 8, PUNCT 0x10, CNTRL 0x20, BLANK 0x40, XDIGIT 0x80, ALPHA 0x100
std::uint16_t to_lower16(std::uint16_t unit);std::uint16_t to_upper16(std::uint16_t unit);
} // namespace sadx
