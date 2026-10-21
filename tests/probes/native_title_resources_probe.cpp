#include "game/title_menu.hpp"
#include "cpu/x87_math.hpp"
#include <algorithm>
#include <cstring>
namespace menu = sadx::title_menu;
namespace {
struct Stop {};
struct Probe : menu::MenuTextureCalls, menu::FadeCalls {
    menu::MenuTextureState &state;
    std::uint32_t *out;
    std::uint32_t stop, count = 0;
    Probe(menu::MenuTextureState &owner, std::uint32_t *result, std::uint32_t stop_at)
        : state(owner), out(result), stop(stop_at) {}
    void event(std::uint32_t kind, std::uint32_t value) {
        const auto offset = 8 + count * 5;
        out[offset] = kind;out[offset+1] = value;
        out[offset+2] = state.loaded[0];out[offset+3] = state.loaded[1];
        out[offset+4] = static_cast<std::uint32_t>(state.ready);
        if (++count == stop) throw Stop{};
    }
    void load_menu_texture(menu::MenuTextureId id) override { event(2,id.value); }
    void disable_alpha_test() override { event(1,0); }
};
}
extern "C" void sadx_native_title_resources_probe(const std::uint32_t *input, std::uint32_t *out) {
    std::fill(out,out+256,0);
    menu::MenuTextureState textures{{input[1],input[2]},static_cast<std::int32_t>(input[3])};
    menu::ModeTextureLists lists;
    for (std::size_t mode = 0; mode < lists.size(); ++mode) {
        const auto offset = 16+mode*31;
        for (std::size_t i = 0; i < input[offset]; ++i) lists[mode].push_back({input[offset+1+i]});
    }
    menu::SequenceWork sequence;
    sequence.old_mode = static_cast<menu::MenuMode>(input[4]);
    menu::MainMenuWork main;
    main.presentation.previous = static_cast<menu::MenuMode>(input[7]);
    sadx::X87Context fp{127};
    Probe calls(textures,out,input[9]);
    float progress;std::memcpy(&progress,input+6,4);
    try {
        switch (input[0]) {
        case 0: menu::load_mode_textures(static_cast<menu::MenuMode>(input[5]),textures,lists,sequence,calls);break;
        case 1: menu::set_main_menu_previous(main);break;
        case 2: out[0]=menu::interpolate_colors(input[7],input[8],progress,fp);break;
        case 3: out[0]=menu::fade_out_color(progress,calls,fp);break;
        case 4: out[0]=menu::fade_in_color(progress,calls,fp);break;
        }
    } catch (const Stop &) { out[5] = 1; }
    out[1]=textures.loaded[0];out[2]=textures.loaded[1];out[3]=static_cast<std::uint32_t>(textures.ready);
    out[4]=static_cast<std::uint32_t>(main.presentation.previous);out[6]=fp.exception_flags();out[7]=calls.count;
}
