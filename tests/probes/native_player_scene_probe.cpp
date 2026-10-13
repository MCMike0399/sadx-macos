#include "player/player_scene_registration.hpp"
#include <cstring>

namespace {
float number(std::uint32_t w){float v;std::memcpy(&v,&w,4);return v;}
std::uint32_t bits(float v){std::uint32_t w;std::memcpy(&w,&v,4);return w;}
sadx::NinjaVector vector(const std::uint32_t* w){return {number(w[0]),number(w[1]),number(w[2])};}
}
extern "C" int sadx_native_player_scene(const std::uint8_t* image,std::size_t image_size,
    const std::uint8_t* table,const std::uint8_t* unit,const std::uint32_t* constants,
    const std::uint32_t* w,std::uint16_t cw,std::uint32_t* out) {
    if(!image || !table || !unit || !constants || !w || !out)return -1;
    try {
        const auto decoded=sadx::decode_stage_scene_landtable(image,image_size,1,0);if(!decoded)return -2;
        const auto& land=*decoded.table;
        sadx::PlayerSceneLandscape scene;scene.objects=land.objects;scene.models=land.models;
        scene.vector_arrays=land.vector_arrays;scene.mesh_set_arrays=land.mesh_set_arrays;scene.index_arrays=land.index_arrays;
        scene.material_arrays=land.material_arrays;
        scene.static_enabled=1;scene.landtable=sadx::SceneResourceReference{0};scene.mesh_groups=sadx::SceneResourceReference{};
        sadx::PlayerTaskState camera{{0,{0,0,0}},0,0,{},vector(w+3)},first{{0,{0,0,0}},0,0,{},vector(w+6)},second{{0,{0,0,0}},0,0,{},vector(w+9)};
        sadx::SceneRegistrationRuntime runtime;runtime.camera=sadx::SceneTaskReadView{w[0]?&camera:nullptr};
        runtime.first_player=sadx::SceneTaskReadView{w[1]?&first:nullptr};runtime.second_player=sadx::SceneTaskReadView{w[2]?&second:nullptr};runtime.registered_count=0;runtime.view_distance=number(w[19]);
        sadx::NinjaMath::SineTable sine;std::memcpy(sine.data(),table,4100);sadx::NinjaMatrix identity;std::memcpy(identity.data(),unit,64);
        sadx::NinjaMatrixStack stack(std::vector<sadx::NinjaMatrix>(3,identity),1,0);sadx::PlayerCoordinates coordinates(sine,identity,stack);sadx::X87Context fp(cw);
        auto result=sadx::register_player_scene_landscape(scene,land,runtime,{number(constants[2])},coordinates,fp);
        if(!result){out[0]=static_cast<std::uint32_t>(result.failure);out[1]=result.source_boundary;return 1;}
        // This source profile requires the nonnull camera branch so the actual
        // registration stores initialize counts; no unknown->zero fallback.
        if(!scene.static_count || !scene.dynamic_count)return 2;
        const auto registration_count=scene.candidate_count;
        sadx::PlayerContactWork contact;contact.center_at_4=vector(w+12);contact.radius_at_0=number(w[15]);contact.displacement_at_10=vector(w+16);
        const sadx::SceneBroadphaseConstants c{number(constants[0]),number(constants[1])};result=sadx::query_player_scene_broadphase(scene,contact,c,coordinates,fp);
        out[0]=static_cast<std::uint32_t>(result.failure);out[1]=result.source_boundary;out[2]=static_cast<std::uint32_t>(*scene.static_count);out[3]=static_cast<std::uint32_t>(*scene.dynamic_count);
        out[4]=static_cast<std::uint32_t>(registration_count);out[5]=static_cast<std::uint32_t>(scene.candidate_count);out[6]=scene.selected_count;out[7]=fp.control_word();out[8]=fp.exception_flags();out[9]=stack.depth();
        for(std::size_t r=0;r<3;++r)for(std::size_t v=0;v<16;++v)out[10+r*16+v]=bits(stack.slots()[r][v]);
        const auto address=[&](const sadx::SceneResourceReference& ref){return ref.id?land.object_locations.at(*ref.id).source_virtual_address:0U;};
        for(std::size_t i=0;i<static_cast<std::size_t>(*scene.static_count);++i){
            const auto& e=scene.static_entries[i];const auto at=58+i*9;
            out[at]=bits(e.center->x);out[at+1]=bits(e.center->y);out[at+2]=bits(e.center->z);out[at+3]=bits(*e.radius);
            out[at+4]=*e.raw_at_10;out[at+5]=*e.raw_at_14;out[at+6]=address(*e.object);out[at+7]=*e.raw_at_1c;out[at+8]=*e.attributes;
        }
        for(std::size_t i=0;i<static_cast<std::size_t>(scene.candidate_count);++i){const auto& e=*scene.candidates[i];const auto at=1210+i*3;out[at]=*e.attributes;out[at+1]=address(*e.object);out[at+2]=0;}
        return 0;
    }catch(const sadx::X87Error&){return -3;}catch(const sadx::CoordinateError&){return -4;}
}
extern "C" const char* sadx_native_player_scene_compiler(){return "Clang " __clang_version__ "; ARM64 C++17 O2 software x87; actual PE32 landtable decode";}
extern "C" int sadx_native_scene_landtable_identity(const std::uint8_t* image,std::size_t size,
    std::int8_t stage,std::int8_t act,std::uint32_t* out){
    if(!image || !out)return -1;const auto decoded=sadx::decode_stage_scene_landtable(image,size,stage,act);
    out[0]=static_cast<std::uint32_t>(decoded.failure);out[1]=decoded.requested_source_address;
    out[2]=decoded.table?static_cast<std::uint32_t>(decoded.table->source_count):0;
    out[3]=decoded.table?decoded.table->source_flags:0;return 0;
}

extern "C" int sadx_native_scene_mesh_bounds(const std::uint8_t* table,const std::uint8_t* unit,
    const std::uint32_t* constants,const std::uint32_t* w,std::size_t words,std::uint16_t cw,std::uint32_t* out){
    if(!table || !unit || !constants || !w || !out || words<18 || w[0]>160 || words!=18+w[0]*6)return -1;
    try {
        const auto ref=[](std::uint32_t id){return sadx::SceneResourceReference{id==0xffffffffU?std::nullopt:std::optional<std::size_t>{id}};};
        sadx::PlayerSceneLandscape scene;scene.dynamic_count=w[13]?1:0;scene.static_enabled=1;scene.landtable=sadx::SceneResourceReference{0};scene.static_count=1;
        scene.dynamic_land_entries.push_back({w[14],sadx::SceneResourceReference{1},sadx::SceneResourceReference{}});
        scene.models[1]={sadx::NinjaVector{1,2,3},1000};
        const auto signed_word=[](std::uint32_t bits){std::int32_t value;std::memcpy(&value,&bits,4);return value;};
        scene.objects[1]={sadx::SceneResourceReference{1},sadx::NinjaVector{4,5,6},std::array<std::int32_t,3>{signed_word(w[15]),signed_word(w[16]),signed_word(w[17])}};
        scene.static_entries.push_back({sadx::NinjaVector{0,0,0},number(w[12]),sadx::SceneResourceReference{1},1});scene.mesh_groups=sadx::SceneResourceReference{0};
        scene.groups[0]={sadx::SceneResourceReference{1},sadx::SceneResourceReference{w[3]?1U:2U},ref(w[1]),ref(w[2]),number(w[4]),17,44};
        scene.groups[1]={sadx::SceneResourceReference{},sadx::SceneResourceReference{3},{},{},{},17,44};
        for(std::size_t i=0;i<w[0];++i){const auto p=w+18+i*6;scene.nodes[i]={ref(p[0]),ref(p[1]),vector(p+2),number(p[5])};}
        sadx::PlayerContactWork contact;contact.center_at_4=vector(w+5);contact.radius_at_0=number(w[8]);contact.displacement_at_10=vector(w+9);
        sadx::NinjaMath::SineTable sine;std::memcpy(sine.data(),table,4100);sadx::NinjaMatrix identity;std::memcpy(identity.data(),unit,64);
        sadx::NinjaMatrixStack stack(std::vector<sadx::NinjaMatrix>(3,identity),1,0);sadx::PlayerCoordinates coordinates(sine,identity,stack);sadx::X87Context fp(cw);
        const auto result=sadx::query_player_scene_broadphase(scene,contact,{number(constants[0]),number(constants[1])},coordinates,fp);
        out[0]=static_cast<std::uint32_t>(result.failure);out[1]=result.source_boundary;out[2]=scene.selected_count;
        out[3]=scene.groups[0].selected_begin?static_cast<std::uint32_t>(*scene.groups[0].selected_begin):0xffffffffU;out[4]=scene.groups[0].selected_count;
        out[5]=scene.groups[1].selected_begin?static_cast<std::uint32_t>(*scene.groups[1].selected_begin):0xffffffffU;out[6]=scene.groups[1].selected_count;
        out[7]=fp.exception_flags();out[8]=fp.control_word();out[9]=stack.depth();
        for(std::size_t i=0;i<scene.selected_count;++i)out[10+i]=static_cast<std::uint32_t>(*scene.selected_nodes[i]);
        for(std::size_t r=0;r<3;++r)for(std::size_t v=0;v<16;++v)out[138+r*16+v]=bits(stack.slots()[r][v]);return 0;
    }catch(const sadx::X87Error&){return -3;}catch(const sadx::CoordinateError&){return -4;}
}
