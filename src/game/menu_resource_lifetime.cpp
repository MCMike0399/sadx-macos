#include "game/menu_resource_lifetime.hpp"
#include <algorithm>

namespace sadx::menu_resource_lifetime {
void release_ids(const std::vector<title_menu::MenuTextureId> *ids, State &state, Services &services) {
    if (ids == nullptr)
        return;
    for (const auto id : *ids) {
        if (id.value >= state.catalog.front().size())
            throw Error(Boundary::TextureId, "menu release ID is outside the imported texture table");
        const auto word = id.value / 32;
        const auto mask = 1U << (id.value % 32);
        if ((state.textures.loaded[word] & mask) != 0) {
            state.textures.loaded[word] &= ~mask;
            if (state.language < 0 || static_cast<std::size_t>(state.language) >= state.catalog.size())
                throw Error(Boundary::Language, "menu release language is outside the imported table");
            services.release_texture(state.catalog[static_cast<std::size_t>(state.language)][id.value].textures);
        }
    }
}
void release_all(State &state, Services &services) {
    std::vector<title_menu::MenuTextureId> ids;
    for (std::uint32_t id = 0; id < 30; ++id)
        ids.push_back({id});
    release_ids(&ids, state, services);
}
void release_mode_textures(const title_menu::SequenceWork &sequence, State &state, Services &services) {
    const auto current = static_cast<std::int32_t>(sequence.mode);
    const auto old = static_cast<std::int32_t>(sequence.old_mode);
    const std::vector<title_menu::MenuTextureId> *keep = nullptr;
    if (current > 0 && static_cast<std::size_t>(current) < state.mode_textures.size())
        keep = &state.mode_textures[static_cast<std::size_t>(current)];
    if (old < 0 || static_cast<std::size_t>(old) >= state.mode_textures.size())
        throw Error(Boundary::Mode, "previous menu mode is outside the imported texture lists");
    std::vector<title_menu::MenuTextureId> ids;
    for (const auto id : state.mode_textures[static_cast<std::size_t>(old)]) {
        const bool retained = keep != nullptr && std::any_of(keep->begin(), keep->end(),
            [&](const auto candidate) { return candidate.value == id.value; });
        if (!retained) {
            if (ids.size() == 30)
                throw Error(Boundary::TemporaryCapacity, "menu release exceeds source temporary ID storage");
            ids.push_back(id);
        }
    }
    release_ids(&ids, state, services);
    state.textures.ready = 1;
}
void release_japanese_title(texture_lifetime::List &list, Services &services) {
    services.release_texture(&list);
}
namespace {
// NOTSA: retain the argument value across destructor changes to its canonical owner.
void destroy_if_present(const TaskHandle &owner, Services &services) {
    const auto task = owner;
    if (task)
        services.destroy_task(task);
}
// FUNCTION: SONIC 0x512520 DestroyCharacterSelectMenu
void destroy_character_select(title_menu::State &menus, Services &services) {
    destroy_if_present(menus.factories.character_select_task, services);
}
// FUNCTION: SONIC 0x5049c0 DestroyFileSelectMenu
void destroy_file_select(title_menu::State &menus, Services &services) {
    destroy_if_present(menus.factories.file_select_task, services);
}
// FUNCTION: SONIC 0x50b340 DestroyLevelSelectMenu
void destroy_level_select(title_menu::State &menus, Services &services) {
    destroy_if_present(menus.factories.level_select_task, services);
}
// FUNCTION: SONIC 0x511480 DestroySoundTestMenu
void destroy_sound_test(title_menu::State &menus, Services &services) {
    destroy_if_present(menus.factories.sound_test_task, services);
}
// FUNCTION: SONIC 0x50e440 DestroyTitleScreen
void destroy_title(title_menu::State &menus, Services &services) {
    destroy_if_present(menus.title_task, services);
}
// FUNCTION: SONIC 0x50bcf0 DestroyMainMenu
void destroy_main_menu(title_menu::State &menus, Services &services) {
    destroy_if_present(menus.factories.main_menu_task, services);
}
// FUNCTION: SONIC 0x5098b0 DestroyOptionsMenu
void destroy_options(title_menu::State &menus, Services &services) {
    destroy_if_present(menus.factories.options_task, services);
}
// FUNCTION: SONIC 0x50e2e0 DestroyExplanationMenu
void destroy_explanation(title_menu::State &menus, Services &services) {
    destroy_if_present(menus.factories.explanation_task, services);
}
// FUNCTION: SONIC 0x50a570 DestroySubgameMenu
void destroy_subgame(title_menu::State &menus, Services &services) {
    destroy_if_present(menus.factories.subgame_task, services);
}
// FUNCTION: SONIC 0x50e080 DestroyEmblemMenu
void destroy_emblem(title_menu::State &menus, Services &services) {
    destroy_if_present(menus.factories.emblem_task, services);
}
// FUNCTION: SONIC 0x50d4e0 DestroyInternetDemoMenu
void destroy_internet_demo(title_menu::State &menus, Services &services) {
    destroy_if_present(menus.factories.internet_demo_task, services);
}
// FUNCTION: SONIC 0x508510 DestroyTvSettingsMenu
void destroy_tv_settings(title_menu::State &menus, Services &services) {
    destroy_if_present(menus.factories.tv_settings_task, services);
}
// FUNCTION: SONIC 0x50c160 FreePutiMenu
void free_puti(title_menu::State &menus, Services &services) {
    const auto task = menus.factories.puti_task;
    services.free_task(task);
}
} // namespace
void destroy_mode_task(title_menu::MenuMode mode, title_menu::State &menus, Services &services) {
    switch (mode) {
    case title_menu::MenuMode::CharacterSelect:
        destroy_character_select(menus, services);
        break;
    case title_menu::MenuMode::FileSelect:
        destroy_file_select(menus, services);
        break;
    case title_menu::MenuMode::TrialActSelect:
        destroy_level_select(menus, services);
        break;
    case title_menu::MenuMode::SoundTest:
        destroy_sound_test(menus, services);
        break;
    case title_menu::MenuMode::Title:
        destroy_title(menus, services);
        break;
    case title_menu::MenuMode::TitleMenu:
        destroy_main_menu(menus, services);
        break;
    case title_menu::MenuMode::OptionSelect:
        destroy_options(menus, services);
        break;
    case title_menu::MenuMode::Explanation:
        destroy_explanation(menus, services);
        break;
    case title_menu::MenuMode::MinigameSelect:
        destroy_subgame(menus, services);
        break;
    case title_menu::MenuMode::EmblemView:
        destroy_emblem(menus, services);
        break;
    case title_menu::MenuMode::InternetDemo:
        destroy_internet_demo(menus, services);
        break;
    case title_menu::MenuMode::TvSettings:
        destroy_tv_settings(menus, services);
        break;
    case title_menu::MenuMode::Puti:
        free_puti(menus, services);
        break;
    default:
        throw Error(Boundary::TaskMode, "menu task mode has no recovered cleanup-table entry");
    }
}
void destroy_dialog(title_menu::State &menus, Services &services) {
    const auto task = menus.dialog_task;
    if (task) {
        services.destroy_task(task);
        menus.dialog_task.reset();
    }
}
void destroy_sequence(const title_menu::SequenceWork &work, std::int32_t &demo_mode,
                       title_menu::State &menus, State &textures, Services &services) {
    demo_mode = 0;
    release_all(textures, services);
    // The source captures this task's work before release, but reads its data
    // member only AFTER the texture callbacks return.
    const auto *data = work.data;
    if (data == nullptr)
        throw Error(Boundary::SequenceData, "menu sequence destructor has no source record list");
    if (data->born_mode != title_menu::MenuMode::None) {
        destroy_mode_task(data->born_mode, menus, services);
        for (const auto &entry : data->following) {
            if (entry.mode == title_menu::MenuMode::None)
                break;
            destroy_mode_task(entry.mode, menus, services);
        }
    }
    destroy_dialog(menus, services);
}
} // namespace sadx::menu_resource_lifetime
