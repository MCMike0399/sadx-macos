#include "engine/ninja_matrix_composition.hpp"
#include "game/game_mode.hpp"
#include "player/animation/animation_native_access.hpp"
#include "player/event_actions.hpp"
#include "runtime/native_gamma_display_context.hpp"
#include "runtime/object_draw_context.hpp"
#include <cstring>

namespace sadx::gamma_display {
namespace {
[[noreturn]] void missing(const std::string &fact) {
    throw flow::UnrecoveredGameFlowCall(fact.c_str());
}
TaskWorkArena &arena(Context &context) {
    const auto owner = context.owner.scheduler.work_arena();
    if (!owner)
        missing("native_gamma_display_task_arena");
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
animation::detail::NativeAccess::StateFields animation_fields(Context &context) {
    animation::View view{physics(context), context.player->allocation};
    return animation::detail::NativeAccess{}.state(view);
}
unsigned axis(Axis value) { return static_cast<unsigned>(value); }
float &component(NinjaVector &value, Axis field) {
    switch (field) {
    case Axis::X:
        return value.x;
    case Axis::Y:
        return value.y;
    case Axis::Z:
        return value.z;
    }
    missing("native_gamma_display_vector_axis");
}
} // namespace
void capture(Context &context, const TaskHandle &task) {
    if (!task)
        missing("native_gamma_display_task");
    context.task = task;
    context.primary = task->primary_work;
    context.primary_state = task->primary_state;
    context.player = gamma_selection::player_for_motion(context.owner, task->motion_work);
}
std::uint16_t equipment(Context &context) {
    const auto &value = physics(context).equipment;
    if (!value)
        missing("native_gamma_display_equipment");
    return *value;
}
void capture_extra(Context &context) {
    context.extra = gamma_selection::extra_work(context.owner, context.player);
}
ObjectTable object_table(Context &context) {
    if (!context.owner.players.gamma_assets)
        missing("native_gamma_display_assets");
    return &context.owner.players.gamma_assets->display_objects;
}
ObjectId object_row(Context &, ObjectTable table, unsigned index) {
    if (!table)
        missing("native_gamma_display_object_table");
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
    if (rows.empty())
        missing("native_gamma_display_weld_owner");
    rows[0].source_object = source ? &context.owner.scene.objects.object(source) : nullptr;
}
void select_palette(Context &context, std::int32_t value) {
    if (!context.owner.display_palette)
        missing("native_gamma_display_palette_binding");
    context.owner.display_palette(value);
}
std::uint16_t requested_animation(Context &context) {
    return gamma_selection::requested_animation(context.owner, context.player);
}
std::uint16_t current_animation(Context &context) {
    return gamma_selection::current_animation(context.owner, context.player);
}
std::int16_t animation_mode(Context &context) {
    const auto &mode = physics(context).animation_mode;
    if (!mode)
        missing("native_gamma_display_animation_mode");
    return *mode;
}
std::uint16_t status(Context &context) {
    if (!context.primary_state)
        missing("native_gamma_display_primary");
    return context.primary_state->orientation.flags;
}
float motion_height(Context &context) {
    constexpr unsigned kMotionHeight = 284;
    return read<float>(context, context.player->allocation, kMotionHeight);
}
std::int32_t angle(Context &context, Axis field) {
    if (!context.primary_state)
        missing("native_gamma_display_primary");
    return context.primary_state->orientation.angles[axis(field)];
}
float position(Context &context, Axis field) {
    if (!context.primary_state || !context.primary_state->position)
        missing("native_gamma_display_position");
    return component(*context.primary_state->position, field);
}
fidelity::ExactFloat exact(Context &context, float value) { return {context.arithmetic(), value}; }
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
    case Axis::X:
        owner.rotate_x(matrix, value, context.arithmetic());
        break;
    case Axis::Y:
        owner.rotate_y(matrix, value, context.arithmetic());
        break;
    case Axis::Z:
        owner.rotate_z(matrix, value, context.arithmetic());
        break;
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
    if (!context.collision)
        missing("native_gamma_display_collision");
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
[[noreturn]] void walking_sway(Context &) { missing("native_gamma_display_walking_sway"); }
[[noreturn]] void event_model(Context &) { missing("native_gamma_display_event_animation"); }
[[noreturn]] void hidden_tail(Context &) { missing("native_gamma_display_hidden_tail"); }
void set_object_callback(Context &context) { context.callback(); }
[[noreturn]] void draw_player_action(Context &context, std::uint16_t animation) {
    ordinary_body(context, animation);
}
float animation_frame(Context &context) {
    const auto &frame = animation_fields(context).frame;
    if (!frame)
        missing("native_gamma_display_animation_frame");
    return *frame;
}
std::int32_t aim_rotation(Context &context) {
    return read<std::int32_t>(context, context.owner.players.gamma.current_extra, 140);
}
float aim_translation(Context &context) {
    return read<float>(context, context.owner.players.gamma.current_extra, 144);
}
void translate_numeric(Context &context, const NinjaVector &value) {
    translate_ninja_matrix(context.matrices().current_matrix(), value.x, value.y, value.z,
                           context.arithmetic());
}
namespace {
// NOTSA: borrowed key storage; imported and generated actions use the same renderer.
class MotionView final : public animated_model::Motion {
  public:
    MotionView(const animation::Motion &motion, const animation::Resources &resources)
        : imported_(&motion), resources_(&resources) {}
    MotionView(const PlayerAnimationAllocatedMotion &motion, TaskWorkArena &arena)
        : generated_(&motion), arena_(&arena) {}
    std::uint32_t frame_count() override {
        if (imported_)
            return imported_->frame_count;
        if (!generated_->frame_count)
            missing("native_gamma_draw_motion_frame_count");
        return *generated_->frame_count;
    }
    std::uint16_t type() override {
        if (imported_)
            return imported_->type;
        if (!generated_->type)
            missing("native_gamma_draw_motion_type");
        return *generated_->type;
    }
    std::uint16_t interpolation() override {
        if (imported_)
            return imported_->input_function;
        if (!generated_->input_function)
            missing("native_gamma_draw_motion_layout");
        return *generated_->input_function;
    }
    bool has_keys(unsigned node, unsigned channel) override {
        if (imported_)
            return nodes().at(node).keys.at(channel).id.has_value();
        const auto reference = arena_->reference(data(), node * 16 + channel * 4);
        if (!reference)
            missing("native_gamma_draw_generated_key_reference");
        return bool(reference.work);
    }
    std::uint32_t key_count(unsigned node, unsigned channel) override {
        if (imported_)
            return nodes().at(node).counts.at(channel);
        std::uint32_t result;
        if (!arena_->read(data(), node * 16 + 8 + channel * 4, &result, 4))
            missing("native_gamma_draw_motion_count");
        return result;
    }
    std::uint32_t key_word(unsigned node, unsigned channel, unsigned key, unsigned word) override {
        std::uint32_t result;
        const auto offset = std::size_t(key) * 16 + word * 4;
        if (imported_) {
            const auto identity = nodes().at(node).keys.at(channel).id;
            if (!identity)
                missing("native_gamma_draw_motion_keys");
            const auto &keys = resources_->key_arrays.at(*identity);
            if (!keys.bytes || offset + 4 > keys.byte_extent ||
                keys.offset + offset + 4 > keys.bytes->size())
                missing("native_gamma_draw_motion_key_extent");
            std::memcpy(&result, keys.bytes->data() + keys.offset + offset, 4);
        } else {
            const auto keys = arena_->reference(data(), node * 16 + channel * 4);
            if (!keys || !arena_->read(keys.work, offset, &result, 4))
                missing("native_gamma_draw_generated_keys");
        }
        return result;
    }

  private:
    const animation::Motion *imported_ = nullptr;
    const animation::Resources *resources_ = nullptr;
    const PlayerAnimationAllocatedMotion *generated_ = nullptr;
    TaskWorkArena *arena_ = nullptr;
    const std::vector<animation::Node> &nodes() {
        if (!imported_->data.id)
            missing("native_gamma_draw_motion_data");
        return resources_->node_arrays.at(*imported_->data.id);
    }
    TaskWorkHandle data() {
        if (!generated_->data || !*generated_->data)
            missing("native_gamma_draw_generated_data");
        return *generated_->data;
    }
};
void submit(Context &context, ObjectId root, animated_model::Motion *motion, float frame) {
    if (!context.owner.model_environment)
        missing("native_gamma_draw_model_environment");
    auto callback = [&context](character_pack::ResourceId object_identity) {
        gamma_display::gamma_object_callback(context, object_identity);
    };
    object_draw::Context draw{context.owner.rendering, *context.owner.model_environment, callback};
    animated_model::action_clip_no_scale(root, motion, frame, draw.animated(), context.draw_state(),
                                         draw);
}
} // namespace
void draw_table_action(Context &context, std::uint16_t ordinal, float frame) {
    animation::View view{physics(context), context.player->allocation};
    const animation::Table *content;
    try {
        content = &animation::detail::NativeAccess{}.table(view, animation::Phase::None);
    } catch (const animation::Stop &) {
        missing("native_gamma_draw_animation_table");
    }
    const auto &resource = content->records.at(ordinal).action;
    if (!animated_model::drawing_enabled(context.draw_state()))
        return;
    if (resource.status == animation::ResourceStatus::Unavailable)
        missing("native_gamma_draw_action_asset_" + resource.unavailable_reason);
    if (resource.status == animation::ResourceStatus::KnownNull) {
        submit(context, 0, nullptr, frame);
        return;
    }
    if (!context.owner.players.gamma_motion_assets)
        missing("native_gamma_draw_animation_runtime");
    const auto &resources = context.owner.players.gamma_motion_assets->resources();
    const auto &action = resources.actions.at(resource.identity);
    if (!action.object.id || !action.motion.id) {
        submit(context, action.object.id ? static_cast<ObjectId>(*action.object.id) : 0, nullptr,
               frame);
        return;
    }
    MotionView motion(resources.motions.at(*action.motion.id), resources);
    submit(context, static_cast<ObjectId>(*action.object.id), &motion, frame);
}
void draw_generated_action(Context &context, float frame) {
    const auto &buffer = animation_fields(context).buffer;
    if (!buffer)
        missing("native_gamma_draw_generated_action");
    if (!animated_model::drawing_enabled(context.draw_state()))
        return;
    if (!*buffer) {
        submit(context, 0, nullptr, frame);
        return;
    }
    const auto &action = **buffer;
    if (!action.object || !action.motion)
        missing("native_gamma_draw_generated_header");
    if (!action.object->id || !*action.motion) {
        submit(context, action.object->id ? static_cast<ObjectId>(*action.object->id) : 0, nullptr,
               frame);
        return;
    }
    MotionView motion(**action.motion, arena(context));
    submit(context, static_cast<ObjectId>(*action.object->id), &motion, frame);
}
void clear_object_callback(Context &context) {
    render_snapshot::set_object_callback(context.render_state().saved,
                                         render_snapshot::ObjectCallback::None);
}
void multiply_arm_matrix(Context &context) {
    multiply_ninja_matrix(context.matrices().current_matrix(),
                          context.owner.players.gamma.node54_matrix, context.arithmetic());
}
std::int32_t arm_rotation(Context &context) {
    return read<std::int32_t>(context, context.extra, 140);
}
[[noreturn]] void draw_arm_action(Context &context) {
    // Source loads the saved E102WK arm frame before reading export 50.
    const auto frame = read<float>(context, context.extra, 132);
    (void)frame;
    if (!context.owner.players.gamma_assets)
        missing("native_gamma_display_arm_asset_table");
    const auto &action = context.owner.players.gamma_assets->declarations.at(50);
    if (action.status == animation::ResourceStatus::Unavailable)
        missing("native_gamma_display_missing_arm_action_export_50_after_body");
    missing("native_gamma_display_arm_action_and_matrix_tail");
}
} // namespace sadx::gamma_display
namespace sadx::gamma_selection {
void display(Context &owner, const TaskHandle &task) {
    gamma_display::Context context(owner);
    gamma_display::display(context, task);
}
} // namespace sadx::gamma_selection
