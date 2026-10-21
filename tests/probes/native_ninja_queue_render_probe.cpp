#include "engine/ninja_queue_native.hpp"
#include <algorithm>
#include <cstring>
#include <vector>
namespace {
struct SourceBoundary {
    unsigned site;
};
} // namespace
extern "C" int sadx_native_ninja_render(const std::uint8_t *bytes, std::size_t size,
                                        const std::uint32_t *input, std::size_t words,
                                        std::uint32_t *out, std::size_t count) {
    if (!bytes || size != 88 || !input || words != 8 || !out || count != 20)
        return 1;
    sadx::queue::BlendTables tables;
    std::memcpy(tables.targets.data(), bytes, 8);
    std::memcpy(tables.modes.data(), bytes + 8, 48);
    std::memcpy(tables.following_depth_modes.data(), bytes + 56, 16);
    sadx::queue::State state;
    state.values.packed_blend = static_cast<std::uint8_t>(input[2]);
    std::fill(out, out + count, 0);
    out[2] = input[3] != 0;
    bool known = input[4] != 0;
    unsigned current_device = input[4] == 1 ? 1 : 0;
    std::vector<std::array<unsigned, 4>> trace;
    std::vector<unsigned> receipts;
    sadx::queue::DeviceAccess access = [&]() -> sadx::queue::RenderStateWriter {
        const bool second = !trace.empty();
        unsigned load, dereference, site;
        if (input[1] == 0) {
            load = second ? 0x77ec7a : 0x77ec68;
            dereference = second ? 0x77ec7f : 0x77ec6d;
            site = second ? 0x77ec86 : 0x77ec74;
        } else if (input[1] == 1) {
            load = second ? 0x77eca4 : 0x77ec92;
            dereference = second ? 0x77eca9 : 0x77ec97;
            site = second ? 0x77ecb0 : 0x77ec9e;
        } else {
            load = 0x77ecbe;
            dereference = 0x77ecc3;
            site = 0x77ecd3;
        }
        if (!known)
            throw SourceBoundary{load};
        if (!current_device)
            throw SourceBoundary{dereference};
        return [&, device = current_device, site](unsigned setting, unsigned value) {
            const auto index = trace.size();
            trace.push_back({device, setting, value, site});
            receipts.push_back(input[5 + index]);
            if (index == 0) {
                if (input[7] == 1)
                    current_device = 2;
                else if (input[7] == 2)
                    known = false;
            }
            return true; // Source ignores every HRESULT, including failures.
        };
    };
    sadx::queue::Services services{};
    services.set_blend = [&](unsigned selector, unsigned mode) {
        sadx::queue::set_device_blend(selector, mode, tables, access);
        return true;
    };
    try {
        if (input[0] < 2 && !input[3])
            throw SourceBoundary{input[0] == 0 ? 0x4030ddU : 0x403103U};
        sadx::queue::set_blend(input[0], input[1], state, services);
    } catch (const SourceBoundary &error) {
        out[1] = error.site;
    } catch (const sadx::queue::Unavailable &error) {
        if (error.boundary() == sadx::queue::Boundary::BlendValue)
            out[1] = 0x77ecb7;
        else if (error.boundary() == sadx::queue::Boundary::BlendSelector)
            out[1] = 0x77ecca;
        else
            return 2;
    }
    out[3] = state.values.packed_blend;
    out[4] = static_cast<unsigned>(trace.size());
    out[5] = static_cast<unsigned>(receipts.size());
    for (std::size_t n = 0; n != trace.size(); ++n)
        std::copy(trace[n].begin(), trace[n].end(), out + 6 + n * 4);
    for (std::size_t n = 0; n != receipts.size(); ++n) {
        out[14 + n * 2] = 1;
        out[15 + n * 2] = receipts[n];
    }
    return 0;
}
