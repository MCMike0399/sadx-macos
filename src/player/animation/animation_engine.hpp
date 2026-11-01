#pragma once
#include "cpu/fidelity_float.hpp"
#include "player/animation/animation_content.hpp"
#include "player/animation/animation_outcome.hpp"
#include "player/player_scene_resources.hpp"
#include <cstring>
#include <memory>
#include <vector>
namespace sadx::animation::detail {
// NOTSA: shared algorithms use lazy access bindings for native values and reference facts.
template <class Access> class Engine {
    using ExactFloat = fidelity::ExactFloat;
    static constexpr std::uint32_t kAngleTurn = 65536;
    Access &access;
    using PlayerAnimationView = typename Access::PlayerAnimationView;
    using PlayerAnimationRuntime = typename Access::PlayerAnimationRuntime;
    using PlayerAnimationRecord = typename Access::PlayerAnimationRecord;
    using PlayerAnimationAction = typename Access::PlayerAnimationAction;
    using PlayerAnimationMotion = typename Access::PlayerAnimationMotion;
    using PlayerAnimationTable = typename Access::PlayerAnimationTable;
    using PlayerAnimationNode = typename Access::PlayerAnimationNode;
    using PlayerAnimationKeyBytes = typename Access::PlayerAnimationKeyBytes;
    using PlayerAnimationAllocatedAction = typename Access::PlayerAnimationAllocatedAction;
    using PlayerAnimationAllocatedMotion = typename Access::PlayerAnimationAllocatedMotion;
    using AnimationResourceReference = typename Access::AnimationResourceReference;

  public:
    explicit Engine(Access &binding) : access(binding) {}

    [[noreturn]] void stop(Failure failure, Phase at, Phase callee = Phase::None) {
        Outcome r;
        r.failure = failure;
        r.phase = at;
        (void)callee;
        throw Stop{r};
    }
    Outcome complete() {
        Outcome r;
        r.completed = true;
        return r;
    }
    template <class T> decltype(auto) read_value(const T &value, Phase phase) {
        return access.read(value, phase);
    }
    const auto &table(const PlayerAnimationView &view, Phase phase) {
        return access.table(view, phase);
    }
    const PlayerAnimationRecord &record(const PlayerAnimationView &v, std::uint16_t index,
                                        Phase at) {
        const auto &t = table(v, at);
        auto r = t.records.find(index);
        if (r == t.records.end())
            stop(Failure::UnresolvedResource, at);
        return r->second;
    }
    const PlayerAnimationAction &action(const PlayerAnimationRecord &r,
                                        const PlayerAnimationRuntime &rt, Phase at) {
        const auto &ref = read_value(r.action, at);
        if (!ref.id)
            stop(Failure::NullResource, at);
        auto a = rt.resources.actions.find(*ref.id);
        if (a == rt.resources.actions.end())
            stop(Failure::UnresolvedResource, at);
        return a->second;
    }
    const PlayerAnimationMotion &motion(const PlayerAnimationRecord &r,
                                        const PlayerAnimationRuntime &rt, Phase at) {
        const auto &a = action(r, rt, at);
        const auto &ref = read_value(a.motion, at);
        if (!ref.id)
            stop(Failure::NullResource, at);
        auto m = rt.resources.motions.find(*ref.id);
        if (m == rt.resources.motions.end())
            stop(Failure::UnresolvedResource, at);
        return m->second;
    }
    ExactFloat unsigned_frame(const PlayerAnimationRecord &r, const PlayerAnimationRuntime &rt,
                              Phase at, fidelity::Context &fp) {
        auto raw = read_value(motion(r, rt, at).frame_count, at);
        std::int32_t signed_value;
        std::memcpy(&signed_value, &raw, 4);
        auto value = ExactFloat::integer(fp, signed_value);
        if (signed_value < 0)
            value = (value + 4294967296.0f);
        return value;
    }
    std::uint32_t bits(float v) {
        std::uint32_t b;
        std::memcpy(&b, &v, 4);
        return b;
    }
    bool greater_or_unordered(X87Comparison c) {
        return c == X87Comparison::greater || c == X87Comparison::unordered;
    }
    void arena_result(const TaskWorkResult &result, Phase at) {
        if (!result) {
            Outcome r;
            r.failure = Failure::WorkFailed;
            r.phase = at;
            r.work_failure = result.failure;
            throw Stop{r};
        }
    }
    PlayerAnimationAllocatedAction &buffer(PlayerAnimationView &v, Phase at) {
        const auto &ref = read_value(access.state(v).buffer, at);
        if (!ref)
            stop(Failure::NullResource, at);
        return *ref;
    }
    PlayerAnimationAllocatedMotion &allocated_motion(PlayerAnimationView &v, Phase at) {
        auto &a = buffer(v, at);
        const auto &m = read_value(a.motion, at);
        if (!m)
            stop(Failure::NullResource, at);
        return *m;
    }
    void require_arena(const PlayerAnimationRuntime &rt, Phase at) {
        if (!rt.arena)
            stop(Failure::MissingArena, at);
    }
    void allocate_initial(PlayerAnimationView &v, PlayerAnimationRuntime &rt, std::uint16_t req) {
        access.state(v).current = req;
        access.state(v).mode = 1;
        access.state(v).start = 0;
        access.state(v).frame = 0;
        require_arena(rt, Phase::AllocateAction);
        auto allocation = rt.arena->allocate(8);
        if (!allocation && allocation.failure != TaskWorkFailure::exhausted)
            arena_result(allocation, Phase::AllocateAction);
        (void)table(v, Phase::AllocationTable);
        std::shared_ptr<PlayerAnimationAllocatedAction> a;
        if (allocation.work) {
            a = std::make_shared<PlayerAnimationAllocatedAction>();
            a->allocation = allocation.work;
        }
        access.state(v).buffer = a;
        const auto &selected =
            action(record(v, req, Phase::AllocationRow), rt, Phase::AllocationObject);
        const auto &object = read_value(selected.object, Phase::AllocationObject);
        if (!a)
            stop(Failure::NullResource, Phase::PublishObject);
        a->object = object;
        auto m_alloc = rt.arena->allocate(12);
        if (!m_alloc && m_alloc.failure != TaskWorkFailure::exhausted)
            arena_result(m_alloc, Phase::AllocateMotion);
        std::shared_ptr<PlayerAnimationAllocatedMotion> m;
        if (m_alloc.work) {
            m = std::make_shared<PlayerAnimationAllocatedMotion>();
            m->allocation = m_alloc.work;
        }
        a->motion = m;
        if (!m)
            stop(Failure::NullResource, Phase::InitializeMotion);
        m->frame_count = 2;
        m->type = 3;
        m->input_function = 2;
        auto data = rt.arena->allocate(111 * 24);
        if (!data && data.failure != TaskWorkFailure::exhausted)
            arena_result(data, Phase::AllocateNodeData);
        if (data.work)
            arena_result(rt.arena->fill(data.work, 0, 0, 111 * 24), Phase::AllocateNodeData);
        m->data = data.work;
    }
    void wrap_frame(PlayerAnimationView &v, const PlayerAnimationRuntime &rt,
                    const PlayerAnimationRecord &selected, Phase boundary, fidelity::Context &fp,
                    Outcome &result) {
        auto period = unsigned_frame(selected, rt, boundary, fp);
        while (
            !greater_or_unordered((period).compare(read_value(access.state(v).frame, boundary)))) {
            auto before = read_value(access.state(v).frame, boundary);
            access.state(v).frame = ((ExactFloat(fp, before) - period)).rounded();
            ++result.wrap_iterations;
            period = unsigned_frame(selected, rt, boundary, fp);
            if (bits(before) == bits(*access.state(v).frame) &&
                !greater_or_unordered((period).compare(*access.state(v).frame)))
                stop(Failure::LoopNoProgress, boundary);
        }
        while ((ExactFloat(fp, 0.0f)).compare(read_value(access.state(v).frame, boundary)) ==
               X87Comparison::greater) {
            auto before = *access.state(v).frame;
            auto p = unsigned_frame(selected, rt, boundary, fp);
            access.state(v).frame = ((p + before)).rounded();
            ++result.wrap_iterations;
            if (bits(before) == bits(*access.state(v).frame) &&
                (ExactFloat(fp, 0.0f)).compare(*access.state(v).frame) == X87Comparison::greater)
                stop(Failure::LoopNoProgress, boundary);
        }
    }

    // FUNCTION: SONIC 0x43f230 ResetPlayerAnimationFrame
    Outcome reset_player_animation_frame(PlayerAnimationView &v, const PlayerAnimationRuntime &rt,
                                         fidelity::Context &fp) {
        try {
            const auto mode = read_value(access.state(v).mode, Phase::ResetMode);
            if (mode == 6 || mode == 7) {
                const auto current = read_value(access.state(v).current, Phase::ResetCurrent);
                auto frame =
                    unsigned_frame(record(v, current, Phase::ResetRow), rt, Phase::ResetLength, fp);
                access.state(v).frame = ((frame - 1.0f)).rounded();
            } else if (mode == 3 || mode == 9 || mode == 10 || mode == 12) {
                access.state(v).frame = read_value(access.state(v).start, Phase::ResetStart);
                access.state(v).start = 0;
            } else
                access.state(v).frame = 0;
            return complete();
        } catch (const Stop &s) {
            return s.result;
        }
    }
    // FUNCTION: SONIC 0x446910 NextPlayerAnimation
    Outcome next_player_animation(PlayerAnimationView &v, const PlayerAnimationRuntime &rt,
                                  std::uint16_t from, fidelity::Context &fp) {
        try {
            auto next = static_cast<std::uint8_t>(
                read_value(record(v, from, Phase::NextRow).next, Phase::NextRow));
            access.state(v).requested = next;
            access.state(v).current = next;
            access.state(v).mode =
                read_value(record(v, next, Phase::NextMode).mode, Phase::NextMode);
            const auto nextnext = static_cast<std::uint8_t>(
                read_value(record(v, next, Phase::FollowingRow).next, Phase::FollowingRow));
            auto flags = read_value(access.state(v).flags, Phase::NextFlags);
            access.state(v).flags = flags | 1;
            access.state(v).next = nextnext;
            return reset_player_animation_frame(v, rt, fp);
        } catch (const Stop &s) {
            return s.result;
        }
    }
    // FUNCTION: SONIC 0x446950 BlendPlayerAnimation
    Outcome blend_player_animation(PlayerAnimationView &v, PlayerAnimationRuntime &rt,
                                   fidelity::Context &fp) {
        try {
            const auto current = read_value(access.state(v).current, Phase::BlendCurrent),
                       req = read_value(access.state(v).requested, Phase::BlendRequested);
            const auto old_type = read_value(
                motion(record(v, current, Phase::BlendCurrentRow), rt, Phase::BlendCurrentMotion)
                    .type,
                Phase::BlendCurrentType);
            const auto new_type = read_value(
                motion(record(v, req, Phase::BlendRequestedRow), rt, Phase::BlendRequestedMotion)
                    .type,
                Phase::BlendRequestedType);
            const auto site =
                ((old_type | new_type) & 4) ? Phase::BlendThreeChannels : Phase::BlendTwoChannels;
            auto result = build_player_animation_transition(v, rt, fp);
            if (!result) {
                result.nested_phase = result.phase;
                result.phase = site;
                result.nested_transition = true;
                if (result.failure == Failure::None)
                    result.failure = Failure::BlendFailed;
            }
            return result;
        } catch (const Stop &s) {
            return s.result;
        }
    }
    // FUNCTION: SONIC 0x43f9f0 ReleasePlayerAnimationKeys
    Outcome release_animation_keys(PlayerAnimationView &v, PlayerAnimationRuntime &rt) {
        try {
            auto req = read_value(access.state(v).requested, Phase::ReleaseRequest);
            auto &m = allocated_motion(v, Phase::ReleaseMotion);
            const auto count =
                read_value(record(v, req, Phase::ReleaseCount).object_count, Phase::ReleaseCount);
            auto data = read_value(m.data, Phase::ReleaseData);
            const auto channels = read_value(m.input_function, Phase::ReleaseLayout);
            if (!count)
                return complete();
            if (!data)
                stop(Failure::NullResource, Phase::ReleasePositionPointer);
            require_arena(rt, Phase::ReleasePositionKeys);
            std::uint32_t cursor = 0;
            for (std::uint32_t node = 0; node < count; ++node) {
                const auto stride = channels == 3 ? 24U : 16U;
                for (std::uint32_t channel = 0; channel < (channels == 3 ? 3U : 2U); ++channel) {
                    const auto slot = cursor + 4 * channel;
                    const Phase load_sites[3] = {Phase::ReleasePositionPointer,
                                                 Phase::ReleaseAnglePointer,
                                                 Phase::ReleaseScalePointer},
                                free_sites[3] = {Phase::ReleasePositionKeys,
                                                 Phase::ReleaseAngleKeys, Phase::ReleaseScaleKeys},
                                clear_sites[3] = {Phase::ClearPositionPointer,
                                                  Phase::ClearAnglePointer,
                                                  Phase::ClearScalePointer},
                                count_sites[3] = {Phase::ClearPositionCount, Phase::ClearAngleCount,
                                                  Phase::ClearScaleCount};
                    auto ref = rt.arena->reference_view(data, slot);
                    if (!ref) {
                        Outcome r;
                        r.failure = Failure::WorkFailed;
                        r.phase = load_sites[channel];
                        r.work_failure = ref.failure;
                        throw Stop{r};
                    }
                    if (ref.byte_offset)
                        stop(Failure::UnresolvedResource, free_sites[channel]);
                    if (ref.work) {
                        arena_result(rt.arena->release(ref.work), free_sites[channel]);
                        arena_result(rt.arena->set_reference(data, slot, {}), clear_sites[channel]);
                        arena_result(rt.arena->set_reference(data, slot + 4 * channels, {}),
                                     count_sites[channel]);
                    }
                }
                cursor += stride;
            }
            return complete();
        } catch (const Stop &s) {
            return s.result;
        }
    }
    // NOTSA: named steps preserve the original dispatcher and immediate publications.
    Outcome cleanup_animation(PlayerAnimationView &v, PlayerAnimationRuntime &rt) {
        auto released = release_animation_keys(v, rt);
        if (!released)
            return released;
        const auto &ref = read_value(access.state(v).buffer, Phase::CleanupBuffer);
        if (ref) {
            auto &m = allocated_motion(v, Phase::CleanupMotion);
            auto data = read_value(m.data, Phase::CleanupData);
            require_arena(rt, Phase::FreeNodeData);
            if (data) {
                arena_result(rt.arena->release(data), Phase::FreeNodeData);
                m.data = TaskWorkHandle{};
            }
            arena_result(rt.arena->release(m.allocation), Phase::FreeMotion);
            ref->motion = std::shared_ptr<PlayerAnimationAllocatedMotion>{};
            arena_result(rt.arena->release(ref->allocation), Phase::FreeAction);
            access.state(v).buffer = std::shared_ptr<PlayerAnimationAllocatedAction>{};
        }
        return complete();
    }
    Outcome advance_transition(PlayerAnimationView &v, PlayerAnimationRuntime &rt,
                               const PlayerAnimationRecord &selected, Phase at,
                               fidelity::Context &fp) {
        auto step = read_value(selected.transition_step, at);
        auto frame = (ExactFloat(fp, step) + read_value(access.state(v).frame, at));
        access.state(v).frame = (frame).rounded();
        auto c = (frame).compare(1.0f);
        if (c == X87Comparison::less || c == X87Comparison::unordered)
            return complete();
        access.state(v).mode = read_value(selected.mode, Phase::TransitionMode);
        auto released = release_animation_keys(v, rt);
        if (!released)
            return released;
        return reset_player_animation_frame(v, rt, fp);
    }
    Outcome select_requested(PlayerAnimationView &v, PlayerAnimationRuntime &rt,
                             std::uint16_t current, std::uint16_t requested,
                             fidelity::Context &fp) {
        const auto &selected = record(v, requested, Phase::SelectRow);
        const auto rate = read_value(selected.transition_step, Phase::SelectRow);
        bool same = false;
        if (bits(rate) != bits(1.0f)) {
            const auto &old =
                action(record(v, current, Phase::SelectOldObject), rt, Phase::SelectOldObject);
            const auto &next = action(selected, rt, Phase::SelectNewAction);
            const auto &next_object = read_value(next.object, Phase::SelectNewObject);
            const auto &old_object = read_value(old.object, Phase::SelectPreviousObject);
            same = next_object.id == old_object.id;
        }
        if (same) {
            auto blended = blend_player_animation(v, rt, fp);
            if (!blended)
                return blended;
            access.state(v).frame = 0;
        } else
            access.state(v).frame = 1;
        access.state(v).current = requested;
        access.state(v).mode = 2;
        access.state(v).next =
            static_cast<std::uint8_t>(read_value(selected.next, Phase::SelectNext));
        return advance_transition(v, rt, selected, Phase::AdvanceTransition, fp);
    }
    Outcome advance_loop(PlayerAnimationView &v, PlayerAnimationRuntime &rt,
                         const PlayerAnimationRecord &selected, std::int16_t mode,
                         fidelity::Context &fp, Outcome &result) {
        auto step = read_value(selected.frame_step, Phase::PlaybackStep);
        auto increment = ExactFloat(fp, step);
        if (mode == 9)
            increment =
                (ExactFloat(fp, access.speed_multiplier(v, rt, Phase::SpeedMultiplier)) * step);
        if (mode == 10)
            increment =
                (ExactFloat(fp, access.charge_multiplier(v, rt, Phase::ChargeMultiplier)) * step);
        access.state(v).frame =
            ((increment + read_value(access.state(v).frame, Phase::LoopFrame))).rounded();
        wrap_frame(v, rt, selected, Phase::LoopLength, fp, result);
        result.completed = true;
        return result;
    }
    Outcome select_saved_next(PlayerAnimationView &v, PlayerAnimationRuntime &rt,
                              const PlayerAnimationRecord &selected, std::uint16_t requested,
                              fidelity::Context &fp) {
        std::uint16_t selected_index = requested;

        access.state(v).timer = 0;
        auto rate = read_value(selected.transition_step, Phase::SavedTransitionStep);
        bool same = false;
        std::uint16_t next = 0;
        if (bits(rate) != bits(1.0f)) {
            next = read_value(access.state(v).next, Phase::SavedNext);
            const auto &a = action(selected, rt, Phase::SavedCurrentAction);
            const auto &b =
                action(record(v, next, Phase::SavedNextAction), rt, Phase::SavedNextAction);
            same = read_value(a.object, Phase::SavedCurrentObject).id ==
                   read_value(b.object, Phase::SavedNextObject).id;
        }
        if (same) {
            selected_index = next;
            access.state(v).requested = static_cast<std::uint8_t>(next);
            auto blended = blend_player_animation(v, rt, fp);
            if (!blended)
                return blended;
            access.state(v).frame = 0;
        } else
            access.state(v).frame = 1;
        const auto &now = record(v, selected_index, Phase::SavedRow);
        access.state(v).current = selected_index;
        access.state(v).next =
            static_cast<std::uint8_t>(read_value(now.next, Phase::SavedFollowing));
        access.state(v).mode = 2;
        return advance_transition(v, rt, now, Phase::AdvanceSavedTransition, fp);
    }
    Outcome advance_bounded(PlayerAnimationView &v, PlayerAnimationRuntime &rt,
                            const PlayerAnimationRecord &selected, std::uint16_t requested,
                            std::int16_t mode, fidelity::Context &fp) {
        ExactFloat advanced =
            ExactFloat(fp, read_value(access.state(v).frame, Phase::PlaybackFrame));
        if (mode == 6 || mode == 7)
            advanced = (advanced - read_value(selected.frame_step, Phase::ReverseStep));
        else if (mode == 11)
            advanced = (ExactFloat(fp, access.charge_multiplier(v, rt, Phase::ChargeIncrement)) +
                        *access.state(v).frame);
        else
            advanced = (ExactFloat(fp, read_value(selected.frame_step, Phase::PlaybackStep)) +
                        *access.state(v).frame);
        access.state(v).frame = (advanced).rounded();
        if (mode == 6 || mode == 7) {
            auto c = (ExactFloat(fp, 0.0f)).compare(advanced);
            if (c != X87Comparison::greater)
                return complete();
            if (mode == 6)
                return next_player_animation(v, rt, requested, fp);
            access.state(v).frame = 0;
            access.state(v).flags = 1;
            return complete();
        }
        auto end = (unsigned_frame(selected, rt, Phase::PlaybackEnd, fp) - 1.0f);
        std::optional<float> stored_end;
        if (mode == 5)
            stored_end = (end).rounded();
        auto comparison = (end).compare(advanced);
        if (greater_or_unordered(comparison))
            return complete();
        if (mode == 4 || mode == 11)
            return next_player_animation(v, rt, requested, fp);
        if (mode == 5) {
            access.state(v).frame = *stored_end;
            access.state(v).flags = 1;
            return complete();
        }
        if (mode == 8)
            return select_saved_next(v, rt, selected, requested, fp);
        return complete();
    }
    Outcome update_player_animation(PlayerAnimationView &v, PlayerAnimationRuntime &rt,
                                    fidelity::Context &fp) {
        Outcome result;
        try {
            const auto current = read_value(access.state(v).current, Phase::UpdateCurrent);
            const auto requested = read_value(access.state(v).requested, Phase::UpdateRequested);
            access.state(v).previous = current;
            access.state(v).flags = 0;
            auto mode = read_value(access.state(v).mode, Phase::UpdateMode);
            if (current != requested && mode != 2 && mode != 0 && mode != 14) {
                mode = 1;
                access.state(v).mode = 1;
                access.state(v).timer = 0;
            } else {
                const auto timer = read_value(access.state(v).timer, Phase::UpdateTimer);
                access.state(v).timer = static_cast<std::uint16_t>(timer + 1);
            }

            // Preserve signed cleanup selection before narrowing to the named mode byte.
            if (mode < 0 || mode > 12)
                return cleanup_animation(v, rt);
            switch (static_cast<Mode>(mode)) {
            case Mode::Initialize:
                allocate_initial(v, rt, requested);
                return complete();
            case Mode::Select:
                return select_requested(v, rt, current, requested, fp);
            case Mode::Transition:
                return advance_transition(v, rt, record(v, requested, Phase::TransitionRow),
                                          Phase::AdvanceTransition, fp);
            default:
                break;
            }
            const auto &selected = record(v, requested, Phase::PlaybackRow);
            switch (static_cast<Mode>(mode)) {
            case Mode::Loop:
            case Mode::SpeedLoop:
            case Mode::ChargeLoop:
                return advance_loop(v, rt, selected, mode, fp, result);
            case Mode::Wrap:
                wrap_frame(v, rt, selected, Phase::WrapLength, fp, result);
                result.completed = true;
                return result;
            default:
                return advance_bounded(v, rt, selected, requested, mode, fp);
            }
        } catch (const Stop &failure) {
            auto result_at_failure = failure.result;
            result_at_failure.wrap_iterations = result.wrap_iterations;
            return result_at_failure;
        }
    }
    std::int32_t signed_word(std::uint32_t x) {
        std::int32_t v;
        std::memcpy(&v, &x, 4);
        return v;
    }
    std::uint32_t word(std::int32_t x) {
        std::uint32_t v;
        std::memcpy(&v, &x, 4);
        return v;
    }
    float real(std::uint32_t x) {
        float v;
        std::memcpy(&v, &x, 4);
        return v;
    }
    std::uint32_t word(float x) {
        std::uint32_t v;
        std::memcpy(&v, &x, 4);
        return v;
    }
    const PlayerAnimationMotion &motion(const PlayerAnimationAction &a,
                                        const PlayerAnimationRuntime &rt, Phase at) {
        const auto &ref = read_value(a.motion, at);
        if (!ref.id)
            stop(Failure::NullResource, at);
        auto i = rt.resources.motions.find(*ref.id);
        if (i == rt.resources.motions.end())
            stop(Failure::UnresolvedResource, at);
        return i->second;
    }
    void checked(const TaskWorkResult &v, Phase at) {
        if (!v) {
            Outcome r;
            r.failure = Failure::WorkFailed;
            r.phase = at;
            r.work_failure = v.failure;
            throw Stop{r};
        }
    }
    void write(TaskWorkArena &arena, const TaskWorkHandle &h, std::uint32_t at, std::uint32_t value,
               Phase boundary) {
        std::uint8_t bytes[4] = {
            static_cast<std::uint8_t>(value), static_cast<std::uint8_t>(value >> 8),
            static_cast<std::uint8_t>(value >> 16), static_cast<std::uint8_t>(value >> 24)};
        checked(arena.write(h, at, bytes, 4), boundary);
    }
    std::uint32_t read(TaskWorkArena &arena, const TaskWorkHandle &h, std::uint32_t at,
                       Phase boundary) {
        std::uint8_t bytes[4];
        checked(arena.read(h, at, bytes, 4), boundary);
        return std::uint32_t(bytes[0]) | (std::uint32_t(bytes[1]) << 8) |
               (std::uint32_t(bytes[2]) << 16) | (std::uint32_t(bytes[3]) << 24);
    }
    const SceneGeometryObject &object(const SceneResourceReference &ref,
                                      const PlayerAnimationRuntime &rt, Phase at) {
        if (!ref.id)
            stop(Failure::NullResource, at);
        if (!rt.resources.objects)
            stop(Failure::UnresolvedResource, at);
        auto i = rt.resources.objects->find(*ref.id);
        if (i == rt.resources.objects->end())
            stop(Failure::UnresolvedResource, at);
        return i->second;
    }
    // Original pre-order traversal writes a fixed 111-object stack.
    // Repeated identities are visited repeatedly; a cycle reaches that actual
    // capacity boundary rather than being silently de-duplicated.
    void hierarchy(const SceneResourceReference &ref, const PlayerAnimationRuntime &rt,
                   std::vector<SceneResourceReference> &output, bool root) {
        if (output.size() == 111)
            stop(Failure::ObjectCapacity, root ? Phase::RootCapacity : Phase::ChildCapacity);
        output.push_back(ref);
        const auto &o = object(ref, rt, root ? Phase::RootChild : Phase::ChildChild);
        const auto child = read_value(o.child, root ? Phase::RootChild : Phase::ChildChild);
        if (child.id)
            hierarchy(child, rt, output, false);
        const auto sibling = read_value(o.sibling, root ? Phase::RootSibling : Phase::ChildSibling);
        if (sibling.id)
            hierarchy(sibling, rt, output, false);
    }
    const std::vector<PlayerAnimationNode> &nodes(const PlayerAnimationMotion &m,
                                                  const PlayerAnimationRuntime &rt, Phase at) {
        const auto ref = read_value(m.data, at);
        if (!ref.id)
            stop(Failure::NullResource, at);
        auto i = rt.resources.node_arrays.find(*ref.id);
        if (i == rt.resources.node_arrays.end())
            stop(Failure::UnresolvedResource, at);
        return i->second;
    }
    const PlayerAnimationKeyBytes *keys(const AnimationResourceReference &ref,
                                        const PlayerAnimationRuntime &rt, Phase at) {
        if (!ref.id)
            return nullptr;
        auto i = rt.resources.key_arrays.find(*ref.id);
        if (i == rt.resources.key_arrays.end())
            stop(Failure::UnresolvedResource, at);
        return &i->second;
    }
    const PlayerAnimationNode &node(const PlayerAnimationMotion &m,
                                    const PlayerAnimationRuntime &rt, unsigned n, Phase at) {
        const auto &array = nodes(m, rt, at);
        if (n >= array.size())
            stop(Failure::UnresolvedResource, at);
        return array[n];
    }
    std::uint32_t key_word(const PlayerAnimationKeyBytes &keys, std::uint32_t n, unsigned component,
                           Phase at) {
        const auto offset = std::uint64_t(n) * 16 + component * 4;
        if (!keys.bytes || offset + 4 > keys.byte_extent || keys.offset > keys.bytes->size() ||
            offset + 4 > keys.bytes->size() - keys.offset)
            stop(Failure::UnavailableKey, at);
        const auto p = keys.bytes->data() + keys.offset + offset;
        return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) | (std::uint32_t(p[2]) << 16) |
               (std::uint32_t(p[3]) << 24);
    }
    ExactFloat frame_value(std::uint32_t frame, const PlayerAnimationRuntime &,
                           fidelity::Context &fp) {
        const auto s = signed_word(frame);
        auto v = ExactFloat::integer(fp, s);
        if (s < 0)
            v = (v + 4294967296.0f);
        return v;
    }
    struct Interval {
        std::uint32_t first = 0, second = 0;
    };
    Interval interval(const PlayerAnimationKeyBytes &keys, std::uint32_t count, float frame,
                      const PlayerAnimationRuntime &rt, fidelity::Context &fp, Phase at,
                      unsigned channel) {
        Interval i;
        if ((frame_value(key_word(keys, 0, 0, at), rt, fp)).compare(frame) != X87Comparison::less)
            return i;
        while (true) {
            i.first = i.second;
            if (i.first >= count - 1U) {
                i.second = 0;
                break;
            }
            ++i.second;
            if ((frame_value(key_word(keys, i.second, 0,
                                      channel == 0   ? Phase::PositionIntervalFrame
                                      : channel == 1 ? Phase::AngleIntervalFrame
                                                     : Phase::ScaleIntervalFrame),
                             rt, fp))
                    .compare(frame) != X87Comparison::less)
                break;
        }
        return i;
    }
    ExactFloat fraction(const PlayerAnimationKeyBytes &keys, Interval i, std::uint32_t period,
                        float frame, const PlayerAnimationRuntime &rt, fidelity::Context &fp,
                        Phase at) {
        const auto a = key_word(keys, i.first, 0, at), b = key_word(keys, i.second, 0, at);
        auto difference = b - a;
        if (i.second < i.first)
            difference += period;
        auto numerator = (frame - frame_value(a, rt, fp));
        auto denominator = frame_value(difference, rt, fp);
        return (numerator / denominator);
    }
    std::array<std::uint32_t, 3> pose(const SceneGeometryObject &o, unsigned channel, Phase at) {
        if (channel == 1) {
            auto a = read_value(o.angles, at);
            return {word(a[0]), word(a[1]), word(a[2])};
        }
        const auto p = channel == 0 ? read_value(o.position, at) : read_value(o.scale, at);
        return {word(p.x), word(p.y), word(p.z)};
    }
    // Source writes each component before loading/interpolating the next. The
    // angle coefficient alone spills tobinary32 before its three FILD/FMUL/ftol.
    void sampled(TaskWorkArena &arena, const TaskWorkHandle &output,
                 const PlayerAnimationKeyBytes *input, const PlayerAnimationNode &n,
                 unsigned channel, const SceneGeometryObject &object,
                 const PlayerAnimationMotion &m, PlayerAnimationView &v,
                 const PlayerAnimationRuntime &rt, fidelity::Context &fp, Phase at) {
        if (!input) {
            auto p = pose(object, channel, at);
            for (unsigned axis = 0; axis < 3; ++axis)
                write(arena, output, 4 + axis * 4, p[axis], at);
            return;
        }
        const auto frame = read_value(access.state(v).frame, at);
        const bool mdata3 = (read_value(m.type, at) & 4) != 0;
        const auto consumed_count =
            channel == 0 && mdata3   ? access.raw_position_count(n, Phase::RawPositionCount)
            : channel == 1 && mdata3 ? read_value(n.counts[0], Phase::RelocatedAngleCount)
                                     : read_value(n.counts[channel], at);
        auto selected = interval(*input, consumed_count, frame, rt, fp, at, channel);
        const Phase copy_sites[3][3] = {
            {Phase::CopyPositionX, Phase::CopyPositionY, Phase::CopyPositionZ},
            {Phase::CopyAngleX, Phase::CopyAngleY, Phase::CopyAngleZ},
            {Phase::CopyScaleX, Phase::CopyScaleY, Phase::CopyScaleZ}};
        for (unsigned axis = 0; axis < 3; ++axis)
            write(arena, output, 4 + axis * 4,
                  key_word(*input, selected.first, axis + 1, copy_sites[channel][axis]),
                  copy_sites[channel][axis]);
        if (selected.first == selected.second)
            return;
        auto f = fraction(*input, selected, read_value(m.frame_count, at), frame, rt, fp, at);
        if (channel == 1) {
            const auto coefficient = (f).rounded();
            for (unsigned axis = 0; axis < 3; ++axis) {
                const auto a = key_word(*input, selected.first, axis + 1, at),
                           b = key_word(*input, selected.second, axis + 1, at);
                auto delta = animation_angle_delta(signed_word(a), signed_word(b));
                auto product = (ExactFloat::integer(fp, delta) * coefficient);
                write(arena, output, 4 + axis * 4,
                      read(arena, output, 4 + axis * 4, at) +
                          static_cast<std::uint32_t>(product.truncated_integer()),
                      at);
            }
        } else
            for (unsigned axis = 0; axis < 3; ++axis) {
                const auto a = key_word(*input, selected.first, axis + 1, at),
                           b = key_word(*input, selected.second, axis + 1, at);
                auto diff = (ExactFloat(fp, real(b)) - real(a));
                auto product = (diff * f);
                auto sum = (product + real(read(arena, output, 4 + axis * 4, at)));
                write(arena, output, 4 + axis * 4, word((sum).rounded()), at);
            }
    }
    void first(TaskWorkArena &arena, const TaskWorkHandle &output,
               const PlayerAnimationKeyBytes *input, const SceneGeometryObject &o, unsigned channel,
               Phase at) {
        if (input) {
            for (unsigned axis = 0; axis < 3; ++axis)
                write(arena, output, 20 + axis * 4, key_word(*input, 0, axis + 1, at), at);
        } else {
            const auto values = pose(o, channel, at);
            for (unsigned axis = 0; axis < 3; ++axis)
                write(arena, output, 20 + axis * 4, values[axis], at);
        }
    }
    // FUNCTION: SONIC 0x4383a0 SubAngle
    std::int32_t animation_angle_delta(std::int32_t first, std::int32_t second) noexcept {
        auto raw = static_cast<std::uint16_t>(word(second) - word(first));
        std::int16_t value;
        std::memcpy(&value, &raw, 2);
        return value;
    }
    // FUNCTION: SONIC 0x4383d0 CreateNinjaAngle
    std::int32_t animation_unwrap_angle(std::int32_t first, std::int32_t second) noexcept {
        const auto difference = animation_angle_delta(first, second);
        if (difference == 0)
            return first;
        auto output = word(second);
        if (difference > 0 && second < first)
            output += (((word(first) - word(second) - 1U) >> 16) + 1U) * kAngleTurn;
        else if (difference < 0 && first < second)
            output -= (((word(second) - word(first) - 1U) >> 16) + 1U) * kAngleTurn;
        return signed_word(output);
    }
    // NOTSA: named output step retains per-axis write order and signed sixteen-bit angles.
    void unwrap_output_angles(TaskWorkArena &arena, TaskWorkHandle output) {

        for (unsigned axis = 0; axis < 3; ++axis) {
            const auto first_raw = read(arena, output, 4 + axis * 4, Phase::ReadAngles);
            const auto second_raw = read(arena, output, 20 + axis * 4, Phase::ReadAngles);
            const auto sign_extend = [](std::uint32_t raw) {
                const auto low = static_cast<std::uint16_t>(raw);
                std::int16_t val;
                std::memcpy(&val, &low, 2);
                return static_cast<std::int32_t>(val);
            };
            auto f = sign_extend(first_raw), s = sign_extend(second_raw);
            write(arena, output, 4 + axis * 4, word(f), Phase::SignExtendOutgoingAngle);
            write(arena, output, 20 + axis * 4, word(s), Phase::SignExtendIncomingAngle);
            write(arena, output, 20 + axis * 4, word(animation_unwrap_angle(f, s)),
                  Phase::UnwrapIncomingAngle);
        }
    }
    // NOTSA: borrowed incoming/outgoing motion descriptors for one transition invocation.
    struct TransitionSources {
        const PlayerAnimationMotion &outgoing, &incoming;
        unsigned outgoing_channels, incoming_channels;
    };
    // NOTSA: one channel's allocation and ordered outgoing/incoming key publications.
    void build_channel(PlayerAnimationView &v, PlayerAnimationRuntime &rt, fidelity::Context &fp,
                       const TransitionSources &sources, TaskWorkArena &arena, TaskWorkHandle data,
                       std::uint32_t cursor, unsigned n, unsigned channel,
                       const SceneGeometryObject &o) {
        const auto &old_motion = sources.outgoing;
        const auto &next_motion = sources.incoming;
        const auto old_channels = sources.outgoing_channels;
        const auto next_channels = sources.incoming_channels;

        const auto base = channel == 0   ? Phase::PositionSources
                          : channel == 1 ? Phase::AngleSources
                                         : Phase::ScaleSources;
        const auto malloc_site = channel == 0   ? Phase::AllocatePositionKeys
                                 : channel == 1 ? Phase::AllocateAngleKeys
                                                : Phase::AllocateScaleKeys;
        const auto write_site = channel == 0   ? Phase::PublishPositionKeys
                                : channel == 1 ? Phase::PublishAngleKeys
                                               : Phase::PublishScaleKeys;
        const auto output_slot = cursor + channel * 4;
        const auto count_slot = cursor + (channel == 2 ? 5U : channel + 2U) * 4;
        if (channel == 2) {
            write(arena, data, cursor + 16, read(arena, data, cursor + 12, Phase::ReadAngleCount),
                  Phase::RelocateAngleCount);
            write(arena, data, cursor + 12, read(arena, data, cursor + 8, Phase::ReadPositionCount),
                  Phase::RelocatePositionCount);
        }
        AnimationResourceReference old_ref, new_ref;
        const PlayerAnimationNode *old_node = nullptr;
        if (channel < old_channels) {
            old_node = &node(old_motion, rt, n, base);
            old_ref = read_value(old_node->keys[channel], base);
        }
        if (!old_ref.id && channel < next_channels)
            new_ref = read_value(node(next_motion, rt, n, base).keys[channel], base);
        TaskWorkHandle output;
        if (old_ref.id || new_ref.id) {
            auto allocation = arena.allocate(32);
            if (!allocation && allocation.failure != TaskWorkFailure::exhausted)
                checked(allocation, malloc_site);
            output = allocation.work;
        }
        checked(arena.set_reference(data, output_slot, output), write_site);
        if (!output) {
            write(arena, data, count_slot, 0,
                  channel == 0   ? Phase::ClearAbsentPositionCount
                  : channel == 1 ? Phase::ClearAbsentAngleCount
                                 : Phase::ClearAbsentScaleCount);
            return;
        }
        write(arena, output, 0, 0, write_site);
        sampled(arena, output,
                keys(old_ref, rt,
                     channel == 0   ? Phase::SamplePosition
                     : channel == 1 ? Phase::SampleAngle
                                    : Phase::SampleScale),
                old_node ? *old_node : PlayerAnimationNode{}, channel, o, old_motion, v, rt, fp,
                channel == 0   ? Phase::SamplePosition
                : channel == 1 ? Phase::SampleAngle
                               : Phase::SampleScale);
        write(arena, output, 16, 1,
              channel == 0   ? Phase::IncomingPositionFrame
              : channel == 1 ? Phase::IncomingAngleFrame
                             : Phase::IncomingScaleFrame);
        if (channel < next_channels)
            new_ref = read_value(node(next_motion, rt, n,
                                      channel == 0   ? Phase::IncomingPositionKeys
                                      : channel == 1 ? Phase::IncomingAngleKeys
                                                     : Phase::IncomingScaleKeys)
                                     .keys[channel],
                                 channel == 0   ? Phase::IncomingPositionKeys
                                 : channel == 1 ? Phase::IncomingAngleKeys
                                                : Phase::IncomingScaleKeys);
        first(arena, output,
              keys(new_ref, rt,
                   channel == 0   ? Phase::CopyIncomingPosition
                   : channel == 1 ? Phase::CopyIncomingAngle
                                  : Phase::CopyIncomingScale),
              o, channel,
              channel == 0   ? Phase::CopyIncomingPosition
              : channel == 1 ? Phase::CopyIncomingAngle
                             : Phase::CopyIncomingScale);
        if (channel == 1)
            unwrap_output_angles(arena, output);
        write(arena, data, count_slot, 2,
              channel == 0   ? Phase::PublishPositionCount
              : channel == 1 ? Phase::PublishAngleCount
                             : Phase::PublishScaleCount);
    }
    // FUNCTION: SONIC 0x43f2a0 BuildPlayerAnimationTransition
    Outcome build_player_animation_transition(PlayerAnimationView &v, PlayerAnimationRuntime &rt,
                                              fidelity::Context &fp) {
        try {
            const auto requested =
                           read_value(access.state(v).requested, Phase::TransitionRequested),
                       current = read_value(access.state(v).current, Phase::TransitionCurrent);
            const auto &selected = record(v, requested, Phase::TransitionRequestedRow);
            const auto &next = action(selected, rt, Phase::TransitionRoot);
            const auto root = read_value(next.object, Phase::TransitionRoot);
            const auto &allocated = read_value(access.state(v).buffer, Phase::TransitionBuffer);
            if (!allocated)
                stop(Failure::NullResource, Phase::PublishTransitionRoot);
            allocated->object = root;
            std::vector<SceneResourceReference> objects;
            hierarchy(root, rt, objects, true);
            const auto &next_motion = motion(next, rt, Phase::TransitionIncomingMotion);
            const unsigned next_channels =
                ((read_value(next_motion.type, Phase::TransitionIncomingMotion) & 4) | 8) >> 2;
            (void)read_value(next_motion.data, Phase::TransitionIncomingData);
            const auto &old_motion = motion(action(record(v, current, Phase::TransitionOutgoingRow),
                                                   rt, Phase::TransitionOutgoingAction),
                                            rt, Phase::TransitionOutgoingMotion);
            const unsigned old_channels =
                ((read_value(old_motion.type, Phase::TransitionOutgoingMotion) & 4) | 8) >> 2;
            (void)read_value(old_motion.data, Phase::TransitionOutgoingData);
            const auto &allocated_motion =
                read_value(allocated->motion, Phase::TransitionAllocatedMotion);
            if (!allocated_motion)
                stop(Failure::NullResource, Phase::TransitionAllocatedData);
            const auto data = read_value(allocated_motion->data, Phase::TransitionAllocatedData);
            const auto count = read_value(selected.object_count, Phase::TransitionObjectCount);
            if (!count) {
                allocated_motion->type = (next_channels == 3 || old_channels == 3) ? 7 : 3;
                allocated_motion->input_function =
                    (next_channels == 3 || old_channels == 3) ? 3 : 2;
                Outcome result;
                result.completed = true;
                return result;
            }
            if (!data)
                stop(Failure::NullResource, Phase::PublishPositionKeys);
            if (!rt.arena)
                stop(Failure::MissingArena, Phase::AllocatePositionKeys);
            auto &arena = *rt.arena;
            std::uint32_t cursor = 0;
            for (unsigned n = 0; n < count; ++n) {
                if (n >= objects.size())
                    stop(Failure::UnavailablePose, Phase::TransitionObjectPose);
                const auto &o = object(objects[n], rt, Phase::TransitionObjectPose);
                for (unsigned channel = 0; channel < 3; ++channel) {
                    if (channel == 2 && next_channels != 3 && old_channels != 3)
                        break;
                    build_channel(v, rt, fp, {old_motion, next_motion, old_channels, next_channels},
                                  arena, data, cursor, n, channel, o);
                }
                cursor += (next_channels == 3 || old_channels == 3) ? 24U : 16U;
            }
            allocated_motion->type = (next_channels == 3 || old_channels == 3) ? 7 : 3;
            allocated_motion->input_function = (next_channels == 3 || old_channels == 3) ? 3 : 2;
            Outcome result;
            result.completed = true;
            return result;
        } catch (const Stop &e) {
            return e.result;
        }
    }
};
} // namespace sadx::animation::detail
