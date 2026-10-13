#include "game/model_dlls.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
// Real model DLLs: export lookup, relocated mapping, a missing slot, and the
// bound Sonic action table read through the mapped images.
namespace {
using namespace sadx;
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
std::shared_ptr<const ExecutableImage> load(const std::string& path){std::ifstream f(path,std::ios::binary);require(bool(f),"model DLL file");
    auto image=ExecutableImage::parse({std::istreambuf_iterator<char>(f),{}});require(image.has_value(),"PE image");return std::make_shared<const ExecutableImage>(std::move(*image));}
}
int main(int argc,char** argv){try{
    require(argc==2,"usage: model-dlls-test SYSTEM_DIRECTORY");const std::string dir=argv[1];
    std::array<std::shared_ptr<const ExecutableImage>,7> preferred{load(dir+"/CHRMODELS_orig.dll"),load(dir+"/ADV00MODELS.DLL"),load(dir+"/ADV01MODELS.DLL"),
        load(dir+"/ADV01CMODELS.DLL"),load(dir+"/ADV02MODELS.DLL"),load(dir+"/ADV03MODELS.DLL"),nullptr};
    const auto sonic=preferred[0]->export_named("___SONIC_ACTIONS");require(sonic&&!sonic->forwarder&&!preferred[0]->export_named("___sonic_actions"),"exact export name");
    std::string error;const auto set=map_model_dlls(preferred,0x20000000,{0x10000000,0,0,0,0,0,0},&error);require(bool(set.images[0])&&!set.images[6],error.c_str());
    for(std::size_t i=1;i<6;++i)require(set.images[i]->image_base()>=set.images[i-1]->image_base()+set.images[i-1]->size_of_image(),"distinct ascending bases");
    // A relocated word: the mapped value moves by the base delta.
    const auto moved=set.images[0]->export_named("___SONIC_ACTIONS");require(moved&&moved->va==sonic->va-0x10000000+0x20000000,"export moves with the base");
    const auto before=preferred[0]->u32(sonic->va),after=set.u32(moved->va);require(before&&after&&*after==*before-0x10000000+0x20000000,"HIGHLOW relocation applied");
    ModelDllBindings bindings;require(bool(bind_model_dlls_401ac0(set,bindings)),"bind");
    require(bindings.value_at(0x38f6e7c)==moved->va&&bindings.handles_at_3ab9170.read(0x3ab9170,4)==0x10000000u&&bindings.handles_at_3ab9170.read(0x3ab9188,4)==0u,"401ac0 bindings and handles");
    require(!bindings.values[84],"missing bosschaos0 slot leaves its exports unbound");
    const auto field=set.u32(*bindings.value_at(0x38f6e7c)+0x190);require(field&&*field>=0x20000000&&*field<0x20000000+set.images[0]->size_of_image(),"action table field points into the mapped DLL");
    std::cout<<"Model DLLs: exact exports, relocated ascending mapping, 85 bindings, missing slot and in-image action pointers passed.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
