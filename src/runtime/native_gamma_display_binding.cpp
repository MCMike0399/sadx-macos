#include "runtime/native_gamma_display_context.hpp"
#include "player/event_actions.hpp"
#include "game/game_mode.hpp"
#include <cstring>

namespace sadx::gamma_display {
namespace {
[[noreturn]] void missing(const std::string &fact) {
    throw flow::UnrecoveredGameFlowCall(fact);
}
TaskWorkArena &arena(Context &context) {
    const auto owner = context.owner.scheduler.work_arena();
    if (!owner) missing("native_gamma_display_task_arena");
    return *owner;
}
PlayerPhysicsState &physics(Context &context) {
    if (!context.player || !context.player->physics)
        missing("native_gamma_display_player");
    return *context.player->physics;
}
template <class T> T read(Context &context, const TaskWorkHandle &work, unsigned at) {
    T value;
    if (!arena(context).read(work, at, &value, sizeof(value)))
        missing("native_gamma_display_work_read");
    return value;
}
unsigned axis(Axis value) { return static_cast<unsigned>(value); }
float &component(NinjaVector &value, Axis field) {
    switch (field) {
    case Axis::X: return value.x;
    case Axis::Y: return value.y;
    case Axis::Z: return value.z;
    }
    missing("native_gamma_display_vector_axis");
}
} // namespace
void capture(Context &context, const TaskHandle &task) {
    if (!task) missing("native_gamma_display_task");
    context.task = task;
    context.primary = task->primary_work;
    context.primary_state = task->primary_state;
    context.player = gamma_selection::player_for_motion(context.owner, task->motion_work);
}
std::uint16_t equipment(Context &context) {
    const auto &value = physics(context).equipment_at_4;
    if (!value) missing("native_gamma_display_equipment");
    return *value;
}
void capture_extra(Context &context) {
    context.extra = gamma_selection::extra_work(context.owner, context.player);
}
ObjectTable object_table(Context &context) {
    if (!context.owner.players.gamma_assets) missing("native_gamma_display_assets");
    return &context.owner.players.gamma_assets->display_objects;
}
ObjectId object_row(Context &, ObjectTable table, unsigned index) {
    if (!table) missing("native_gamma_display_object_table");
    const auto &rows = *static_cast<const std::map<unsigned, ObjectId> *>(table);
    const auto found = rows.find(index);
    if (found == rows.end())
        missing("native_gamma_display_object_row_" + std::to_string(index));
    return found->second;
}
ObjectId sibling(Context &context, ObjectId object) {
    return context.owner.scene.objects.object(object).sibling;
}
ObjectId child(Context &context, ObjectId object) {
    return context.owner.scene.objects.object(object).child;
}
void store_sibling(Context &context, ObjectId object, ObjectId sibling) {
    context.owner.scene.objects.object(object).sibling = sibling;
}
void store_weld_source(Context &context, ObjectId source) {
    auto &rows = context.owner.players.gamma_weld.rows;
    if (rows.empty()) missing("native_gamma_display_weld_owner");
    rows[0].source_object = source ? &context.owner.scene.objects.object(source) : nullptr;
}
void select_palette(Context &context, std::int32_t value) {
    if (!context.owner.display_palette) missing("native_gamma_display_palette_binding");
    context.owner.display_palette(value);
}
std::uint16_t requested_animation(Context &context) {
    const auto &value = physics(context).requested_animation_at_124;
    if (!value) missing("native_gamma_display_requested_animation");
    return *value;
}
std::uint16_t current_animation(Context &context) {
    const auto &value = physics(context).current_animation_at_126;
    if (!value) missing("native_gamma_display_current_animation");
    return *value;
}
std::int16_t animation_mode(Context &context) {
    const auto &mode = physics(context).animation_mode_at_120;
    if (!mode) missing("native_gamma_display_animation_mode");
    return *mode;
}
std::uint16_t status(Context &context) {
    if (!context.primary_state) missing("native_gamma_display_primary");
    return context.primary_state->orientation.flags;
}
float motion_height(Context &context) {
    constexpr unsigned kMotionHeight = 284;
    return read<float>(context, context.player->allocation, kMotionHeight);
}
std::int32_t angle(Context &context, Axis field) {
    if (!context.primary_state) missing("native_gamma_display_primary");
    return context.primary_state->orientation.angles[axis(field)];
}
float position(Context &context, Axis field) {
    if (!context.primary_state || !context.primary_state->position)
        missing("native_gamma_display_position");
    return component(*context.primary_state->position, field);
}
fidelity::ExactFloat exact(Context &context, float value) {
    return {context.arithmetic(), value};
}
void Context::textures() {
    constexpr unsigned kGamma = 6;
    const auto &catalog = owner.character_textures;
    if (!catalog.tables || catalog.count <= kGamma)
        missing("native_gamma_display_texture_catalog");
    const auto &table = catalog.tables[kGamma];
    if (!table.entries || table.count == 0 || !table.entries[0].textures ||
        table.entries[0].archive != std::optional<std::string_view>("E102"))
        missing("native_gamma_display_E102_texture_list");
    owner.rendering.output().select_texture_list(*table.entries[0].textures);
}
void select_body_textures(Context &context) { context.textures(); }
void save_control(Context &context) { render_snapshot::save_control(context.render_state()); }
void set_control(Context &context, std::uint32_t value) {
    render_snapshot::set_control(context.render_state(), value);
}
void set_white_material(Context &context) { context.material(); }
void save_attributes(Context &context) { render_snapshot::save_attributes(context.render_state()); }
void clear_attributes(Context &context, std::uint32_t mask, std::uint32_t attributes) {
    render_snapshot::clear_attributes(context.render_state(), mask, attributes);
}
void push_current(Context &context) { context.matrices().push_current_frame(); }
void push_unit(Context &context) { context.matrices().push_unit_frame(); }
void rotate(Context &context, Axis field, std::uint16_t value) {
    auto &owner = context.matrices();
    auto &matrix = owner.current_matrix();
    switch (field) {
    case Axis::X: owner.rotate_x(matrix, value, context.arithmetic()); break;
    case Axis::Y: owner.rotate_y(matrix, value, context.arithmetic()); break;
    case Axis::Z: owner.rotate_z(matrix, value, context.arithmetic()); break;
    }
}
NinjaVector transform_vector(Context &context, const NinjaVector &input) {
    NinjaVector result;
    if (!context.matrices().calculate_vector(context.matrices().current_matrix(), input, result,
                                             context.arithmetic()))
        missing("native_gamma_display_vector_transform");
    return result;
}
void pop(Context &context) { context.matrices().end_frame(); }
void translate(Context &context, const NinjaVector &value) {
    context.matrices().translate_matrix(context.matrices().current_matrix(), value,
                                         context.arithmetic());
}
void capture_collision_info(Context &context) {
    context.collision = collision_component::component(arena(context), context.primary);
    if (!context.collision) missing("native_gamma_display_collision");
}
void store_collision_center(Context &context, unsigned record, Axis field, float value) {
    if (!context.collision || record >= context.collision->info.size())
        missing("native_gamma_display_collision_record");
    component(context.collision->info[record].center, field) = value;
}
void store_walk_phase(Context &context, std::uint32_t value) {
    constexpr unsigned kWalkPhase = 44;
    if (!arena(context).write(context.extra, kWalkPhase, &value, sizeof(value)))
        missing("native_gamma_display_walk_phase");
}
bool has_event_action(Context &context) {
    return event_actions::action_head(arena(context), context.primary) != nullptr;
}
[[noreturn]] void walking_sway(Context &) {
    missing("native_gamma_display_walking_sway");
}
[[noreturn]] void event_model(Context &) {
    missing("native_gamma_display_event_animation");
}
[[noreturn]] void hidden_tail(Context &) {
    missing("native_gamma_display_hidden_tail");
}
void set_object_callback(Context &context) { context.callback(); }
[[noreturn]] void draw_player_action(Context &, std::uint16_t) {
    missing("native_gamma_display_action_clip_no_scale");
}
} // namespace sadx::gamma_display
namespace sadx::gamma_selection {
void display(Context &owner, const TaskHandle &task) {
    gamma_display::Context context(owner);
    gamma_display::display(context, task);
}
} // namespace sadx::gamma_selection
