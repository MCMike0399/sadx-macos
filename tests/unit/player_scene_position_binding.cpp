#include "player/player_scene_position_binding.hpp"
#include "player/sonic_player.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace sadx;
namespace {
void require(bool condition,const char* text){if(!condition)throw std::runtime_error(text);}
std::vector<std::uint8_t> read(const char* path){std::ifstream f(path,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
template<class T>T exact(const char* path){const auto b=read(path);require(b.size()==sizeof(T),"Exact source asset required");T out;std::memcpy(&out,b.data(),sizeof(T));return out;}
float number(std::uint32_t bits){float v;std::memcpy(&v,&bits,4);return v;}
NinjaVector vector(const std::uint32_t* words){return {number(words[0]),number(words[1]),number(words[2])};}
PlayerContactWork zero_contact(){PlayerContactWork c;const NinjaVector z{0,0,0};c.radius_at_0=0;c.center_at_4=z;c.displacement_at_10=z;c.direction_at_1c=z;c.contact_count_at_28=0;c.selected_contact_at_2a=0;c.angle_x_at_34=0;c.angle_z_at_38=0;c.vector_at_3c=z;c.vector_at_48=z;c.vector_at_54=z;c.vector_at_60=z;c.vector_at_6c=z;c.vector_at_78=z;return c;}
struct Assets {
    NinjaMath::SineTable sine;NinjaMatrix unit;std::vector<std::uint8_t> rows,image,cases;
    PlayerScenePositionConstants scene;PlayerRotationConstants rotation;PlayerContactInitializationConstants initializer;
    PlayerContactResponseConstants response;PlayerContactAttributeConstants attributes;FloorAngleConstants floor;
    PlayerMeshQueryConstants query;CoordinateAxisConstants axis;std::array<float,3> broadphase;
    PlayerPositionConstants position;SonicPlayerConstants sonic;ContactParameterTable defaults{};
    Assets(char** v):sine(exact<NinjaMath::SineTable>(v[1])),unit(exact<NinjaMatrix>(v[2])),rows(read(v[3])),
        axis(exact<CoordinateAxisConstants>(v[11])),broadphase(exact<std::array<float,3>>(v[13])){
        require(rows.size()==8*132,"All eight source parameter rows required");
        auto b=read(v[4]);auto sc=decode_scene_position_constants(b.data(),b.size());require(sc.has_value(),"Scene constants");scene=*sc;
        auto r=exact<std::array<std::uint8_t,88>>(v[5]);std::memcpy(rotation.float_bits.data(),r.data(),64);std::memcpy(rotation.double_bits.data(),r.data()+64,24);
        auto i=exact<std::array<std::uint8_t,52>>(v[6]);std::memcpy(initializer.bits.data(),i.data(),52);
        b=read(v[7]);auto re=decode_contact_response_constants(b.data(),b.size());require(re.has_value(),"Response constants");response=*re;
        b=read(v[8]);auto at=decode_contact_attribute_constants(b.data(),b.size());require(at.has_value(),"Attribute constants");attributes=*at;
        auto f=exact<std::array<std::uint8_t,36>>(v[9]);std::memcpy(&floor.upper_cut,f.data(),4);std::memcpy(&floor.lower_cut,f.data()+4,4);std::memcpy(&floor.axis_epsilon,f.data()+8,4);std::memcpy(&floor.angle_units,f.data()+12,8);std::memcpy(&floor.positive_factor,f.data()+20,8);std::memcpy(&floor.negative_factor,f.data()+28,8);
        auto q=exact<std::array<std::uint8_t,32>>(v[10]);std::memcpy(&query.half_radius,q.data(),4);std::memcpy(&query.three,q.data()+4,4);std::memcpy(&query.negative_one,q.data()+8,4);std::memcpy(&query.triangle_expansion,q.data()+12,4);std::memcpy(&query.angle_units,q.data()+16,8);std::memcpy(&query.angle_divisor,q.data()+24,8);
        image=read(v[12]);b=read(v[14]);auto po=decode_player_position_constants(b.data(),b.size());require(po.has_value(),"Position constants");position=*po;
        b=read(v[15]);auto so=decode_sonic_player_constants(b.data(),b.size());require(so.has_value(),"Sonic constants");sonic=*so;
        cases=read(v[16]);require(cases.size()==136*18*4,"Pinned real-scene fixture corpus required");
        for(std::size_t n=0;n<8;++n){auto p=decode_player_parameters(rows.data()+132*n,132);require(static_cast<bool>(p),"Source parameter row");defaults[n]={p.parameters->weight,p.parameters->air_resist_y,p.parameters->run_accel};}
    }
};
struct Fixture {
    const Assets& a;TaskScheduler scheduler;TaskHandle object=scheduler.append_task(0,&TaskScheduler::source_no_op).task;
    std::shared_ptr<PlayerTaskState> task=std::make_shared<PlayerTaskState>();
    std::shared_ptr<PlayerStartupPlayer> allocation=std::make_shared<PlayerStartupPlayer>();
    PlayerStartupRegistry players;PlayerPositionCollisionScene collision;PlayerSceneLandscape landscape;PlayerMeshQueryState contacts;
    PlayerScenePositionGlobals globals;PlayerMeshQueryRuntime math{{1,{}},{1,{}},{1,0,{}},10000};
    std::optional<NinjaVector> gravity=NinjaVector{0,-1,0};PlayerContactResponseRuntime response_math;
    PlayerInputRuntime input;ForcedInputAngleConversion forced;SceneBroadphaseConstants broadphase;std::unordered_map<std::size_t,TaskHandle> geometry_tasks;
    NinjaMatrixStack stack;PlayerCoordinates coordinates;X87Context fp{0x007f};PlayerPositionRuntime position_runtime;
    Fixture(const Assets& assets,const std::array<std::uint32_t,18>& w):a(assets),response_math{math.inverse_math,gravity,&a.sine,0},broadphase{a.broadphase[0],a.broadphase[1]},
        stack(std::vector<NinjaMatrix>(8,a.unit),1,0),coordinates(a.sine,a.unit,stack){
        auto decoded=decode_stage_scene_landtable(a.image.data(),a.image.size(),1,0);require(static_cast<bool>(decoded),"Actual ECoast LANDTABLE decode");auto& land=*decoded.table;land.source_count=static_cast<std::int16_t>(w[1]);
        landscape.objects=land.objects;landscape.models=land.models;landscape.vector_arrays=land.vector_arrays;landscape.mesh_set_arrays=land.mesh_set_arrays;landscape.index_arrays=land.index_arrays;landscape.material_arrays=land.material_arrays;landscape.static_enabled=1;landscape.landtable=SceneResourceReference{0};
        MeshRegistryPools pools;for(std::size_t n=0;n<256;++n)pools.groups[n]=n;for(std::size_t n=0;n<16384;++n)pools.nodes[n]=n;
        PlayerMeshRegistryState registry;require(static_cast<bool>(initialize_mesh_registry(landscape,registry,contacts,pools)),"Actual registry initializer");registry.force_refresh=1;SceneRegistrationRuntime registration;registration.registered_count=0;CrtFabsRuntime absolute{1,0,{}};
        require(static_cast<bool>(refresh_mesh_registry(landscape,registry,{land,registration,{a.broadphase[2]},absolute},coordinates,fp)),"Actual registry produces real polygon resources");
        object->primary_state=task;allocation->task=object;allocation->player=std::make_shared<PlayerPhysicsState>();allocation->motion=std::make_shared<PlayerMotionState>();
        task->position=vector(w.data()+2);task->orientation.flags=static_cast<std::uint16_t>(w[8]);for(std::size_t n=0;n<3;++n)std::memcpy(&task->orientation.angles[n],w.data()+9+n,4);
        task->player_index=0;task->character_id=static_cast<std::uint8_t>(w[16]);task->action_at_0=static_cast<std::uint8_t>(w[17]);task->task_id_at_2=2;task->control_timer_at_6=0;
        auto& world=*allocation->motion;world.motion.speed=vector(w.data()+5);world.friction_scale=1;world.angle_x_at_1c=0;world.angle_z_at_24=0;
        auto& p=*allocation->player;p.motion.speed=world.motion.speed;p.dotp=number(w[15]);p.floor_normal=NinjaVector{0,1,0};p.attributes=w[13];p.shadow_attributes_top=w[14];p.flags_at_10=static_cast<std::uint16_t>(w[12]);p.no_control_time_at_a=0;p.moving_task_at_6c=TaskHandle{};p.cached_land_object_at_74=SceneResourceReference{};p.contact_work=zero_contact();
        auto row=decode_player_parameters(a.rows.data()+132*task->character_id,132);require(static_cast<bool>(row),"Live parameter owner");allocation->parameters=*row.parameters;
        players.tasks[0]=object;players.task_work[0]=task;players.player_work[0]=allocation;players.motion_work[0]=allocation->motion;
        collision.task_collision=PositionResourceRef{0};collision.works[0].flags=0;
        globals.special_surface_at_3b42e74=0;globals.motion_override_at_3b42e70=0;globals.event_task_at_3b2c578=TaskHandle{};globals.environment_at_3b22dec=0;globals.render_at_3b0f0ec=-1;
        input.stage=1;input.act=0;input.current_character=task->character_id;input.normalized[0]=NormalizedPlayerInput{0,0};input.no_control_timer[0]=0;input.camera_yaw=0;
        // Alternate entry is tested with ECoast resources as a synthetic caller
        // profile; this does not assert that stage9 uses the ECoast table.
        position_runtime.stage=w[0]?9:1;position_runtime.act=w[0]?2:0;position_runtime.crt_math.matherr_control=1;
    }
    PlayerScenePositionRuntime runtime(){return {globals,contacts,math,response_math,input,forced,{&players,nullptr},&geometry_tasks,a.defaults,a.attributes,a.response,a.initializer,a.rotation,a.floor,a.query,broadphase,a.axis};}
};
void tests(int argc,char** argv){
    require(argc==17,"usage: scene-binding TRIG UNIT PARAMS SCENE124 ROTATION88 INIT52 RESPONSE52 ATTR40 FLOOR36 QUERY32 AXIS8 EXE BROADPHASE12 POSITION32 SONIC_CONSTANTS CASES");
    Assets a(argv);std::size_t completed=0,responses=0,ordinary=0,alternate=0;bool mutated=false,render_boundary=false;std::array<std::uint32_t,18> success_case{};
    for(std::size_t n=0;n<136;++n){std::array<std::uint32_t,18>w;std::memcpy(w.data(),a.cases.data()+n*72,72);Fixture f(a,w);auto runtime=f.runtime();
        PlayerPositionCallees callees;bind_player_scene_position(callees,f.landscape,a.scene,runtime);
        auto& live=*f.allocation->parameters;auto* identity=&live;const auto initial=live.weight;
        f.allocation->player->shadow_attributes_top=2;
        auto result=set_player_position(*f.task,f.allocation->motion.get(),*f.allocation->player,live,f.collision,a.position,a.rotation,f.position_runtime,callees,&f.coordinates,f.fp);
        if(!result)std::cerr<<"case "<<n<<" parent "<<static_cast<unsigned>(result.failure)<<" site "<<std::hex<<result.source_boundary<<" child "<<(result.scene_diagnostics?static_cast<unsigned>(result.scene_diagnostics->failure):999)<<" site "<<(result.scene_diagnostics?result.scene_diagnostics->source_boundary:0)<<std::dec<<'\n';
        require(static_cast<bool>(result)&&result.scene_diagnostics&&static_cast<bool>(*result.scene_diagnostics),"PSetPosition invokes a complete real scene owner");
        require(&*f.allocation->parameters==identity&&f.fp.control_word()==0x007f&&f.stack.depth()==1,"One live parameter owner, caller CW and balanced matrix owner");
        ++completed;if(w[0])++alternate;else ++ordinary;
        if(result.scene_diagnostics->response){++responses;require(result.scene_diagnostics->mesh_query&&result.scene_diagnostics->broadphase,"Response retains actual query and broadphase diagnostics");success_case=w;}
        mutated|=live.weight!=initial;
        // An unobserved field stops the actual scene body after PSetPosition's
        // own committed prefix, retaining the original child reason and site.
        f.allocation->player->flags_at_10.reset();auto failed=set_player_position(*f.task,f.allocation->motion.get(),*f.allocation->player,live,f.collision,a.position,a.rotation,f.position_runtime,callees,&f.coordinates,f.fp);
        require(failed.failure==PlayerPositionFailure::collision_owner_failed&&failed.scene_diagnostics&&failed.scene_diagnostics->failure==ScenePositionFailure::unavailable_field&&failed.nested_source_boundary==failed.scene_diagnostics->source_boundary,"Child failure diagnostics survive the parent wrapper");
    }
    require(completed==136&&ordinary&&alternate&&responses&&mutated,"Actual source-derived contact corpus exercises both owners and live parameter mutation");
    // Find a real contact path reaching the unported renderer, without a flat
    // plane or successful draw callback. Its partial matrix/state must survive.
    for(std::size_t n=0;n<136&&!render_boundary;++n){std::array<std::uint32_t,18>w;std::memcpy(w.data(),a.cases.data()+n*72,72);Fixture f(a,w);auto runtime=f.runtime();f.globals.render_at_3b0f0ec=0;PlayerPositionCallees calls;bind_player_scene_position(calls,f.landscape,a.scene,runtime);
        auto result=set_player_position(*f.task,f.allocation->motion.get(),*f.allocation->player,*f.allocation->parameters,f.collision,a.position,a.rotation,f.position_runtime,calls,&f.coordinates,f.fp);
        if(result.scene_diagnostics&&result.scene_diagnostics->failure==ScenePositionFailure::render_owner_unavailable){require(!result&&result.scene_diagnostics->source_boundary==0x44499e&&f.stack.depth()>1,"Renderer boundary preserves child source site and preceding pushed frame");render_boundary=true;}
    }
    require(render_boundary,"Real ECoast contact reaches the retained renderer boundary");
    Fixture f(a,success_case);auto scene_runtime=f.runtime();PlayerPositionCallees calls;bind_player_scene_position(calls,f.landscape,a.scene,scene_runtime);
    f.task->action_at_0=11;PlayerAccelerationRuntime acceleration;acceleration.world_gravity=f.gravity;acceleration.input=f.input;
    GroundAccelerationConstants ground{};GroundAccelerationCallees ground_calls;AirAccelerationConstants air{};AirAccelerationCallees air_calls;CrtAsinRuntime inverse{1,{}};
    SonicPlayerNativeMotion motion{f.coordinates,acceleration,ground,ground_calls,air,air_calls,a.floor,inverse,a.position,a.rotation,f.position_runtime,calls};
    SonicPlayerGlobals sonic_globals;SonicPlayerRuntime sonic_runtime{sonic_globals,f.scheduler,f.players,nullptr,&motion,a.sonic,{}};SonicPlayerFrame frame{f.object,f.task,f.allocation->motion,f.allocation,&f.collision};
    auto sonic=dispatch_sonic_player_action(frame,sonic_runtime,f.fp);
    require(!sonic&&sonic.failure==SonicPlayerFailure::missing_native_owner&&sonic.callee_address==0x44a800&&sonic.position&&sonic.position->scene_diagnostics&&static_cast<bool>(*sonic.position->scene_diagnostics),"Completed scene diagnostics survive Sonic's later common-tail owner boundary");
    f.allocation->player->flags_at_10.reset();f.task->action_at_0=11;
    sonic=dispatch_sonic_player_action(frame,sonic_runtime,f.fp);
    require(!sonic&&sonic.failure==SonicPlayerFailure::position_failed&&sonic.position&&sonic.position->scene_diagnostics&&sonic.position->scene_diagnostics->failure==ScenePositionFailure::unavailable_field,"Sonic retains the full failed scene result through its position wrapper");
    Fixture missing_matrix(a,success_case);auto unavailable_runtime=missing_matrix.runtime();PlayerPositionCallees unavailable_calls;bind_player_scene_position(unavailable_calls,missing_matrix.landscape,a.scene,unavailable_runtime);
    auto unavailable=unavailable_calls.ordinary_collision(*missing_matrix.task,missing_matrix.allocation->motion.get(),*missing_matrix.allocation->player,*missing_matrix.allocation->parameters,missing_matrix.collision,nullptr,missing_matrix.fp);
    require(unavailable.failure==PlayerPositionFailure::missing_matrix_owner&&unavailable.source_boundary==0x449500&&!unavailable.outcome&&!unavailable.scene_diagnostics,"Missing native matrix prevents invocation without inventing a child scene result");
    std::cout<<"Scene binding: "<<completed<<" actual ECoast PSetPosition paths, "<<responses<<" retained responses, live parameters, renderer failure and Sonic later-boundary diagnostics passed.\n";
}
}
int main(int argc,char** argv){try{tests(argc,argv);return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
