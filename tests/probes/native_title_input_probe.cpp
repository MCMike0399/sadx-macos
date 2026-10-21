#include "game/title_menu.hpp"
#include <algorithm>
#include <cstring>
namespace menu = sadx::title_menu;
namespace {
struct Stop {};
struct Calls final : menu::TitleInputCalls {
    menu::TitleWork &work;
    menu::SequenceWork &sequence;
    menu::TitleInputState &state;
    const std::uint32_t *input;
    std::uint32_t *out;
    std::uint32_t count = 0;
    std::uint32_t &buttons;
    std::int32_t &ready;
    bool &bgm;
    Calls(menu::TitleWork &w, menu::SequenceWork &q, menu::TitleInputState &s,
          const std::uint32_t *i, std::uint32_t *o, std::uint32_t &b, std::int32_t &r, bool &music)
        : work(w), sequence(q), state(s), input(i), out(o), buttons(b), ready(r), bgm(music) {}
    void snapshot(std::uint32_t *o) {
        o[0]=static_cast<std::uint32_t>(work.submode);o[1]=static_cast<std::uint32_t>(work.timer);
        o[2]=work.blink_timer;o[3]=work.music_started;o[4]=state.loop_count;
        o[5]=state.title_logic_ticks;o[6]=static_cast<std::uint32_t>(state.pending_logic_ticks);
        o[7]=state.skip_title;o[8]=static_cast<std::uint32_t>(state.saving_disabled);
        o[9]=state.save_word_after_selection;o[10]=state.save_word_before_selection;o[11]=state.exit_requested;
        o[12]=state.demo_wait_ticks;o[13]=static_cast<std::uint32_t>(sequence.advance);
    }
    void call(std::uint32_t id, std::uint32_t arg=0) {
        auto *receipt=out+32+count*16;receipt[0]=id;receipt[1]=arg;snapshot(receipt+2);
        ++count;if (input[21] == count) throw Stop{};
        if (input[23] == id) {
            buttons=input[24];ready=static_cast<std::int32_t>(input[25]);bgm=input[26]!=0;
        }
    }
    void calculate_ticks() override {call(1);state.loop_count=input[20];}
    void request_menu_music(std::int32_t i) override {call(2,static_cast<std::uint32_t>(i));}
    std::int32_t key_pressed(std::int32_t key) override {call(3,static_cast<std::uint32_t>(key));return static_cast<std::int32_t>(input[17]);}
    std::uint32_t pressed_buttons() override {return buttons;}
    void transition_menu(menu::MenuMode mode) override {call(4,static_cast<std::uint32_t>(mode));}
    void play_title_confirm_sound() override {call(5);}
    std::int32_t audio_ready() override {call(6);return static_cast<std::int32_t>(input[18]);}
    std::uint32_t music_state() override {call(7);return input[19];}
};
}
extern "C" void sadx_native_title_input_probe(const std::uint32_t *i,std::uint32_t *o) {
    std::fill(o,o+160,0);
    menu::TitleWork w;w.submode=static_cast<menu::TitleSubmode>(i[0]);w.timer=static_cast<std::int32_t>(i[1]);
    w.blink_timer=i[2];w.music_started=i[3];w.next=static_cast<menu::MenuMode>(i[4]);
    auto loop=i[5],ticks=i[6];auto pending=static_cast<std::int32_t>(i[7]);
    auto skip=static_cast<std::uint8_t>(i[8]);auto saving=static_cast<std::int32_t>(i[9]);
    auto slot=i[10],dialog=i[11],exit=i[12],wait=i[16];
    auto ready=static_cast<std::int32_t>(i[14]);bool bgm=i[15]!=0;
    menu::TitleInputState state{loop,ticks,pending,skip,saving,slot,dialog,exit,ready,bgm,wait};
    menu::SequenceWork sequence;sequence.advance=static_cast<menu::MenuResult>(i[22]);
    auto buttons=i[13];
    Calls calls(w,sequence,state,i,o,buttons,ready,bgm);
    try {menu::update_title_input(w,sequence,state,calls);}catch(const Stop &) {o[15]=1;}
    calls.snapshot(o);o[14]=calls.count;
}
namespace {
struct SequenceCalls final : menu::SequenceCalls {
    void set_entrance(std::int32_t) override { throw Stop{}; }
    void release_menu_textures() override { throw Stop{}; }
    void finalize_selection() override { throw Stop{}; }
    void create_menu_objects(const menu::SequenceData *) override { throw Stop{}; }
    void destroy_sequence_child(const sadx::TaskHandle &) override { throw Stop{}; }
};
struct TransitionCalls final : menu::ModeTransitionCalls {
    menu::SequenceWork &sequence;
    std::uint32_t *out;
    bool stop;
    TransitionCalls(menu::SequenceWork &s,std::uint32_t *o,bool pause) : sequence(s),out(o),stop(pause) {}
    void stop_menu_music() override {
        out[2]=1;out[3]=sequence.waiting;out[4]=static_cast<std::uint32_t>(sequence.waiting_mode);
        if(stop)throw Stop{};
    }
};
}
extern "C" void sadx_native_title_transition_probe(const std::uint32_t *i,std::uint32_t *o) {
    std::fill(o,o+12,0);menu::SequenceWork sequence;
    sequence.mode=menu::MenuMode::Title;
    sequence.waiting=static_cast<std::uint8_t>(i[0]);sequence.waiting_mode=static_cast<menu::MenuMode>(i[1]);
    TransitionCalls calls(sequence,o,i[3]!=0);
    try {
        menu::request_menu_transition(sequence,static_cast<menu::MenuMode>(i[2]),calls);
        if(i[4]) {SequenceCalls next;std::int32_t demo=0;menu::update_sequence(sequence,sequence,demo,next);}
    }
    catch(const Stop &) {o[5]=1;}
    o[0]=sequence.waiting;o[1]=static_cast<std::uint32_t>(sequence.waiting_mode);
    o[6]=static_cast<std::uint16_t>(sequence.frames);o[7]=static_cast<std::uint32_t>(sequence.mode);
}
