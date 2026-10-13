#include "player/player_hold.hpp"
#include <cstring>

namespace {
struct Fixture {
    sadx::TaskScheduler scheduler;
    std::shared_ptr<sadx::TaskWorkArena> arena=std::make_shared<sadx::TaskWorkArena>();
    sadx::TaskHandle held=scheduler.append_task(0,&sadx::TaskScheduler::source_no_op).task;
    sadx::TaskWorkHandle primary=arena->allocate(64).work,motion=arena->allocate(64).work,
        collision=arena->allocate(1).work;
};
}
// Fully known source facts; unknown-field failures are exercised by owner
// tests. All Task/Work resources come from the actual native factories.
extern "C" int sadx_native_player_hold(const std::uint32_t* in,std::uint32_t* out){
    if(!in||!out)return 1;std::memset(out,0,24);
    static thread_local Fixture fixture;
    sadx::PlayerTaskState task{{static_cast<std::uint16_t>(in[0]),{}},
        static_cast<std::uint8_t>(in[1]),static_cast<std::uint8_t>(in[2]),{}};
    sadx::PlayerPhysicsState player{};
    auto& f=fixture;
    f.held->exec=in[6]==0?nullptr:in[6]==1?&sadx::TaskScheduler::destroy_callback:&sadx::TaskScheduler::source_no_op;
    f.held->motion_work=in[7]?f.motion:sadx::TaskWorkHandle{};
    f.held->primary_work=in[8]?f.primary:sadx::TaskWorkHandle{};
    if(!f.arena->set_reference(f.primary,0x38,in[9]?f.collision:sadx::TaskWorkHandle{}))return 2;
    const auto id=static_cast<std::uint8_t>(in[3]);if(!f.arena->write(f.primary,2,&id,1))return 2;
    player.held_task_at_68=in[5]?f.held:sadx::TaskHandle{};
    sadx::PlayerHoldRuntime runtime{
        [&](std::uint8_t)->std::optional<const sadx::PlayerPhysicsState*>{
            return in[4]?&player:static_cast<const sadx::PlayerPhysicsState*>(nullptr);},f.arena};
    sadx::PlayerPositionCollisionScene scene;
    scene.task_collision=sadx::PositionResourceRef{in[10]?std::optional<std::size_t>(1):std::nullopt};
    auto& own=scene.works[1];own.flags=static_cast<std::uint16_t>(in[11]);
    own.hit_work=sadx::PositionResourceRef{in[12]?std::optional<std::size_t>(2):std::nullopt};
    scene.works[2].owning_task=sadx::PositionResourceRef{in[13]?std::optional<std::size_t>(3):std::nullopt};
    scene.tasks[3].work=sadx::PositionResourceRef{in[14]?std::optional<std::size_t>(4):std::nullopt};
    auto& actor=scene.actors[4];actor.task_id=static_cast<std::uint8_t>(in[3]);
    actor.flags_bytes_at_4={static_cast<std::uint8_t>(in[15]),static_cast<std::uint8_t>(in[15]>>8)};
    const auto result=sadx::query_player_hold(task,scene,runtime);
    out[0]=static_cast<std::uint32_t>(result.failure);out[1]=result.source_boundary;
    out[2]=static_cast<std::uint32_t>(result.work_failure);out[3]=result.flags.has_value();
    if(result.flags)out[4]=*result.flags;
    return 0;
}
