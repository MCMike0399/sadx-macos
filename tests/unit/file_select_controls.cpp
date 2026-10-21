#include "game/file_select.hpp"
#include "math/ninja_sine_table.hpp"
#include <cassert>
#include <vector>
namespace files=sadx::file_select;
namespace title=sadx::title_menu;
struct Calls final:files::ControlCalls {
    files::ControlState &state;
    std::uint32_t buttons=0;
    std::vector<int> events;
    explicit Calls(files::ControlState &borrowed):state(borrowed){}
    std::uint32_t pressed_buttons()override{return buttons;}
    void request_dialog(std::int32_t)override{assert(false);}
    void prepare_new_save()override{assert(false);}
    void apply_save()override{events.push_back(3);}
    void request_save()override{assert(false);}
    std::int32_t close_warning()override{assert(false);return 0;}
    void load_selected_save()override{
        assert(state.card_file==2&&state.displayed_save==2&&state.selected_file==1);
        events.push_back(6);
    }
    void transition_menu(title::MenuMode mode)override{assert(mode==title::MenuMode::Title);events.push_back(7);}
    void play_navigation_sound()override{events.push_back(8);}
    void play_cancel_sound()override{events.push_back(9);}
};
int main(){
    std::uint8_t count=2,selected=1,displayed=1,card=1,selected_card=1,pending=1,alert=0,new_file=0;
    std::int16_t first=0;std::int32_t ready=1;std::uint32_t ticks=1;std::uint16_t character=8;
    files::ControlState state{count,selected,displayed,card,first,selected_card,pending,alert,ready,ticks,character,new_file};
    Calls calls(state);title::FileSelectWork work;work.presentation.previous=title::MenuMode::Title;
    sadx::X87Context fp{127};sadx::NinjaMath math(sadx::ninja_sine_table(),fp);
    files::update_controls(work,state,calls,math,fp);
    assert(work.submode==1&&work.cursor_angle==728&&calls.events.empty());
    calls.buttons=32;files::update_controls(work,state,calls,math,fp);
    assert((calls.events==std::vector<int>{8,6,3}));assert(selected==2&&card==2&&displayed==2&&selected_card==2&&pending==0);
    calls.events.clear();calls.buttons=2;work.presentation.previous=title::MenuMode::OptionSelect;
    files::update_controls(work,state,calls,math,fp);assert(work.submode==1&&calls.events.empty());
    work.presentation.previous=title::MenuMode::Title;
    files::update_controls(work,state,calls,math,fp);assert(work.submode==9&&calls.events.empty());
    files::update_controls(work,state,calls,math,fp);assert(work.submode==10&&(calls.events==std::vector<int>{7,9}));
}
