// Reference-only representation adapter. This binary uses the actual arena/core.
#include "player/event_actions.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <map>
namespace e = sadx::event_actions;
namespace {
std::uint32_t bits(float value) {
    std::uint32_t result;
    std::memcpy(&result, &value, 4);
    return result;
}
float number(std::uint32_t value) {
    float result;
    std::memcpy(&result, &value, 4);
    return result;
}
struct Fixture final : e::ActionSource {
    sadx::TaskWorkArena arena;
    e::State state;
    e::Context context{arena, state, *this};
    sadx::TaskWorkHandle primary, event, replacement;
    std::map<const sadx::TaskWork *, unsigned> ids;
    std::array<sadx::TaskWorkHandle, 48> allocations{};
    std::array<sadx::texture_lifetime::List, 48> textures{};
    sadx::TaskHandle task;
    std::uint32_t object_value, motion_value;
    const std::uint32_t *v;
    std::vector<std::uint32_t> trace;
    unsigned missing = 0;
    explicit Fixture(const std::uint32_t *input)
        : object_value(input[6]), motion_value(input[7]), v(input) {}
    unsigned id(const sadx::TaskWorkHandle &work) const { return work ? ids.at(work.get()) : 0; }
    sadx::TaskWorkHandle create(unsigned identity, unsigned size) {
        auto allocation = arena.allocate(size).work;
        arena.fill(allocation, 0, 0, size);
        allocations[identity] = allocation;
        ids[allocation.get()] = identity;
        return allocation;
    }
    unsigned reference(const sadx::TaskWorkHandle &owner, unsigned field) {
        const auto r = arena.reference(owner, field);
        if (!r && r.failure != sadx::TaskWorkFailure::ReleasedWork)
            return 0xffffffffU;
        return id(r.work);
    }
    void snapshot(const sadx::TaskWorkHandle &work, std::uint32_t *out, unsigned bytes,
                  bool event) {
        for (unsigned at = 0; at < bytes; at += 4) {
            if ((event && (at == 64 || at == 68)) || (!event && at == 60))
                out[at / 4] = reference(work, at);
            else {
                std::uint32_t value = 0;
                arena.read(work, at, &value, 4);
                out[at / 4] = value;
            }
        }
    }
    sadx::PlayerEventActionNode &new_node() { return *allocations[32]->event_state->node; }
    std::uint32_t alias_word(unsigned offset) {
        auto &node = new_node();
        switch (offset) {
        case 0:
            return static_cast<std::uint8_t>(node.mode) |
                   (std::uint32_t(static_cast<std::uint8_t>(node.link_frames)) << 8);
        case 4:
            return node.action.object;
        case 8:
            return node.action.motion;
        case 12:
            return node.textures ? 900 : 0;
        case 16:
            return reference(allocations[32], 16);
        case 20:
            return bits(node.speed);
        default:
            return 0;
        }
    }
    std::uint32_t object(sadx::ActionHandle handle) override {
        if (!handle.identity)
            throw e::Error(e::Boundary::ActionObject);
        return v[13] ? alias_word((v[13] - 1) * 4) : ((v[2] & 16) ? v[6] : object_value);
    }
    std::uint32_t motion(sadx::ActionHandle handle) override {
        if (!handle.identity || (v[2] & 16))
            throw e::Error(e::Boundary::ActionMotion);
        return v[13] ? alias_word(v[13] * 4) : motion_value;
    }
    void setup() {
        primary = create(40, 64);
        replacement = create(41, 64);
        if (v[15]) {
            for (const auto &work : {primary, replacement}) {
                std::array<std::uint8_t, 64> bytes{};
                const unsigned salt = work == primary ? 41 : 73;
                for (unsigned offset = 0; offset < bytes.size(); ++offset)
                    bytes[offset] = 1 + ((v[15] % 255 + salt + 37 * offset) % 255);
                if (!arena.write(work, 0, bytes.data(), bytes.size()))
                    throw std::runtime_error("primary sentinel setup failed");
            }
        }
        event = create(42, 124);
        auto other = create(43, 124);
        for (auto work : {event, other}) {
            work->event_state = std::make_shared<e::Work>();
            std::array<std::uint8_t, 124> bytes{};
            for (unsigned n = 0; n < bytes.size(); ++n)
                bytes[n] = static_cast<std::uint8_t>(17 + 37 * n);
            arena.write(work, 0, bytes.data(), 124);
            arena.set_reference(work, 64, {});
            arena.set_reference(work, 68, {});
        }
        const std::uint8_t initial_mode = 129;
        arena.write(event, 0, &initial_mode, 1);
        std::uint32_t timer = 0x7fc01234;
        arena.write(event, 60, &timer, 4);
        arena.set_reference(primary, 60, (v[2] & 4) ? sadx::TaskWorkHandle{} : event);
        arena.set_reference(replacement, 60, other);
        for (unsigned n = 1; n <= v[1]; ++n) {
            const auto allocation = create(n, 24);
            allocation->event_state = std::make_shared<e::Work>();
            auto node = std::make_shared<sadx::PlayerEventActionNode>();
            node->mode = static_cast<std::uint8_t>(n);
            node->link_frames = static_cast<std::uint8_t>(n + 20);
            node->action = {100 + n, 200 + n};
            node->textures = &textures[n];
            node->speed = number(0x3f000000 + n);
            node->allocation = allocation;
            allocation->event_state->node = node;
            state.node_identities.push_back(allocation);
            arena.set_reference(n == 1 ? event : allocations[n - 1], n == 1 ? 64 : 16, allocation);
        }
        if ((v[14] == 4 || v[14] >= 6) && v[12] != 1)
            create(31, 24);
        if (v[12] == 1) {
            const auto old = create(31, 24);
            arena.set_reference(event, 68, old);
        } else if (v[12] == 2 && v[1])
            arena.set_reference(event, 68, allocations[1]);
        if (v[11]) {
            const auto head = arena.free_head().value();
            for (const auto &block : arena.blocks())
                if (block.offset == head) {
                    arena.allocate(*block.physical_next - head - 32);
                    break;
                }
        }
        context.allocate = [&](unsigned bytes) {
            const auto result = arena.allocate(bytes);
            trace.insert(trace.end(), {1, bytes, result ? 32U : 0U});
            if (result) {
                allocations[32] = result.work;
                ids[result.work.get()] = 32;
            }
            switch (v[14]) {
            case 1:
                task->primary_work = replacement;
                break;
            case 2:
                arena.set_reference(primary, 60, allocations[43]);
                break;
            case 3:
                arena.set_reference(event, 64, {});
                break;
            case 4:
                if (v[1])
                    arena.set_reference(allocations[v[1]], 16, allocations[31]);
                break;
            case 5:
                object_value ^= 0xffffffffU;
                motion_value ^= 0x12345678U;
                break;
            default:
                break;
            }
            return result;
        };
        context.release = [&](const sadx::TaskWorkHandle &work) {
            trace.insert(trace.end(), {2, id(work), 0});
            if (work->released())
                return arena.release(work);
            if (v[14] == 6)
                arena.set_reference(event, 64, allocations[31]);
            if (v[14] == 7)
                arena.set_reference(work, 16, allocations[31]);
            if (v[14] == 8)
                arena.set_reference(event, 68, allocations[31]);
            if (v[14] == 9) {
                const std::uint32_t timer = 0x3f800000;
                arena.write(event, 60, &timer, 4);
            }
            return arena.release(work);
        };
    }
};
unsigned error(const e::Error &failure) {
    switch (failure.boundary) {
    case e::Boundary::EventReference:
        return 1;
    case e::Boundary::ActionHead:
        return 2;
    case e::Boundary::NodeMode:
        return 3;
    case e::Boundary::ActionObject:
        return 4;
    case e::Boundary::ActionMotion:
        return 5;
    case e::Boundary::EventMode:
        return 6;
    case e::Boundary::OldRelease:
        return 7;
    case e::Boundary::RegisteredPlayer:
        return 8;
    default:
        return 99;
    }
}
} // namespace
extern "C" unsigned sadx_event_actions_probe(const std::uint32_t *v, std::uint32_t *out) {
    std::fill(out, out + 512, 0);
    Fixture f(v);
    f.setup();
    sadx::TaskScheduler scheduler;
    f.task = scheduler.append_task(1, nullptr).task;
    f.task->primary_work = (v[2] & 2) ? sadx::TaskWorkHandle{} : f.primary;
    std::array<std::shared_ptr<sadx::PlayerTaskState>, 8> registered{};
    auto primary = std::make_shared<sadx::PlayerTaskState>();
    primary->orientation.flags = static_cast<std::uint16_t>(v[6]);
    primary->secondary_mode_at_1 = static_cast<std::uint8_t>(v[7]);
    auto alternate = std::make_shared<sadx::PlayerTaskState>();
    alternate->orientation.flags = 0;
    alternate->secondary_mode_at_1 = 0;
    if (!(v[2] & 32) && (v[8] & 255) < 8)
        registered[v[8] & 255] = primary;
    try {
        const auto task = (v[2] & 1) ? sadx::TaskHandle{} : f.task;
        auto *texture = v[10] == 1 ? nullptr : &f.textures[v[10] == 2 ? 47 : 0];
        switch (v[0]) {
        case 0:
            e::set_input(registered, static_cast<std::uint8_t>(v[8]),
                         static_cast<std::int8_t>(v[9]),
                         [&](sadx::PlayerTaskState &state, std::uint8_t mode) {
                             if (v[14] == 1)
                                 registered[v[8] & 255] = alternate;
                             if (v[14] == 2)
                                 registered[v[8] & 255].reset();
                             state.secondary_mode_at_1 = mode;
                         });
            break;
        case 1:
            e::initialize(f.context, f.task->primary_work);
            break;
        case 2:
            e::set_action(f.context, task, {(v[2] & 8) ? 0U : 1U}, texture, number(v[5]), v[3],
                          v[4]);
            break;
        case 3:
            e::append_action(f.context, f.task->primary_work, {(v[2] & 8) ? 0U : 1U}, texture,
                             number(v[5]), v[3], v[4]);
            break;
        case 4:
            e::clear_actions(f.context, f.task->primary_work);
            break;
        case 5:
            e::clear_action(f.context, task);
            break;
        }
    } catch (const e::Error &failure) {
        out[0] = error(failure);
        switch (out[0]) {
        case 1:
            out[13] = v[0] == 1 ? 0x417e85 : (v[0] == 4 || v[0] == 5) ? 0x417ea6 : 0x417ef5;
            break;
        case 2:
            out[13] = (v[0] == 4 || v[0] == 5) ? 0x417ea9 : 0x417ef8;
            break;
        case 3:
            out[13] = 0x417f21;
            break;
        case 4:
            out[13] = 0x417f2a;
            break;
        case 5:
            out[13] = 0x417f2f;
            break;
        case 6:
            out[13] = 0x417e9b;
            break;
        case 7:
            out[13] = 0x40b310;
            break;
        default:
            out[13] = 0xffffffffU;
            break;
        }
    }
    out[15] = out[0] == 0;
    out[1] = f.reference(f.primary, 60);
    out[2] = f.reference(f.event, 64);
    out[3] = f.reference(f.event, 68);
    std::uint8_t mode = 0;
    std::uint32_t timer = 0;
    f.arena.read(f.event, 0, &mode, 1);
    f.arena.read(f.event, 60, &timer, 4);
    out[4] = mode;
    out[5] = timer;
    out[6] = primary->orientation.flags;
    out[7] = primary->secondary_mode_at_1.value();
    out[8] = f.id(f.task->primary_work);
    out[9] = static_cast<unsigned>(f.trace.size() / 3);
    const auto current = registered[v[8] & 255];
    out[10] = current ? (current == primary ? 1 : 2) : 0;
    out[11] = alternate->orientation.flags;
    out[12] = alternate->secondary_mode_at_1.value();
    std::copy(f.trace.begin(), f.trace.end(), out + 16);
    for (unsigned n = 1; n <= 32; ++n) {
        const auto a = f.allocations[n];
        if (!a)
            continue;
        auto *dst = out + 64 + (n - 1) * 10;
        dst[0] = 1;
        dst[1] = a->released();
        if (a->event_state && a->event_state->node) {
            const auto &node = *a->event_state->node;
            dst[2] = static_cast<std::uint8_t>(node.mode);
            dst[3] = static_cast<std::uint8_t>(node.link_frames);
            dst[4] = node.action.object;
            dst[5] = node.action.motion;
            dst[6] = node.textures
                         ? (n == 32 ? (node.textures == &f.textures[47] ? 901 : 900) : 300 + n)
                         : 0;
            dst[7] = a->released() ? 0 : f.reference(a, 16);
            dst[8] = bits(node.speed);
            // Source padding is zero-filled; live native arena storage supplies it.
            if (!a->released()) {
                std::uint16_t padding = 0;
                f.arena.read(a, 2, &padding, 2);
                dst[9] = padding;
            }
        } else if (n == 32 && a->event_state) {
            std::uint8_t event_mode = 0;
            f.arena.read(a, 0, &event_mode, 1);
            dst[2] = event_mode;
        }
    }
    f.snapshot(f.primary, out + 384, 64, false);
    f.snapshot(f.event, out + 400, 124, true);
    f.snapshot(f.allocations[43], out + 432, 124, true);
    if (f.allocations[32] && f.allocations[32]->requested_size() == 124)
        f.snapshot(f.allocations[32], out + 464, 124, true);
    f.snapshot(f.replacement, out + 495, 64, false);
    return 1;
}
