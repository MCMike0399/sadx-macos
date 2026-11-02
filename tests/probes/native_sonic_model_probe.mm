#include "apps/native_sonic_model_assets.hpp"
#include <algorithm>

namespace sadx::native {
// NOTSA: identities come from the actual owner's storage, never expected row metadata.
struct SonicModelProbeAccess {
    static const auto &pairs(const SonicModelAssets &assets) { return assets.declarations_.pairs; }
};
}

namespace {
using namespace sadx;
using Id = character_pack::ResourceId;
std::uint32_t bits(float value) {
    std::uint32_t result;
    std::memcpy(&result, &value, 4);
    return result;
}
float scalar(std::uint32_t value) {
    float result;
    std::memcpy(&result, &value, 4);
    return result;
}
struct Stop { unsigned boundary; };
struct Calls final : sonic_model::Calls {
    TaskWorkArena &arena;
    const TaskHandle &task;
    std::uint16_t &status;
    const std::vector<collision_component::Info> &source;
    fidelity::Context &fp;
    float height;
    Calls(TaskWorkArena &arena, const TaskHandle &task, std::uint16_t &status,
          const std::vector<collision_component::Info> &source, fidelity::Context &fp, float height)
        : arena(arena), task(task), status(status), source(source), fp(fp), height(height) {}
    void join_vertices() override { throw Stop{1}; }
    void initialize_collision() override {
        (void)collision_component::initialize(arena, task, status, source, 2, 0, fp);
    }
    void store_collision_height() override {
        const auto work = collision_component::component(arena, task->primary_work);
        if (!work)
            throw Stop{3};
        work->info.at(0).a = height;
    }
};
} // namespace
// New, bounded prefix judge. Inputs are source-site live state, not a claimed ABI entry.
static int observe_sonic_model(const char *directory, const std::uint32_t *in,
                               std::uint32_t *out) {
    @autoreleasepool {
        try {
            const std::filesystem::path path(directory);
            const auto pack = character_pack::CharacterPack::load(
                path / "characters/sonic-selection.actions");
            character_pack::ObjectState objects(pack);
            const auto metadata = native::app::read_metadata(path / "characters/sonic-actor.json");
            auto assets = native::app::load_sonic_model_assets(metadata, pack);
            assets->bind_scene(objects);
            auto &state = assets->state();
            auto &root = objects.object(assets->root());
            root.sibling = in[1];
            root.position[0] = scalar(in[9]);
            state.model_sibling = in[2];
            state.joins.rows[11].destination_object = in[3] ? &objects.object(in[3]) : nullptr;
            state.joins.rows[10].source_depth = in[10];
            state.joins.rows[12].destination_depth = in[11];
            auto arena = std::make_shared<TaskWorkArena>();
            TaskScheduler scheduler;
            if (!scheduler.initialize_source_pool(arena, std::shared_ptr<void>{}))
                return 2;
            const auto task = scheduler.create_element(2, 3, nullptr).task;
            if (!task)
                return 3;
            if (in[8] != UINT32_MAX) {
                const auto free_start = *arena->free_head();
                const auto free_bytes = TaskWorkArena::usable_bytes - free_start;
                if (free_bytes > in[8] + 32 && !arena->allocate(free_bytes - in[8] - 32))
                    return 4;
            }
            std::uint16_t status = in[7], timer = in[5];
            std::int16_t join_mode = static_cast<std::int16_t>(in[4]);
            std::uint8_t mode = in[6];
            PlayerJoinTable previous;
            auto *join = &previous;
            sonic_model::Player player{join, join_mode, timer, mode};
            fidelity::Context fp(127);
            Calls calls(*arena, task, status, assets->collision(), fp, scalar(in[12]));
            unsigned boundary = 2;
            try {
                sonic_model::initialize_prefix(state, objects, assets->attach(), assets->root(),
                                              player, in[0] != 0, calls);
            } catch (const Stop &stop) { boundary = stop.boundary; }
            constexpr std::size_t kOutputWords = 1024;
            std::fill(out, out + kOutputWords, 0);
            auto id = [&](const character_pack::ObjectResource *object) -> Id {
                if (!object)
                    return 0;
                for (std::size_t index = 1; index <= pack.record_count(); ++index)
                    if (pack.record(index).kind == character_pack::ResourceKind::Object &&
                        &objects.object(index) == object)
                        return static_cast<Id>(index);
                throw std::runtime_error("foreign model object");
            };
            out[0] = boundary;
            out[1] = root.sibling;
            out[2] = state.model_sibling;
            out[3] = join == &state.joins ? 1 : 0;
            out[4] = static_cast<std::uint16_t>(join_mode);
            out[5] = timer;
            out[6] = mode;
            out[7] = status;
            const auto component = arena->reference(task->primary_work, 56).work;
            if (component) {
                const auto &work = *component->collision_state;
                out[8] = component->data_offset();
                out[9] = arena->reference(component, 12).work->data_offset();
                out[11] = bits(work.range);
                out[12] = work.info_count;
                out[13] = work.flags;
                for (unsigned index = 0; index < 2; ++index) {
                    const auto &info = work.info.at(index);
                    auto *dest = out + 251 + index * 12;
                    dest[0] = info.kind | (unsigned(info.form) << 8) |
                              (unsigned(info.push) << 16) | (unsigned(info.damage) << 24);
                    dest[1] = info.attributes;
                    dest[2] = bits(info.center.x);
                    dest[3] = bits(info.center.y);
                    dest[4] = bits(info.center.z);
                    dest[5] = bits(info.a);
                    dest[6] = bits(info.b);
                    dest[7] = bits(info.c);
                    dest[8] = bits(info.d);
                    for (unsigned axis = 0; axis < 3; ++axis)
                        dest[9 + axis] = static_cast<std::uint32_t>(info.angles[axis]);
                }
            }
            out[10] = arena->free_head().value_or(0);
            out[15] = fp.exception_flags();
            auto *object = out + 16;
            object[0] = root.evaluation_flags;
            object[1] = root.model;
            for (unsigned axis = 0; axis < 3; ++axis) {
                object[2 + axis] = bits(root.position[axis]);
                object[5 + axis] = static_cast<std::uint32_t>(root.rotation[axis]);
                object[8 + axis] = bits(root.scale[axis]);
            }
            object[11] = root.child;
            object[12] = root.sibling;
            const auto &pairs = native::SonicModelProbeAccess::pairs(*assets);
            std::size_t pair_output = 320;
            for (std::size_t index = 0; index < state.joins.rows.size(); ++index) {
                const auto &row = state.joins.rows[index];
                auto *dest = out + 29 + index * 6;
                dest[0] = id(row.object);
                dest[1] = id(row.source_object);
                dest[2] = id(row.destination_object);
                dest[3] = row.vertex_count | (unsigned(std::uint8_t(row.input_mode)) << 8) |
                          (unsigned(row.source_depth) << 16) | (unsigned(row.destination_depth) << 24);
                if (row.original_vertices)
                    throw std::runtime_error("prefix must not allocate join backups");
                std::size_t identity = 0;
                if (row.vertex_pairs) {
                    const auto found = std::find_if(pairs.begin(), pairs.end(), [&](const auto &pair) {
                        return &pair == row.vertex_pairs;
                    });
                    if (found == pairs.end())
                        throw std::runtime_error("foreign Sonic vertex-pair identity");
                    identity = static_cast<std::size_t>(found - pairs.begin()) + 1;
                }
                dest[5] = static_cast<std::uint32_t>(identity);
                const auto count = row.vertex_pairs ? row.vertex_pairs->size() : 0;
                if (pair_output + 1 + count > kOutputWords)
                    throw std::runtime_error("Sonic pair observation capacity");
                out[pair_output++] = static_cast<std::uint32_t>(count);
                if (row.vertex_pairs)
                    for (auto value : *row.vertex_pairs)
                        out[pair_output++] = value;
            }
            return 0;
        } catch (...) { return 1; }
    }
}

// Preserve the original 320-word probe ABI for retained callers.
extern "C" int sadx_sonic_model(const char *directory, const std::uint32_t *in,
                                std::uint32_t *out) {
    std::array<std::uint32_t, 1024> observed{};
    const auto result = observe_sonic_model(directory, in, observed.data());
    if (result == 0)
        std::copy_n(observed.data(), 320, out);
    return result;
}
extern "C" int sadx_sonic_model_observed(const char *directory, const std::uint32_t *in,
                                         std::uint32_t *out, std::size_t capacity) {
    if (capacity < 1024)
        return 2;
    return observe_sonic_model(directory, in, out);
}
