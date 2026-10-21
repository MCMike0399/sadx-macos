#include "engine/task.hpp"
#include <cassert>
#include <memory>

namespace {
using namespace sadx;
struct Context final : TaskCallbackContext {
    TaskHandle canonical;
    std::weak_ptr<Task> original;
    bool replace = false;
    unsigned calls = 0, released = 0;
};
void clear_owner(TaskScheduler &scheduler, const TaskHandle &task) {
    auto context = std::static_pointer_cast<Context>(scheduler.callback_context(clear_owner));
    assert(task == context->canonical && task == context->original.lock());
    ++context->calls;
    context->canonical.reset();
    if (context->replace) context->canonical = scheduler.create_element(0, 1, nullptr).task;
    assert(task && task == context->original.lock());
}
void run(bool replace) {
    TaskScheduler scheduler;
    auto arena = std::make_shared<TaskWorkArena>();
    assert(scheduler.initialize_source_pool(arena, {}));
    auto context = std::make_shared<Context>();
    context->replace = replace;
    assert(scheduler.register_callback_context(clear_owner, context));
    context->canonical = scheduler.create_element(15, 1, nullptr).task;
    context->original = context->canonical;
    const auto primary = context->canonical->primary_work;
    const auto motion = context->canonical->motion_work;
    const auto force = context->canonical->force_work;
    const auto auxiliary = context->canonical->auxiliary_work;
    context->canonical->destructor = clear_owner;
    context->canonical->auxiliary_native_work = std::shared_ptr<int>(new int(7),
        [context, primary, motion, force, auxiliary](int *work) {
            assert(primary->released() && motion->released() && force->released() && auxiliary->released());
            const auto task = context->original.lock();
            assert(task && !task->retired());
            assert(!task->primary_work && !task->motion_work && !task->force_work && !task->auxiliary_work);
            ++context->released;
            delete work;
        });
    // Pass the canonical owner itself: its destructor clears/replaces this very argument.
    assert(scheduler.destroy_task(context->canonical));
    assert(context->calls == 1 && context->released == 1);
    assert(primary->released() && motion->released() && force->released() && auxiliary->released());
    assert(context->original.expired());
    assert(static_cast<bool>(context->canonical) == replace);
    if (replace) {
        assert(!context->canonical->retired());
        assert(scheduler.destroy_task(context->canonical));
        context->canonical.reset();
    }
}
}
int main() { run(false); run(true); }
