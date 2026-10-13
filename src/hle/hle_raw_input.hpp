#pragma once
#include <cstdint>
#include <optional>
#include <set>
#include <map>
#include <vector>
#include <string>
namespace sadx {
class HleProcess;
struct HleXInputGamepad;
// The synthetic user-0 pad's raw-input/HID identity. Handles are guest tokens.
struct HleRawInputState {
    static constexpr std::uint32_t device=0x71000000;
    static const char* device_name() noexcept;
    std::set<std::uint32_t> open_handles;
    struct Registration {std::uint32_t flags=0,target=0;};
    std::map<std::uint32_t,Registration> registrations;
    std::map<std::uint32_t,std::vector<std::uint8_t>> packets;
    void sample(HleProcess&,bool connection_changed);
    void emit(HleProcess&,std::uint32_t target);
    std::optional<std::uint32_t> open(HleProcess&,const std::string&);
    bool close(std::uint32_t);
};
void register_hle_raw_input(HleProcess&);
} // namespace sadx
