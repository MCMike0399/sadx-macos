#include "game/selection_preview.hpp"
#include "game/game_cleanup.hpp"
#include "cpu/x87_math.hpp"
#include <algorithm>
#include <cstring>
#include <unordered_map>
#include <vector>
namespace p = sadx::selection_preview;
namespace {
float number(std::uint32_t v) { float x; std::memcpy(&x,&v,4); return x; }
std::uint32_t bits(float x) { std::uint32_t v; std::memcpy(&v,&x,4); return v; }
void update_callback(sadx::TaskScheduler &, const sadx::TaskHandle &) {}
void draw_callback(sadx::TaskScheduler &, const sadx::TaskHandle &) {}
void destroy_callback(sadx::TaskScheduler &, const sadx::TaskHandle &) {}
struct Fields {
    sadx::NinjaVector position{};
    std::array<std::int32_t,3> rotation{};
    std::uint8_t mode=0, task_id=0, player=0, character=0;
    std::int32_t aim=0;
};
struct Fixture final : p::Calls {
    const std::uint32_t *in;
    sadx::TaskScheduler scheduler;
    p::State state;
    std::shared_ptr<p::Work> work=std::make_shared<p::Work>();
    sadx::TaskHandle preview, other;
    std::array<sadx::TaskHandle,16> tasks{};
    std::unordered_map<const sadx::Task *,unsigned> ids,field_ids;
    std::array<Fields,16> fields{};
    std::array<sadx::NinjaVector,7> source_positions{};
    std::array<std::uint16_t,7> characters{};
    std::int32_t sonic_aim=0, saved_fov=0;
    float horizontal=1;
    std::uint8_t selected=0;
    std::array<std::int8_t,9> loaded{};
    std::array<sadx::regular_textures::List,18> lists{};
    std::array<std::vector<sadx::regular_textures::Binding>,9> groups{};
    std::array<const std::vector<sadx::regular_textures::Binding> *,9> catalog{};
    std::array<sadx::cleanup::CharacterTextureLists,9> cleanup_groups{};
    std::array<const sadx::cleanup::CharacterTextureLists *,9> cleanup_catalog{};
    sadx::sprite::TextureList *common=nullptr;
    std::array<sadx::sprite::TextureList *,32> regular{};
    std::vector<std::uint32_t> trace;
    std::uint32_t error=0,error_actor=0;
    unsigned factory_index=0;
    Fixture(const std::uint32_t *i,const std::uint32_t *table):in(i) {
        auto make=[&](unsigned id) { auto t=scheduler.append_task(0,nullptr).task; ids[t.get()]=id; return t; };
        preview=make(30);other=make(31);
        for(unsigned n=0;n<16;++n) {
            tasks[n]=make(n<7?n+1:n<14?n+4:n==14?21:22);field_ids[tasks[n].get()]=n;
            fields[n]={{number(i[48]),number(i[49]),number(i[50])},
                       {static_cast<std::int32_t>(i[51]),static_cast<std::int32_t>(i[52]),static_cast<std::int32_t>(i[53])},
                       static_cast<std::uint8_t>(i[55]),static_cast<std::uint8_t>(i[58]),static_cast<std::uint8_t>(i[56]),static_cast<std::uint8_t>(i[57]),static_cast<std::int32_t>(i[54])};
        }
        for(unsigned n=0;n<7;++n) {
            work->actors[n]=(i[5]&(1U<<n))?sadx::TaskHandle{}:tasks[n];
            work->motion_counts[n]=static_cast<std::int8_t>(i[32+n]);
            source_positions[n]={number(table[3*n]),number(table[3*n+1]),number(table[3*n+2])};
            characters[n]=static_cast<std::uint16_t>(table[21+n]);
            state.positions[n]={number(i[64+3*n]),number(i[65+3*n]),number(i[66+3*n])};
        }
        work->camera=i[40]?tasks[14]:sadx::TaskHandle{};
        work->location_x=number(i[27]);work->location_y=number(i[28]);work->scale=number(i[29]);
        work->warmup_frames=static_cast<std::int8_t>(i[1]);work->selected_motion=static_cast<std::uint8_t>(i[39]);
        preview->auxiliary_native_work=i[30]?std::shared_ptr<void>{}:work;
        state.task=i[41]?preview:sadx::TaskHandle{};state.reset_marker=i[11];
        horizontal=number(i[3]);selected=static_cast<std::uint8_t>(i[2]);saved_fov=static_cast<std::int32_t>(i[12]);sonic_aim=static_cast<std::int32_t>(i[13]);
        for(unsigned n=0;n<9;++n) {
            loaded[n]=static_cast<std::int8_t>(i[16+n]);
            if(!(i[7]&(1U<<n))) { catalog[n]=&groups[n];cleanup_catalog[n]=&cleanup_groups[n]; }
            if(i[8]&(1U<<n))continue;
            for(unsigned k=0;k<2;++k) {
                std::optional<std::string> name;
                if(k!=0 || !(i[9]&(1U<<n)))name="A"+std::to_string(n)+std::to_string(k);
                groups[n].push_back({name,&lists[2*n+k]});cleanup_groups[n].lists.push_back(&lists[2*n+k]);
            }
        }
    }
    p::Assets assets() { return {characters,source_positions,sonic_aim}; }
    p::View view() { return {horizontal,selected,saved_fov}; }
    unsigned id(const sadx::TaskHandle &task) { return task?ids.at(task.get()):0; }
    void event(unsigned kind,unsigned a=0,unsigned b=0,unsigned c=0) {trace.insert(trace.end(),{kind,a,b,c});}
    sadx::TaskHandle create_preview_task(std::uint32_t flags,std::int32_t level) override {
        event(1,flags,static_cast<unsigned>(level));if(in[4]&1)return {};
        preview->exec=update_callback;preview->display=nullptr;preview->destructor=nullptr;preview->auxiliary_native_work.reset();return preview;
    }
    std::shared_ptr<p::Work> allocate_work(const sadx::TaskHandle &) override {
        event(2,1,56,id(state.task));if(in[4]&2)return {};
        work=std::make_shared<p::Work>();return work;
    }
    void install_callbacks(const sadx::TaskHandle &task) override {task->exec=update_callback;task->display=draw_callback;task->destructor=destroy_callback;}
    void select_palette(std::int32_t value) override {event(3,static_cast<unsigned>(value));if(in[15]!=0xffffffffU)horizontal=number(in[15]);}
    sadx::TaskHandle create_camera(std::uint32_t flags,std::int32_t level) override {event(4,flags,static_cast<unsigned>(level));return in[4]&4?sadx::TaskHandle{}:tasks[15];}
    p::Pose camera_pose(const sadx::TaskHandle &task) override {auto &f=fields[field_ids.at(task.get())];return {f.position,f.rotation};}
    sadx::TaskHandle create_actor(p::ActorKind kind,std::uint32_t flags,std::int32_t level) override {
        const auto slot=static_cast<unsigned>(kind);event(5,slot,flags,static_cast<unsigned>(level));++factory_index;return in[4]&(1U<<(8+slot))?sadx::TaskHandle{}:tasks[slot+7];
    }
    p::Actor actor(const sadx::TaskHandle &task) override {auto &f=fields[field_ids.at(task.get())];return {{f.position,f.rotation},f.task_id,f.player,f.character,f.aim};}
    void load_character_textures(std::uint16_t character) override {p::load_textures(character,{catalog,loaded},*this);}
    unsigned texture_id(sadx::regular_textures::List &list) {return static_cast<unsigned>(&list-lists.data());}
    void loaded_mutation(unsigned t) {if(in[10]!=0xffffffffU)loaded[t/2]=static_cast<std::int8_t>(in[10]);}
    void load_named_texture(std::string_view name,sadx::regular_textures::List &list) override {auto t=texture_id(list);const auto code=static_cast<std::uint32_t>(name[0])|(static_cast<std::uint32_t>(name[1])<<8)|(static_cast<std::uint32_t>(name[2])<<16);event(6,t,code);loaded_mutation(t);}
    void load_individual_texture(sadx::regular_textures::List &list) override {auto t=texture_id(list);event(7,t);loaded_mutation(t);}
    void release_texture(sadx::regular_textures::List &list) override {event(8,texture_id(list));}
    void release_actor_textures(std::uint16_t character) override {p::release_textures(character,{catalog,loaded},*this);}
    void destroy_task(const sadx::TaskHandle &task) override {event(9,id(task));}
    void set_perspective(std::int32_t angle) override {event(10,static_cast<unsigned>(angle));}
    void update_actor_actions(p::Work &value) override {event(11,static_cast<std::uint8_t>(value.warmup_frames));value.selected_motion=static_cast<std::uint8_t>(in[42]);}
};
}
extern "C" void sadx_native_selection_preview_probe(const std::uint32_t *in,std::uint32_t *out,const std::uint32_t *table) {
    std::fill_n(out,1024,0);Fixture f(in,table);sadx::X87Context fp{static_cast<std::uint16_t>(in[6])};
    try {
        switch(in[0]) {
        case 0:p::create(f.state,f.assets(),f.view(),f,fp);break;
        case 1:p::initialize(f.preview,f.state,f.assets(),f.view(),f,fp);break;
        case 2:p::prepare_actors(*f.work,f.state,f.assets(),f.view(),f,fp);break;
        case 3:p::reset_actors(*f.work,f.assets(),f);break;
        case 4:p::update(f.preview,f.state,f.assets(),f.view(),f);break;
        case 5:p::position_selected(*f.work,f.state,f.assets(),f.view(),f);break;
        case 6:p::draw(f.preview,f.view(),f);break;
        case 7:p::release_actors(*f.work,f.assets(),f);break;
        case 8:p::destroy(f.preview,f.assets(),f);break;
        case 9:p::remove(f.state,f);break;
        case 10:p::load_textures(static_cast<std::uint16_t>(in[2]),{f.catalog,f.loaded},f);break;
        }
    } catch(const p::Error &error) {f.error=static_cast<std::uint32_t>(error.boundary())+1;f.error_actor=static_cast<std::uint32_t>(error.actor());}
    auto &w=*f.work;out[0]=f.id(f.state.task);out[1]=f.state.reset_marker;out[2]=static_cast<std::uint32_t>(f.saved_fov);
    for(unsigned n=0;n<7;++n)out[3+n]=f.id(w.actors[n]);out[10]=f.id(w.camera);
    out[11]=bits(w.location_x);out[12]=bits(w.location_y);out[13]=bits(w.scale);out[14]=static_cast<std::uint8_t>(w.warmup_frames);
    for(unsigned n=0;n<7;++n)out[15+n]=static_cast<std::uint8_t>(w.motion_counts[n]);out[22]=w.selected_motion;
    out[23]=f.preview->exec==update_callback?1:0;out[24]=f.preview->display==draw_callback?2:0;out[25]=f.preview->destructor==destroy_callback?3:0;
    out[26]=f.preview->auxiliary_native_work?1:0;out[27]=f.error;out[28]=f.error_actor;out[31]=fp.exception_flags();
    for(unsigned n=0;n<7;++n){out[32+3*n]=bits(f.state.positions[n].x);out[33+3*n]=bits(f.state.positions[n].y);out[34+3*n]=bits(f.state.positions[n].z);}
    for(unsigned n=0;n<9;++n)out[56+n]=static_cast<std::uint8_t>(f.loaded[n]);
    for(unsigned n=0;n<16;++n){const auto &v=f.fields[n];auto p=out+80+8*n;p[0]=v.mode|(std::uint32_t(v.player)<<8)|(std::uint32_t(v.character)<<16)|(std::uint32_t(v.task_id)<<24);for(unsigned k=0;k<3;++k)p[k+1]=static_cast<std::uint32_t>(v.rotation[k]);p[4]=bits(v.position.x);p[5]=bits(v.position.y);p[6]=bits(v.position.z);p[7]=static_cast<std::uint32_t>(v.aim);}
    out[220]=static_cast<std::uint32_t>(f.trace.size());std::copy(f.trace.begin(),f.trace.end(),out+224);
}
