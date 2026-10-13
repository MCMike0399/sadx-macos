#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
namespace sadx {
// Native owner of the Win32 private-profile reads WinMain makes on
// ".\sonicDX.ini". The text is the caller's file bytes; lookups are
// case-insensitive (ASCII) on section and key, the first matching key in the
// first matching section wins, and values are trimmed of spaces/tabs. This is
// a re-implementation of the documented Win32 behaviour, not a traced one:
// GetPrivateProfileIntA parses optional leading blanks, an optional sign and
// decimal digits (stopping at the first non-digit; no digits gives 0), and
// returns `fallback` when the key is absent. Quoted values and hex forms are
// not interpreted (explicit, see `unsupported`).
class Win32Profile {
public:
    explicit Win32Profile(const std::vector<std::uint8_t>& text);
    std::optional<std::string> value(const std::string& section,const std::string& key)const;
    std::int32_t get_int(const std::string& section,const std::string& key,std::int32_t fallback)const;
    // `size` is the caller's buffer size: at most size-1 characters are returned.
    std::string get_string(const std::string& section,const std::string& key,const std::string& fallback,std::uint32_t size)const;
    // Values whose form this owner does not interpret identically to Win32 (quotes, 0x, overflow).
    std::vector<std::string> unsupported;
private:
    struct Entry {std::string section,key,value;};
    std::vector<Entry> entries_;
};
} // namespace sadx
