#pragma once
#include "engine/character_pack.hpp"
#include "runtime/native_player_system.hpp"
#include "engine/character_geometry.hpp"
#include "engine/graphics_frame.hpp"
#include "game/game_bootstrap.hpp"
#include "game/game_mode_services.hpp"
#include "runtime/logo_scene.hpp"
#include "game/movie.hpp"
#include "runtime/native_input.hpp"
#include "runtime/character_texture_assets.hpp"
#include "game/game_cleanup.hpp"
#include "game/audio_playback.hpp"
#include "game/title_menu.hpp"
#include "game/sound_bank.hpp"
#include "game/stage_lighting.hpp"
#include "engine/frame_loop_native.hpp"
#include "runtime/regular_texture_assets.hpp"
#include "runtime/menu_texture_assets.hpp"
#include "game/music.hpp"
#include "game/voice_playback.hpp"
#include "game/save_file_list.hpp"
#include "game/menu_voice.hpp"
#include "game/menu_caption.hpp"
#include "game/save_file_name.hpp"
#include "engine/font_native.hpp"
#include "runtime/font_assets.hpp"
#include "runtime/file_select_display_assets.hpp"
#include "runtime/menu_dialog_assets.hpp"
#include "runtime/main_menu_assets.hpp"
#include "runtime/character_selection_assets.hpp"
#include "game/character_completion.hpp"
#include "runtime/message_font_assets.hpp"
#include "game/save_creation.hpp"
#include "game/save_write.hpp"
#include "game/save_frame.hpp"
#include "game/sound_effects.hpp"

namespace sadx::native {
class MovieScene;
class Display;
class TitleScene;
// NOTSA: native game session owns canonical flow/bootstrap/save state and imported actions.
// Starts at the original Start mode; unrecovered later modes keep their explicit failures.
class Session final : private flow::NativeServices {
  public:
    Session(LogoOutput &, std::shared_ptr<const character_pack::CharacterPack>,
            LogoFrame, X87Context &, frame::ModeState &, const CharacterTextureAssets &,
            std::int32_t frame_rate_setting,
            std::function<void(std::uint32_t)> stop_vibration);
    void advance();
    void complete_menu_messages();
    void release_lazy_textures();
    void before_save_frame_completion();
    void after_save_frame_completion();
    void attach_save_writer(save::WriteCalls &calls) { save_writer_ = &calls; }
    void attach_message_fonts(MessageFontAssets);
    void capture_live_save();
    void write_live_save();
    void begin_frame() { loop_.loop_starting = 1; }
    void complete_frame_counters() {
        frame_loop::complete_counter_state(loop_, timing_, renderer_);
    }
    const flow::GameFlowState &flow_state() const { return flow_; }
    const bootstrap::State &game_state() const { return bootstrap_; }
    const save::Data &save_data() const { return save_; }
    // NOTSA: read-only journey evidence from the canonical scheduler/writer fields.
    struct SaveProgress {
        std::int32_t phase;
        std::uint8_t ready, count, executing, complete, message_pending;
    };
    SaveProgress save_progress() const {
        return {flow_.save_phase, flow_.save_ready, save_count_, save_exec_, save_ok_,
                saving_message_pending_};
    }
    const menu_caption::State &menu_caption_state() const { return menu_caption_; }
    const char *active_save_name() const { return save_name_.value; }
    std::int32_t current_music_song() const { return tracks_.current_song; }
    // NOTSA: read-only journey evidence from the canonical signed-byte owners.
    const std::array<std::int8_t, 9> &character_texture_counts() const {
        return character_textures_loaded_;
    }

    std::uint32_t logic_loop_count() const { return secondary_loop_count_; }
    std::optional<file_select::FileInfo> selected_save_file() const {
        auto node = list_data_;
        for (unsigned index = 0; node && index < bootstrap_.card_state; ++index)
            node = node->next;
        return node ? node->file : std::nullopt;
    }
    const LogoScene &logo_scene() const { return logo_; }
    std::uint32_t global_timer() const { return global_timer_; }
    std::int16_t stage() const { return stage_; }
    std::int16_t act() const { return act_; }
    std::int16_t rings() const { return rings_; }
    frame::RendererState &renderer_timing() { return renderer_; }
    const frame::ModeState &frame_mode() const { return timing_; }
    const frame_loop::State &loop_state() const { return loop_; }
    frame::ModeState &frame_mode() { return timing_; }
    movie::SharedState movie_state() { return {quit_, flow_.movie_number, presented_frames_}; }
    void attach_movie(MovieScene &scene) { movie_ = &scene; }
    bool quit_requested() const { return quit_ != 0; }
    void request_quit() { quit_ = 1; }
    // Normal GameLoop completion increments once; the nested movie loop owns its own increments.
    void complete_frame() { ++presented_frames_; }
    std::uint32_t completed_frames() const { return presented_frames_; }
    InputFrameContext input_frame_context() const {
        return {static_cast<std::int32_t>(flow_.mode), logo_.demo_mode(),
                demo_enabled_, logo_.demo_event()};
    }
    SoftResetState &soft_reset_state() { return soft_reset_; }
    void attach_cleanup(Display &display, audio::AudioConfiguration &configuration) {
        display_ = &display;
        attach_logo_scaling(display);
        audio_configuration_ = &configuration;
    }
    void attach_title(TitleScene &scene) { title_ = &scene; }
    void attach_languages(std::int32_t text, std::int32_t voice) {
        text_language_ = text;
        voice_language_ = voice;
    }
    void apply_frame_rate_setting(std::int32_t value) {
        // NOTSA: WinMain's signed profile integer normalization, before frame completion uses it.
        renderer_.configured_frame_interval = value <= 0 ? 1 : (value > 3 ? 3 : value);
    }
    void attach_lighting(lighting::Tables imported) {
        lighting_ = std::make_unique<lighting::State>(std::move(imported));
    }
    void attach_regular_textures(RegularTextureAssets);
    void attach_menu_textures(const MenuTextureAssets &);
    void attach_menu_dialogs(MenuDialogAssets);
    void attach_main_menu(MainMenuAssets);
    void attach_character_selection(CharacterSelectionAssets);
    void load_player_assets(GammaActorAssets assets) {
        if (players_.gamma_assets)
            throw std::runtime_error("native player assets already loaded");
        players_.gamma_assets = std::make_unique<GammaActorAssets>(std::move(assets));
    }
    void attach_character_completion(character_completion::Catalog catalog) {
        if (character_completion_)
            throw std::runtime_error("native character completion resources already attached");
        character_completion_ = std::make_unique<character_completion::Catalog>(std::move(catalog));
    }
    void attach_character_labels(std::array<std::uint8_t, 7> labels) {
        if (character_label_textures_)
            throw std::runtime_error("native character label resources already attached");
        character_label_textures_ = std::make_unique<std::array<std::uint8_t, 7>>(labels);
    }
    sound_bank::Channels sound_channels() { return {sound_.records, sound_tasks_, bank_channels_}; }
    void attach_sound_banks(const sound_bank::Catalog &catalog, sound_bank::Services services,
                            std::string cd_prefix) {
        bank_catalog_ = &catalog;
        bank_services_ = std::move(services);
        cd_sound_prefix_ = std::move(cd_prefix);
    }
    const sound_bank::State &sound_banks() const { return banks_; }
    sound_bank::State &sound_banks() { return banks_; }
    sound_effects::Queue sound_queue() { return {sound_, bank_channels_, sound_effects_}; }
    void attach_sound_effects(sound_effects::BankTable table, sound_effects::ServerCalls &calls) {
        sound_bank_table_ = std::move(table);
        sound_server_calls_ = &calls;
    }
    void update_sounds();
    audio::PcmState &pcm_state() { return pcm_; }
    audio::MediaState &media_state() { return media_; }
    const audio::MediaState &media_state() const { return media_; }
    void attach_music(music::Catalog catalog, music::MediaServices services,
                      std::function<std::uint32_t()> milliseconds) {
        music_catalog_ = std::make_unique<music::Catalog>(std::move(catalog));
        music_media_services_ = std::move(services);
        music_milliseconds_ = std::move(milliseconds);
    }
    void attach_voice(voice_playback::Services services) { voice_services_ = std::move(services); }
    void attach_save_files(file_select::SaveFileCalls &files) { save_files_ = &files; }
    void attach_menu_voices(menu_voice::Catalog catalog) { menu_voices_ = std::move(catalog); }
    void attach_font(FontAssets);
    void attach_file_select_display(FileSelectDisplayAssets assets,
        std::function<file_select_display::Calendar(std::uint64_t)> calendar) {
        file_select_display_assets_ = std::make_unique<FileSelectDisplayAssets>(std::move(assets));
        file_time_calendar_ = std::move(calendar);
    }
    // The nested movie loop and the game-mode dispatcher share these exact owners.
    void clear_game() override;
    void reset_game() override;

  private:
    friend class TitleScene;
    void attach_logo_scaling(Display &);
    flow::GameFlowState flow_;
    std::uint32_t global_timer_ = 0;
    std::uint32_t quit_ = 0, presented_frames_ = 0;
    MovieScene *movie_ = nullptr;
    TitleScene *title_ = nullptr;
    std::unique_ptr<lighting::State> lighting_;
    std::int32_t text_language_ = 0, voice_language_ = 0;
    Display *display_ = nullptr;
    audio::AudioConfiguration *audio_configuration_ = nullptr;
    std::int32_t demo_enabled_ = 0;
    SoftResetState soft_reset_;
    std::int32_t card_result_ = 0;
    // Native session storage: SONIC 0x3b29198 GB_vmsdisable
    std::int32_t saving_disabled_ = 0;
    // Native session storage: SONIC 0x3b2919c
    std::uint32_t save_word_before_selection_ = 0;
    // Native session storage: SONIC 0x3b291a0
    std::uint32_t save_word_after_selection_ = 0;
    // Native session storage: SONIC 0x3c5fefc
    std::uint32_t demo_wait_ticks_ = 0;
    // Native session storage: SONIC 0x91268c Music_Enabled
    std::int32_t music_requested_enabled_ = 1;
    // Native session storage: SONIC 0x912690 Serif_Enabled
    std::int32_t voice_requested_enabled_ = 1;
    // Native session storage: SONIC 0x3c5e8a4, 0x3c5e8b4, 0x10d77ad.
    std::int16_t first_visible_file_ = 0;
    std::uint8_t selected_card_index_ = 0, displayed_save_index_ = 0;
    // Native session storage: SONIC 0x3b2c58c
    std::uint8_t file_select_fade_in_marker_ = 0;
    // GLOBAL: SONIC 0x3b291b8 FileOperationPending size=1
    std::uint8_t file_operation_pending_ = 0;
    // GLOBAL: SONIC 0x3b291b3 FirstCreateFlag size=1
    std::uint8_t new_file_requested_ = 0;
    // GLOBAL: SONIC 0x3c5e8bc FileMessageVisible size=1
    std::uint8_t file_message_visible_ = 0;
    // GLOBAL: SONIC 0x3c5e8b0 FileMessageTask size=4
    TaskHandle file_message_;
    // GLOBAL: SONIC 0x3c5e8a8 SaveWarningTask size=4
    TaskHandle save_warning_message_;
    std::int16_t rings_ = 0, stage_ = 0;
    std::uint16_t game_mode_ = 0;
    bootstrap::State bootstrap_;
    save::Data save_;
    save::Data save_backup_;
    std::shared_ptr<const character_pack::CharacterPack> character_pack_;
    bootstrap::SonicActions actions_;
    character_pack::SceneState character_scene_;
    PlayerSystem players_;
    ActionHandle knuckles_clear_, knuckles_transition_;
    ActionHandle big_sit_, big_sit_repeat_, big_prepare_, big_cast_, big_cast_repeat_;
    bootstrap::Services services_;
    frame::ModeState &timing_;
    frame_loop::State loop_;
    frame::RendererState renderer_;
    LogoScene logo_;
    CharacterTextureAssets texture_assets_;
    std::array<std::vector<sprite::TextureList>, 9> character_textures_;
    std::array<cleanup::CharacterTextureLists, 9> character_texture_lists_;
    std::array<const cleanup::CharacterTextureLists *, 9> character_texture_bindings_{};
    std::array<std::int8_t, 9> character_textures_loaded_{};
    sprite::TextureList *common_textures_ = nullptr;
    std::array<sprite::TextureList *, 32> regular_textures_{};
    RegularTextureAssets regular_assets_{};
    std::vector<sprite::TextureList> regular_resource_lists_;
    std::vector<regular_textures::Group> regular_groups_;
    regular_textures::Catalog regular_catalog_{};
    std::unique_ptr<regular_textures::State> regular_texture_state_;
    regular_textures::Services regular_texture_services();
    void load_regular_object_textures(std::uint32_t);
    std::vector<sprite::TextureList> menu_resource_lists_;
    std::vector<sprite::TextureList *> menu_list_bindings_;
    MenuTextureCatalog menu_texture_catalog_{};
    regular_textures::Binding japanese_title_texture_{};
    bool menu_textures_attached_ = false;
    menu_voice::Catalog menu_voices_;
    std::unique_ptr<FontAssets> font_assets_;
    sprite::TextureList font_textures_;
    font::Info font_info_;
    font::State font_state_;
    std::unique_ptr<FileSelectDisplayAssets> file_select_display_assets_;
    std::unique_ptr<MenuDialogAssets> menu_dialog_assets_;
    std::unique_ptr<MainMenuAssets> main_menu_assets_;
    std::unique_ptr<CharacterSelectionAssets> character_selection_assets_;
    std::unique_ptr<character_completion::Catalog> character_completion_;
    std::unique_ptr<std::array<std::uint8_t, 7>> character_label_textures_;
    // GLOBAL: SONIC 0x3b11184 CharacterSelectSavedFrameIncrement size=4
    std::int32_t selection_saved_frame_increment_ = 0;
    menu_caption::State menu_caption_;
    std::unique_ptr<MessageFontAssets> message_font_assets_;
    message_text::State message_fonts_;
    save::WriteCalls *save_writer_ = nullptr;
    // Source native-world storage:40cad0/421fd0 BSS counterparts, not reference availability.
    std::uint8_t save_count_ = 0, save_exec_ = 0, saving_message_pending_ = 0;
    std::uint8_t save_ok_ = 0, save_retry_ = 0, file_delete_flag_ = 0;
    menu_caption::Context saving_message_;
    std::uint8_t skip_message_mipmaps_ = 0;
    // Native BSS storage: SONIC 0x3abdf80; source conditional4326f0 caller.
    std::uint32_t text_window_pending_ = 0;
    // Native BSS storage: SONIC 0x3b2c584; source42f990 deferred list owner.
    void *lazy_texture_release_list_ = nullptr;
    // GLOBAL: SONIC 0x3c5e8c4 FileSelectionBackup size=2
    std::uint16_t file_selection_backup_ = 0;
    // GLOBAL: SONIC 0x3b2c5e4 DialogWindowAfterContent size=4
    std::uint32_t dialog_window_after_content_ = 0;
    // NOTSA: owns the allocation published by the source SaveName pointer.
    std::unique_ptr<char[]> selected_save_name_;
    save::SaveNameState save_name_;
    std::function<file_select_display::Calendar(std::uint64_t)> file_time_calendar_;
    const regular_textures::Binding &menu_texture_binding(std::uint32_t) const;
    void load_menu_texture(std::uint32_t);
    void load_japanese_title_textures();
    void select_title_texture(title_menu::TitleTextureList);
    SoundInitialState sound_;
    sound_effects::State sound_effects_;
    sound_effects::BankTable sound_bank_table_{};
    sound_effects::ServerCalls *sound_server_calls_ = nullptr;
    // Native session storage: SONIC 0x90a0ac opponent character; 0x3b42e14 opponent task.
    std::int32_t opponent_character_ = -1;
    TaskHandle opponent_task_;
    // Native session storage: SONIC 0x3b2cbb0 positional sound listener task.
    TaskHandle sound_listener_;
    audio::PcmState pcm_{sound_.stream_initialized};
    audio::MediaState media_;
    std::unique_ptr<music::Catalog> music_catalog_;
    music::VoiceTiming voice_timing_;
    music::MediaServices music_media_services_;
    voice_playback::Services voice_services_;
    std::function<std::uint32_t()> music_milliseconds_;
    std::array<TaskHandle, 36> sound_tasks_{};
    cleanup::TrackState tracks_;
    TaskHandle sound_manager_;
    sound_bank::State banks_;
    sound_bank::ChannelBindings bank_channels_;
    const sound_bank::Catalog *bank_catalog_ = nullptr;
    sound_bank::Services bank_services_;
    std::string cd_sound_prefix_;
    void select_sound_bank(std::int32_t);
    // Native session storage: SONIC 0x3b29b88
    std::int32_t sound_queue_reset_word_ = 0;
    // Native session storage: SONIC 0x3b29208
    std::int32_t sound_bank_reset_word_ = 0;
    cleanup::AuxiliaryWork auxiliary_;
    cleanup::MessageState messages_;
    std::int32_t sequence_running_ = cleanup::kInitialSequenceRunning;
    std::int32_t demo_paused_ = 0;
    std::uint32_t secondary_loop_count_ = 0;
    // adva_loop_count is separate from ResetGame's loop bookkeeping.
    std::uint32_t title_logic_ticks_ = 0;
    std::uint8_t hide_hud_ = 0;
    // File-backed default; LoadSave subsequently writes the saved vibration byte.
    std::uint8_t rumble_enabled_ = 1;
    std::uint16_t editor_status_ = 0;
    std::uint32_t reset_status_ = 0, reset_work_ = 0, saved_stage_ = 0, market_rings_ = 0;
    std::int16_t act_ = 0, previous_stage_ = 0;
    std::uint32_t previous_act_ = 0;
    std::array<std::uint8_t, 60> missions_{};
    // These source allocations are absent at cold start. Their future allocating
    // callers must establish native ownership before a nonnull release is supported.
    void *temporary_buffer_ = nullptr, *secondary_buffer_ = nullptr;
    std::uint32_t *set_entry_count_ = nullptr;
    cleanup::PlacementEntry *set_entries_ = nullptr;
    std::array<void *, 6> set_files_{}, camera_files_{};
    cleanup::LevelDestructor level_destructor_ = nullptr;
    void *level_destructor_context_ = nullptr;
    file_select::List list_data_;
    file_select::SaveFileCalls *save_files_ = nullptr;
    std::string debug_message_;
    cleanup::TextureState texture_state();
    cleanup::LevelState level_state();
    void release_texture_list(sprite::TextureList &);
    void release_buffer(void *);
    void unload_character_textures(std::uint16_t);
    void load_save();
    void set_stage_number(std::uint8_t, std::uint8_t);

    void initialize_game() override;
    void initialize_demo_state() override;
    std::uint32_t startup_status() override;
    std::int32_t startup_readiness() override;
    void initialize_sega_logo() override;
    std::uint32_t run_logos() override;
    void load_sega_logo() override;
    void set_frame_rate_mode(std::int32_t, std::int32_t) override;
    void set_wait_vsync_count(std::int32_t) override;
    void draw_screen_fade() override;
    void increment_frame_counter() override;
    std::uint32_t initialize_level_game() override;
    void setup_character(std::uint32_t) override;
    void reset_player_work() override;
    std::int32_t run_movie(std::uint32_t) override;
    void prepare_title_initialization() override;
    void run_level_destructor(std::uint32_t) override;
    void free_regular_object_texlists() override;
    void free_character_texlists() override;
    void release_all_textures() override;
    void prepare_title_assets() override;
    void initialize_title_menu() override;
    void load_level_sounds() override;
    std::int32_t run_title_menu() override;
    void close_title_menu(std::int32_t) override;
};
} // namespace sadx::native
