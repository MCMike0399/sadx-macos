#include "player/gamma_effects.hpp"
#include <cstring>
#include <stdexcept>

namespace sadx::gamma_effects {
Asset decode(const std::vector<std::uint8_t> &bytes, texture_lifetime::List &textures) {
    constexpr std::size_t kHeaderBytes = 20, kFrameBytes = 44, kFrameCount = 22;
    if (bytes.size() != kHeaderBytes + kFrameBytes * kFrameCount ||
        std::memcmp(bytes.data(), "SDXGEF01", 8) != 0 || textures.entries.size() != 17)
        throw std::runtime_error("gamma_effects_import_format");
    // NOTSA: explicit little-endian integer reading preserves every declaration word.
    const auto word = [&](std::size_t offset) {
        std::uint32_t value = 0;
        for (unsigned byte = 0; byte < 4; ++byte)
            value |= std::uint32_t(bytes.at(offset + byte)) << (byte * 8);
        return value;
    };
    if (word(8) != 17 || word(12) != 17 || word(16) != 5)
        throw std::runtime_error("gamma_effects_import_counts");
    Asset asset{};
    asset.textures = &textures;
    std::size_t offset = kHeaderBytes;
    const auto decode_table = [&](auto &table) {
        for (auto &frame : table) {
            const auto header = word(offset);
            frame.texture_index = static_cast<std::int16_t>(header);
            frame.flags = static_cast<std::uint8_t>(header >> 16);
            frame.padding = static_cast<std::uint8_t>(header >> 24);
            const auto global_index = word(offset + 4);
            std::memcpy(&frame.global_index, &global_index, sizeof(global_index));
            std::array<float *, 9> scalars{&frame.width, &frame.height, &frame.center_x,
                &frame.center_y, &frame.parameters[0], &frame.parameters[1],
                &frame.parameters[2], &frame.parameters[3], &frame.parameters[4]};
            for (std::size_t index = 0; index < scalars.size(); ++index) {
                const auto bits = word(offset + 8 + index * 4);
                std::memcpy(scalars[index], &bits, sizeof(bits));
            }
            offset += kFrameBytes;
        }
        for (std::size_t index = 0; index < table.size(); ++index) {
            if ((index + 1 == table.size() && table[index].texture_index >= 0) ||
                (index + 1 < table.size() &&
                 (table[index].texture_index < 0 || table[index].texture_index >= 17)))
                throw std::runtime_error("gamma_effects_import_texture_index");
        }
    };
    decode_table(asset.hit);
    decode_table(asset.burst);
    return asset;
}

void initialize_burst(State &state, Asset &asset, TaskScheduler &scheduler,
                      TaskWorkArena &arena, fidelity::Context &arithmetic) {
    if (!state.burst_task) {
        particles::bind_frames(*asset.textures, asset.burst.data(), 0, arithmetic);
        const particles::Parameters parameters{0.03f, 0.0f};
        state.burst_task = particles::create_pool(scheduler, arena, 40, parameters,
                                                asset.burst.data(), nullptr);
    }
}

void initialize(State &state, Asset &asset, TaskScheduler &scheduler,
                TaskWorkArena &arena, fidelity::Context &arithmetic) {
    particles::bind_frames(*asset.textures, asset.hit.data(), 0, arithmetic);
    initialize_burst(state, asset, scheduler, arena, arithmetic);
}
} // namespace sadx::gamma_effects
