// player/player_task: the child-state factory the player layer installs into the engine's
// TaskScheduler (0x40b960..0x40b9a3 for a parent with a live PlayerTaskState).
#include "player/player_task.hpp"

#include <iostream>
#include <stdexcept>

using namespace sadx;
namespace {
void require(bool condition,const char* message) { if(!condition) throw std::runtime_error(message); }
}

int main() {
    try {
        TaskScheduler scheduler;
        require(static_cast<bool>(scheduler.initialize_source_pool(std::make_shared<TaskWorkArena>(),std::shared_ptr<void>{})),"pool");
        auto parent=scheduler.create_element(2,0,nullptr).task;
        parent->primary_state=std::make_shared<PlayerTaskState>();
        parent->primary_state->position=NinjaVector{1,2,3};
        parent->primary_state->orientation.angles={4,5,6};
        parent->primary_state->control_timer_at_6=77;

        const auto missing=scheduler.create_child(2,nullptr,parent);
        require(missing.failure==TaskFailure::child_state_factory_missing && missing.source_boundary==0x40b98d,"no factory is explicit");

        install_player_child_state(scheduler);
        const auto child=scheduler.create_child(2,nullptr,parent).task;
        require(child && child->primary_state && child->primary_state!=parent->primary_state,"own child state");
        const auto& state=*child->primary_state;
        require(state.position->x==1 && state.position->y==2 && state.position->z==3,"position copied");
        require(state.orientation.angles==parent->primary_state->orientation.angles,"angles copied");
        require(state.control_timer_at_6==0 && state.scale_at_2c->x==0 && !state.event_move,"zeroed live fields");

        // Direct call: an unobserved parent position stops at 0x40b98d with the zeroed child kept.
        parent->primary_state->position.reset();
        require(copy_player_child_state(*parent,*child)==std::optional<std::uint32_t>{0x40b98d} &&
                child->primary_state->position->x==0,"unobserved position boundary");
        std::cout<<"Player child-state factory: installed copy, explicit missing factory and unobserved boundary passed.\n";
        return 0;
    } catch(const std::exception& error) { std::cerr<<error.what()<<'\n'; return 1; }
}
