#pragma once
#include "guest/x86_guest.hpp"
#include <optional>
#include <string>
#include <vector>
namespace sadx {
// printf-family formatting as the UCRT does it, reading the arguments from a guest va_list (each argument
// 4-byte aligned, doubles and 64-bit integers 8 bytes). Conversions: d i u o x X c C s S p e E f F g G a A
// and %%, flags - + space # 0, width/precision (numbers or *), lengths hh h l ll L I32 I64 I j z t w.
// `wide` selects the wide functions' legacy meaning of %s/%c (UTF-16) versus %S/%C (narrow). %n is
// rejected (the UCRT disables it by default). Text is produced as UTF-16 code units; narrow callers get
// the low bytes (code page 1252 is applied by the caller). nullopt + error: an unsupported or unreadable
// conversion.
std::optional<std::vector<std::uint16_t>> format_guest(const X86Guest&,const std::vector<std::uint16_t>& format,std::uint32_t va_list,bool wide,std::string* error);
} // namespace sadx
