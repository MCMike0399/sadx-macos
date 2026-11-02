#include "engine/render_state_snapshot.hpp"
#include <algorithm>
#include <cstring>

extern "C" int sadx_native_render_state_snapshot(const std::uint32_t *input,
                                                 std::uint32_t *output) {
    using namespace sadx;
    render_snapshot::State saved;
    saved.control = input[4];
    saved.mask = input[5];
    saved.attributes = input[6];
    std::uint32_t control = input[1], mask = input[2], attributes = input[3];
    render_snapshot::View view{saved, control, mask, attributes};
    switch (input[0]) {
    case 0: render_snapshot::save_control(view); break;
    case 1: render_snapshot::load_control(view); break;
    case 2: render_snapshot::save_attributes(view); break;
    case 3: render_snapshot::load_attributes(view); break;
    case 4: render_snapshot::clear_attributes(view, input[7], input[8]); break;
    case 5: render_snapshot::set_control(view, input[7]); break;
    case 6:
        render_snapshot::set_object_callback(saved, input[7] ? render_snapshot::ObjectCallback::Gamma
                                                           : render_snapshot::ObjectCallback::None);
        break;
    default: return 0;
    }
    output[0] = control;
    output[1] = mask;
    output[2] = attributes;
    output[3] = saved.control;
    output[4] = saved.mask;
    output[5] = saved.attributes;
    output[6] = saved.object_callback == render_snapshot::ObjectCallback::Gamma ? 0x47f990 : 0;
    return 1;
}
