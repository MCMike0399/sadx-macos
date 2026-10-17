#include "guest/ninja_fog_reference.hpp"
#include <cstring>
#include <iostream>
#include <stdexcept>
using namespace sadx;
namespace {
void require(bool value, const char *why) {
    if (!value)
        throw std::runtime_error(why);
}
float from_bits(std::uint32_t bits) {
    float value;
    std::memcpy(&value, &bits, 4);
    return value;
}
struct Fixture {
    NinjaQueueConsumerState state;
    NinjaFogDeviceState device;
    NinjaFogTable table{};
    NinjaRenderState renderer;
    Fixture() {
        state.fog_supported_at_3d128a4 = 1;
        device.end_index_at_3d0fce0 = 0;
        device.start_index_at_3d12950 = 1;
        device.start_value_at_3d0fac0 = 0;
        device.end_value_at_3d12894 = 0;
        for (unsigned i = 0; i < 128; ++i) {
            device.w_at_3d12a00[i] = 1;
            device.reciprocal_at_3d12c00[i] = 1;
            table[i] = 0;
        }
        table[0] = 0.9F;
        table[1] = 0;
        renderer.device_at_3d128b0 = std::make_shared<NinjaRenderDevice>(NinjaRenderDevice{1});
        renderer.set_render_state = [](const auto &) { return NinjaRenderCallResult{true, {}}; };
    }
    NinjaFogDeviceOwners owners() { return {state, device, &renderer}; }
};
void test() {
    // Use set_table to enter apply_table without exposing another public reference entry.
    for (bool first : {true, false}) {
        Fixture fixture;
        X87Context fp(0x7f);
        auto owners = fixture.owners();
        fixture.device.reciprocal_at_3d12c00[first ? 1 : 0].reset();
        const auto result = ninja_fog_set_table_794e40(fixture.table, owners, fp);
        const auto expected = first ? 0x7949f8U : 0x794a0eU;
        require(result.failure == NinjaFogFailure::unknown_fact, "missing reciprocal rejected");
        require(result.source_boundary == expected && result.last_fp_site == expected,
                "reciprocal source marker precedes optional read");
    }
    {
        Fixture fixture;
        X87Context fp(0x7f);
        auto owners = fixture.owners();
        for (auto &value : fixture.table)
            value = 1.0F;
        fixture.table[127].reset();
        const auto result = ninja_fog_set_table_794e40(fixture.table, owners, fp);
        require(result.failure == NinjaFogFailure::unknown_fact &&
                    result.source_boundary == 0x794e98,
                "missing end factor retains published indices");
        require(result.last_fp_site == 0x794e98 && fixture.device.end_index_at_3d0fce0 == 127 &&
                    fixture.device.start_index_at_3d12950 == 126,
                "end load marker and partial stores");
    }
    {
        Fixture fixture;
        X87Context fp(0x7f);
        auto owners = fixture.owners();
        fixture.device.end_index_at_3d0fce0 = 128;
        const auto result =
            ninja_fog_generate_table_787260(fixture.table, -65536, -65535, owners, fp);
        require(result.failure == NinjaFogFailure::index_out_of_range &&
                    result.source_boundary == 0x7949f8 && result.last_fp_site == 0x7949f8,
                "out-of-range reciprocal retains pre-read marker");
    }
    {
        Fixture fixture;
        X87Context fp(0x7e);
        auto owners = fixture.owners();
        fixture.table[126] = from_bits(0x7f800001);
        const auto result = ninja_fog_set_table_794e40(fixture.table, owners, fp);
        require(result.failure == NinjaFogFailure::fp_failed &&
                    result.source_boundary == 0x794e70 && result.last_fp_site == 0x794e70,
                "unmasked SNaN stops at trailing load");
    }
    {
        Fixture fixture;
        X87Context fp(0x7f);
        auto owners = fixture.owners();
        fixture.renderer.device_at_3d128b0 = std::shared_ptr<NinjaRenderDevice>{};
        fixture.state.fog_enabled_at_3d12798 = 1;
        const auto result = ninja_fog_initialize_794ec0(owners, {1U << 20, 0U}, fp);
        require(result.failure == NinjaFogFailure::render_failed &&
                    result.source_boundary == 0x794ed2 && fixture.state.fog_enabled_at_3d12798 == 0,
                "known null stops after enabled store");
    }
}
} // namespace
int main() {
    try {
        test();
        std::cout << "Fog diagnostics: lazy reciprocals, partial edges, unmasked load and "
                     "known-null order pass\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
