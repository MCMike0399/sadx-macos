#include "guest/saved_fov_reference.hpp"
#include <array>
#include <cassert>
#include <cstdint>
int main() {
    using namespace sadx;
    view::SavedFovState state;
    reference::SavedFovReference transport;
    assert(state.value == 0 && !transport.export_value(state));
    for (auto bits : std::array<std::uint32_t, 7>{0, 1, 0x31c7, 0xdeadbeef, 0x80000000, 0xffffffff,
                                                  0x7fc12345}) {
        transport.import_value(state, bits);
        assert(view::saved_fov_bits(state) == bits && transport.export_value(state) == bits);
        const auto snapshot = state;
        const auto snapshot_transport = transport;
        transport.import_value(state, std::nullopt);
        assert(view::saved_fov_bits(state) == bits && !transport.export_value(state));
        assert(view::saved_fov_bits(snapshot) == bits &&
               snapshot_transport.export_value(snapshot) == bits);
        state.value = 0; // Actual source can store the same zero already present.
        transport.observe_write();
        assert(transport.export_value(state) == 0);
        view::set_saved_fov_bits(state, bits);
        assert(transport.export_value(state) == bits);
    }
    view::SavedFovState native;
    auto &message_saved_fov = native.value;
    auto &preview_saved_fov = native.value;
    message_saved_fov = 12743;
    assert(preview_saved_fov == 12743);
    preview_saved_fov = 0;
    assert(message_saved_fov == 0);
}
