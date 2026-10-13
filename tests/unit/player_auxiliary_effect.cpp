#include "player/player_auxiliary_effect.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace sadx;
namespace {
template<class T>void check(const T& v){if(!static_cast<bool>(v))throw std::runtime_error("auxiliary-effect assertion");}
TaskHandle task(TaskScheduler& s){auto r=s.create_element(2,2,&player_auxiliary_effect_initialize_callback);check(r);return r.task;}
void startup_to_shadow(const std::shared_ptr<const PlayerAuxiliaryEffectAssetTable>& table,const PlayerStartupParameterTable& parameters){
    TaskScheduler s;auto arena=std::make_shared<TaskWorkArena>();check(s.initialize_source_pool(arena,std::shared_ptr<void>{}));
    auto main=s.create_element(3,1,nullptr).task;check(main);check(bind_player_task_primary(s,main));
    main->primary_state->character_id=0;PlayerStartupRegistry registry;PlayerEventFlags events{};X87Context fp(0x37f);
    auto ctx=std::make_shared<PlayerAuxiliaryEffectRuntime>();ctx->players=&registry;ctx->table=table;ctx->fp=&fp;
    check(install_player_auxiliary_effect_context(s,ctx));
    const PlayerStartupRuntime runtime{&parameters,&events,&player_auxiliary_effect_initialize_callback};
    auto startup=initialize_player_startup(0,main,std::make_shared<PlayerMotionState>(),s,registry,runtime,fp);
    check(startup && startup.partial->auxiliary_task && !startup.partial->auxiliary_task->primary_state);
    auto auxiliary=startup.partial->auxiliary_task;check(s.run_index(2));
    check(auxiliary->auxiliary_effect_work && (*auxiliary->auxiliary_effect_work->selection_at_8)->group==2);
    check(auxiliary->exec==&player_auxiliary_effect_update_callback);
    auto pending=s.run_index(2);check(pending.failure==TaskFailure::callback_failed);
    const auto details=std::dynamic_pointer_cast<const PlayerAuxiliaryEffectDiagnostic>(pending.callback_diagnostic);
    check(details && details->result.source_boundary==0x4a2687 && details->result.nested_source_boundary==0x441b00);
    main->primary_state->secondary_mode_at_1=0x27;
    pending=s.run_index(2);const auto changed=std::dynamic_pointer_cast<const PlayerAuxiliaryEffectDiagnostic>(pending.callback_diagnostic);
    check(changed && changed->result.source_boundary==0x4a264d);
    std::uint8_t variant=0;check(arena->read(auxiliary->primary_work,3,&variant,1) && variant==2);
    check(bind_player_task_primary(s,auxiliary).failure==TaskWorkFailure::conflicting_live_view);
}
void initialization(const std::shared_ptr<const PlayerAuxiliaryEffectAssetTable>& table){
    TaskScheduler s;check(s.initialize_source_pool(std::make_shared<TaskWorkArena>(),std::shared_ptr<void>{}));
    PlayerStartupRegistry registry;auto primary=std::make_shared<PlayerTaskState>();primary->character_id=0;registry.task_work[0]=primary;
    auto context=std::make_shared<PlayerAuxiliaryEffectRuntime>();context->players=&registry;context->table=table;
    check(install_player_auxiliary_effect_context(s,context));check(install_player_auxiliary_effect_context(s,context));
    check(install_player_auxiliary_effect_context(s,std::make_shared<PlayerAuxiliaryEffectRuntime>()).failure==TaskCallbackContextFailure::conflicting_owner);
    auto t=task(s);auto opaque=std::make_shared<int>(7);t->field30=opaque;
    std::weak_ptr<PlayerAuxiliaryEffectRuntime> retained=context;context.reset();check(!retained.expired());
    context=retained.lock(); // scheduler owns callback context lifetime
    check(s.run_index(2));check(t->auxiliary_effect_work && !t->primary_state);
    check(t->exec==&player_auxiliary_effect_update_callback && t->display==&player_auxiliary_effect_display_callback && t->destructor==&player_auxiliary_effect_destroy_callback);
    check((*t->auxiliary_effect_work->selection_at_8)->group==2 && (*t->auxiliary_effect_work->selection_at_8)->table==table);
    check(t->field30==opaque); // source allocation context remains unchanged
    primary->character_id=7;check(initialize_player_auxiliary_effect(s,t,*context));check((*t->auxiliary_effect_work->selection_at_8)->group==1);
    std::optional<std::uint16_t> character=5;registry.task_work[0]=std::shared_ptr<PlayerTaskState>{};context->current_character_at_3b22dc0=&character;
    check(initialize_player_auxiliary_effect(s,t,*context));check((*t->auxiliary_effect_work->selection_at_8)->group==4);
    character=6;check(initialize_player_auxiliary_effect(s,t,*context));check(t->auxiliary_effect_work->selection_at_8 && !*t->auxiliary_effect_work->selection_at_8);
    registry.task_work[0].reset();auto pending=initialize_player_auxiliary_effect(s,t,*context);
    check(pending.failure==PlayerAuxiliaryEffectFailure::unknown_registration && pending.source_boundary==0x4a2b21);
    t->exec=&player_auxiliary_effect_initialize_callback;auto transferred=s.run_index(2);
    check(transferred.failure==TaskFailure::callback_failed && transferred.callback_diagnostic && transferred.source_boundary==0x4a2b21);
    const auto details=std::dynamic_pointer_cast<const PlayerAuxiliaryEffectDiagnostic>(transferred.callback_diagnostic);
    check(details && details->result.failure==PlayerAuxiliaryEffectFailure::unknown_registration);
    check(s.context().current_function==&player_auxiliary_effect_initialize_callback && s.context().field30==opaque);
    check(bind_player_task_primary(s,t).failure==TaskWorkFailure::conflicting_live_view);
    auto wrong=task(s);wrong->primary_state=std::make_shared<PlayerTaskState>();
    check(initialize_player_auxiliary_effect(s,wrong,*context).failure==PlayerAuxiliaryEffectFailure::conflicting_view);
}
TaskWorkHandle node(TaskWorkArena& a){auto r=a.allocate(52);check(r);check(a.fill(r.work,0,0,52));return r.work;}
void nodes_and_display(const std::shared_ptr<const PlayerAuxiliaryEffectAssetTable>& table){
    TaskScheduler s;auto arena=std::make_shared<TaskWorkArena>();check(s.initialize_source_pool(arena,std::shared_ptr<void>{}));
    PlayerStartupRegistry registry;registry.task_work[0]=std::make_shared<PlayerTaskState>();
    auto ctx=std::make_shared<PlayerAuxiliaryEffectRuntime>();ctx->players=&registry;ctx->table=table;check(install_player_auxiliary_effect_context(s,ctx));
    auto t=task(s);check(initialize_player_auxiliary_effect(s,t,*ctx));auto a=node(*arena),b=node(*arena);
    check(arena->set_reference(t->primary_work,0x10,a));check(arena->set_reference(a,0x30,b));check(arena->set_reference(b,0x2c,a));
    const NinjaVector position{1,2,3};const float factor=0.5F,scale=2;const std::int16_t angle=-123;
    for(auto n:{a,b}){check(arena->write(n,4,&position,12));check(arena->write(n,0x20,&factor,4));check(arena->write(n,0x24,&scale,4));check(arena->write(n,0x1c,&angle,2));}
    std::optional<std::int32_t> display_switch=1;ctx->display_switch_at_3b1117c=&display_switch;check(display_player_auxiliary_effect(s,t,*ctx));
    display_switch=0;check(display_player_auxiliary_effect(s,t,*ctx).failure==PlayerAuxiliaryEffectFailure::missing_render_state);
    std::vector<unsigned> events;ctx->render_state_4030d0=[&](unsigned selector,std::uint32_t v){events.push_back(selector*100+v);return PlayerAuxiliaryEffectExternalResult{true,0};};
    X87Context fp(0x37f);ctx->fp=&fp;
    auto packet=display_player_auxiliary_effect(s,t,*ctx);check(packet.failure==PlayerAuxiliaryEffectFailure::missing_sprite_owner && packet.pending_node==a && packet.pending_sprite->scale_y==1 && packet.pending_sprite->angle==-123);
    unsigned draws=0;ctx->sprite_4072f0=[&](const auto& p,unsigned option,std::uint32_t flags){++draws;check(p.position.z==3 && p.scale_x==2 && p.scale_y==1 && option==0 && flags==0x31);return PlayerAuxiliaryEffectExternalResult{true,0};};
    check(display_player_auxiliary_effect(s,t,*ctx));check(draws==2 && events[0]==8 && events[1]==110);
    display_switch=-1;check(display_player_auxiliary_effect(s,t,*ctx));check(draws==2); // signed entry vs!=0 inner gate
    display_switch=0;
    ctx->render_state_4030d0=[&](unsigned,std::uint32_t){display_switch.reset();return PlayerAuxiliaryEffectExternalResult{true,0};};
    const auto invalidated=display_player_auxiliary_effect(s,t,*ctx);check(invalidated.failure==PlayerAuxiliaryEffectFailure::unknown_display_switch && invalidated.pending_node==a && invalidated.pending_sprite);
    ctx->render_state_4030d0=[](unsigned,std::uint32_t){return PlayerAuxiliaryEffectExternalResult{true,0};};display_switch=0;
    std::uint32_t signaling=0x7f812345;check(arena->write(a,0x20,&signaling,4));X87Context unmasked(0x37e);ctx->fp=&unmasked;
    const auto trapped=display_player_auxiliary_effect(s,t,*ctx);check(trapped.failure==PlayerAuxiliaryEffectFailure::fp_failed && trapped.fp_failure==X87Failure::unmasked_exception && trapped.fp_exceptions==1 && trapped.source_boundary==0x4a07c8 && trapped.pending_node==a);
    check(s.destroy_task(t));check(a->released() && b->released() && t->retired());
}
void failures(){
    TaskScheduler s;auto a=std::make_shared<TaskWorkArena>();check(s.initialize_source_pool(a,std::shared_ptr<void>{}));
    auto t=task(s);PlayerAuxiliaryEffectRuntime unknown;
    auto result=initialize_player_auxiliary_effect(s,t,unknown);check(result.failure==PlayerAuxiliaryEffectFailure::missing_context && t->exec==&player_auxiliary_effect_update_callback);
    auto n=node(*a);check(a->set_reference(t->primary_work,0x10,n));std::uint32_t unbound=0x12345678;check(a->write(n,0x30,&unbound,4));
    result=destroy_player_auxiliary_effect(s,t);check(result.failure==PlayerAuxiliaryEffectFailure::work_failed && result.source_boundary==0x4a0738 && !n->released());
    check(a->set_reference(n,0x30,{}));check(destroy_player_auxiliary_effect(s,t));check(n->released());
    auto null=task(s);null->primary_work.reset();result=initialize_player_auxiliary_effect(s,null,unknown);
    check(result.failure==PlayerAuxiliaryEffectFailure::missing_primary && null->exec==&player_auxiliary_effect_update_callback && null->display==&player_auxiliary_effect_display_callback);
    std::optional<std::int32_t> unknown_switch=1;unknown.display_switch_at_3b1117c=&unknown_switch;check(display_player_auxiliary_effect(s,null,unknown));
}
}
int main(int argc,char** argv){try{
    check(argc==3);std::array<std::uint8_t,0x438> bytes{};std::ifstream f(argv[1],std::ios::binary);f.read(reinterpret_cast<char*>(bytes.data()),bytes.size());check(f.gcount()==0x438 && f.peek()==EOF);
    PlayerStartupParameterTable parameters{};std::ifstream pf(argv[2],std::ios::binary);pf.read(reinterpret_cast<char*>(parameters.data()),1056);check(pf.gcount()==1056 && pf.peek()==EOF);
    const auto table=decode_player_auxiliary_effect_table(bytes.data(),bytes.size());check(table && table->character(0)==2 && table->character(4)==5);
    initialization(table);nodes_and_display(table);failures();startup_to_shadow(table,parameters);
    std::cout<<"Auxiliary-effect init/resource callbacks: source table, live lookup, failure context, node cleanup and explicit renderer boundaries passed.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
