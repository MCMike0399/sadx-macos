#include "game/title_menu.hpp"
#include "cpu/x87_math.hpp"
#include <algorithm>
#include <cstring>
namespace menu = sadx::title_menu;
extern "C" void sadx_native_title_dialog_probe(const std::uint32_t *input, std::uint32_t *out) {
    std::fill(out,out+32,0);
    sadx::X87Context fp{127};
    menu::DialogWork work;
    menu::DialogParameters parameters{static_cast<menu::DialogStyle>(input[5])};
    work.parameters = input[6] ? &parameters : nullptr;
    work.animation_angle = input[2];
    std::memcpy(&work.fade,input+3,4);
    work.phase = static_cast<std::int32_t>(input[4]);
    work.upper_color = input[7];work.lower_color = input[2];work.color = input[7];
    std::memcpy(&work.scale_x,input+8,4);std::memcpy(&work.scale_y,input+9,4);
    if (input[0]==0) {
        menu::update_dialog_animation(work.animation_value,work.animation_angle,work.fade,input[1],fp);
    } else if (input[0]==1) {
        out[0] = static_cast<std::uint32_t>(menu::update_dialog(work,work,input[1],fp));
    } else {
        out[0] = static_cast<std::uint32_t>(menu::draw_dialog(work,fp));
    }
    out[1] = *work.animation_angle;
    std::memcpy(out+2,&work.fade,4);
    out[3] = work.animation_value;out[4] = work.upper_color;out[5] = work.lower_color;
    out[6] = work.color;
    std::memcpy(out+7,&work.scale_x,4);std::memcpy(out+8,&work.scale_y,4);
    out[9] = static_cast<std::uint16_t>(work.repeat_frames) |
        (static_cast<std::uint32_t>(static_cast<std::uint16_t>(work.opening_frames)) << 16);
    std::memcpy(out+10,work.choice_ids.data(),16);
    out[14] = static_cast<std::uint32_t>(work.previous_choice);
    out[15] = fp.exception_flags();
}
