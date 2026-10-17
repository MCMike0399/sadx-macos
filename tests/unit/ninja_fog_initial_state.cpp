#include "engine/ninja_fog_initial_state.hpp"
#include "guest/ninja_fog_reference.hpp"
#include <iostream>
#include <stdexcept>

int main() {
    auto values = sadx::ninja_fog_initial_state();
    sadx::NinjaQueueConsumerState consumer{};
    consumer.fog_supported_at_3d128a4 = values.table_supported;
    sadx::NinjaFogDeviceState device{};
    device.color_at_3d12898 = values.device.color;
    sadx::NinjaFogDeviceOwners owners{consumer, device, nullptr};
    const auto color = sadx::ninja_fog_color_7949b0(0x12345678U, owners);
    if (!color || device.color_at_3d12898 != 0x345678U || !color.render.commands.empty())
        throw std::runtime_error(
            "load-time fog capability permits color storage before a device exists");
    if (values.cached_start != 65535.0F || values.cached_end != 65535.0F ||
        values.device.w_depth[127] != 0 || values.device.reciprocal[127] != 0)
        throw std::runtime_error("load-time caches/tables must not use post-device fog state");
    values.table[0] = 1;
    if (values.device.w_depth[0] || sadx::ninja_fog_initial_state().table[0])
        throw std::runtime_error("fog tables and sessions must own independent values");
    std::cout << "Fog initial state: pre-device color, caches and independent tables passed.\n";
}
