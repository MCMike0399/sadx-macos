#include "game/task.hpp"
#include "player/player_task.hpp"

#include <array>
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace sadx;
namespace {
template<class T> void check(const T& v) { if (!static_cast<bool>(v)) throw std::runtime_error("task lifecycle assertion"); }
std::vector<unsigned> visited;
void record(TaskScheduler&,const TaskHandle& task) { visited.push_back(*task->source_slot()); }
void mark_child(TaskScheduler& s,const TaskHandle& t) { check(s.free_task(t)); }
void prepend_on_destroy(TaskScheduler& s,const TaskHandle&) { check(s.create_element(0,0,&record)); }
TaskHandle replacement;
TaskHandle snapshot_target;
void replace_snapshot(TaskScheduler& s,const TaskHandle& t) {
    check(s.destroy_task(snapshot_target));
    replacement=s.allocate_task(1,&record).task;
    check(replacement && replacement->source_slot()==snapshot_target->source_slot());
    check(s.free_task(t));
}
void replace_snapshot_detached(TaskScheduler& s,const TaskHandle&) {
    check(s.destroy_task(snapshot_target));
    replacement=s.allocate_task(7,nullptr).task;
}
void source_lifecycle() {
    auto arena=std::make_shared<TaskWorkArena>(); TaskScheduler s;
    check(s.initialize_source_pool(arena,std::shared_ptr<void>{}));
    check(s.source_free_slots()==4096);
    auto a=s.create_element(0x10f,0,&record).task;
    check(a && *a->source_slot()==0 && a->primary_work->data_offset()==0x34040);
    check(a->force_work->data_offset()==0x340a0 && a->motion_work->data_offset()==0x34100);
    check(a->auxiliary_work->data_offset()==0x34160);
    std::array<unsigned char,64> raw{};
    check(arena->read(a->primary_work,0,raw.data(),64));
    for(auto b:raw) check(b==0);
    auto b=s.allocate_task(0,nullptr).task;
    check(s.list_head(0)==b && s.next_task(b)==a && b->exec==&TaskScheduler::source_no_op);
    auto detached=s.allocate_task(7,nullptr).task;
    auto last=s.allocate_task(8,nullptr).task;
    check(s.list_head(7)==last && s.list_head(8)==last && !s.active_member(detached));
    check(s.destroy_task(detached).failure==TaskFailure::inactive_task);
    auto resource=std::make_shared<int>(9);a->field30=resource;
    const std::uint32_t words[]={0x7f812345,0x80000000,0xdeadbeef};
    check(arena->write(a->primary_work,32,words,12));
    check(arena->write(a->primary_work,20,words,12));
    auto child=s.create_child(2,&mark_child,a).task;
    check(child && child->field30==resource && s.first_child(a)==child);
    std::uint32_t copied[3]{};
    check(arena->read(child->primary_work,32,copied,12));
    for(int i=0;i<3;++i)check(copied[i]==words[i]);
    auto child2=s.create_child(0,&mark_child,a).task;
    check(s.first_child(a)==child2 && s.next_task(child2)==child);
    check(s.free_task(a));
    check(!a->retired() && a->primary_work);
    check(s.run_index(0));
    check(a->retired() && child->retired() && child2->retired() && !a->primary_work);
    check(a->field30==resource); // source recycle does not clear +30
    check(s.source_free_slots()==4093); // detached,b,last remain
    auto reused=s.allocate_task(0,nullptr).task;
    check(reused!=a && reused->source_slot()==a->source_slot());
    check(s.free_task(a).failure==TaskFailure::retired_task);
}
void real_cleanup_and_partial() {
    auto arena=std::make_shared<TaskWorkArena>();TaskScheduler s;
    check(s.initialize_source_pool(arena,std::shared_ptr<void>{}));
    auto t=s.create_element(2,0,nullptr).task;
    auto collision=arena->allocate(32).work,nested=arena->allocate(16).work;
    check(arena->fill(collision,0,0,32)); unsigned char flag=0x80;
    check(arena->write(collision,5,&flag,1));check(arena->set_reference(collision,12,nested));
    check(arena->set_reference(t->primary_work,56,collision));
    auto object=arena->allocate(80).work;check(arena->fill(object,0,0,80));
    auto chain1=arena->allocate(20).work,chain2=arena->allocate(20).work;
    auto chain3=arena->allocate(16).work,extra=arena->allocate(8).work;
    check(arena->fill(chain1,0,0,20)&&arena->fill(chain2,0,0,20)&&arena->fill(chain3,0,0,16));
    check(arena->set_reference(chain1,16,chain2));
    check(arena->set_reference(object,64,chain1));check(arena->set_reference(object,68,extra));
    check(arena->set_reference(object,76,chain3));check(arena->set_reference(t->primary_work,60,object));
    auto control=std::make_shared<Task::ObjectControl>();control->flags_at_2=0xff;control->task_at_4=t;t->object_control=control;
    check(s.destroy_task(t));
    for(auto w:{collision,nested,object,chain1,chain2,chain3,extra})check(w->released());
    check(control->flags_at_2==0xfe && !control->task_at_4);
    auto partial=s.create_element(2,0,nullptr).task;
    std::uint32_t unknown=0x11223344;check(arena->write(partial->primary_work,56,&unknown,4));
    check(s.free_task(partial));
    auto failed=s.run_index(0);
    check(failed.failure==TaskFailure::work_failure && failed.work_failure==TaskWorkFailure::unbound_reference && failed.source_boundary==0x40b617);
    check(!s.active_member(partial) && partial->primary_work && !partial->retired());
    check(s.context().current_function==&TaskScheduler::destroy_callback);
    check(s.destroy_task(partial).failure==TaskFailure::inactive_task);
    check(arena->set_reference(partial->primary_work,56,{}));check(s.resume_destruction(partial));
    check(partial->retired());
    auto staged=s.create_element(2,0,nullptr).task;
    auto object2=arena->allocate(80).work;check(arena->fill(object2,0,0,80));
    auto head=arena->allocate(20).work,tail=arena->allocate(20).work;
    check(arena->fill(head,0,0,20)&&arena->fill(tail,0,0,20));
    check(arena->set_reference(head,16,tail)&&arena->set_reference(object2,64,head));
    check(arena->set_reference(staged->primary_work,60,object2));
    check(arena->write(tail,16,&unknown,4));
    failed=s.destroy_task(staged);
    check(failed.source_boundary==0x417eb3 && head->released() && !tail->released() && !object2->released());
    check(arena->set_reference(tail,16,{}));check(s.resume_destruction(staged));
    check(tail->released() && object2->released() && staged->retired());
}
void limits_and_mutation() {
    auto arena=std::make_shared<TaskWorkArena>();TaskScheduler s;
    check(s.initialize_source_pool(arena,std::shared_ptr<void>{}));
    auto old=s.allocate_task(0,nullptr).task;old->destructor=&prepend_on_destroy;
    check(s.destroy_task(old));check(s.list_head(0) && s.list_head(0)!=old);
    snapshot_target=s.allocate_task(0,&record).task;
    auto first=s.allocate_task(0,&replace_snapshot).task;
    visited.clear();check(s.run_index(0));
    check(!visited.empty() && visited[0]==*replacement->source_slot());
    check(s.run_index(0)); // old first is scheduled for destruction
    while(s.source_free_slots())check(s.allocate_task(7,nullptr));
    check(s.allocate_task(0,nullptr).failure==TaskFailure::source_pool_exhausted);
    check(s.retire(first).failure==TaskFailure::source_task_requires_destructor || first->retired());
    TaskScheduler unknown;check(unknown.initialize_source_pool(std::make_shared<TaskWorkArena>()));
    auto pending=unknown.create_element(2,0,nullptr);
    check(pending.failure==TaskFailure::default_field30_unobserved && pending.task->primary_work && unknown.active_member(pending.task));
    auto exhausted_arena=std::make_shared<TaskWorkArena>();TaskScheduler exhausted;
    check(exhausted.initialize_source_pool(exhausted_arena,std::shared_ptr<void>{}));
    auto remaining=exhausted_arena->allocate(TaskWorkArena::usable_bytes-0x34020-32).work;check(remaining);
    auto leak=exhausted.create_element(2,7,nullptr);
    check(leak.failure==TaskFailure::detached_creation_rollback_rejected && leak.task && !leak.task->retired() && exhausted.source_free_slots()==4095);
    auto rolled=exhausted.create_element(2,0,nullptr);
    check(rolled.failure==TaskFailure::work_failure && rolled.task->retired() && exhausted.source_free_slots()==4095);
    auto partial_arena=std::make_shared<TaskWorkArena>();TaskScheduler partial;
    check(partial.initialize_source_pool(partial_arena,std::shared_ptr<void>{}));
    check(partial_arena->allocate(TaskWorkArena::usable_bytes-0x34020-96-32));
    auto retained=partial.create_element(6,7,nullptr);
    check(retained.failure==TaskFailure::detached_creation_rollback_rejected && retained.task->primary_work && !retained.task->primary_work->released() && !retained.task->force_work);
    check(partial.destroy_task(retained.task).source_boundary==0x401000);
    TaskScheduler unsupported;
    check(unsupported.initialize_source_pool(std::make_shared<TaskWorkArena>(),std::shared_ptr<void>{}));
    snapshot_target=unsupported.allocate_task(0,nullptr).task;
    check(unsupported.allocate_task(0,&replace_snapshot_detached));
    auto stopped=unsupported.run_index(0);
    check(stopped.failure==TaskFailure::detached_snapshot_next && stopped.task==replacement && stopped.source_boundary==0x40b0e0);
}
void live_primary_view() {
    TaskScheduler s;auto arena=std::make_shared<TaskWorkArena>();
    check(s.initialize_source_pool(arena,std::shared_ptr<void>{}));
    auto parent=s.create_element(2,0,nullptr).task;
    parent->primary_state=std::make_shared<PlayerTaskState>();
    auto& state=*parent->primary_state;
    state.position=NinjaVector{7,8,9};state.orientation.flags=0xffff;
    state.orientation.angles={111,222,333};state.character_id=6;state.task_id_at_2=9;
    auto child=s.create_child(2,nullptr,parent).task;
    check(child && child->primary_state && child->primary_state!=parent->primary_state);
    check(child->primary_state->position->x==7 && child->primary_state->position->y==8 && child->primary_state->position->z==9);
    check(child->primary_state->orientation.angles==state.orientation.angles);
    check(child->primary_state->orientation.flags==0 && child->primary_state->character_id==0 && child->primary_state->task_id_at_2==0 && child->primary_state->action_at_0==0);
    check(child->primary_state->secondary_mode_at_1==0 && child->primary_state->control_timer_at_6==0);
    check(child->primary_state->source_counter_at_c==0 && !child->primary_state->event_move);
    state.position->x=42;auto later=s.create_child(2,nullptr,parent).task;
    check(later->primary_state->position->x==42 && child->primary_state->position->x==7);
    state.position.reset();auto partial=s.create_child(2,nullptr,parent);
    check(partial.failure==TaskFailure::work_failure && partial.work_failure==TaskWorkFailure::unobserved_live_field && partial.source_boundary==0x40b98d);
    check(partial.task && s.first_child(parent)==partial.task && partial.task->primary_state->position->x==0);
}
}
int main(){try{source_lifecycle();real_cleanup_and_partial();limits_and_mutation();live_primary_view();std::cout<<"Source task creation, slots, work cleanup, partial failures and callback mutations passed.\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
