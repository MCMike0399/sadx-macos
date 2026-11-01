#include "engine/ninja_render.hpp"
#include <algorithm>
#include <cstring>

// Reference-only bridge to the existing depth comparison/write interfaces.
// Input: kind, full-width value, device facts, table fact, backend fact, HRESULT.
extern "C" int sadx_native_ninja_render_depth(const std::uint32_t *table,
                                              const std::uint32_t *input, std::uint32_t *output) {
    if (!table || !input || !output || input[0] > 1 || input[2] > 2 || input[4] > 2)
        return 1;
    std::array<std::uint32_t, 8> comparisons;
    std::copy(table, table + comparisons.size(), comparisons.begin());
    sadx::NinjaRenderState state;
    std::vector<sadx::NinjaRenderStateCommand> callbacks;
    auto device = std::make_shared<sadx::NinjaRenderDevice>();
    device->identity = 1;
    if (input[2] != 0)
        state.device_at_3d128b0 = input[2] == 1 ? device : sadx::NinjaRenderDeviceHandle{};
    if (input[4] != 0) {
        state.set_render_state = [&](const auto &command) {
            callbacks.push_back(command);
            std::int32_t hresult;
            std::memcpy(&hresult, input + 5, sizeof(hresult));
            return sadx::NinjaRenderCallResult{input[4] == 1, hresult};
        };
    }
    const auto result =
        input[0] == 0
            ? sadx::ninja_set_depth_comparison(input[1], input[3] ? &comparisons : nullptr, state)
            : sadx::ninja_set_depth_write(input[1], state);
    std::fill(output, output + 20, 0);
    output[0] = static_cast<bool>(result);
    output[1] = result.source_boundary;
    output[2] = static_cast<std::uint32_t>(result.commands.size());
    output[3] = static_cast<std::uint32_t>(result.ignored_hresults.size());
    output[4] = result.pending_command.has_value();
    if (!result.commands.empty()) {
        const auto &command = result.commands.front();
        output[5] = static_cast<std::uint32_t>(command.device->identity);
        output[6] = command.state;
        output[7] = command.value;
        output[8] = command.source_call;
    }
    if (!result.ignored_hresults.empty() && result.ignored_hresults.front()) {
        output[9] = 1;
        output[10] = static_cast<std::uint32_t>(*result.ignored_hresults.front());
    }
    output[11] = static_cast<std::uint32_t>(callbacks.size());
    if (!callbacks.empty()) {
        const auto &command = callbacks.front();
        output[12] = command.device ? static_cast<std::uint32_t>(command.device->identity) : 0;
        output[13] = command.state;
        output[14] = command.value;
        output[15] = command.source_call;
    }
    if (result.pending_command) {
        const auto &command = *result.pending_command;
        output[16] = command.device ? static_cast<std::uint32_t>(command.device->identity) : 0;
        output[17] = command.state;
        output[18] = command.value;
        output[19] = command.source_call;
    }
    return 0;
}
