#include "player/player_mesh_query.hpp"
#include <cstring>

namespace {
float number(std::uint32_t w){float f;std::memcpy(&f,&w,4);return f;}
std::uint32_t bits(float f){std::uint32_t w;std::memcpy(&w,&f,4);return w;}
sadx::NinjaVector vector(const std::uint32_t* w){return {number(w[0]),number(w[1]),number(w[2])};}
void store(std::uint32_t* w,const sadx::NinjaVector& v){w[0]=bits(v.x);w[1]=bits(v.y);w[2]=bits(v.z);}
}
extern "C" int sadx_native_player_mesh_query(const std::uint8_t* table,const std::uint8_t* unit,
    const std::uint32_t* axis,const std::uint8_t* floor_bytes,const std::uint8_t* query_bytes,
    const std::uint32_t* w,std::size_t count,std::uint16_t cw,std::uint32_t* output){
    if(!table || !unit || !axis || !floor_bytes || !query_bytes || !w || !output || count<8 || w[7]>128 || count!=8+w[7]*10)return -1;
    try {
        sadx::FloorAngleConstants fc;std::memcpy(&fc.upper_cut,floor_bytes,4);std::memcpy(&fc.lower_cut,floor_bytes+4,4);std::memcpy(&fc.axis_epsilon,floor_bytes+8,4);
        std::memcpy(&fc.angle_units,floor_bytes+12,8);std::memcpy(&fc.positive_factor,floor_bytes+20,8);std::memcpy(&fc.negative_factor,floor_bytes+28,8);
        sadx::PlayerMeshQueryConstants qc;std::memcpy(&qc.half_radius,query_bytes,4);std::memcpy(&qc.three,query_bytes+4,4);std::memcpy(&qc.negative_one,query_bytes+8,4);std::memcpy(&qc.triangle_expansion,query_bytes+12,4);std::memcpy(&qc.angle_units,query_bytes+16,8);std::memcpy(&qc.angle_divisor,query_bytes+24,8);
        sadx::PlayerContactWork contact;contact.radius_at_0=number(w[0]);contact.center_at_4=vector(w+1);contact.displacement_at_10=vector(w+4);contact.direction_at_1c=sadx::NinjaVector{11,12,13};
        contact.contact_count_at_28=23;contact.selected_contact_at_2a=24;contact.angle_x_at_34=0x11111111;contact.angle_z_at_38=0x22222222;
        contact.vector_at_3c=sadx::NinjaVector{101,102,103};contact.vector_at_48=sadx::NinjaVector{104,105,106};contact.vector_at_54=sadx::NinjaVector{107,108,109};contact.vector_at_60=sadx::NinjaVector{110,111,112};contact.vector_at_6c=sadx::NinjaVector{113,114,115};contact.vector_at_78=sadx::NinjaVector{116,117,118};
        sadx::PlayerSceneLandscape scene;scene.mesh_groups=sadx::SceneResourceReference{0};auto& group=scene.groups[0];group.next=sadx::SceneResourceReference{};group.object=sadx::SceneResourceReference{1};group.selected_begin=0;group.selected_count=w[7];
        for(std::size_t n=0;n<w[7];++n){const auto p=w+8+n*10;auto& node=scene.nodes[n];node.next=sadx::SceneResourceReference{n+1<w[7]?std::optional<std::size_t>{n+1}:std::nullopt};node.previous=sadx::SceneResourceReference{n?std::optional<std::size_t>{n-1}:std::nullopt};node.triangle_vertices=std::array<sadx::NinjaVector,3>{vector(p),vector(p+3),vector(p+6)};node.raw_at_3c=p[9];scene.selected_nodes[n]=n;}
        sadx::PlayerMeshQueryState state;state.contact_count=17;for(auto& r:state.records)r.untouched_word_at_0=0xa5a5a5a5;
        sadx::PlayerMeshQueryRuntime runtime{{1,{}},{1,{}},{1,0x12345678,{}},10000};
        sadx::NinjaMath::SineTable sine;std::memcpy(sine.data(),table,4100);sadx::NinjaMatrix identity;std::memcpy(identity.data(),unit,64);
        sadx::NinjaMatrixStack stack(std::vector<sadx::NinjaMatrix>(4,identity),1,0);sadx::PlayerCoordinates coordinates(sine,identity,stack);sadx::X87Context fp(cw);
        const auto result=sadx::query_mesh_object_contacts(contact,scene,sadx::SceneResourceReference{1},state,qc,fc,{number(axis[0]),number(axis[1])},runtime,coordinates,fp);
        output[0]=static_cast<std::uint32_t>(result.failure);output[1]=result.source_boundary;output[2]=result.outcome?static_cast<std::uint32_t>(*result.outcome):0xdeadbeef;
        output[3]=*state.contact_count;output[4]=*contact.contact_count_at_28;output[5]=*contact.selected_contact_at_2a;output[6]=static_cast<std::uint32_t>(*contact.angle_x_at_34);output[7]=static_cast<std::uint32_t>(*contact.angle_z_at_38);
        store(output+8,*contact.vector_at_3c);store(output+11,*contact.vector_at_48);store(output+14,*contact.vector_at_54);store(output+17,*contact.vector_at_60);store(output+20,*contact.vector_at_6c);store(output+23,*contact.vector_at_78);
        output[26]=fp.exception_flags();output[27]=fp.control_word();output[28]=stack.depth();
        for(std::size_t n=0;n<*state.contact_count && n<16;++n){if(state.records[n].payload)std::memcpy(output+29+n*22,&*state.records[n].payload,88);}
        for(std::size_t row=0;row<4;++row)for(std::size_t v=0;v<16;++v)output[381+row*16+v]=bits(stack.slots()[row][v]);return 0;
    }catch(const sadx::X87Error&){return -3;}catch(const sadx::CoordinateError&){return -4;}
}
extern "C" const char* sadx_native_player_mesh_query_compiler(){return "Clang " __clang_version__ "; ARM64 C++17 O2; actual453820 native owner/software x87";}
