#include "game/message_texture.hpp"
#include <algorithm>
namespace {
struct Calls final : sadx::message_texture::Calls {
    const std::uint32_t *input;
    std::uint32_t *output;
    sadx::menu_caption::Context &context;
    std::uint8_t &policy;
    sadx::texture_lifetime::Memory resident{};
    Calls(const std::uint32_t *i, std::uint32_t *o, sadx::menu_caption::Context &c,
          std::uint8_t &p) : input(i), output(o), context(c), policy(p) {}
    std::int32_t load_image(sadx::texture_lifetime::List &list,
                           const sadx::message_texture::Image &image) override {
        output[8]=1;output[9]=policy;output[10]=context.texture_names_attached;
        output[11]=static_cast<std::uint32_t>(list.entries.size());
        output[12]=image.pixels?1:0;output[13]=image.type<<16;
        output[14]=image.width;output[15]=image.height;
        output[16]=image.attributes;output[17]=image.global_index;
        context.flags=input[7];policy=static_cast<std::uint8_t>(input[8]);
        if(input[9])list.entries[0]=&resident;
        return static_cast<std::int32_t>(input[6]);
    }
    void reload_image(std::uint32_t id,const std::uint16_t *pixels,
                      std::uint32_t attributes,std::uint32_t palette) override {
        output[8]=2;output[9]=policy;output[10]=id;output[11]=pixels?1:0;
        output[12]=attributes;output[13]=palette;
        context.flags=input[7];policy=static_cast<std::uint8_t>(input[8]);
    }
};
}
extern "C" void sadx_native_message_texture_probe(const std::uint32_t *input,std::uint32_t *out) {
    std::fill_n(out,32,0);sadx::menu_caption::Context context;std::uint16_t bitmap=0;
    context.bitmap=input[0]?&bitmap:nullptr;context.texture_names_attached=input[1]!=0;
    context.flags=input[2];context.bitmap_width=static_cast<std::uint16_t>(input[3]);
    context.bitmap_height=static_cast<std::uint16_t>(input[4]);context.global_index=input[5];
    context.textures.entries.resize(input[10],nullptr);
    auto policy=static_cast<std::uint8_t>(input[11]);Calls calls(input,out,context,policy);
    sadx::message_texture::upload(context,policy,calls);
    out[0]=context.flags;out[1]=context.texture_names_attached;out[2]=policy;
    out[3]=static_cast<std::uint32_t>(context.textures.entries.size());
    out[4]=!context.textures.entries.empty()&&context.textures.entries[0]==&calls.resident;
}
