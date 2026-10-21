#include "game/title_menu.hpp"
#include <algorithm>
#include <cstring>
using namespace sadx;
namespace menu = sadx::title_menu;
namespace {
TaskHandle handle(std::uint32_t id) {
    if (!id) return {};
    auto owner = std::make_shared<std::uint32_t>(id);
    return TaskHandle(owner, reinterpret_cast<Task *>(owner.get()));
}
std::uint32_t identity(const TaskHandle &task) {
    return task ? *reinterpret_cast<const std::uint32_t *>(task.get()) : 0;
}
struct Stop {};
struct Probe : menu::MenuFactoryCalls {
    const std::uint32_t *input;
    std::uint32_t *out;
    std::uint32_t calls = 0, cursor = 64;
    explicit Probe(const std::uint32_t *in, std::uint32_t *result) : input(in), out(result) {}
    void trace(std::uint32_t kind, std::uint32_t a, std::uint32_t b = 0, std::uint32_t c = 0) {
        for (auto value : {kind,a,b,c}) out[cursor++] = value;
        if (++calls == input[9]) throw Stop{};
    }
    TaskHandle create_menu_task(menu::MenuMode mode, std::uint32_t flags, std::uint32_t level) override {
        trace(1, flags, level, static_cast<std::uint32_t>(mode));
        if (input[8]) return {};
        out[3] = 1;
        return handle(1);
    }
    void install_menu_callbacks(menu::MenuMode, const TaskHandle &) override { out[4] = 1; }
    template<class Work> std::shared_ptr<Work> allocate(std::uint32_t size) {
        trace(2,size);
        out[2] = 1;
        out[6] = size;
        return std::make_shared<Work>();
    }
    std::shared_ptr<menu::CharacterSelectWork> allocate_character_select(const TaskHandle &) override {
        return allocate<menu::CharacterSelectWork>(56);
    }
    std::shared_ptr<menu::FileSelectWork> allocate_file_select(const TaskHandle &) override {
        return allocate<menu::FileSelectWork>(56);
    }
    std::shared_ptr<menu::MainMenuWork> allocate_main_menu(const TaskHandle &) override {
        return allocate<menu::MainMenuWork>(40);
    }
    std::shared_ptr<menu::ExplanationWork> allocate_explanation(const TaskHandle &) override {
        return allocate<menu::ExplanationWork>(32);
    }
    std::shared_ptr<menu::StageSelectWork> allocate_level_select(const TaskHandle &) override {
        return allocate<menu::StageSelectWork>(208);
    }
    std::shared_ptr<menu::StageSelectWork> allocate_subgame(const TaskHandle &) override {
        return allocate<menu::StageSelectWork>(208);
    }
    std::shared_ptr<menu::SoundTestWork> allocate_sound_test(const TaskHandle &) override {
        return allocate<menu::SoundTestWork>(72);
    }
    std::shared_ptr<menu::OptionWork> allocate_options(const TaskHandle &) override {
        return allocate<menu::OptionWork>(84);
    }
    std::shared_ptr<menu::EmblemWork> allocate_emblem(const TaskHandle &) override {
        return allocate<menu::EmblemWork>(60);
    }
    std::shared_ptr<menu::TvSettingsWork> allocate_tv_settings(const TaskHandle &) override {
        return allocate<menu::TvSettingsWork>(56);
    }
    std::shared_ptr<menu::InternetDemoWork> allocate_internet_demo(const TaskHandle &) override {
        return allocate<menu::InternetDemoWork>(40);
    }
};
void presentation(std::uint32_t *out, const menu::MenuPresentation &work, bool main = false) {
    out[8] = static_cast<std::uint32_t>(work.phase);
    out[9] = static_cast<std::uint32_t>(work.previous);
    out[10] = static_cast<std::uint32_t>(work.next);
    std::memcpy(out + (main ? 12 : 11), &work.depth, 4);
    std::memcpy(out + (main ? 13 : 12), &work.fade_progress, 4);
    out[main ? 14 : 13] = work.color;
}
void stage_work(std::uint32_t *out, const menu::StageSelectWork &work) {
    presentation(out,work.presentation);
    out[14] = static_cast<std::uint32_t>(work.submode);
    std::memcpy(out+15,work.available_stages.data(),20);
    out[20] = 1;
    out[21] = work.available_count;
    for (std::size_t i = 0; i < work.stage_ids.size(); ++i) out[22+i] = work.stage_ids[i];
    out[55] = work.stage_count;
    out[56] = work.cursor_color;
    std::memcpy(out+57,&work.cursor_scale,4);
    out[58] = static_cast<std::uint32_t>(work.cursor_angle);
    out[59] = work.cursor_alpha | (work.display_stage << 8) | (work.selected_stage << 16);
}
}
extern "C" void sadx_native_title_factory_probe(const std::uint32_t *input, std::uint32_t *out) {
    std::fill(out,out+128,0);
    Probe calls(input,out);
    menu::MenuFactoryState state;
    auto initial = handle(2);
    state.character_select_task = initial; state.file_select_task = initial;
    state.main_menu_task = initial; state.explanation_task = initial;
    state.internet_demo_task = initial; state.puti_task = initial;
    state.level_select_task = initial; state.subgame_task = initial;
    state.sound_test_task = initial; state.options_task = initial;
    state.emblem_task = initial; state.tv_settings_task = initial;
    menu::StageSelectionData data;
    std::size_t offset = 16;
    for (auto *catalog : {&data.trial_stages,&data.subgame_stages}) {
        for (auto &stages : *catalog) {
            const auto count = input[offset++];
            for (std::size_t i = 0; i < 10; ++i) {
                const auto pair = input[offset++];
                if (i < count) stages.push_back({static_cast<std::uint8_t>(pair),
                                                 static_cast<std::uint8_t>(pair >> 8)});
            }
        }
    }
    for (auto &id : data.character_ids) id = static_cast<std::uint16_t>(input[offset++]);
    for (auto &id : data.display_stage_ids) id = static_cast<std::uint8_t>(input[offset++]);
    std::array<std::uint8_t,344> completed_stages{};
    std::memcpy(completed_stages.data(),input+256,344);
    const auto selected_character = static_cast<std::uint8_t>(input[10]);
    const auto include_all = static_cast<std::uint8_t>(input[11]);
    menu::StageSelectionInputs selection{selected_character,include_all,completed_stages,data};
    auto character = static_cast<std::uint8_t>(input[4]);
    auto selected_file = static_cast<std::uint8_t>(input[5]);
    auto ready = static_cast<std::uint8_t>(input[6]);
    auto file = static_cast<std::uint8_t>(input[6] >> 16);
    auto tutorial = static_cast<std::uint8_t>(input[7]);
    menu::MenuFactoryInputs inputs{character,selected_file,ready,file,tutorial};
    menu::ModeSelection transition{static_cast<menu::MenuMode>(input[1]),
        static_cast<menu::MenuMode>(input[2]),static_cast<menu::MenuMode>(input[3])};
    try {
        TaskHandle result;
        switch (input[0]) {
        case 1: result = menu::create_character_select(state,transition,inputs,calls); break;
        case 2: result = menu::create_file_select(state,transition,inputs,calls); break;
        case 3: result = menu::create_level_select(state,transition,selection,calls); break;
        case 4: result = menu::create_sound_test(state,transition,calls); break;
        case 7: result = menu::create_options(state,transition,calls); break;
        case 9: result = menu::create_subgame_list(state,transition,selection,calls); break;
        case 10: result = menu::create_emblem_results(state,transition,calls); break;
        case 12: result = menu::create_tv_settings(state,transition,calls); break;
        case 6: result = menu::create_main_menu(state,transition,inputs,calls); break;
        case 8: result = menu::create_explanation(state,transition,calls); break;
        case 11: result = menu::create_internet_demo(state,transition,calls); break;
        case 13: result = menu::create_puti(state,calls); break;
        default: return;
        }
        out[0] = identity(result);
    } catch (const Stop &) { out[7] = 1; }
    out[5] = selected_file;
    switch (input[0]) {
    case 1:
        out[1] = identity(state.character_select_task);
        if (const auto work = state.character_select_work) {
            presentation(out,work->presentation);
            out[14] = static_cast<std::uint32_t>(work->cursor_angle);
            out[15] = static_cast<std::uint32_t>(work->submode);
            out[16] = static_cast<std::uint32_t>(work->selected_character);
            out[17] = work->selected_file;
            std::memcpy(out+18,&work->angle_progress,4);
            out[19] = static_cast<std::uint16_t>(work->frames);
            out[20] = static_cast<std::uint32_t>(work->dialog_status);
            out[21] = work->wake;
        }
        break;
    case 2:
        out[1] = identity(state.file_select_task);
        if (const auto work = state.file_select_work) {
            presentation(out,work->presentation);
            std::memcpy(out+14,&work->cursor_scale,4);
            out[15] = work->cursor_color;
            out[16] = static_cast<std::uint32_t>(work->cursor_angle);
            out[17] = static_cast<std::uint32_t>(work->submode);
            out[18] = static_cast<std::uint32_t>(work->player_backup);
            out[19] = static_cast<std::uint32_t>(work->selected_character);
            out[20] = static_cast<std::uint16_t>(work->wait_frames) | (work->cursor_alpha << 16);
            std::memcpy(out+21,&work->offset_x,4);
        }
        break;
    case 3:
        out[1] = identity(state.level_select_task);
        if (const auto work = state.level_select_work) stage_work(out,*work);
        break;
    case 9:
        out[1] = identity(state.subgame_task);
        if (const auto work = state.subgame_work) stage_work(out,*work);
        break;
    case 4:
        out[1] = identity(state.sound_test_task);
        if (const auto work = state.sound_test_work) {
            presentation(out,work->presentation); out[14] = static_cast<std::uint32_t>(work->submode);
        }
        break;
    case 7:
        out[1] = identity(state.options_task);
        if (const auto work = state.options_work) {
            presentation(out,work->presentation); out[14] = static_cast<std::uint32_t>(work->submode);
            std::memcpy(out+21,&work->cursor_scale,4);
            out[22] = static_cast<std::uint32_t>(work->cursor_angle);
            out[23] = work->cursor_alpha;
            out[26] = work->content == menu::OptionContent::Menu ? 1 : 0;
            out[27] = static_cast<std::uint32_t>(work->item_count);
        }
        break;
    case 10:
        out[1] = identity(state.emblem_task);
        if (const auto work = state.emblem_work) {
            presentation(out,work->presentation); out[14] = static_cast<std::uint32_t>(work->submode);
            std::memcpy(out+15,work->selected_entries.data(),10);
            std::memcpy(out+19,&work->cursor_scale,4);
            out[21] = static_cast<std::uint32_t>(work->cursor_angle);
            out[22] = work->entry_count | (work->cursor_alpha << 8);
        }
        break;
    case 12:
        out[1] = identity(state.tv_settings_task);
        if (const auto work = state.tv_settings_work) {
            presentation(out,work->presentation); out[14] = static_cast<std::uint32_t>(work->submode);
        }
        break;
    case 6:
        out[1] = identity(state.main_menu_task);
        if (const auto work = state.main_menu_work) {
            presentation(out,work->presentation,true);
            out[11] = static_cast<std::uint32_t>(work->next_sub);
            out[15] = static_cast<std::uint32_t>(work->submode);
            out[16] = static_cast<std::uint32_t>(work->selected_menu);
            out[17] = static_cast<std::uint16_t>(work->frames) | (work->wake << 16);
        }
        break;
    case 8:
        out[1] = identity(state.explanation_task);
        if (const auto work = state.explanation_work) {
            presentation(out,work->presentation); out[14] = static_cast<std::uint32_t>(work->submode);
        }
        break;
    case 11:
        out[1] = identity(state.internet_demo_task);
        if (const auto work = state.internet_demo_work) {
            presentation(out,work->presentation); out[14] = static_cast<std::uint32_t>(work->submode);
        }
        break;
    case 13: out[1] = identity(state.puti_task); break;
    }
}
