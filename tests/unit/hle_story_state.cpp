#include "apps/app_harness.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
void require(bool condition, const std::string& message) {
    if (!condition)
        throw std::runtime_error(message);
}
} // namespace

int main() {
    try {
        std::string error;
        auto process = sadx::HleProcess::create(".", "/private/tmp/sadx-story-state-overlay", &error);
        require(bool(process), error);
        auto& guest = process->guest();
        for (const auto address : {0x3b22000U, 0x3c58000U, 0x3c5a000U, 0x10000000U})
            require(guest.map(address, 4096, false, &error), error);
        const auto stage = [&](std::int16_t value) {
            require(guest.write(0x3b22dcc, &value, sizeof value), "stage fixture");
        };
        const auto snapshot = [&] { return sadx::harness::game_state_json(*process, 1); };
        stage(26);
        guest.set_u32(0x3c5a7d8, 0x10000000);
        require(snapshot().find("\"boss\":null") != std::string::npos,
                "shared boss globals are ignored outside Chaos 0");
        stage(15);
        const std::uint8_t mode = 6;
        const float position[3] = {268, 0, 310};
        const float life = 2;
        require(guest.write(0x10000000, &mode, sizeof mode) &&
                    guest.write(0x10000020, position, sizeof position) &&
                    guest.write(0x3c58158, &life, sizeof life),
                "boss state fixtures");
        require(snapshot().find("\"boss\":{\"kind\":\"chaos-zero\",\"mode\":6,"
                                "\"life\":2,\"position\":[268,0,310]}") != std::string::npos,
                "source-named boss mode, life and position are observed");
        guest.set_u32(0x3c5a7d8, 0);
        require(snapshot().find("\"boss\":null") != std::string::npos,
                "absent boss remains absent");
        guest.set_u32(0x3c5a7d8, 0x20000000);
        require(snapshot().find("\"boss_error\":\"unmapped Chaos 0 state\"") != std::string::npos,
                "unmapped task reports an error");
        guest.set_u32(0x3c5a7d8, 0x10000000);
        const auto unavailable = std::numeric_limits<float>::quiet_NaN();
        require(guest.write(0x3c58158, &unavailable, sizeof unavailable), "non-finite fixture");
        require(snapshot().find("\"boss_error\":\"non-finite Chaos 0 state\"") != std::string::npos,
                "non-finite life is reported without a fabricated value");
        std::cout << "Story observation: Chaos 0 mode/life/position, stage scope, absent and "
                     "unmapped owners, and non-finite rejection pass\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
