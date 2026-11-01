#include "engine/task.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace sadx {
class TaskSourceProbe {
public:
    static bool retained(TaskScheduler& scheduler, const TaskHandle& task) {
        return scheduler.storage_.count(task.get()) != 0;
    }
    static std::size_t depth(TaskScheduler& scheduler) { return scheduler.walk_depth_; }
};
}
namespace {
using namespace sadx;
struct Context final : TaskCallbackContext {
    std::vector<unsigned> trace;
    TaskHandle target;
    unsigned calls = 0;
    unsigned fail_at = 0;
};
void observed(TaskScheduler& scheduler, const TaskHandle& task) {
    auto context = std::static_pointer_cast<Context>(scheduler.callback_context(observed));
    assert(context && task);
    assert(scheduler.context().current_function == observed);
    context->trace.push_back(*task->source_slot());
    if (++context->calls == context->fail_at)
        TaskScheduler::raise_callback_failure({TaskFailure::callback_failed, task, {}, 0x40b10b});
}
void replacement(TaskScheduler& scheduler, const TaskHandle& task) {
    auto context = std::static_pointer_cast<Context>(scheduler.callback_context(replacement));
    assert(context);
    context->trace.push_back(*task->source_slot());
    assert(scheduler.destroy_task(context->target));
    auto next = scheduler.allocate_task(1, observed).task;
    assert(next && next != context->target && next->source_slot() == context->target->source_slot());
    assert(context->target->retired());
    context->target = next;
}
void destructor_observed(TaskScheduler& scheduler, const TaskHandle&) {
    auto context = std::static_pointer_cast<Context>(scheduler.callback_context(destructor_observed));
    if (++context->calls == context->fail_at) throw std::runtime_error("destructor");
}
void throwing(TaskScheduler&, const TaskHandle&) { throw std::runtime_error("callback"); }
void noop(TaskScheduler&, const TaskHandle&) {}
struct World {
    TaskScheduler scheduler;
    std::shared_ptr<TaskWorkArena> arena = std::make_shared<TaskWorkArena>();
    World() { assert(scheduler.initialize_source_pool(arena, std::shared_ptr<void>{})); }
};
void emit(const char* label, std::initializer_list<unsigned> words) {
    std::cout << label;
    for (auto word : words) std::cout << ' ' << word;
    std::cout << '\n';
}
void availability() {
    TaskScheduler scheduler;
    auto arena = std::make_shared<TaskWorkArena>();
    assert(scheduler.initialize_source_pool(arena));
    auto missing = scheduler.create_element(15, 0, nullptr);
    assert(missing.failure == TaskFailure::default_field30_unobserved);
    assert(missing.source_boundary == 0x40b918 && missing.task && !missing.task->field30);
    assert(missing.task->primary_work && missing.task->motion_work && missing.task->force_work);
    assert(missing.task->auxiliary_work && scheduler.active_member(missing.task));
    assert(scheduler.source_free_slots() == 4095);
    scheduler.set_default_field30(std::shared_ptr<void>{});
    auto known_null = scheduler.create_element(0, 0, nullptr);
    assert(known_null && !known_null.task->field30);
    auto graph = scheduler.source_graph();
    assert(graph.heads[0] == 1 && graph.nodes[1].next == 0);
    assert(graph.nodes[0].slot == 0 && !graph.nodes[0].next);
    assert(graph.nodes[0].previous == 1 && !graph.nodes[0].parent);
    emit("availability-slot-zero", {unsigned(missing.failure), missing.source_boundary,
                                    *missing.task->source_slot(), *known_null.task->source_slot()});
}
void contexts() {
    World first, second;
    auto owner = std::make_shared<Context>();
    auto other = std::make_shared<Context>();
    assert(first.scheduler.register_callback_context(observed, owner));
    assert(first.scheduler.register_callback_context(observed, owner));
    assert(first.scheduler.register_callback_context(observed, other).failure ==
           TaskCallbackContextFailure::conflicting_owner);
    assert(first.scheduler.register_callback_context(nullptr, owner).failure ==
           TaskCallbackContextFailure::null_callback);
    assert(first.scheduler.register_callback_context(noop, {}).failure ==
           TaskCallbackContextFailure::null_context);
    assert(!second.scheduler.callback_context(observed));
    assert(second.scheduler.register_callback_context(observed, other));
    std::weak_ptr<Context> retained = owner;
    owner.reset();
    assert(!retained.expired());
    auto task = first.scheduler.allocate_task(0, observed).task;
    task->field30 = std::make_shared<int>(7);
    assert(first.scheduler.run_index(0));
    assert(retained.lock()->calls == 1 && other->calls == 0);
    assert(!first.scheduler.context().field30 && !first.scheduler.context().current_function);
    emit("context-table", {retained.lock()->calls, other->calls});
}
void saved_next() {
    World world;
    auto& s = world.scheduler;
    auto c = std::make_shared<Context>();
    assert(s.register_callback_context(observed, c));
    assert(s.register_callback_context(replacement, c));
    c->target = s.allocate_task(0, observed).task;
    assert(s.allocate_task(0, replacement));
    assert(s.run_index(0));
    assert((c->trace == std::vector<unsigned>{1, 0}));
    assert(c->target->source_slot() == 0 && !c->target->retired());
    assert(!s.context().current_function && !s.context().field30);
    // Public continuation snapshots a native value. Graph import replaces handles;
    // the cursor must resolve the same slot's current generation on its next step.
    World external;
    auto old = external.scheduler.allocate_task(0, noop).task;
    assert(external.scheduler.allocate_task(0, noop));
    auto cursor = external.scheduler.begin_index_dispatch(0);
    auto call = external.scheduler.advance_dispatch(cursor);
    assert(call.result && call.task->source_slot() == 1);
    auto graph = external.scheduler.source_graph();
    graph.nodes[0].exec = observed;
    assert(external.scheduler.restore_source_graph(graph));
    auto later = external.scheduler.advance_dispatch(cursor);
    assert(later.result && later.task != old && later.task->source_slot() == old->source_slot());
    assert(later.callback == observed);
    emit("saved-next-generation", {unsigned(c->trace.size()), *later.task->source_slot()});
}
void callback_failures() {
    for (unsigned fail_at = 1; fail_at <= 3; ++fail_at) {
        World world;
        auto& s = world.scheduler;
        auto c = std::make_shared<Context>();
        c->fail_at = fail_at;
        assert(s.register_callback_context(observed, c));
        for (unsigned i = 0; i != 3; ++i) assert(s.allocate_task(0, observed));
        auto result = s.run_index(0);
        assert(result.failure == TaskFailure::callback_failed && result.source_boundary == 0x40b10b);
        assert(c->calls == fail_at && c->trace.size() == fail_at);
        assert(s.context().current_function == observed);
        emit("failure-at-call", {fail_at, unsigned(result.failure), result.source_boundary,
                                 *result.task->source_slot()});
    }
    World world;
    auto& s = world.scheduler;
    auto root = s.allocate_task(0, throwing).task;
    root->field30 = std::make_shared<int>(9);
    try { s.run_index(0); assert(false); } catch (const std::runtime_error&) {}
    assert(!s.context().current_function && !s.context().field30);
    auto graph = s.source_graph();
    graph.context.current_function = noop;
    assert(s.restore_source_graph(graph));
    root = s.source_task(0);
    auto child = s.create_child(0, throwing, root).task;
    child->field30 = std::make_shared<int>(8);
    try { s.run_children(root); assert(false); } catch (const std::runtime_error&) {}
    assert(s.context().current_function == noop && !s.context().field30);
    child->destructor = throwing;
    try { s.destroy_task(child); assert(false); } catch (const std::runtime_error&) {}
    assert(s.active_member(child) && child->destructor == throwing && !child->retired());
    child->destructor = nullptr;
    assert(s.destroy_task(child));
    emit("host-exception-context", {unsigned(s.context().current_function == noop),
                                    unsigned(child->retired())});
}
void read_failures() {
    for (unsigned fail_at = 1; fail_at <= 2; ++fail_at) {
        World world;
        auto& s = world.scheduler;
        auto task = s.create_element(15, 0, nullptr).task;
        auto primary = task->primary_work;
        auto auxiliary = task->auxiliary_work;
        unsigned reads = 0, released = 0;
        task->auxiliary_native_work = std::shared_ptr<int>(new int(1), [&](int* p) {
            assert(primary->released() && auxiliary->released());
            assert(!task->retired() && !task->primary_work && !task->auxiliary_work);
            ++released;
            delete p;
        });
        std::vector<unsigned> offsets;
        world.arena->set_payload_transport([&](unsigned at, void* out, unsigned bytes) {
            offsets.push_back(at - primary->data_offset());
            if (++reads == fail_at) return false;
            std::memcpy(out, world.arena->source_bytes().data() + at, bytes);
            return true;
        }, {});
        const auto result = s.destroy_task(task);
        assert(result.failure == TaskFailure::work_failure);
        assert(result.work_failure == TaskWorkFailure::unobserved_live_field);
        assert(result.source_boundary == (fail_at == 1 ? 0x40b617U : 0x40b628U));
        assert(reads == fail_at && !s.active_member(task) && !task->retired());
        assert(!s.source_graph().nodes.front().linked);
        assert(task->primary_work == primary && released == 0);
        assert(offsets.front() == 56 && (fail_at == 1 || offsets.back() == 60));
        assert(s.destroy_task(task).failure == TaskFailure::inactive_task);
        world.arena->set_payload_transport({}, {});
        assert(s.resume_destruction(task));
        assert(task->retired() && released == 1);
        emit("cleanup-read-prefix", {fail_at, result.source_boundary, reads, released});
    }
}
void write_failures() {
    for (unsigned fail_at = 1; fail_at <= 4; ++fail_at) {
        World world;
        unsigned calls = 0;
        world.arena->set_payload_transport({}, [&](unsigned, const void*, unsigned) {
            return ++calls != fail_at;
        });
        auto result = world.scheduler.create_element(15, 0, nullptr);
        assert(result.failure == TaskFailure::work_failure && result.task);
        const unsigned boundaries[] = {0x40b884, 0x40b8ab, 0x40b8ce, 0x40b8f1};
        assert(result.source_boundary == boundaries[fail_at - 1]);
        assert(result.work_failure == TaskWorkFailure::unobserved_live_field);
        assert(calls == fail_at && world.scheduler.active_member(result.task));
        assert(result.task->primary_work);
        assert(bool(result.task->force_work) == (fail_at >= 2));
        assert(bool(result.task->motion_work) == (fail_at >= 3));
        assert(bool(result.task->auxiliary_work) == (fail_at >= 4));
        world.arena->set_payload_transport({}, {});
        emit("creation-write-prefix", {fail_at, result.source_boundary, calls,
                                       unsigned(world.scheduler.source_free_slots())});
    }
}
void pool_and_graph() {
    World world;
    auto& s = world.scheduler;
    for (unsigned i = 0; i < 4096; ++i) {
        auto result = s.allocate_task(0, nullptr);
        assert(result && result.task->source_slot() == i);
    }
    auto before = s.source_graph();
    const auto failure = s.create_element(15, 0, nullptr);
    assert(failure.failure == TaskFailure::source_pool_exhausted && !failure.task);
    assert(failure.source_boundary == 0x40b037 && s.source_free_slots() == 0);
    auto corrupt = before;
    corrupt.heads[0] = 0;
    assert(s.restore_source_graph(corrupt).failure == TaskFailure::invalid_source_graph);
    assert(s.source_graph().heads == before.heads && s.source_task(0) && s.source_task(4095));
    World failed;
    // Pool fill fails after reservation/publication, before free-slot publication.
    TaskScheduler unpublished;
    auto arena = std::make_shared<TaskWorkArena>();
    arena->set_payload_transport({}, [](unsigned, const void*, unsigned) { return false; });
    auto partial = unpublished.initialize_source_pool(arena, std::shared_ptr<void>{});
    assert(partial.failure == TaskFailure::work_failure && partial.source_boundary == 0x40b4bd);
    assert(unpublished.work_arena() == arena && unpublished.source_pool_work());
    assert(unpublished.source_free_slots() == 0);
    assert(unpublished.initialize_source_pool(arena).failure == TaskFailure::source_pool_already_initialized);
    emit("pool-publication", {unsigned(failure.failure), failure.source_boundary,
                              partial.source_boundary, unsigned(before.nodes.size())});
}
void lifetime_continuation() {
    World world;
    auto& s = world.scheduler;
    auto neighbor = s.allocate_task(0, noop).task;
    auto task = s.allocate_task(0, noop).task;
    task->destructor = noop;
    task->display = noop;
    auto cursor = s.begin_destruction(task);
    auto call = s.advance_lifetime(cursor);
    assert(call.result && call.callee == TaskLifetimeCallee::destructor);
    assert(call.task == task && call.callback == noop && task->exec == noop);
    assert(s.destroy_task(neighbor));
    auto graph = s.source_graph();
    assert(s.restore_source_graph(graph));
    call = s.advance_lifetime(cursor);
    assert(call.result && call.callee == TaskLifetimeCallee::free_children);
    assert(call.task != task && call.task->source_slot() == task->source_slot());
    assert(!call.task->exec && !call.task->display && !call.task->destructor);
    assert(!cursor.next && !cursor.previous);
    assert(s.free_children(call.task));
    auto canonical = call.task;
    call = s.advance_lifetime(cursor);
    assert(call.result && call.completed && canonical->retired());
    emit("lifetime-value-continuation", {unsigned(cursor.phase), unsigned(canonical->retired())});
}
void destructor_failures() {
    for (unsigned fail_at = 1; fail_at <= 3; ++fail_at) {
        World world;
        auto& s = world.scheduler;
        auto context = std::make_shared<Context>();
        context->fail_at = fail_at;
        assert(s.register_callback_context(destructor_observed, context));
        auto parent = s.create_element(15, 0, nullptr).task;
        std::vector<TaskHandle> children;
        for (unsigned i = 0; i < 3; ++i) {
            auto child = s.create_child(15, nullptr, parent).task;
            child->destructor = destructor_observed;
            children.push_back(child);
        }
        try { s.destroy_task(parent); assert(false); } catch (const std::runtime_error&) {}
        assert(context->calls == fail_at && s.active_member(parent) && parent->primary_work);
        unsigned retired = 0;
        for (const auto& child : children) retired += child->retired();
        assert(retired == fail_at - 1);
        context->fail_at = 0;
        assert(s.destroy_task(parent));
        assert(parent->retired() && context->calls == 4);
        for (const auto& child : children) assert(child->retired());
        emit("destructor-failure-prefix", {fail_at, retired, context->calls});
    }
}
void inner_retirement(TaskScheduler& scheduler, const TaskHandle&) {
    auto c = std::static_pointer_cast<Context>(scheduler.callback_context(inner_retirement));
    assert(TaskSourceProbe::depth(scheduler) == 2);
    assert(scheduler.retire(c->target));
    assert(TaskSourceProbe::retained(scheduler, c->target));
}
void outer_retirement(TaskScheduler& scheduler, const TaskHandle& task) {
    auto c = std::static_pointer_cast<Context>(scheduler.callback_context(outer_retirement));
    assert(TaskSourceProbe::depth(scheduler) == 1);
    assert(scheduler.retire(task));
    assert(scheduler.run_index(1));
    assert(TaskSourceProbe::depth(scheduler) == 1);
    assert(TaskSourceProbe::retained(scheduler, c->target));
    assert(TaskSourceProbe::retained(scheduler, task));
}
void deferred_walk() {
    TaskScheduler scheduler;
    auto c = std::make_shared<Context>();
    assert(scheduler.register_callback_context(inner_retirement, c));
    assert(scheduler.register_callback_context(outer_retirement, c));
    auto outer = scheduler.append_task(0, outer_retirement).task;
    assert(scheduler.append_task(1, inner_retirement));
    c->target = scheduler.append_task(2, noop).task;
    assert(scheduler.run_index(0));
    assert(!TaskSourceProbe::retained(scheduler, outer));
    assert(!TaskSourceProbe::retained(scheduler, c->target));
    assert(TaskSourceProbe::depth(scheduler) == 0);
    emit("outermost-walk-reclamation", {unsigned(outer->retired()), unsigned(c->target->retired())});
}
void deferred_retire() {
    TaskScheduler scheduler;
    auto task = scheduler.append_task(0, noop).task;
    std::weak_ptr<Task> weak = task;
    assert(scheduler.retire(task));
    assert(task->retired() && !weak.expired());
    task.reset();
    assert(weak.expired());
    emit("nonowning-links", {1});
}
} // namespace
int main(int argc, char** argv) {
    const std::string selected = argc > 1 ? argv[1] : "all";
    const std::pair<const char*, void(*)()> cases[] = {
        {"availability", availability}, {"contexts", contexts}, {"saved_next", saved_next},
        {"callback_failures", callback_failures}, {"read_failures", read_failures},
        {"write_failures", write_failures}, {"pool_and_graph", pool_and_graph},
        {"deferred_retire", deferred_retire}, {"lifetime_continuation", lifetime_continuation},
        {"destructor_failures", destructor_failures}, {"deferred_walk", deferred_walk}};
    bool found = false;
    for (const auto& test : cases) if (selected == "all" || selected == test.first) {
        found = true;
        test.second();
    }
    assert(found);
}
