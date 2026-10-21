#include "game/title_menu.hpp"
#include <algorithm>
#include <cstring>
namespace menu = sadx::title_menu;
namespace {
void load_presentation(menu::MenuPresentation &work, const std::uint32_t *input) {
    work.phase = static_cast<menu::TitlePhase>(input[1]);
    std::memcpy(&work.fade_progress,input+4,4);
    work.color = input[5];
}
void store_presentation(const menu::MenuPresentation &work, std::uint32_t *out) {
    out[1] = static_cast<std::uint32_t>(work.phase);
    std::memcpy(out+2,&work.fade_progress,4);
    out[3] = work.color;
}
}
extern "C" void sadx_native_title_inactive_probe(const std::uint32_t *input, std::uint32_t *out) {
    std::fill(out,out+32,0);
    menu::SequenceWork sequence;
    sequence.mode = static_cast<menu::MenuMode>(input[2]);
    sequence.old_mode = static_cast<menu::MenuMode>(input[3]);
    auto cursor = static_cast<std::uint8_t>(input[6]);
    auto character = static_cast<std::uint8_t>(input[7]);
    menu::MenuCallbackResult result = menu::MenuCallbackResult::Complete;
    switch (input[0]) {
    case 1: {
        menu::CharacterSelectWork work; load_presentation(work.presentation,input);
        work.selected_character = static_cast<std::int32_t>(input[8]);
        result = menu::update_character_select(work,sequence,cursor,character);
        store_presentation(work.presentation,out);
        out[6] = static_cast<std::uint32_t>(work.selected_character);
        break;
    }
#define CHECK_MODE(number, Type, function) case number: { \
    menu::Type work; load_presentation(work.presentation,input); \
    result = menu::function(work,sequence); store_presentation(work.presentation,out); break; }
    CHECK_MODE(2,FileSelectWork,update_file_select)
    CHECK_MODE(3,StageSelectWork,update_level_select)
    CHECK_MODE(4,SoundTestWork,update_sound_test)
    CHECK_MODE(6,MainMenuWork,update_main_menu)
    CHECK_MODE(8,ExplanationWork,update_explanation)
    CHECK_MODE(9,StageSelectWork,update_subgame_list)
    CHECK_MODE(10,EmblemWork,update_emblem_results)
    CHECK_MODE(11,InternetDemoWork,update_internet_demo)
    CHECK_MODE(12,TvSettingsWork,update_tv_settings)
#undef CHECK_MODE
    case 7: {
        menu::OptionWork work; load_presentation(work.presentation,input);
        work.rotation_angle = input[8];
        auto load = [&](menu::MenuAnimation &animation, std::size_t index) {
            animation.frames = static_cast<std::uint8_t>(input[9+index]);
            animation.frame = static_cast<std::uint8_t>(input[9+index] >> 8);
        };
        load(work.eight_frame_cycle,0);load(work.twelve_frame_cycle,1);
        load(work.four_frame_cycle,2);load(work.first_long_cycle,3);
        load(work.second_long_cycle,4);load(work.five_frame_cycle,5);
        load(work.twenty_four_frame_cycle,6);
        result = menu::update_options(work,sequence); store_presentation(work.presentation,out);
        out[6] = work.rotation_angle;
        std::size_t index = 7;
        for (const auto &animation : {work.eight_frame_cycle,work.twelve_frame_cycle,
             work.four_frame_cycle,work.first_long_cycle,work.second_long_cycle,
             work.five_frame_cycle,work.twenty_four_frame_cycle}) {
            out[index++] = animation.frames | (animation.frame << 8);
        }
        break;
    }
    }
    out[0] = static_cast<std::uint32_t>(result);
    out[4] = cursor; out[5] = character;
}
extern "C" void sadx_native_title_draw_probe(const std::uint32_t *input, std::uint32_t *out) {
    std::fill(out,out+32,0);
    menu::MenuDrawResult result = menu::MenuDrawResult::Complete;
    switch (input[0]) {
#define DRAW_MODE(number, Type, function) case number: { \
    menu::Type work; work.presentation.color=input[5]; \
    result=menu::function(work); break; }
    DRAW_MODE(1,CharacterSelectWork,draw_character_select)
    DRAW_MODE(2,FileSelectWork,draw_file_select)
    DRAW_MODE(3,StageSelectWork,draw_level_select)
    DRAW_MODE(4,SoundTestWork,draw_sound_test)
    DRAW_MODE(6,MainMenuWork,draw_main_menu)
    DRAW_MODE(7,OptionWork,draw_options)
    DRAW_MODE(8,ExplanationWork,draw_explanation)
    DRAW_MODE(9,StageSelectWork,draw_subgame_list)
    DRAW_MODE(10,EmblemWork,draw_emblem_results)
    DRAW_MODE(12,TvSettingsWork,draw_tv_settings)
#undef DRAW_MODE
    case 11: {
        menu::InternetDemoWork work;
        work.presentation.color=input[5];work.submode=static_cast<std::int32_t>(input[8]);
        result=menu::draw_internet_demo(work);break;
    }
    }
    out[0]=static_cast<std::uint32_t>(result);
}
namespace {
struct PutiStop {};
struct PutiProbeCalls : menu::PutiCalls {
    std::uint32_t *output;
    bool stop;
    PutiProbeCalls(std::uint32_t *out, bool stop_at_allocation) : output(out), stop(stop_at_allocation) {}
    std::shared_ptr<menu::PutiWork> allocate_puti_work(const sadx::TaskHandle &) override {
        output[7] = 1;
        if (stop) throw PutiStop{};
        return std::make_shared<menu::PutiWork>();
    }
    void install_puti_callbacks(const sadx::TaskHandle &) override { output[5]=1;output[6]=1; }
};
}
extern "C" void sadx_native_title_puti_probe(const std::uint32_t *input, std::uint32_t *out) {
    std::fill(out,out+32,0);
    sadx::TaskScheduler scheduler;
    const auto task=scheduler.append_task(4,sadx::TaskScheduler::source_no_op).task;
    std::shared_ptr<menu::PutiWork> work;
    if (input[0]) {
        work=std::make_shared<menu::PutiWork>();work->phase=input[1];work->waiting_frames=input[2];
    }
    task->auxiliary_native_work=work;
    menu::SequenceWork sequence;sequence.mode=static_cast<menu::MenuMode>(input[3]);
    PutiProbeCalls calls(out,input[4]!=0);
    try { out[0]=static_cast<std::uint32_t>(menu::update_puti(task,sequence,calls)); }
    catch (const PutiStop &) { out[8]=1; }
    work=std::static_pointer_cast<menu::PutiWork>(task->auxiliary_native_work);
    if (work) { out[1]=1;out[2]=work->phase;out[3]=work->resource?1:0;out[4]=work->waiting_frames; }
}
