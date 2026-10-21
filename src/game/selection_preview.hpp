#pragma once
#include "engine/task.hpp"
#include "game/regular_textures.hpp"
#include <optional>
#include <string>
#include <string_view>
#include "math/ninja_math.hpp"
#include <array>
#include <cstdint>
#include <memory>
#include <stdexcept>

namespace sadx::selection_preview {
// CharMdlWk, represented by native task handles and source-initialized values.
struct Work {
    std::array<TaskHandle, 7> actors{};
    TaskHandle camera;
    float location_x = 0, location_y = 0, scale = 0;
    std::int8_t warmup_frames = 0;
    std::array<std::int8_t, 7> motion_counts{};
    std::uint8_t selected_motion = 0;
};
struct State {
    // GLOBAL: SONIC 0x3c5ff80 CharSelPreviewTask size=4
    TaskHandle task;
    // GLOBAL: SONIC 0x3c5ff64 CharSelResetMarker size=4
    std::uint32_t reset_marker = 0;
    // GLOBAL: SONIC 0x3c5ff10 CharSelPositions size=84
    std::array<NinjaVector, 7> positions = {};
};
enum class ActorKind { Sonic, Miles, Knuckles, Amy, Big, Gamma, QuestionMark };
// NOTSA: imported source declarations; no source code pointers are runtime resources.
struct Assets {
    const std::array<std::uint16_t, 7> &characters;
    const std::array<NinjaVector, 7> &positions;
    const std::int32_t &sonic_aim_y;
};
// NOTSA: borrowed existing task/player pose owners, never duplicate actor state.
struct Pose {
    NinjaVector &position;
    std::array<std::int32_t, 3> &rotation;
};
struct Actor {
    Pose pose;
    std::uint8_t &task_id, &player_index, &character_id;
    std::int32_t &aim_y;
};
struct View {
    const float &horizontal_scale;
    const std::uint8_t &selected_character;
    std::int32_t &saved_view_angle;
};
enum class Boundary { Task, Work, Actor, ActorIndex, TextureCharacter, TextureBinding };
class Error : public std::runtime_error {
  public:
    Error(Boundary boundary, std::size_t actor, const char *message)
        : std::runtime_error(message), boundary_(boundary), actor_(actor) {}
    Boundary boundary() const { return boundary_; }
    std::size_t actor() const { return actor_; }
  private:
    Boundary boundary_;
    std::size_t actor_;
};
struct Calls {
    virtual ~Calls() = default;
    // NOTSA: TaskScheduler creates the actual task with the named preview callbacks.
    virtual TaskHandle create_preview_task(std::uint32_t flags, std::int32_t level) = 0;
    virtual std::shared_ptr<Work> allocate_work(const TaskHandle &) = 0;
    virtual void install_callbacks(const TaskHandle &) = 0;
    virtual void select_palette(std::int32_t palette) = 0;
    // NOTSA: delegates the source camera task to selection_camera::update.
    virtual TaskHandle create_camera(std::uint32_t flags, std::int32_t level) = 0;
    virtual Pose camera_pose(const TaskHandle &) = 0;
    // NOTSA: each ActorKind must bind its real player task callback or fail explicitly.
    virtual TaskHandle create_actor(ActorKind, std::uint32_t flags, std::int32_t level) = 0;
    virtual Actor actor(const TaskHandle &) = 0;
    virtual void load_character_textures(std::uint16_t character) = 0;
    virtual void load_named_texture(std::string_view, regular_textures::List &) = 0;
    virtual void load_individual_texture(regular_textures::List &) = 0;
    virtual void release_texture(regular_textures::List &) = 0;
    virtual void release_actor_textures(std::uint16_t character) = 0;
    virtual void destroy_task(const TaskHandle &) = 0;
    virtual void set_perspective(std::int32_t) = 0;
    // STUB: SONIC 0x512910 CharSel_KeepCharactersFromFalling
    virtual void update_actor_actions(Work &) = 0;
};
// NOTSA: archive declarations borrow the same texlist identities and loaded counters as cleanup.
struct TextureView {
    const std::array<const std::vector<regular_textures::Binding> *, 9> &characters;
    std::array<std::int8_t, 9> &loaded;
};
// FUNCTION: SONIC 0x512550 LoadCharSelTextures
void load_textures(std::uint16_t character, TextureView, Calls &);
// NOTSA: source inline teardown releases every declared list, unlike UnloadCharTextures.
void release_textures(std::uint16_t character, TextureView, Calls &);
// FUNCTION: SONIC 0x512840 PrepareCharSelActors
void prepare_actors(Work &, State &, Assets, View, Calls &, X87Context &);
// FUNCTION: SONIC 0x512640 ResetCharSelActors
void reset_actors(Work &, Assets, Calls &);
// FUNCTION: SONIC 0x5127f0 PositionCharSelActor
void position_selected(Work &, State &, Assets, View, Calls &);
// FUNCTION: SONIC 0x512af0 InitializeCharSelPreview
void initialize(const TaskHandle &, State &, Assets, View, Calls &, X87Context &);
// FUNCTION: SONIC 0x512bc0 CharSel_LoadA
void create(State &, Assets, View, Calls &, X87Context &);
// FUNCTION: SONIC 0x512b40 CharSel_Main
void update(const TaskHandle &, State &, Assets, View, Calls &);
// FUNCTION: SONIC 0x512b60 CharSel_Display
void draw(const TaskHandle &, View, Calls &);
// FUNCTION: SONIC 0x512a70 ReleaseCharSelActors
void release_actors(Work &, Assets, Calls &);
// FUNCTION: SONIC 0x512ba0 CharSel_Delete
void destroy(const TaskHandle &, Assets, Calls &);
// FUNCTION: SONIC 0x512c20 DeleteCharSel
void remove(State &, Calls &);
} // namespace sadx::selection_preview
