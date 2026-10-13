#include "player/player_scene_position.hpp"
#include <cstring>
namespace {
float number(std::uint32_t w){float v;std::memcpy(&v,&w,4);return v;}
std::uint32_t bits(float v){std::uint32_t w;std::memcpy(&w,&v,4);return w;}
sadx::NinjaVector vector(const std::uint32_t* w){return {number(w[0]),number(w[1]),number(w[2])};}
sadx::PlayerContactWork zero_contact(){sadx::PlayerContactWork c;const sadx::NinjaVector z{0,0,0};c.direction_at_1c=z;c.radius_at_0=0;c.center_at_4=z;c.displacement_at_10=z;c.angle_x_at_34=0;c.angle_z_at_38=0;c.contact_count_at_28=0;c.selected_contact_at_2a=0;c.vector_at_3c=z;c.vector_at_48=z;c.vector_at_54=z;c.vector_at_60=z;c.vector_at_6c=z;c.vector_at_78=z;return c;}
}
// Packed constants: scene124,rotation88,initializer52,response52,attributes40,
// floor36,meshquery32,axis8,broadphase12. Caller supplies exact source bytes.
extern "C" int sadx_native_player_scene_position(const std::uint8_t* image,std::size_t size,
    const std::uint8_t* table,const std::uint8_t* unit,const std::uint8_t* packed,
    const std::uint8_t* parameter_rows,const std::uint32_t* w,std::uint16_t cw,std::uint32_t* out){
    if(!image||!table||!unit||!packed||!parameter_rows||!w||!out)return -1;
    try{
        const auto constants=sadx::decode_scene_position_constants(packed,124);if(!constants)return -2;std::size_t offset=124;
        sadx::PlayerRotationConstants rotation;std::memcpy(rotation.float_bits.data(),packed+offset,64);std::memcpy(rotation.double_bits.data(),packed+offset+64,24);offset+=88;
        sadx::PlayerContactInitializationConstants initializer;std::memcpy(initializer.bits.data(),packed+offset,52);offset+=52;
        const auto response=sadx::decode_contact_response_constants(packed+offset,52);offset+=52;const auto attribute=sadx::decode_contact_attribute_constants(packed+offset,40);offset+=40;if(!response||!attribute)return -3;
        sadx::FloorAngleConstants floor;std::memcpy(&floor.upper_cut,packed+offset,4);std::memcpy(&floor.lower_cut,packed+offset+4,4);std::memcpy(&floor.axis_epsilon,packed+offset+8,4);std::memcpy(&floor.angle_units,packed+offset+12,8);std::memcpy(&floor.positive_factor,packed+offset+20,8);std::memcpy(&floor.negative_factor,packed+offset+28,8);offset+=36;
        sadx::PlayerMeshQueryConstants query;std::memcpy(&query.half_radius,packed+offset,4);std::memcpy(&query.three,packed+offset+4,4);std::memcpy(&query.negative_one,packed+offset+8,4);std::memcpy(&query.triangle_expansion,packed+offset+12,4);std::memcpy(&query.angle_units,packed+offset+16,8);std::memcpy(&query.angle_divisor,packed+offset+24,8);offset+=32;
        sadx::CoordinateAxisConstants axis;std::memcpy(&axis,packed+offset,8);offset+=8;std::array<float,3> scene_constants;std::memcpy(scene_constants.data(),packed+offset,12);
        auto decoded=sadx::decode_stage_scene_landtable(image,size,1,0);if(!decoded)return -4;auto& land=*decoded.table;land.source_count=static_cast<std::int16_t>(w[1]);
        sadx::PlayerSceneLandscape scene;scene.objects=land.objects;scene.models=land.models;scene.vector_arrays=land.vector_arrays;scene.mesh_set_arrays=land.mesh_set_arrays;scene.index_arrays=land.index_arrays;scene.material_arrays=land.material_arrays;scene.static_enabled=1;scene.landtable=sadx::SceneResourceReference{0};
        sadx::PlayerMeshRegistryState registry;registry.force_refresh=1;sadx::PlayerMeshQueryState contacts;sadx::MeshRegistryPools pools;for(std::size_t i=0;i<256;++i)pools.groups[i]=i;for(std::size_t i=0;i<16384;++i)pools.nodes[i]=i;
        const auto initialized=sadx::initialize_mesh_registry(scene,registry,contacts,pools);if(!initialized)return -5;
        sadx::NinjaMath::SineTable sine;std::memcpy(sine.data(),table,4100);sadx::NinjaMatrix identity;std::memcpy(identity.data(),unit,64);sadx::NinjaMatrixStack stack(std::vector<sadx::NinjaMatrix>(8,identity),1,0);sadx::PlayerCoordinates coordinates(sine,identity,stack);sadx::X87Context fp(cw);
        sadx::CrtFabsRuntime absolute{1,0,{}};sadx::SceneRegistrationRuntime registration;registration.registered_count=0;
        const auto generated=sadx::refresh_mesh_registry(scene,registry,{land,registration,{scene_constants[2]},absolute},coordinates,fp);if(!generated)return -6;
        auto allocation=std::make_shared<sadx::PlayerStartupPlayer>();allocation->player=std::make_shared<sadx::PlayerPhysicsState>();allocation->motion=std::make_shared<sadx::PlayerMotionState>();
        auto& player=*allocation->player;auto& world=*allocation->motion;sadx::PlayerTaskState task{};
        task.player_index=0;task.character_id=static_cast<std::uint8_t>(w[16]);task.action_at_0=static_cast<std::uint8_t>(w[17]);task.orientation.flags=static_cast<std::uint16_t>(w[8]);for(std::size_t i=0;i<3;++i){std::memcpy(&task.orientation.angles[i],&w[9+i],4);}
        task.position=vector(w+2);world.motion.speed=vector(w+5);world.friction_scale=1;world.angle_x_at_1c=0;world.angle_z_at_24=0;
        player.motion.speed=world.motion.speed;player.dotp=number(w[15]);player.floor_normal=sadx::NinjaVector{0,1,0};player.prior_attributes=w[13];player.shadow_attributes_top=w[14];player.source_vector_at_44=sadx::NinjaVector{0,0,0};player.flags_at_10=static_cast<std::uint16_t>(w[12]);player.no_control_time_at_a=0;player.moving_task_at_6c=sadx::TaskHandle{};player.cached_land_object_at_74=sadx::SceneResourceReference{};player.contact_work=zero_contact();
        const auto parameters=sadx::decode_player_parameters(parameter_rows+task.character_id*132,132);if(!parameters)return -7;allocation->parameters=*parameters.parameters;auto& live=*allocation->parameters;
        sadx::ContactParameterTable defaults{};for(std::size_t i=0;i<8;++i){const auto row=sadx::decode_player_parameters(parameter_rows+i*132,132);if(!row)return -8;defaults[i]={row.parameters->weight,row.parameters->air_resist_y,row.parameters->run_accel};}
        sadx::PlayerStartupRegistry players;players.player_work[0]=allocation;players.motion_work[0]=allocation->motion;
        sadx::PlayerScenePositionGlobals globals;globals.special_surface_at_3b42e74=0;globals.motion_override_at_3b42e70=0;globals.event_task_at_3b2c578=sadx::TaskHandle{};globals.environment_at_3b22dec=0;globals.render_at_3b0f0ec=-1;
        sadx::PlayerInputRuntime input;input.stage=1;input.act=0;input.current_character=task.character_id;input.normalized[0]=sadx::NormalizedPlayerInput{0,0};input.no_control_timer[0]=0;input.camera_yaw=0;const sadx::ForcedInputAngleConversion forced;
        sadx::PlayerMeshQueryRuntime math{{1,{}},{1,{}},{1,0,{}},10000};const std::optional<sadx::NinjaVector> gravity=sadx::NinjaVector{0,-1,0};sadx::PlayerContactResponseRuntime response_math{math.inverse_math,gravity,&sine,0};std::unordered_map<std::size_t,sadx::TaskHandle> geometry_tasks;
        const sadx::SceneBroadphaseConstants broadphase{scene_constants[0],scene_constants[1]};sadx::PlayerScenePositionRuntime runtime{globals,contacts,math,response_math,input,forced,{&players,nullptr},&geometry_tasks,defaults,*attribute,*response,initializer,rotation,floor,query,broadphase,axis};
        const auto result=w[0]?sadx::position_player_in_scene_alternate(task,&world,player,live,scene,*constants,runtime,coordinates,fp):sadx::position_player_in_scene(task,&world,player,live,scene,*constants,runtime,coordinates,fp);
        out[0]=static_cast<std::uint32_t>(result.failure);out[1]=result.source_boundary;out[2]=result.nested_source_boundary;if(!result)return 1;
        out[3]=static_cast<std::uint32_t>(*result.outcome);out[4]=task.orientation.flags;for(std::size_t i=0;i<3;++i)out[5+i]=static_cast<std::uint32_t>(task.orientation.angles[i]);out[8]=bits(task.position->x);out[9]=bits(task.position->y);out[10]=bits(task.position->z);
        out[11]=bits(world.motion.speed.x);out[12]=bits(world.motion.speed.y);out[13]=bits(world.motion.speed.z);out[14]=static_cast<std::uint32_t>(*world.angle_x_at_1c);out[15]=static_cast<std::uint32_t>(*world.angle_z_at_24);out[16]=static_cast<std::uint32_t>(world.target_yaw);out[17]=bits(world.friction_scale);
        out[18]=bits(player.motion.speed.x);out[19]=bits(player.motion.speed.y);out[20]=bits(player.motion.speed.z);out[21]=player.attributes;out[22]=*player.flags_at_10;out[23]=bits(player.floor_normal->x);out[24]=bits(player.floor_normal->y);out[25]=bits(player.floor_normal->z);out[26]=bits(player.source_vector_at_44->x);out[27]=bits(player.source_vector_at_44->y);out[28]=bits(player.source_vector_at_44->z);out[29]=bits(live.weight);out[30]=bits(live.air_resist_y);out[31]=bits(live.run_accel);
        out[32]=player.cached_land_object_at_74->id?land.object_locations.at(*player.cached_land_object_at_74->id).source_virtual_address:0;
        out[33]=*globals.special_surface_at_3b42e74;out[34]=scene.candidate_count;out[35]=scene.selected_count;out[36]=*contacts.contact_count;out[37]=stack.depth();out[38]=fp.control_word();out[39]=fp.exception_flags();
        std::array<std::uint32_t,33> words{};const auto& contact=*player.contact_work;words[0]=bits(*contact.radius_at_0);
        const auto put=[&](std::size_t at,const sadx::NinjaVector& v){words[at]=bits(v.x);words[at+1]=bits(v.y);words[at+2]=bits(v.z);};put(1,*contact.center_at_4);put(4,*contact.displacement_at_10);put(7,*contact.direction_at_1c);words[10]=*contact.contact_count_at_28|(std::uint32_t(*contact.selected_contact_at_2a)<<16);words[13]=static_cast<std::uint32_t>(*contact.angle_x_at_34);words[14]=static_cast<std::uint32_t>(*contact.angle_z_at_38);put(15,*contact.vector_at_3c);put(18,*contact.vector_at_48);put(21,*contact.vector_at_54);put(24,*contact.vector_at_60);put(27,*contact.vector_at_6c);put(30,*contact.vector_at_78);
        for(std::size_t i=0;i<33;++i)out[40+i]=words[i];for(std::size_t i=0;i<8;++i)for(std::size_t j=0;j<16;++j)out[73+i*16+j]=bits(stack.slots()[i][j]);return 0;
    }catch(const sadx::X87Error&){return -9;}catch(const sadx::CoordinateError&){return -10;}
}
extern "C" const char* sadx_native_player_scene_position_compiler(){return "Clang " __clang_version__ "; ARM64 C++17 O2 native real ECoast registry and full scene-position bodies; explicit policy1";}
