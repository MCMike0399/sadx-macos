#include "player/gamma_selection_core.hpp"
#include <algorithm>
#include <cstring>
#include <vector>

namespace sadx::gamma_selection {
namespace {
float number(std::uint32_t v) { float f; std::memcpy(&f, &v, 4); return f; }
std::uint32_t bits(float f) { std::uint32_t v; std::memcpy(&v, &f, 4); return v; }
struct Stop { std::uint32_t boundary; };
struct ExtraData {
    std::uint8_t laser = 0;
    std::uint16_t timer = 0;
    float ground = 0, distance = 0;
    std::uint32_t attributes = 0;
    float walk = 0;
};
void callback(TaskScheduler &, const TaskHandle &) { throw Stop{0x47fd50}; }
void destructor(TaskScheduler &, const TaskHandle &) { throw Stop{0x47eed0}; }
constexpr std::uint32_t addresses[]{0, 0x42fb00, 0x426690, 0x4c3780, 0x4258f0,
    0x481460, 0x47fb90, 0x442750, 0x41caf0, 0x421280, 0x40b2d0, 0x4c2e10,
    0x4c5420, 0x40b940, 0x49fb10, 0x44a800, 0x43fa90, 0x4c3870, 0x4b82d0,
    0x784fb0, 0x4c2e90, 0x47fd50, 0x4c4da0, 0x4c44a0, 0x481180, 0x49ee30,
    0x4c2f50, 0x4c33c0, 0x41bac0, 0x41c280, 0x446f10, 0x40b420};
}
struct Context {
    const std::uint32_t *w;
    X87Context fp;
    std::shared_ptr<TaskWorkArena> arena = std::make_shared<TaskWorkArena>();
    TaskScheduler scheduler;
    TaskHandle task;
    std::array<Player, 2> players;
    std::array<TaskWorkHandle, 2> extras;
    std::array<ExtraData, 2> extra_data;
    Player live;
    TaskWorkHandle published_primary, published_player;
    State globals;
    std::array<std::array<SceneMaterial, 7>, 6> materials{};
    std::array<std::uint32_t,2> line_colors{};
    std::array<std::uint8_t, 2> damage{{0xa5, 0x5a}};
    std::uint32_t collision_flags = 0x11223344;
    float collision_extent = 0;
    bool collision = false;
    std::array<bool,2> table{}, weld{};
    unsigned calls = 0;
    std::vector<std::uint32_t> trace;
    bool native_initialize = false;
    player_initialization::Registry registry;
    player_initialization::ParameterTable parameters{};
    PlayerEventFlags events{};
    explicit Context(const std::uint32_t *input, std::uint16_t cw) : w(input), fp(cw) {
        if (!scheduler.initialize_source_pool(arena, std::shared_ptr<void>{}))
            throw Stop{1};
        task = scheduler.create_element(7, 3, nullptr).task;
        player_initialization::establish_zeroed_primary(scheduler, task);
        player_initialization::establish_zeroed_motion(scheduler, task);
        auto p = player_initialization::bind_primary_view(task);
        p.status = w[0]; p.mode = w[1]; p.id = w[2]; p.player_index = w[3];
        p.character = 99; p.position.y = number(w[17]); p.rotation = {111, 222, 333};
        task->motion_state->motion.speed.y = number(w[5]);
        for (unsigned n = 0; n < 2; ++n) {
            players[n] = std::make_shared<player_initialization::Player>();
            players[n]->task = task;
            players[n]->allocation = arena->allocate(512).work;
            if (!arena->fill(players[n]->allocation,0,0,512)) throw Stop{2};
            players[n]->physics = std::make_shared<PlayerPhysicsState>();
            players[n]->parameters.height = number(w[26]);
            auto &physics = *players[n]->physics;
            physics.source_flags_at_6 = w[10]; physics.timer_at_80 = 0x1234;
            physics.requested_animation_at_124 = 77; physics.current_animation_at_126 = 12;
            physics.motion.speed.x = number(w[16]);
            extras[n] = arena->allocate(580).work;
            extra_data[n].laser = w[9]; extra_data[n].timer = w[22];
            extra_data[n].walk = number(0x12345678);
        }
        for (auto &p : players)
            if (!arena->set_reference(p->allocation,0x7c,extras[1])) throw Stop{2};
        if (w[4]) live = players[0];
        globals.hover = 0x7b; globals.previous_hover = 0x6a;
        globals.time_zero = 0x59; globals.event_enabled = 0x48;
        globals.time_hud_enabled = 0x37;
        collision = w[25];
        for (auto &row : materials)
            for (auto &entry : row) entry.words = {0x10203040, 2, 3, 4, w[21]};
    }
    unsigned player_id(const Player &p) const { return !p ? 0 : p == players[0] ? 1 : 2; }
    unsigned work_id(const TaskWorkHandle &p) const {
        return !p ? 0 : p == extras[0] ? 1 : p == extras[1] ? 2 : 3;
    }
    unsigned allocation_id(const TaskWorkHandle &p) const {
        return !p ? 0 : p == players[0]->allocation ? 1 : p == players[1]->allocation ? 2 : 3;
    }
    TaskWorkHandle extra_handle(const Player &p) {
        const auto result = arena->reference(p->allocation,0x7c);
        if (!result) throw Stop{0x48409d};
        return result.work;
    }
    std::array<std::uint32_t, 64> snapshot() {
        auto p = player_initialization::bind_primary_view(task);
        auto active = live ? live : players[1];
        auto &physics = *active->physics;
        std::array<std::uint32_t, 64> s{};
        s[0] = !published_primary ? 0 : published_primary == task->primary_work ? 1 : 3;
        s[1] = allocation_id(published_player);
        s[2] = globals.hover; s[3] = globals.previous_hover; s[4] = globals.time_zero;
        s[5] = globals.event_enabled; s[6] = p.mode; s[7] = p.id; s[8] = p.player_index;
        s[9] = p.character; s[10] = p.status; s[11] = p.rotation[0]; s[12] = p.rotation[2];
        s[13] = player_id(live); s[14] = table[player_id(active)-1];
        s[15] = *physics.requested_animation_at_124;
        s[16] = *physics.current_animation_at_126; s[17] = weld[player_id(active)-1];
        s[18] = work_id(extra_handle(active)); s[19] = static_cast<std::uint16_t>(*physics.timer_at_80);
        s[20] = task->destructor ? 0x47eed0 : 0; s[21] = task->display ? 0x47fd50 : 0;
        s[22] = bits(extra_data[0].walk); s[23] = globals.line_points ? 1 : 0;
        s[24] = globals.line_texture ? 1 : 0; s[25] = globals.line_point_count;
        s[26] = globals.line_colors ? 1 : 0;
        s[27] = globals.headlight_material ? 2 : 0;
        s[28] = globals.time_hud_enabled; s[29] = work_id(globals.current_extra);
        for (unsigned n = 0; n < 5; ++n) {
            const unsigned row = n == 0 ? 1 : 6 - n;
            const auto actual_row = (w[31] & 256) && row > 1 ? 2 : row;
            s[30+n] = materials[actual_row][n == 0 ? 6 : 2].words[4];
            if (n) s[34+n] = materials[actual_row][2].words[0];
        }
        s[39] = bits(globals.node54_matrix[0]); s[40] = bits(globals.node30_matrix[0]);
        s[41] = bits(globals.node39_matrix[0]); s[42] = bits(globals.node2_matrix[0]);
        s[43] = bits(globals.node37_matrix[0]);
        s[44] = damage[0]; s[45] = damage[1]; s[46] = collision_flags;
        s[47] = bits(collision_extent); s[48] = bits(active->parameters.eyes_height);
        for (unsigned n = 0; n < 2; ++n) {
            const auto &e = extra_data[n]; const auto i = 49 + n * 4;
            s[i] = bits(e.ground); s[i+1] = bits(e.distance); s[i+2] = e.timer; s[i+3] = e.attributes;
        }
        return s;
    }
    void receipt(unsigned kind, unsigned a = 0, unsigned b = 0, unsigned c = 0) {
        trace.insert(trace.end(), {kind, a, b, c});
        auto s = snapshot(); trace.insert(trace.end(), s.begin(), s.end());
        if (kind && ++calls == w[12]) throw Stop{addresses[kind]};
    }
};
State &state(Context &c) { return c.globals; }
CurrentView current(Context &c) { return {c.published_primary, c.published_player}; }
Primary primary(Context &c, const TaskWorkHandle &w) {
    if (w != c.task->primary_work) throw Stop{0x48343f};
    return player_initialization::bind_primary_view(c.task);
}
Player player_for_motion(Context &c, const TaskWorkHandle &) { return c.live; }
TaskWorkHandle player_allocation(Context &, const Player &p) { return p ? p->allocation : TaskWorkHandle{}; }
TaskWorkHandle extra_work(Context &c, const Player &p) {
    if (!p) throw Stop{0x48409d};
    return c.extra_handle(p);
}
void store_extra_work(Context &c, const Player &p, const TaskWorkHandle &w) {
    if (!c.arena->set_reference(p->allocation,0x7c,w)) throw Stop{0x483662};
}
ExtraView extra(Context &c, const TaskWorkHandle &w) {
    if (!w) throw Stop{0x483675};
    auto &e = c.extra_data[c.work_id(w) - 1];
    return {e.laser, e.timer, e.ground, e.distance, e.attributes, e.walk};
}
std::uint16_t &player_timer(Context &, const Player &p) {
    return reinterpret_cast<std::uint16_t &>(*p->physics->timer_at_80);
}
std::uint16_t &player_flags(Context &, const Player &p) {
    if (!p) throw Stop{0x483534};
    return *p->physics->source_flags_at_6;
}
std::uint16_t &requested_animation(Context &, const Player &p) { return *p->physics->requested_animation_at_124; }
std::uint16_t &current_animation(Context &, const Player &p) { return *p->physics->current_animation_at_126; }
float &eyes_height(Context &, const Player &p) { return p->parameters.eyes_height; }
float player_height(Context &, const Player &p) { return p->parameters.height; }
float forward_speed(Context &, const Player &p) { return p->physics->motion.speed.x; }
float vertical_speed(Context &c, const TaskWorkHandle &) { return c.task->motion_state->motion.speed.y; }
bool has_held_task(Context &c, const Player &) { return c.w[13] & 1; }
bool has_collision(Context &c, const TaskWorkHandle &) { return c.collision; }
std::uint8_t &collision_damage(Context &c, const TaskWorkHandle &, unsigned n) { return c.damage[n]; }
std::uint32_t &collision_attributes(Context &c, const TaskWorkHandle &, unsigned) { return c.collision_flags; }
float &collision_first_extent(Context &c, const TaskWorkHandle &) { return c.collision_extent; }
std::int32_t floor_angle(Context &c, const TaskWorkHandle &, unsigned n) {
    if (!c.collision) throw Stop{0x4834aa};
    return c.w[n == 0 ? 27 : 28];
}
SceneMaterial &material(Context &c, unsigned row, unsigned index) {
    if ((c.w[31] & 255) == row) {
        constexpr std::uint32_t faults[]{0,0x4836b2,0x483736,0x483716,0x4836f6,0x4836d3};
        throw Stop{faults[row]};
    }
    return c.materials[(c.w[31] & 256) && row > 1 ? 2 : row][index];
}
ColorPointer line_colors(Context &c) { return c.line_colors.data(); }
TaskCallback display_callback(Context &) { return callback; }
TaskCallback destructor_callback(Context &) { return destructor; }
fidelity::ExactFloat exact(Context &c, float f) { return fidelity::ExactFloat(c.fp, f); }
void bind_animation_table(Context &c, const Player &p) { c.table[c.player_id(p)-1] = true; }
void bind_weld_table(Context &c, const Player &p) { c.weld[c.player_id(p)-1] = true; }
[[noreturn]] void unrecovered_mode(Context &, std::uint8_t) { throw Stop{0x4835ca}; }
std::int32_t event_cancel(Context &c) { c.receipt(1); return c.w[6]; }
std::uint8_t is_time_zero(Context &c) { c.receipt(2); return c.w[7]; }
void update_laser_mode(Context &c, const TaskWorkHandle &w, std::uint8_t i, bool b) { c.receipt(3,c.work_id(w),i,b); }
std::int32_t check_edit_mode(Context &c) { c.receipt(4); return c.w[8]; }
void process_action(Context &c, const TaskHandle &, const TaskWorkHandle &, const Player &p) { c.receipt(5,c.player_id(p)); }
void update_action_state(Context &c, Primary, const Player &p) { c.receipt(6,c.player_id(p)); }
void initialize_player(Context &c, std::int32_t index, const TaskHandle &) {
    c.receipt(7,index);
    if (c.native_initialize) {
        player_initialization::NativeAccess access(c.parameters,c.events,callback);
        player_initialization::initialize(index,c.task,c.task->motion_state,c.scheduler,
                                         c.registry,access,c.fp,c.live);
        c.players[1] = c.live;
        return;
    }
    c.live = c.players[1];
    auto p = player_initialization::bind_primary_view(c.task);
    p.player_index = c.w[23]; p.id = c.w[24];
}
void initialize_collision(Context &c, const TaskHandle &, unsigned n, std::uint8_t id) {
    c.receipt(8,n,id); c.collision = true;
    if (c.w[13] & 16) c.live->parameters.height = number(c.w[26] ^ 0x80000000);
}
void load_character_textures(Context &c, std::uint8_t n) { c.receipt(9,n); }
TaskWorkHandle allocate_zero(Context &c, unsigned n, unsigned bytes) {
    c.receipt(10,n,bytes);
    if (c.w[11]) return {};
    if (c.native_initialize) {
        const auto allocation = c.arena->allocate(n * bytes);
        if (!allocation || !c.arena->fill(allocation.work,0,0,n * bytes)) return {};
        c.extras[0] = allocation.work;
    }
    c.extra_data[0] = {};
    return c.extras[0];
}
void initialize_cursors(Context &c, const TaskWorkHandle &w) { c.receipt(11,c.work_id(w),0xb0,0xd0); }
void load_setup_resource(Context &c) { c.receipt(12); }
void create_time_hud(Context &c, unsigned n, const TaskHandle &) { c.receipt(13,n,0x4c51d0); }
void initialize_effects(Context &c) { c.receipt(14); }
void set_motion(Context &c, const Player &p) {
    c.receipt(15,c.player_id(p));
    *p->physics->requested_animation_at_124 = c.w[14];
    *p->physics->current_animation_at_126 = c.w[15];
    if (c.w[13] & 8) player_initialization::bind_primary_view(c.task).player_index = c.w[23] ^ 7;
    if (c.w[13] & 64) c.live = c.players[0];
}
void join_vertices(Context &c, Primary, const TaskWorkHandle &, const Player &p) { c.receipt(16,c.player_id(p)); }
void update_laser_targets(Context &c, const TaskWorkHandle &w, std::uint8_t i) { c.receipt(17,c.work_id(w),i); }
void get_model_matrix(Context &c, unsigned n, NinjaMatrix &m) { c.receipt(18,n); m[0] = number(n); }
void rotate_z(Context &c, NinjaMatrix &m, std::int32_t a) { c.receipt(19,a); m[0] = number(a); }
void update_fan(Context &c, Primary, const Player &p, const TaskWorkHandle &w) { c.receipt(20,c.player_id(p),c.work_id(w)); }
void display(Context &c, const TaskHandle &) {
    c.receipt(21);
    if (c.w[30]) store_extra_work(c,c.live,c.extras[1]);
}
void update_target_effects(Context &c, Primary, const Player &p, const TaskWorkHandle &w) { c.receipt(22,c.player_id(p),c.work_id(w)); }
void update_gun_effects(Context &c, const TaskHandle &, const Player &p, const TaskWorkHandle &w) { c.receipt(23,c.player_id(p),c.work_id(w)); }
void update_held_task(Context &c, Primary, const TaskWorkHandle &, const Player &p) { c.receipt(24,c.player_id(p)); }
fidelity::ExactFloat shadow(Context &c, Primary, float scale) {
    c.receipt(25,bits(scale));
    if (c.w[13] & 2) store_extra_work(c,c.live,c.extras[1]);
    if (c.w[13] & 32)
        return fidelity::ExactFloat::retained(c.fp,c.fp.from_double(1.000000059604644775390625));
    return exact(c,number(c.w[18]));
}
std::uint32_t ground_attributes(Context &c, Primary) {
    c.receipt(26);
    if (c.w[13] & 4) store_extra_work(c,c.live,c.extras[0]);
    return c.w[29];
}
void running_effects(Context &c, Primary, const TaskWorkHandle &, const Player &p) { c.receipt(27,c.player_id(p)); }
void calculate_collision_range(Context &c, Primary) { c.receipt(28); }
void enter_collision_list(Context &c, Primary) { c.receipt(29); }
void underwater(Context &c, Primary, const TaskWorkHandle &, const Player &p) { c.receipt(30,c.player_id(p)); }
void run_children(Context &c, const TaskHandle &) { c.receipt(31); }
std::int16_t stage(Context &c) { return c.w[19]; }
std::uint16_t act(Context &c) { return c.w[20]; }
} // namespace sadx::gamma_selection

extern "C" int sadx_native_gamma_selection(const std::uint32_t *input, std::size_t count,
    std::uint16_t cw, std::uint32_t *output, std::size_t capacity, std::uint32_t *metadata) {
    if (!input || count != 32 || !output || !metadata) return 1;
    try {
        sadx::gamma_selection::Context context(input,cw);
        std::uint32_t boundary = 0;
        try { sadx::gamma_selection::run(context,context.task); }
        catch (const sadx::gamma_selection::Stop &stop) { boundary = stop.boundary; }
        context.receipt(0,boundary);
        if (context.trace.size() > capacity) return 2;
        std::copy(context.trace.begin(),context.trace.end(),output);
        metadata[0] = context.trace.size(); metadata[1] = boundary;
        metadata[2] = context.fp.exception_flags(); metadata[3] = context.calls;
        return 0;
    } catch (...) { return 3; }
}
