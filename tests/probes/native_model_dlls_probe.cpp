#include "game/model_dlls.hpp"
#include <fstream>
#include <iterator>
#include <string>
// Native 401ac0 for tools/verify_original_model_dlls.py: slots present per
// `mask` (bit i), images from `system`; out: 7 handles then 85 values.
extern "C" int sadx_native_model_dlls(const char* system,std::uint32_t mask,std::uint32_t* out){
    static const char* files[7]={"CHRMODELS_orig.dll","ADV00MODELS.DLL","ADV01MODELS.DLL","ADV01CMODELS.DLL","ADV02MODELS.DLL","ADV03MODELS.DLL","BOSSCHAOS0MODELS.DLL"};
    std::array<std::shared_ptr<const sadx::ExecutableImage>,7> preferred{};
    for(int i=0;i<7;++i){if(!(mask>>i&1))continue;std::ifstream f(std::string(system)+"/"+files[i],std::ios::binary);if(!f)continue;
        auto image=sadx::ExecutableImage::parse({std::istreambuf_iterator<char>(f),{}});if(!image)return 0;preferred[i]=std::make_shared<const sadx::ExecutableImage>(std::move(*image));}
    std::array<std::uint32_t,7> handles{0x10000000};
    auto set=sadx::map_model_dlls(preferred,0x20000000,handles);
    for(int i=1;i<7;++i)if(set.images[i])set.handles[i]=set.images[i]->image_base();
    sadx::ModelDllBindings b;if(!sadx::bind_model_dlls_401ac0(set,b))return 0;
    for(int i=0;i<7;++i)out[i]=static_cast<std::uint32_t>(b.handles_at_3ab9170.read(0x3ab9170+4*i,4).value_or(0xdeadbeef));
    for(std::size_t i=0;i<b.values.size();++i)out[7+i]=b.values[i].value_or(0xdeadbeef);
    return 1;
}
