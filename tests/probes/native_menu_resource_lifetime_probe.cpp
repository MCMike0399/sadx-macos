#include "game/menu_resource_lifetime.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <sstream>

namespace {
using namespace sadx;
struct Stop {};
void copy_trace(std::string value,char *trace,std::size_t size) {
    if(value.size()+1>size)throw std::runtime_error("menu resource probe trace capacity");
    std::memcpy(trace,value.c_str(),value.size()+1);
}
}
extern "C" int sadx_native_menu_texture_release(const std::uint32_t *in,const std::uint32_t *data,
                                                  std::uint32_t *out,char *trace,std::size_t capacity) {
    using namespace sadx;
    namespace resource=menu_resource_lifetime;
    std::fill_n(out,32,0);
    std::array<texture_lifetime::List,151> lists{};
    resource::TextureCatalog catalog{};
    for(std::size_t language=0;language<5;++language)
        for(std::size_t id=0;id<30;++id)
            catalog[language][id]={std::nullopt,&lists[in[21]?(id%5):(language*30+id)]};
    title_menu::ModeTextureLists modes{};
    for(std::size_t mode=0;mode<14;++mode)
        for(unsigned i=0;i<data[mode*32];++i)modes[mode].push_back({data[mode*32+1+i]});
    title_menu::MenuTextureState textures{{in[4],in[5]},static_cast<std::int32_t>(in[6])};
    auto language=static_cast<std::int32_t>(in[3]);
    auto demo=static_cast<std::int32_t>(in[7]);
    title_menu::SequenceWork sequence{};
    sequence.mode=static_cast<title_menu::MenuMode>(in[1]);
    sequence.old_mode=static_cast<title_menu::MenuMode>(in[2]);
    resource::State state{textures,language,catalog,modes};
    std::vector<title_menu::MenuTextureId> ids;
    for(unsigned i=0;i<in[8];++i)ids.push_back({data[448+i]});
    unsigned calls=0;
    std::ostringstream events;
    resource::Services services{[&](texture_lifetime::List *list) {
        events<<1<<'|'<<(list?list-lists.data()+1:0)<<'|'<<static_cast<std::uint32_t>(language)
              <<'|'<<textures.loaded[0]<<'|'<<textures.loaded[1]<<'|'<<static_cast<std::uint32_t>(textures.ready)
              <<'|'<<static_cast<std::uint32_t>(demo)<<'|'<<static_cast<std::uint32_t>(sequence.mode)
              <<'|'<<static_cast<std::uint32_t>(sequence.old_mode)<<'\n';
        if(++calls==in[10])throw Stop{};
        if(calls==in[11]) {
            language=static_cast<std::int32_t>(in[12]);textures.loaded={in[13],in[14]};
            textures.ready=static_cast<std::int32_t>(in[15]);
            sequence.mode=static_cast<title_menu::MenuMode>(in[16]);
            sequence.old_mode=static_cast<title_menu::MenuMode>(in[17]);
        }
    },{}, {}};
    try {
        switch(in[0]) {
        case 0:resource::release_ids(in[9]?nullptr:&ids,state,services);break;
        case 1:resource::release_all(state,services);break;
        case 2:resource::release_mode_textures(sequence,state,services);break;
        case 3:resource::release_japanese_title(lists[150],services);break;
        }
    } catch(const Stop &) {out[0]=1;}
    catch(const resource::Error &error) {out[0]=2+static_cast<unsigned>(error.boundary());}
    out[1]=textures.loaded[0];out[2]=textures.loaded[1];out[3]=static_cast<std::uint32_t>(textures.ready);
    out[4]=static_cast<std::uint32_t>(language);out[5]=static_cast<std::uint32_t>(sequence.mode);
    out[6]=static_cast<std::uint32_t>(sequence.old_mode);out[7]=calls;
    copy_trace(events.str(),trace,capacity);return 1;
}

namespace sadx {
class TaskSourceProbe {
  public:
    static TaskHandle make() { return TaskHandle(new Task); }
};
}
extern "C" int sadx_native_menu_task_release(const std::uint32_t *in,const std::uint32_t *data,
                                               std::uint32_t *out,char *trace,std::size_t capacity) {
    using namespace sadx;
    namespace resource=menu_resource_lifetime;
    std::fill_n(out,64,0);
    std::array<TaskHandle,17> handles{};
    for(unsigned i=1;i<handles.size();++i)handles[i]=TaskSourceProbe::make();
    const auto id=[&](const TaskHandle &task)->unsigned {for(unsigned i=1;i<handles.size();++i)if(handles[i]==task)return i;return 0;};
    title_menu::State menus{};
    std::array<TaskHandle *,14> slots{nullptr,&menus.factories.character_select_task,&menus.factories.file_select_task,
        &menus.factories.level_select_task,&menus.factories.sound_test_task,&menus.title_task,&menus.factories.main_menu_task,
        &menus.factories.options_task,&menus.factories.explanation_task,&menus.factories.subgame_task,&menus.factories.emblem_task,
        &menus.factories.internet_demo_task,&menus.factories.tv_settings_task,&menus.factories.puti_task};
    for(unsigned i=1;i<14;++i)*slots[i]=handles[in[31+i]];
    menus.dialog_task=handles[in[46]];
    std::array<texture_lifetime::List,151> lists{};
    resource::TextureCatalog catalog{};
    for(unsigned language=0;language<5;++language)
        for(unsigned texture=0;texture<30;++texture)catalog[language][texture]={std::nullopt,&lists[in[21]?(texture%5):(language*30+texture)]};
    title_menu::MenuTextureState textures{{in[4],in[5]},static_cast<std::int32_t>(in[6])};
    auto language=static_cast<std::int32_t>(in[3]);
    auto demo=static_cast<std::int32_t>(in[7]);
    title_menu::ModeTextureLists modes{};
    resource::State state{textures,language,catalog,modes};
    title_menu::SequenceData sequence{},alternate{};
    const auto fill=[&](title_menu::SequenceData &value,unsigned offset) {
        value.born_mode=static_cast<title_menu::MenuMode>(data[offset]);
        for(unsigned i=1;i<in[23];++i)value.following.push_back({static_cast<title_menu::MenuMode>(data[offset+i]),{}});
    };
    fill(sequence,480);fill(alternate,496);
    title_menu::SequenceWork work{};work.data=in[25]?nullptr:&sequence;
    unsigned calls=0;
    std::ostringstream events;
    const auto event=[&](unsigned kind,unsigned identity) {
        events<<kind<<'|'<<identity<<'|'<<static_cast<std::uint32_t>(demo)<<'|'<<textures.loaded[0]<<'|'<<textures.loaded[1]
              <<'|'<<static_cast<std::uint32_t>(textures.ready)<<'|'<<id(menus.dialog_task);
        for(unsigned i=1;i<14;++i)events<<'|'<<id(*slots[i]);
        events<<'\n';
        if(++calls==in[10])throw Stop{};
        if(calls==in[11]) {
            if(in[19]>0 && in[19]<14)*slots[in[19]]=handles[in[20]];
            menus.dialog_task=handles[in[24]];
            demo=static_cast<std::int32_t>(in[26]);
            work.data=&alternate;
        }
    };
    resource::Services services{[&](texture_lifetime::List *list){event(1,static_cast<unsigned>(list-lists.data())+1);},
        [&](const TaskHandle &task){event(2,id(task));},[&](const TaskHandle &task){event(3,id(task));}};
    try {
        if(in[0]==4)resource::destroy_mode_task(static_cast<title_menu::MenuMode>(in[18]),menus,services);
        else if(in[0]==5)resource::destroy_sequence(work,demo,menus,state,services);
        else resource::destroy_dialog(menus,services);
    } catch(const Stop &){out[0]=1;}
    catch(const resource::Error &error){out[0]=2+static_cast<unsigned>(error.boundary());}
    out[1]=textures.loaded[0];out[2]=textures.loaded[1];out[3]=static_cast<std::uint32_t>(textures.ready);
    out[4]=static_cast<std::uint32_t>(demo);out[5]=id(menus.dialog_task);out[6]=calls;
    for(unsigned i=1;i<14;++i)out[7+i]=id(*slots[i]);
    copy_trace(events.str(),trace,capacity);return 1;
}
