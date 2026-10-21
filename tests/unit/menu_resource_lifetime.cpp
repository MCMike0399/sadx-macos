#include "game/menu_resource_lifetime.hpp"
#include <cassert>
#include <vector>

namespace sadx {
class TaskSourceProbe {
  public:
    static TaskHandle make() {return TaskHandle(new Task);}
};
}
int main() {
    using namespace sadx;
    namespace resource=menu_resource_lifetime;
    std::array<texture_lifetime::List,30> lists{};
    resource::TextureCatalog catalog{};
    for(auto &row:catalog)for(std::size_t i=0;i<30;++i)row[i]={std::nullopt,&lists[i]};
    title_menu::ModeTextureLists modes{};
    modes[5]={{18},{17},{1},{29}};modes[2]={{0},{1},{4}};
    title_menu::MenuTextureState loaded{{0xffffffffU,0xdeadbeefU},0};
    std::int32_t language=1;
    resource::State state{loaded,language,catalog,modes};
    title_menu::SequenceWork sequence{};sequence.mode=title_menu::MenuMode::FileSelect;sequence.old_mode=title_menu::MenuMode::Title;
    std::vector<unsigned> released;
    resource::Services services{[&](texture_lifetime::List *list) {
        const auto id=static_cast<unsigned>(list-lists.data());
        assert((loaded.loaded[0]&(1U<<id))==0);
        assert(loaded.ready==0);released.push_back(id);
    },{}, {}};
    resource::release_mode_textures(sequence,state,services);
    assert((released==std::vector<unsigned>{18,17,29}));
    assert((loaded.loaded[0]&(1U<<1))!=0 && loaded.loaded[1]==0xdeadbeefU && loaded.ready==1);
    loaded.loaded={0xffffffffU,0xdeadbeefU};loaded.ready=0;released.clear();
    services.release_texture=[&](texture_lifetime::List *list) {released.push_back(static_cast<unsigned>(list-lists.data()));if(released.size()==2)throw 1;};
    try {resource::release_mode_textures(sequence,state,services);assert(false);}catch(int){}
    assert((loaded.loaded[0]&(1U<<18))==0 && (loaded.loaded[0]&(1U<<17))==0);
    assert((loaded.loaded[0]&(1U<<29))!=0 && loaded.ready==0);
    loaded.loaded={0xffffffffU,0xdeadbeefU};services.release_texture=[](auto *){};
    resource::release_all(state,services);
    assert(loaded.loaded[0]==0xc0000000U && loaded.loaded[1]==0xdeadbeefU && loaded.ready==0);

    // Actual native texture-manager integration: unload title-only surfaces while
    // retaining the shared window and an immutable draw snapshot already submitted.
    std::vector<texture_lifetime::Memory> pool(8);
    std::int32_t cached=-1;texture_lifetime::Memory *active=nullptr;
    const texture_lifetime::List *current=nullptr;
    std::array<std::uint32_t,8> temporary{};
    texture_lifetime::State manager{pool,cached,active,current,temporary};
    const std::array<unsigned,4> ids{18,17,1,29};
    std::vector<texture_lifetime::Memory *> memories;
    for(auto id:ids) {
        auto image=std::make_shared<graphics::Texture>();image->width=1;image->height=1;image->bgra={1,2,3,4};
        auto *memory=texture_lifetime::acquire_prepared(texture_lifetime::prepare_argb_image(100+id,nullptr,1,1),image,manager);
        assert(memory && memory->references==1);lists[id].entries={memory};memories.push_back(memory);
    }
    auto submitted=memories[0]->surface->texture;
    loaded.loaded={(1U<<18)|(1U<<17)|(1U<<1)|(1U<<29),0};
    texture_lifetime::Services device{[] {}};
    services.release_texture=[&](auto *list) {texture_lifetime::release_texture_list(list,manager,device);};
    resource::release_mode_textures(sequence,state,services);
    assert(!memories[0]->surface && !memories[1]->surface && !memories[3]->surface);
    assert(memories[2]->surface && memories[2]->references==1);
    assert(submitted->bgra[0]==1); // independent in-flight image retention

    title_menu::State menus{};
    const auto character=TaskSourceProbe::make(),puti=TaskSourceProbe::make(),dialog=TaskSourceProbe::make();
    menus.factories.character_select_task=character;menus.factories.puti_task=puti;menus.dialog_task=dialog;
    unsigned destroys=0,frees=0;
    services.destroy_task=[&](const TaskHandle &task) {assert(task==character || task==dialog);++destroys;};
    services.free_task=[&](const TaskHandle &task) {assert(task==puti);++frees;};
    resource::destroy_mode_task(title_menu::MenuMode::CharacterSelect,menus,services);
    assert(menus.factories.character_select_task==character && destroys==1);
    resource::destroy_mode_task(title_menu::MenuMode::Puti,menus,services);
    assert(menus.factories.puti_task==puti && frees==1);
    resource::destroy_dialog(menus,services);
    assert(!menus.dialog_task && destroys==2);
    title_menu::SequenceData initial{title_menu::MenuMode::CharacterSelect,{},{}},replacement{title_menu::MenuMode::Puti,{},{}};
    sequence.data=&initial;loaded.loaded={1,0};std::int32_t demo=7;
    services.release_texture=[&](auto *) {assert(demo==0);sequence.data=&replacement;};
    resource::destroy_sequence(sequence,demo,menus,state,services);
    assert(frees==2 && destroys==2); // data pointer reloaded after resource callbacks
}
