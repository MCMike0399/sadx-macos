#include "game/file_select.hpp"
#include <algorithm>
#include <cstring>
namespace files=sadx::file_select;
namespace {
struct Stop {};
struct Calls final:files::InitCalls {
    sadx::title_menu::FileSelectWork &work;
    files::InitState &state;
    const std::uint32_t *input;
    std::uint32_t *output;
    unsigned count=0;
    Calls(sadx::title_menu::FileSelectWork &w,files::InitState &s,const std::uint32_t *i,std::uint32_t *o):work(w),state(s),input(i),output(o){}
    void snapshot(std::uint32_t *o) {
        o[0]=static_cast<std::uint32_t>(work.submode);o[1]=state.selected_file;o[2]=state.card_file;o[3]=state.file_count;
        o[4]=state.save_word_before_selection;o[5]=static_cast<std::uint16_t>(state.menu_counter);o[6]=static_cast<std::uint32_t>(state.saving_disabled);o[7]=state.list_has_files;o[8]=state.selection_has_files;
        o[9]=static_cast<std::uint32_t>(work.cursor_angle);o[10]=work.cursor_color;o[11]=static_cast<std::uint32_t>(work.player_backup);o[12]=static_cast<std::uint32_t>(work.selected_character);
        std::memcpy(o+13,&work.cursor_scale,4);o[14]=static_cast<std::uint16_t>(work.wait_frames);o[15]=work.cursor_alpha;std::memcpy(o+16,&work.offset_x,4);
    }
    void call(std::uint32_t id,std::uint32_t argument=0) {auto *o=output+32+20*count;o[0]=id;o[1]=argument;snapshot(o+2);if(++count==input[14])throw Stop{};}
    void load_mode_textures(sadx::title_menu::MenuMode mode) override {call(1,static_cast<std::uint32_t>(mode));}
    void request_menu_music(std::int32_t i) override {call(2,static_cast<std::uint32_t>(i));}
    void request_menu_voice(std::int32_t i) override {call(3,static_cast<std::uint32_t>(i));}
    std::uint8_t count_save_files() override {call(4);return static_cast<std::uint8_t>(input[13]);}
    void load_selected_save() override {call(5);}
    void load_font() override {call(6,1);}
    void copy_font() override {call(7,1);}
    void apply_save() override {call(8);}
};
}
extern "C" void sadx_native_file_select_probe(const std::uint32_t *i,std::uint32_t *o) {
    std::fill(o,o+256,0);sadx::title_menu::FileSelectWork work;
    work.submode=static_cast<std::int32_t>(i[0]);auto selected=static_cast<std::uint8_t>(i[1]),card=static_cast<std::uint8_t>(i[2]),count=static_cast<std::uint8_t>(i[3]);auto backup=i[4];auto counter=static_cast<std::int16_t>(i[5]);auto disabled=static_cast<std::int32_t>(i[6]);auto list=static_cast<std::uint8_t>(i[7]),selection=static_cast<std::uint8_t>(i[8]);
    work.cursor_angle=static_cast<std::int32_t>(i[9]);work.cursor_color=i[10];work.player_backup=static_cast<std::int32_t>(i[11]);work.selected_character=static_cast<std::int32_t>(i[12]);std::memcpy(&work.cursor_scale,i+15,4);work.wait_frames=static_cast<std::int16_t>(i[16]);work.cursor_alpha=static_cast<std::uint8_t>(i[17]);std::memcpy(&work.offset_x,i+18,4);
    files::InitState state{selected,card,count,backup,counter,disabled,list,selection};Calls calls(work,state,i,o);
    try{files::initialize(work,state,calls);}catch(const Stop &){o[25]=1;}
    calls.snapshot(o);o[24]=calls.count;
}
