#include "game/model_dlls.hpp"
namespace sadx {
std::optional<std::uint32_t> ModelDllSet::u32(std::uint32_t va)const{
    for(const auto& image:images)if(image&&va>=image->image_base()&&va-image->image_base()<image->size_of_image())return image->u32(va);
    return std::nullopt;
}
ModelDllSet map_model_dlls(const std::array<std::shared_ptr<const ExecutableImage>,7>& preferred,std::uint32_t first_base,
    const std::array<std::uint32_t,7>& handles,std::string* error){
    ModelDllSet set;set.handles=handles;std::uint32_t base=first_base;
    for(std::size_t i=0;i<preferred.size();++i){
        if(!preferred[i])continue;
        auto mapped=preferred[i]->rebased(base,error);if(!mapped)return {};
        const auto size=mapped->size_of_image();set.images[i]=std::make_shared<const ExecutableImage>(std::move(*mapped));
        base=static_cast<std::uint32_t>((base+size+0xffffu)&~0xffffu);
    }
    return set;
}
std::optional<std::uint32_t> ModelDllBindings::value_at(std::uint32_t global)const{
    for(std::size_t i=0;i<kModelDllBindings.size();++i)if(kModelDllBindings[i].global==global)return values[i];
    return std::nullopt;
}
ModelDllResult bind_model_dlls_401ac0(const ModelDllSet& set,ModelDllBindings& b){
    ModelDllResult r;
    for(std::uint32_t slot=0;slot<7;++slot){
        const auto& image=set.images[slot];
        b.handles_at_3ab9170.write(0x3ab9170+4*slot,image?set.handles[slot]:0,4);
        if(!image)continue; // the slot's exports are skipped
        for(std::size_t i=0;i<kModelDllBindings.size();++i){
            const auto& binding=kModelDllBindings[i];if(binding.slot!=slot)continue;
            const auto e=image->export_named(binding.name);
            if(e&&e->forwarder){r.failure=ModelDllFailure::forwarded_export;r.export_name=binding.name;return r;}
            b.values[i]=e?e->va:0;
        }
    }
    return r;
}
} // namespace sadx
