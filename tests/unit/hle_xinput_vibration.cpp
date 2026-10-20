#include "hle/hle_process.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
void require(bool ok, const char* why) { if (!ok) throw std::runtime_error(why); }
}
int main() { try {
    using namespace sadx;
    for (const char* dll : {"XInput1_4.dll","XInput1_3.dll","XInput9_1_0.dll"}) {
        std::string error;
        auto process = HleProcess::create(".","build/xinput-vibration-overlay",&error);
        require(process != nullptr,"process creation");
        auto& guest = process->guest();
        const auto buffer = process->memory().allocate(32);
        require(buffer.has_value(),"buffer allocation");
        const auto call = [&](const char* name, std::initializer_list<std::uint32_t> arguments) {
            const auto entry = process->hle().trap(guest,dll,name,&error);
            require(entry.has_value(),"trap registration");
            return guest.call(*entry,arguments);
        };
        const auto word = [&](unsigned offset) { return guest.u32(*buffer+offset).value_or(0); };
        process->xinput.sample(HleXInputGamepad{});
        process->xinput.vibration_supported = true;
        std::vector<std::array<std::uint16_t,2>> delivered;
        std::uint32_t owner_status = 0;
        process->xinput.set_vibration = [&](std::uint16_t low, std::uint16_t high) {
            delivered.push_back({low,high});
            return owner_status;
        };
        process->last_error = 12345;
        auto result = call("XInputGetCapabilities",{0,1,*buffer+4});
        require(bool(result) && result.eax == 0,"supported capabilities");
        require(word(4) == 0x00010101 && word(20) == 0xffffffff,"motors and FFB capability");
        guest.set_u32(*buffer,0xa5a5a5a5);
        guest.set_u32(*buffer+4,0xabcd1234);
        guest.set_u32(*buffer+8,0x5a5a5a5a);
        result = call("XInputSetState",{0,*buffer+4});
        require(bool(result) && result.eax == 0,"motor delivery success");
        require(delivered.back() == std::array<std::uint16_t,2>{0x1234,0xabcd},"little-endian unsigned16 motors");
        require(word(0) == 0xa5a5a5a5 && word(8) == 0x5a5a5a5a,"request memory unchanged");
        require(bool(call("XInputEnable",{0})),"disable");
        require(delivered.back() == std::array<std::uint16_t,2>{0,0},"disable stops physical motors");
        const auto disabled_count = delivered.size();
        guest.set_u32(*buffer+4,0xffffffff);
        result = call("XInputSetState",{0,*buffer+4});
        require(bool(result) && result.eax == 0 && delivered.size() == disabled_count,"disabled request retained without delivery");
        require(bool(call("XInputEnable",{1})),"enable");
        require(delivered.back() == std::array<std::uint16_t,2>{65535,65535},"enable restores last request");
        owner_status = 31;
        guest.set_u32(*buffer+4,0x00020001);
        result = call("XInputSetState",{0,*buffer+4});
        require(bool(result) && result.eax == 31 && process->xinput.vibration_status == 31,"backend error propagated");
        require(process->xinput.requested_vibration == std::array<std::uint16_t,2>{65535,65535},"failed request not committed");
        process->xinput.vibration_supported = false;
        const auto unsupported_count = delivered.size();
        result = call("XInputSetState",{0,*buffer+4});
        require(bool(result) && result.eax == 50 && delivered.size() == unsupported_count,"unsupported owner never called");
        process->xinput.vibration_supported = true;
        process->xinput.sample(std::nullopt);
        result = call("XInputSetState",{0,*buffer+4});
        require(bool(result) && result.eax == 1167 && delivered.size() == unsupported_count,"disconnected owner never called");
        process->xinput.sample(HleXInputGamepad{});
        require(process->xinput.requested_vibration == std::array<std::uint16_t,2>{0,0},"replacement does not inherit previous vibration");
        require(process->last_error == 12345,"XInput never modifies LastError");
        result = call("XInputSetState",{0,0});
        require(!result && result.error.find("vibration buffer") != std::string::npos,"invalid buffer stops explicitly");
        require(delivered.size() == unsupported_count,"invalid buffer never reaches device");
    }
    std::cout << "XInput native vibration: three aliases, capabilities, wire motor values, enable/disable, "
                 "backend failure, replacement and bad-buffer boundaries pass\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
} }
