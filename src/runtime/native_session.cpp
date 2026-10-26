#include "runtime/native_session.hpp"
#include "runtime/movie_scene.hpp"
#include "runtime/native_display.hpp"
#include "runtime/native_title_scene.hpp"
#include "game/level_sounds.hpp"
#include <iostream>
#include <cstring>
#include <stdexcept>

namespace sadx::native {
namespace {
// NOTSA: imported handles are resolved before the session starts, never synthesized.
bootstrap::SonicActions actions(const std::shared_ptr<const character_pack::CharacterPack> &pack) {
    if (!pack)
        throw std::runtime_error("native startup requires an imported character pack");
    return {pack->action(character_pack::StartupAction::ClearPose),
            pack->action(character_pack::StartupAction::ClearPoseTransition)};
}
} // namespace

Session::Session(LogoOutput &output, std::shared_ptr<const character_pack::CharacterPack> pack,
                 LogoFrame frame, X87Context &fp, frame::ModeState &mode,
                 const CharacterTextureAssets &textures, std::int32_t frame_rate_setting,
                 std::function<void(std::uint32_t)> stop)
    : bootstrap_({card_result_, global_timer_, rings_, flow_.movie_number, game_mode_}),
      character_pack_(std::move(pack)), actions_(actions(character_pack_)),
      character_scene_(*character_pack_),
    services_{[this](std::uint16_t character) { unload_character_textures(character); },
                std::move(stop)}, timing_(mode),
      logo_(output, timing_, renderer_, save_, rings_, frame, fp), texture_assets_(textures) {
    for (std::size_t character = 0; character < texture_assets_.size(); ++character) {
        const auto &declarations = texture_assets_[character];
        if (declarations.empty())
            throw std::runtime_error("missing imported character texture declarations");
        auto &lists = character_textures_[character];
        lists.resize(declarations.size());
        for (std::size_t index = 0; index < lists.size(); ++index) {
            if (declarations[index].texture_count == 0)
                throw std::runtime_error("empty imported character texture declaration");
            lists[index].entries.resize(declarations[index].texture_count);
            character_texture_lists_[character].lists.push_back(&lists[index]);
        }
        character_texture_bindings_[character] = &character_texture_lists_[character];
    }
    apply_frame_rate_setting(frame_rate_setting);
    // OnInitGameLoop consumes the earlier WinMain profile setting before its first handler call.
    frame::set_frame_rate_mode(1, 1, timing_, renderer_);
}

void Session::attach_logo_scaling(Display &display) { logo_.attach_ui_scaling(display.ui_scaling()); }
void Session::advance() { flow::advance_game_mode(flow_, *this); }
void Session::update_sounds() {
    if (!sound_server_calls_)
        throw flow::UnrecoveredGameFlowCall("native_sound_server_resources");
    auto queue = sound_queue();
    sound_effects::ServerView state{
        queue, sound_tasks_, sound_bank_table_, {opponent_character_, opponent_task_}, sound_listener_};
    sound_effects::update_sound_server(state, *sound_server_calls_);
}
void Session::initialize_game() {
    bootstrap::start(bootstrap_, save_, actions_, stage_, services_);
}
void Session::initialize_demo_state() {
    bootstrap::initialize_advertise(bootstrap_.menu, flow_.tutorial_variant);
}
std::uint32_t Session::startup_status() { return bootstrap::loading_status(); }
std::int32_t Session::startup_readiness() {
    return static_cast<std::int32_t>(bootstrap::loading_ready());
}
void Session::initialize_sega_logo() { logo_.initialize(); }
std::uint32_t Session::run_logos() { return static_cast<std::uint32_t>(logo_.advance()); }
void Session::load_sega_logo() { logo_.load(); }
void Session::set_frame_rate_mode(std::int32_t render_ratio, std::int32_t logic_ratio) {
    frame::set_frame_rate_mode(render_ratio, logic_ratio, timing_, renderer_);
}
void Session::set_wait_vsync_count(std::int32_t count) {
    frame::set_frame_interval_multiplier(count, renderer_);
}
void Session::draw_screen_fade() {
    if (logo_.fade_color() >> 24)
        throw flow::UnrecoveredGameFlowCall("native_screen_fade_queue");
}
void Session::increment_frame_counter() {
    global_timer_ += static_cast<std::uint32_t>(timing_.logic_increment);
}
std::uint32_t Session::initialize_level_game() {
    return bootstrap::initialize_game(bootstrap_, save_, stage_, services_);
}
void Session::setup_character(std::uint32_t character) {
    bootstrap::setup_character(static_cast<std::uint16_t>(character), bootstrap_,
                                save_, actions_, services_);
}
void Session::reset_player_work() { bootstrap::reset_rings(bootstrap_); }
std::int32_t Session::run_movie(std::uint32_t movie_id) {
    if (!movie_)
        throw flow::UnrecoveredGameFlowCall("native_movie_scene");
    return movie_->play(movie_id);
}
cleanup::TextureState Session::texture_state() {
    return {common_textures_, regular_textures_, character_texture_bindings_,
            character_textures_loaded_};
}
cleanup::LevelState Session::level_state() {
    return {set_entry_count_, set_entries_, set_files_, camera_files_,
            level_destructor_, level_destructor_context_, debug_message_};
}
void Session::release_texture_list(sprite::TextureList &list) {
    if (!display_)
        throw flow::UnrecoveredGameFlowCall("native_texture_release_owner");
    display_->release_texture_list(list);
}
void Session::unload_character_textures(std::uint16_t character) {
    auto state = texture_state();
    cleanup::unload_character_textures(character, state,
                                      [this](auto &list) { release_texture_list(list); });
}
void Session::release_buffer(void *) {
    throw flow::UnrecoveredGameFlowCall("native_heap_buffer_release");
}
void Session::clear_game() {
    if (!display_)
        throw flow::UnrecoveredGameFlowCall("native_cleanup_display");
    cleanup::ClearServices services{
        [this] { free_regular_object_texlists(); },
        [this] { free_character_texlists(); },
        [this] { display_->clear_texture_binding(); }
    };
    cleanup::clear(services);
}
void Session::reset_game() {
    if (!display_ || !audio_configuration_)
        throw flow::UnrecoveredGameFlowCall("native_cleanup_owners");
    cleanup::State state{reset_status_, editor_status_, temporary_buffer_, secondary_buffer_,
                         bootstrap_.random_seed};
    const auto stop_pcm = [this](std::int32_t channel) {
        if (!audio::stop_pcm(pcm_, channel))
            throw std::runtime_error("native PCM channel cleanup failed");
    };
    cleanup::Services services{
        [this] {
            cleanup::resume_voice_volume(audio_configuration_->voice_attenuation,
                                         audio_configuration_->saved_voice_attenuation);
        },
        [this](std::uint32_t mode) { display_->set_depth_compare_mode(mode); },
        services_.stop_vibration,
        [this] { display_->set_blank_queue_mode(); },
        [this] { cleanup::reset_song_state(tracks_); },
        [this] { cleanup::reset_current_voice(tracks_); },
        [this, &stop_pcm] {
            cleanup::AudioQueueView queue{sound_.records, sound_tasks_};
            cleanup::stop_all_sounds(queue, stop_pcm);
        },
        stop_pcm,
        [this](std::uint32_t mode) {
            if (!audio::close_media(media_, mode))
                throw std::runtime_error("native media cleanup failed");
        },
        [this] { display_->setup_screen(flow_.pause_state); },
        [this](void *message) {
            cleanup::display_message(static_cast<const cleanup::Message *>(message), messages_,
                                     bootstrap_.saved_option_gate, timing_.pending_logic_ticks);
        },
        [this] { logo_.initialize_heap(); },
        [this] { logo_.initialize_tasks(); },
        [this](std::uint32_t mode) { run_level_destructor(mode); },
        [this] {
            // The recovered reset snapshots list presence; the session retains its typed owner.
            const void *list = list_data_.get();
            cleanup::LoopState state{reset_work_, secondary_loop_count_, list};
            cleanup::reset_loop_state(state, [this] { file_select::free_save_list(list_data_); });
        },
        [this](void *buffer) { release_buffer(buffer); },
        [this] { display_->enable_fog(); }
    };
    cleanup::reset(state, services);
}
void Session::load_save() {
    cleanup::LoadSaveState state{
        global_timer_, bootstrap_.character, bootstrap_.lives, soft_reset_.metal_sonic, saved_stage_,
        bootstrap_.sequence_variables, bootstrap_.cutscene_flags, bootstrap_.section_work.data() + 12,
        bootstrap_.sequences, bootstrap_.completed_stages, missions_, rumble_enabled_, market_rings_};
    cleanup::load_save(save_, state);
}
void Session::set_stage_number(std::uint8_t stage, std::uint8_t act) {
    cleanup::StageState state{stage_, act_, previous_stage_, previous_act_};
    cleanup::set_stage_number(stage, act, state);
}
void Session::prepare_title_initialization() {
    const auto logo_state = logo_.startup_state();
    cleanup::TitleState state{
        logo_state.demo_mode, logo_state.demo_event, logo_state.demo_reset, demo_enabled_,
        demo_paused_, auxiliary_, sequence_running_, hide_hud_, flow_.pause_state,
        bootstrap_.story_event_mode, save_, save_backup_};
    cleanup::TitleServices services{
        [this] { logo_.initialize_heap(); },
        [this] { logo_.initialize_tasks(); },
        [this] { load_save(); },
        [this](std::uint32_t color) { logo_.set_fade_color(color); },
        [this](std::uint8_t stage, std::uint8_t act) { set_stage_number(stage, act); }
    };
    cleanup::prepare_title(state, services);
}
void Session::run_level_destructor(std::uint32_t mode) {
    auto state = level_state();
    cleanup::run_level_destructor(mode, state, [this](void *buffer) { release_buffer(buffer); });
}
void Session::free_regular_object_texlists() {
    auto state = texture_state();
    cleanup::release_regular_object_textures(
        state, [this](auto &list) { release_texture_list(list); });
}
void Session::free_character_texlists() {
    auto state = texture_state();
    cleanup::release_character_textures(state, [this](auto &list) { release_texture_list(list); });
}
void Session::release_all_textures() {
    if (!display_)
        throw flow::UnrecoveredGameFlowCall("native_texture_release_owner");
    display_->release_all_textures();
}
void Session::prepare_title_assets() {
    using character_pack::TitleAction;
    const title_menu::CharacterActions source{
        character_pack_->action(TitleAction::SonicClearPose),
        character_pack_->action(TitleAction::SonicClearPoseTransition),
        character_pack_->action(TitleAction::KnucklesClearPose),
        character_pack_->action(TitleAction::KnucklesClearPoseTransition),
        character_pack_->action(TitleAction::BigSittingToFish),
        character_pack_->action(TitleAction::BigPreparingToCast),
        character_pack_->action(TitleAction::BigCast)};
    title_menu::ActionBindings destination{
        bootstrap_.actions.clear_pose, bootstrap_.actions.clear_pose_transition,
        knuckles_clear_, knuckles_transition_, big_sit_, big_sit_repeat_,
        big_prepare_, big_cast_, big_cast_repeat_};
    title_menu::prepare_title_assets(destination, source);
}
void Session::select_sound_bank(std::int32_t index) {
    if (!bank_catalog_ || !audio_configuration_)
        throw flow::UnrecoveredGameFlowCall("native_sound_bank_owners");
    sound_bank::select(index, *bank_catalog_, banks_, pcm_.initialized,
                       audio_configuration_->voice_enabled, cd_sound_prefix_, bank_services_);
}
void Session::load_level_sounds() {
    level_sounds::State state{
        stage_, act_, bootstrap_.character, voice_language_, soft_reset_.metal_sonic,
        sound_.paused, sound_.positional_paused, sound_queue_reset_word_, sound_bank_reset_word_};
    level_sounds::Services services{
        [this](std::int32_t index) { select_sound_bank(index); },
        [this] {
            cleanup::AudioQueueView queue{sound_.records, sound_tasks_};
            return cleanup::stop_all_sounds(queue, [this](std::int32_t channel) {
                if (!audio::stop_pcm(pcm_, channel))
                    throw std::runtime_error("native PCM channel cleanup failed");
            });
        }
    };
    level_sounds::load(state, services);
}
void Session::attach_regular_textures(RegularTextureAssets assets) {
    if (regular_texture_state_)
        throw std::runtime_error("native regular texture owners already initialized");
    regular_assets_ = std::move(assets);
    regular_resource_lists_.resize(regular_assets_.lists.size());
    for (std::size_t index = 0; index < regular_assets_.lists.size(); ++index)
        regular_resource_lists_[index] = bind_texture_list(regular_assets_.lists[index]);
    regular_groups_.resize(regular_assets_.groups.size());
    const auto bind = [this](const RegularTextureBindingAsset &source) {
        return regular_textures::Binding{
            source.archive,
            source.texture_list ? &regular_resource_lists_.at(*source.texture_list) : nullptr};
    };
    for (std::size_t index = 0; index < regular_groups_.size(); ++index) {
        const auto &source = regular_assets_.groups[index];
        if (source.empty())
            throw std::runtime_error("regular texture group lacks its common descriptor");
        auto &group = regular_groups_[index];
        group.common = bind(source.front());
        for (std::size_t entry = 1; entry < source.size(); ++entry)
            group.regular.push_back(bind(source[entry]));
    }
    for (std::size_t index = 0; index < regular_catalog_.size(); ++index) {
        const auto group = regular_assets_.entries[index];
        regular_catalog_[index] = group ? &regular_groups_.at(*group) : nullptr;
    }
    regular_texture_state_ = std::make_unique<regular_textures::State>(
        regular_textures::State{loop_.resync, text_language_, common_textures_, regular_textures_,
                                regular_assets_.localizations, {}, {}});
    if (!display_)
        throw flow::UnrecoveredGameFlowCall("native_regular_texture_display");
    display_->output().bind_archive_loader([this](std::string_view name, sprite::TextureList &list) {
        auto services = regular_texture_services();
        regular_textures::load_archive(name, &list, *regular_texture_state_, services);
    });
}
void Session::attach_font(FontAssets assets) {
    if (font_assets_)
        throw std::runtime_error("native font resources are already attached");
    font_assets_ = std::make_unique<FontAssets>(std::move(assets));
    font_textures_ = bind_texture_list(font_assets_->texture_list);
    std::array<float, 4> dimensions{};
    static_assert(sizeof(dimensions) == sizeof(font_assets_->dimension_words));
    std::memcpy(dimensions.data(), font_assets_->dimension_words.data(), sizeof(dimensions));
    font_info_ = {dimensions[0], dimensions[1], dimensions[2], dimensions[3], &font_textures_,
                   &font_assets_->proportions, font_assets_->colors};
}
regular_textures::Services Session::regular_texture_services() {
    if (!display_)
        throw flow::UnrecoveredGameFlowCall("native_regular_texture_display");
    return {
        [this](std::string_view name) {
            return regular_textures::ArchiveResult{1, display_->output().load_pvm_archive(name)};
        },
        [this](sprite::TextureList &list) {
            return display_->output().load_individual_textures(list);
        },
        [](std::string_view message) { std::cerr << message; }
    };
}
void Session::load_regular_object_textures(std::uint32_t index) {
    if (!regular_texture_state_)
        throw flow::UnrecoveredGameFlowCall("native_regular_texture_declarations");
    auto services = regular_texture_services();
    regular_textures::load_regular(index, regular_catalog_, *regular_texture_state_, services);
}
void Session::attach_menu_textures(const MenuTextureAssets &assets) {
    if (menu_textures_attached_ || !regular_texture_state_)
        throw std::runtime_error("native menu textures require initialized regular resource owners");
    menu_resource_lists_.resize(assets.lists.size());
    menu_list_bindings_.resize(assets.lists.size());
    for (std::size_t index = 0; index < assets.lists.size(); ++index) {
        const auto &source = assets.lists[index];
        if (source.regular_texture_list) {
            auto &list = regular_resource_lists_.at(*source.regular_texture_list);
            if (list.entries.size() != source.declaration.texture_count)
                throw std::runtime_error("aliased menu texture declaration count differs");
            menu_list_bindings_[index] = &list;
        } else {
            menu_resource_lists_[index] = bind_texture_list(source.declaration);
            menu_list_bindings_[index] = &menu_resource_lists_[index];
        }
    }
    const auto bind = [this](const RegularTextureBindingAsset &source) {
        return regular_textures::Binding{
            source.archive, source.texture_list ? menu_list_bindings_.at(*source.texture_list) : nullptr};
    };
    for (std::size_t language = 0; language < menu_texture_catalog_.size(); ++language)
        for (std::size_t index = 0; index < menu_texture_catalog_[language].size(); ++index)
            menu_texture_catalog_[language][index] = bind(assets.languages[language][index]);
    japanese_title_texture_ = bind(assets.japanese_title);
    menu_textures_attached_ = true;
}
const regular_textures::Binding &Session::menu_texture_binding(std::uint32_t index) const {
    if (!menu_textures_attached_ || text_language_ < 0 ||
        static_cast<std::size_t>(text_language_) >= menu_texture_catalog_.size())
        throw flow::UnrecoveredGameFlowCall("native_menu_texture_language");
    return menu_texture_catalog_.at(static_cast<std::size_t>(text_language_)).at(index);
}
void Session::load_menu_texture(std::uint32_t index) {
    const auto &binding = menu_texture_binding(index);
    if (!binding.archive)
        throw flow::UnrecoveredGameFlowCall("native_menu_texture_archive_name");
    auto services = regular_texture_services();
    regular_textures::load_archive(*binding.archive, binding.textures, *regular_texture_state_, services);
}
void Session::load_japanese_title_textures() {
    if (!menu_textures_attached_ || !japanese_title_texture_.archive)
        throw flow::UnrecoveredGameFlowCall("native_japanese_title_texture_declaration");
    auto services = regular_texture_services();
    regular_textures::load_archive(*japanese_title_texture_.archive, japanese_title_texture_.textures,
                                   *regular_texture_state_, services);
}
void Session::select_title_texture(title_menu::TitleTextureList selected) {
    const sprite::TextureList *list = nullptr;
    switch (selected) {
    case title_menu::TitleTextureList::Common:
        list = menu_texture_binding(18).textures;
        break;
    case title_menu::TitleTextureList::Small:
        list = menu_texture_binding(29).textures;
        break;
    case title_menu::TitleTextureList::Title:
        list = menu_texture_binding(17).textures;
        break;
    case title_menu::TitleTextureList::JapaneseTitle:
        list = japanese_title_texture_.textures;
        break;
    }
    if (!display_ || !list)
        throw flow::UnrecoveredGameFlowCall("native_title_texture_selection");
    display_->output().select_texture_list(*list);
}
void Session::initialize_title_menu() {
    if (!title_)
        throw flow::UnrecoveredGameFlowCall("native_title_scene");
    title_->initialize();
}
std::int32_t Session::run_title_menu() {
    if (!title_)
        throw flow::UnrecoveredGameFlowCall("native_title_scene");
    return title_->run();
}
void Session::close_title_menu(std::int32_t) {
    if (!title_)
        throw flow::UnrecoveredGameFlowCall("native_title_scene");
    (void)title_->close();
}
} // namespace sadx::native
