#include "player/player_initialization.hpp"
#include "player/sonic_model.hpp"
#include <algorithm>
#include <cstdint>
#include <vector>

namespace {
using namespace sadx;
struct Stop {};
struct Calls final : sonic_model::InitializationCalls {
    const std::uint32_t *input;
    sonic_model::State &state;
    character_pack::ObjectResource *objects;
    std::uint8_t alternate;
    std::uint32_t free[2];
    unsigned table = 7, callbacks = 0;
    std::vector<std::uint32_t> trace;
    Calls(const std::uint32_t *input, sonic_model::State &state,
          character_pack::ObjectResource *objects)
        : input(input), state(state), objects(objects), alternate(input[0]),
          free{input[10], input[11]} {}
    void call(unsigned event) {
        trace.push_back(event);
        if (input[8] == trace.size())
            throw Stop{};
    }
    void load_textures(std::uint8_t character) override {
        call(10 + character);
        alternate = input[character ? 2 : 1];
    }
    std::uint8_t metal() override { return alternate; }
    void clear_free_word(unsigned index) override { free[index] = 0; }
    std::uint8_t player_index() override { return input[7]; }
    void load_collision_task(std::uint8_t player) override { call(20 + (unsigned(player) << 8)); }
    void set_motion() override {
        call(30);
        alternate = input[3];
    }
    std::int16_t stage() override { return input[5]; }
    std::uint16_t act() override { return input[6]; }
    character_pack::ObjectResource *join_target() override { return objects + input[17] - 1; }
    void publish_join_table(PlayerJoinTable &value) override {
        table = &value == &state.joins ? 0 : 8;
    }
    void join_vertices() override {
        call(40);
        alternate = input[4];
    }
    void create_effect(unsigned flags) override { call(50 + flags); }
    void install_callbacks() override { callbacks = 3; }
};
} // namespace
// NOTSA: controlled source-site fixture. Production resources are qualified by
// the separate real callback witness; these lower calls expose ordered prefixes.
extern "C" int sadx_native_sonic_initialization(const std::uint32_t *in, std::uint32_t *out) {
    try {
        sadx::character_pack::ObjectResource objects[2]{};
        sadx::sonic_model::State state;
        for (unsigned table = 0; table < 2; ++table) {
            auto &rows = table ? state.metal_joins.rows : state.joins.rows;
            for (unsigned index = 0; index < 3; ++index) {
                sadx::PlayerJoinVertex row{};
                row.object = index < 2 ? objects + index : nullptr;
                row.input_mode = static_cast<std::int8_t>(in[13 + table * 2 + (index % 2)]);
                row.vertex_count = 23 + index;
                row.source_depth = 45 + index;
                row.destination_depth = 67 + index;
                rows.push_back(row);
            }
        }
        Calls calls(in, state, objects);
        unsigned stop = 0;
        try {
            sadx::sonic_model::initialize_resources(state, calls);
        } catch (const Stop &) {
            stop = calls.trace.size();
        }
        std::fill(out, out + 64, 0);
        out[0] = stop;
        out[1] = calls.alternate;
        out[2] = calls.free[0];
        out[3] = calls.free[1];
        out[4] = calls.table;
        out[5] = calls.callbacks;
        out[6] = calls.trace.size();
        std::copy(calls.trace.begin(), calls.trace.end(), out + 8);
        for (unsigned table = 0; table < 2; ++table) {
            const auto &rows = table ? state.metal_joins.rows : state.joins.rows;
            for (unsigned index = 0; index < 3; ++index) {
                const auto &row = rows[index];
                out[24 + table * 12 + index * 4] = static_cast<std::uint8_t>(row.input_mode);
                out[25 + table * 12 + index * 4] = row.vertex_count;
                out[26 + table * 12 + index * 4] = row.source_depth;
                out[27 + table * 12 + index * 4] = row.destination_depth;
            }
        }
        return 1;
    } catch (...) {
        return 0;
    }
}

extern "C" int sadx_native_sonic_collision_setup(const std::uint32_t *in, std::uint32_t *out) {
    try {
        auto arena = std::make_shared<sadx::TaskWorkArena>();
        sadx::TaskScheduler scheduler;
        if (!scheduler.initialize_source_pool(arena, std::shared_ptr<void>{}))
            return 0;
        if (in[0] == 1) {
            for (unsigned i = 0; i < 4096; ++i)
                if (!scheduler.create_element(0, 0, nullptr))
                    return 0;
        } else if (in[0] == 2) {
            const auto available = sadx::TaskWorkArena::usable_bytes - *arena->free_head();
            if (!arena->allocate(available - 160 - 32))
                return 0;
        }
        if (in[0] == 3) {
            const auto available = sadx::TaskWorkArena::usable_bytes - *arena->free_head();
            if (!arena->allocate(available - 64))
                return 0;
        }
        const std::array<sadx::sonic_model::ObjectId, 3> objects{in[2], in[3], in[4]};
        const auto success =
            sadx::sonic_model::load_collision_task(objects, in[1], scheduler, *arena);
        std::fill(out, out + 16, 0);
        out[0] = success;
        const auto task = scheduler.list_head(2);
        out[1] = bool(task);
        if (task) {
            out[2] = task->motion_work ? task->motion_work->requested_size() : 0;
            std::uint8_t player = 0;
            if (task->primary_state)
                player = sadx::player_initialization::bind_primary_view(task).player_index;
            else if (!arena->read(task->primary_work, 8, &player, 1))
                return 0;
            out[3] = player;
            out[4] = task->exec == sadx::TaskScheduler::destroy_callback;
            if (task->auxiliary_native_work) {
                const auto work = std::static_pointer_cast<sadx::sonic_model::CollisionTaskWork>(
                    task->auxiliary_native_work);
                if (work->allocation != task->motion_work)
                    return 0;
                std::copy(work->objects.begin(), work->objects.end(), out + 5);
            }
        }
        return 1;
    } catch (...) {
        return 0;
    }
}

extern "C" int sadx_native_sonic_common_links(const char *path, const std::uint32_t *in,
                                              std::uint32_t *out) {
    try {
        using namespace sadx;
        const auto pack = character_pack::CharacterPack::load(path);
        character_pack::ObjectState objects(pack);
        std::vector<sonic_model::ObjectId> ids;
        for (unsigned id = 1; id <= pack.record_count() && ids.size() < 9; ++id)
            if (pack.record(id).kind == character_pack::ResourceKind::Object)
                ids.push_back(id);
        if (ids.size() != 9)
            return 0;
        for (unsigned i = 0; i < ids.size(); ++i)
            objects.object(ids[i]).sibling = ids[(i + 1) % 9];
        struct Common final : sonic_model::CommonTailCalls {
            const std::uint32_t *in;
            const std::vector<sonic_model::ObjectId> &ids;
            std::vector<unsigned> trace;
            Common(const std::uint32_t *in, const std::vector<sonic_model::ObjectId> &ids)
                : in(in), ids(ids) {}
            std::uint16_t status() override { return in[0]; }
            std::uint8_t metal() override { return in[1]; }
            std::uint16_t equipment() override { return in[2]; }
            sonic_model::ObjectId model_slot(unsigned index) override {
                trace.push_back(index);
                if (in[3] == trace.size())
                    throw Stop{};
                const std::array<unsigned, 9> slots{3, 14, 15, 19, 20, 58, 60, 62, 63};
                for (unsigned i = 0; i < slots.size(); ++i)
                    if (slots[i] == index)
                        return ids[i];
                throw std::runtime_error("unexpected source slot");
            }
            void join_vertices() override {
                trace.push_back(100);
                throw Stop{};
            }
        } common(in, ids);
        try {
            sonic_model::update_common_model_links(objects, common);
        } catch (const Stop &) {
        }
        std::fill(out, out + 32, 0);
        out[0] = common.trace.size();
        std::copy(common.trace.begin(), common.trace.end(), out + 1);
        for (unsigned i = 0; i < ids.size(); ++i) {
            const auto id = objects.object(ids[i]).sibling;
            out[16 + i] = std::find(ids.begin(), ids.end(), id) - ids.begin();
        }
        return 1;
    } catch (...) {
        return 0;
    }
}
