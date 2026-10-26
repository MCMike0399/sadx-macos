#include "engine/collision_component.hpp"
#include "math/ninja_length.hpp"
#include "player/player_initialization.hpp"
#include <iostream>
#include <stdexcept>

namespace cc = sadx::collision_component;
namespace pi = sadx::player_initialization;
using namespace sadx;
namespace {
template <class T> void require(const T &value, const char *message) {
    if (!static_cast<bool>(value))
        throw std::runtime_error(message);
}
struct World {
    std::shared_ptr<TaskWorkArena> arena = std::make_shared<TaskWorkArena>();
    TaskScheduler scheduler;
    TaskHandle task;
    cc::Context context{127};
    World() {
        require(scheduler.initialize_source_pool(arena, std::shared_ptr<void>{}), "source pool");
        const auto created = scheduler.create_element(7, 3, nullptr);
        require(created, "actual flags7 task");
        task = created.task;
        pi::establish_zeroed_primary(scheduler, task);
        pi::establish_zeroed_motion(scheduler, task);
    }
    std::uint16_t &status() { return pi::bind_primary_view(task).status; }
};
cc::Info sphere() {
    cc::Info info;
    info.center = {3, 4, 0};
    info.a = 2;
    info.b = 100;
    info.c = 200;
    info.damage = 255;
    return info;
}
void lifecycle() {
    World w;
    const auto primary_owner = w.task->primary_state;
    const auto motion_owner = w.task->motion_state;
    const auto position = &pi::bind_primary_view(w.task).position;
    const std::vector<cc::Info> infos{sphere(), sphere(), sphere()};
    w.status() = 65535;
    bool published_before_clear = false;
    w.arena->set_metadata_writer([&](std::uint32_t offset, std::optional<std::uint32_t> value) {
        if (offset == w.task->primary_work->data_offset() + 56 && !value) {
            const auto component = cc::component(*w.arena, w.task->primary_work);
            if (component) {
                published_before_clear = component->task.expired() && component->own_number == 0 &&
                                         component->hits[0].own_number == 0 && w.status() == 65535;
            }
        }
    });
    // No reference base: nonnull publication has unknown pointer bytes, but a real native
    // reference.
    require(cc::initialize(*w.arena, w.task, w.status(), infos, 3, 0, w.context) ==
                cc::InitResult::Initialized,
            "real init enum");
    w.arena->set_metadata_writer({});
    require(published_before_clear, "primary publication precedes task backlink/clear stores");
    auto component = cc::component(*w.arena, w.task->primary_work);
    auto allocation = w.arena->reference(w.task->primary_work, 56).work;
    auto info_allocation = w.arena->reference(allocation, 12).work;
    require(component && component == allocation->collision_state, "one typed allocation owner");
    require(component->range > 7.06f && component->range < 7.08f, "sphere uses length plus a");
    require(component->info.size() == 3 && component->info[0].damage == 239 &&
                component->own_number == -1 && component->other_number == -1 &&
                component->hits[15].own_number == 255,
            "complete initialized clear state");
    require(w.status() == 65475 && w.task->primary_state == primary_owner &&
                w.task->motion_state == motion_owner &&
                &pi::bind_primary_view(w.task).position == position,
            "status mutates actual canonical primary; pose and motion stay in same owners");
    require(cc::initialize(*w.arena, w.task, w.status(), infos, 3, 7, w.context) ==
                cc::InitResult::Initialized,
            "reentry initializes");
    require(allocation->released() && info_allocation->released(),
            "reentry frees both old allocations");
    auto current = w.arena->reference(w.task->primary_work, 56).work;
    auto current_info = w.arena->reference(current, 12).work;
    require(cc::component(*w.arena, w.task->primary_work)->flags == 32832,
            "special list flag retained");
    require(w.scheduler.destroy_task(w.task), "actual native scheduler cleanup");
    require(current->released() && current_info->released() && w.task->retired(),
            "scheduler consumes same metadata and frees actual collision allocations");
    cc::free_collision(*w.arena, {});
    cc::free_work(*w.arena, {});
}
void allocation_prefix() {
    const std::vector<cc::Info> infos{sphere()};
    for (auto remaining : {0U, 224U}) {
        World w;
        w.status() = 12345;
        auto start = *w.arena->free_head();
        const auto available = TaskWorkArena::usable_bytes - start;
        require(w.arena->allocate(available - remaining - 32), "exhaust actual arena");
        const auto before = w.arena->blocks().size();
        const auto free_before = *w.arena->free_head();
        require(cc::initialize(*w.arena, w.task, w.status(), infos, 1, 0, w.context) ==
                    cc::InitResult::AllocationFailed,
                "genuine native allocation failure");
        require(!cc::component(*w.arena, w.task->primary_work) && w.status() == 12345,
                "failed allocation publishes no component/status stores");
        require(w.arena->blocks().size() == before &&
                    *w.arena->free_head() == free_before + remaining,
                "second allocation failure leaves source orphan prefix, first leaves none");
    }
    World zero;
    auto before = zero.arena->blocks().size();
    require(cc::initialize(*zero.arena, zero.task, zero.status(), {}, 0, 0, zero.context) ==
                cc::InitResult::AllocationFailed,
            "zero count CAlloc fails");
    require(zero.arena->blocks().size() == before + 1, "zero-count failure also orphans work");
    World wrap;
    before = wrap.arena->blocks().size();
    bool stopped = false;
    try {
        cc::initialize(*wrap.arena, wrap.task, wrap.status(), infos, 89478486, 0, wrap.context);
    } catch (const cc::Error &e) {
        stopped = e.boundary == cc::Boundary::PartialInfoLayout;
    }
    require(stopped && !cc::component(*wrap.arena, wrap.task->primary_work) &&
                wrap.arena->blocks().size() == before + 2,
            "wrapped partial record stops after both genuine allocations, before publication");
}
void range_and_clear() {
    cc::Context context(127);
    cc::Work w;
    w.info_count = 1;
    w.info.push_back(sphere());
    w.info[0].center = {};
    w.info[0].a = .2f;
    cc::calculate_range(&w, context);
    require(w.range > .2f && w.range < .21f, "small radius is not clamped to 25");
    w.info[0].a = 0;
    cc::calculate_range(&w, context);
    require(w.range == 25, "only max below threshold selects fallback");
    w.info[0].form = cc::Form::Cylinder;
    w.info[0].a = 3;
    w.info[0].b = 4;
    w.info[0].c = 1000;
    cc::calculate_range(&w, context);
    require(w.range == 5.05f, "cylinder excludes c");
    w.info[0].form = cc::Form::Capsule;
    cc::calculate_range(&w, context);
    require(w.range > 7.06f && w.range < 7.08f, "capsule adds b then a");
    w.flags = 65535;
    w.hits[0].flags = 65535;
    w.normal = {1, 2, 3};
    std::uint16_t status = 65535;
    cc::clear_info(w, status);
    require(w.flags == 65472 && w.hits[0].flags == 65532 && w.normal.y == 2 && status == 65475,
            "clear preserves unrelated high flags and normal");
    cc::calculate_range(nullptr, context);
    const auto negative = fidelity::ExactFloat::signed_square_root(context, -4.0f).rounded();
    require(negative == -2 && !(context.exception_flags() & 1),
            "source signed sqrt preserves negative sign without invalid sqrt");
    require(vector_length({3, 4, 12}, context).rounded() == 13,
            "source length retains its sum and binary32 spill");
}
void null_boundaries() {
    World w;
    auto result = w.scheduler.create_element(0, 3, nullptr);
    require(result, "real task without primary");
    require(cc::initialize(*w.arena, result.task, w.status(), {}, 0, 0, w.context) ==
                cc::InitResult::NoPrimary,
            "genuine no-primary outcome");
    cc::free_collision(*w.arena, result.task);
    cc::free_collision(*w.arena, w.task);
    cc::free_work(*w.arena, w.task->primary_work);
    bool stopped = false;
    try {
        cc::initialize(*w.arena, {}, w.status(), {}, 0, 0, w.context);
    } catch (const cc::Error &e) {
        stopped = e.boundary == cc::Boundary::Task;
    }
    require(stopped, "null initializer task is a fault boundary, never fabricated success");
}
} // namespace
int main() {
    try {
        lifecycle();
        allocation_prefix();
        range_and_clear();
        null_boundaries();
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    std::cout << "Canonical primary/motion, real scheduler cleanup/reentry, store prefix and "
                 "allocation failures pass\n";
}
