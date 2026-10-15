#include "game/game_mode_leaves.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
// Native 40c090 leaves with the byte layout of tools/verify_original_game_mode_leaves.py.
// in: [0] callee, [1] argument, [2] fail-at nested call (0: all complete), then
// `bytes` (the field table below, little-endian) of length kBytes.
namespace {
using namespace sadx;
struct Field {std::uint32_t address;std::size_t width;};
constexpr Field kScalars[]={
 {0x3b0ef34,1},{0x3b0ef35,1},{0x3b0f128,1},{0x3b0ef48,1},{0x3abe134,1},{0x3b0f0e0,1},{0x3b290d8,1},{0x3b290e0,1},{0x3b290e8,1},{0x3b2a2f8,1},{0x3b2a2f9,1},{0x3b2a2fb,1},{0x3b2c565,1},{0x3b2c574,1},
 {0x3b22dc0,2},{0x3b0f0e4,2},{0x3b2a300,2},{0x3b22e1c,2},{0x3b18db8,2},
 {0x3b0f13c,4},{0x3b0f108,4},{0x3b0ef44,4},{0x3b290dc,4},{0x3b2a2f0,4},{0x3b2a2f4,4},{0x3b2a2fc,4},{0x3b2b358,4},{0x3b2c47c,4},{0x3b2919c,4},{0x3b0f0f8,4},{0x3b0f0fc,4},{0x3b0f100,4},{0x3b0f10c,4},{0x3b0f0f4,4},{0x3b2c560,4},{0x914788,4},{0x3b18244,4},{0x3b22e20,4},{0x3d08420,4},
 // borrowed: mode 3b290e4/3b2a2fa, level pointer 3b0ef4c, timing 3b11178/3b2a2e4, cutscene 3b2a2e8, event task 3b2c578, seed 2bcbdf8
 {0x3b290e4,4},{0x3b2a2fa,1},{0x3b0ef4c,4},{0x3b11178,4},{0x3b2a2e4,4},{0x3b2a2e8,4},{0x3b2c578,4},{0x2bcbdf8,4},
 // model tables (borrowed 401ac0 bindings), then the 422680 motion words (owned)
 {0x38f6e7c,4},{0x3aad104,4},{0x3a7cf28,4},
 {0x3b22de4,2},{0x3b22dcc,2},
 {0x3c566c0,4},{0x3c566d0,4},{0x3c559c0,4},{0x3c559d0,4},{0x3c559e0,4},{0x3c559f0,4},{0x3c55a00,4},{0x3c53510,4},{0x3c53520,4},
 // 42c4f0: fade colour (owned), screen scale (borrowed)
 {0x3b29d5c,4},{0x8928c0,4},{0x8928c4,4},
 // 42c730: 42e470 word, 3b29d6c/3b29d68 (owned), the 780c00 clear colours (borrowed)
 {0x3b2c420,4},{0x3b29d6c,4},{0x3b29d68,4},{0x3ce7174,4},{0x3ce7178,4},{0x3ce717c,4},{0x3ce7180,4}};
constexpr std::size_t kScalarBytes=[]{std::size_t n=0;for(const auto& f:kScalars)n+=f.width;return n;}();
constexpr std::size_t kBytes=kScalarBytes+0xb64+0x3c+0x3638+8+0x20+7+0x78;
template<class T>void put(T& o,const std::uint8_t* p,std::size_t w){std::uint64_t v=0;std::memcpy(&v,p,w);o=static_cast<typename T::value_type>(v);}
template<class T>void get(const T& o,std::uint8_t* p,std::size_t w){const std::uint64_t v=o?static_cast<std::uint64_t>(*o):0xdeadbeefdeadbeefULL;std::memcpy(p,&v,w);}
}
namespace {ModelDllSet g_models;ModelDllBindings g_bindings;bool g_loaded=false;}
extern "C" std::size_t sadx_native_game_mode_leaves_bytes(){return kBytes;}
extern "C" int sadx_native_game_mode_leaves_models(const char* system){
    const char* files[7]={"CHRMODELS_orig.dll","ADV00MODELS.DLL","ADV01MODELS.DLL","ADV01CMODELS.DLL","ADV02MODELS.DLL","ADV03MODELS.DLL","BOSSCHAOS0MODELS.DLL"};
    std::array<std::shared_ptr<const ExecutableImage>,7> preferred{};
    for(int i=0;i<7;++i){std::ifstream f(std::string(system)+"/"+files[i],std::ios::binary);if(!f)continue;auto image=ExecutableImage::parse({std::istreambuf_iterator<char>(f),{}});if(!image)return 0;preferred[i]=std::make_shared<const ExecutableImage>(std::move(*image));}
    std::array<std::uint32_t,7> handles{0x10000000};g_models=map_model_dlls(preferred,0x20000000,handles);
    for(int i=1;i<7;++i)if(g_models.images[i])g_models.handles[i]=g_models.images[i]->image_base();
    if(!bind_model_dlls_401ac0(g_models,g_bindings))return 0;g_loaded=true;
    for(int i=0;i<7;++i)if(g_models.images[i]&&g_models.images[i]->image_base()==0)return 0;
    return static_cast<int>(g_models.images[0]?g_models.images[0]->image_base()>>16:0);
}
extern "C" int sadx_native_game_mode_leaves(const std::uint32_t* in,const std::uint8_t* bytes,std::size_t count,std::uint8_t* out,std::uint32_t* result){
    if(!in||!bytes||count!=kBytes||!out||!result)return 0;
    GameModeState mode;GameLogicTimingState timing;std::optional<std::int32_t> cutscene;std::optional<TaskHandle> event;CrtRandState rng;std::optional<std::uint16_t> game_mode,stage;
    GameModeLeafState s(mode,timing,cutscene,event,rng,game_mode,stage);
    X87Context fp(static_cast<std::uint16_t>(in[4]));s.fp=&fp;std::optional<float> screen_w,screen_h;s.screen_width_at_8928c0=&screen_w;s.screen_height_at_8928c4=&screen_h;
    const auto as_float=[](std::uint32_t b){float v;std::memcpy(&v,&b,4);return v;};const auto as_bits=[](const std::optional<float>& v){std::uint32_t b=0xdeadbeef;if(v)std::memcpy(&b,&*v,4);return b;};
    std::array<std::uint32_t,7> rect{};std::array<std::optional<std::uint32_t>,4> clear{};s.clear_colors_at_3ce7174=&clear;
    TaskScheduler scheduler; // a live task stands for a nonzero 3b2c578
    ModelDllBindings bindings=g_bindings;
    std::optional<std::uint32_t>* motions[]={&s.motion_at_3c566c0,&s.motion_at_3c566d0,&s.motion_at_3c559c0,&s.motion_at_3c559d0,&s.motion_at_3c559e0,&s.motion_at_3c559f0,&s.motion_at_3c55a00,&s.motion_at_3c53510,&s.motion_at_3c53520};
    const auto table_slot=[&](std::uint32_t global)->std::optional<std::uint32_t>&{for(std::size_t i=0;i<kModelDllBindings.size();++i)if(kModelDllBindings[i].global==global)return bindings.values[i];return bindings.values[0];};
    const std::uint32_t tables[3]={0x38f6e7c,0x3aad104,0x3a7cf28};
    std::optional<std::uint32_t>* scalars32[]={&s.word_at_3b0f13c,&s.word_at_3b0f108,&s.word_at_3b0ef44,&s.word_at_3b290dc,&s.word_at_3b2a2f0,&s.word_at_3b2a2f4,&s.word_at_3b2a2fc,&s.word_at_3b2b358,&s.word_at_3b2c47c,&s.word_at_3b2919c,
        &s.word_at_3b0f0f8,&s.word_at_3b0f0fc,&s.word_at_3b0f100,&s.word_at_3b0f10c,&s.word_at_3b0f0f4,&s.word_at_3b2c560,&s.word_at_914788,&s.word_at_3b18244,&s.word_at_3b22e20,&s.callback_at_3d08420};
    std::optional<std::uint8_t>* scalars8[]={&s.byte_at_3b0ef34,&s.byte_at_3b0ef35,&s.byte_at_3b0f128,&s.byte_at_3b0ef48,&s.byte_at_3abe134,&s.byte_at_3b0f0e0,&s.byte_at_3b290d8,&s.byte_at_3b290e0,&s.byte_at_3b290e8,&s.byte_at_3b2a2f8,&s.byte_at_3b2a2f9,&s.byte_at_3b2a2fb,&s.byte_at_3b2c565,&s.byte_at_3b2c574};
    std::optional<std::uint16_t>* scalars16[]={&s.character_at_3b22dc0,&s.half_at_3b0f0e4,&s.half_at_3b2a300,&s.half_at_3b22e1c,&s.half_at_3b18db8};
    const auto each=[&](auto&& f8,auto&& f16,auto&& f32,auto&& borrowed){
        std::size_t o=0,i8=0,i16=0,i32=0,ib=0;
        for(const auto& f:kScalars){
            if(&f-kScalars<14)f8(*scalars8[i8++],o);else if(&f-kScalars<19)f16(*scalars16[i16++],o);else if(&f-kScalars<39)f32(*scalars32[i32++],o);else borrowed(ib++,o);
            o+=f.width;}};
    each([&](auto& v,std::size_t o){put(v,bytes+o,1);},[&](auto& v,std::size_t o){put(v,bytes+o,2);},[&](auto& v,std::size_t o){put(v,bytes+o,4);},
        [&](std::size_t i,std::size_t o){std::uint32_t v=0;std::memcpy(&v,bytes+o,kScalars[39+i].width);
            switch(i){case 0:mode.word_at_3b290e4=v;break;case 1:mode.byte_at_3b2a2fa=static_cast<std::uint8_t>(v);break;case 2:if(v)mode.level_word_via_3b0ef4c=0x1234;break;
            case 3:timing.logic_increment=static_cast<std::int32_t>(v);break;case 4:timing.word_at_3b2a2e4=static_cast<std::int32_t>(v);break;case 5:cutscene=static_cast<std::int32_t>(v);break;
            case 6:event=v?scheduler.append_task(0,&TaskScheduler::source_no_op).task:TaskHandle{};break;case 7:rng.seed_at_2bcbdf8=v;break;
            case 11:game_mode=static_cast<std::uint16_t>(v);break;case 12:stage=static_cast<std::uint16_t>(v);break;
            case 22:s.color_at_3b29d5c=v;break;case 23:screen_w=as_float(v);break;case 24:screen_h=as_float(v);break;
            case 25:s.word_at_3b2c420=v;break;case 26:s.state_at_3b29d6c=v;break;case 27:s.word_at_3b29d68=v;break;case 28:case 29:case 30:case 31:clear[i-28]=v;break;
            default:if(i<11)table_slot(tables[i-8])=v;else *motions[i-13]=v;break;}});
    auto blocks=[&](auto&& f){std::size_t o=kScalarBytes;f(s.block_at_3b18250,o);o+=0xb64;f(s.block_at_3b0ef9c,o);o+=0x3c;f(s.block_at_3c4ae08,o);o+=0x3638;f(s.block_at_3b290c4,o);o+=8;f(s.block_at_90ed54,o);o+=0x20;f(s.block_at_3b2b5f9,o);o+=7;f(s.block_at_3b2c5e8,o);};
    blocks([&](auto& b,std::size_t o){for(std::size_t i=0;i<b.size;++i)b.bytes[i]=bytes[o+i];});
    if(in[3]&&g_loaded){s.model_bindings=&bindings;s.model_dlls=&g_models;}
    std::uint32_t calls=0;bool release_walk=false;
    const auto r=run_game_mode_leaf(in[0],in[1],s,[&](const GameModeLeafCall& c){++calls;release_walk|=c.callee==0x403290;if(c.callee==0x4073b0)for(int k=0;k<7;++k)rect[k]=c.arguments[k].value_or(0xdeadbeef);return GameLogicOwnerResult{in[2]&&calls==in[2]?GameLogicOwnerState::unavailable:GameLogicOwnerState::completed,c.callee,{}};});
    each([&](auto& v,std::size_t o){get(v,out+o,1);},[&](auto& v,std::size_t o){get(v,out+o,2);},[&](auto& v,std::size_t o){get(v,out+o,4);},
        [&](std::size_t i,std::size_t o){std::uint32_t v=0xdeadbeef;
            switch(i){case 0:v=mode.word_at_3b290e4.value_or(0xdeadbeef);break;case 1:v=mode.byte_at_3b2a2fa.value_or(0xef);break;case 2:v=mode.level_word_via_3b0ef4c?1U:0U;break;
            case 3:v=static_cast<std::uint32_t>(timing.logic_increment.value_or(0));break;case 4:v=static_cast<std::uint32_t>(timing.word_at_3b2a2e4.value_or(0));break;case 5:v=static_cast<std::uint32_t>(cutscene.value_or(0));break;
            case 6:v=event&&*event?1U:0U;break;case 7:v=rng.seed_at_2bcbdf8.value_or(0xdeadbeef);break;
            case 11:v=game_mode.value_or(0xbeef);break;case 12:v=stage.value_or(0xbeef);break;
            case 22:v=s.color_at_3b29d5c.value_or(0xdeadbeef);break;case 23:v=as_bits(screen_w);break;case 24:v=as_bits(screen_h);break;
            case 25:v=s.word_at_3b2c420.value_or(0xdeadbeef);break;case 26:v=s.state_at_3b29d6c.value_or(0xdeadbeef);break;case 27:v=s.word_at_3b29d68.value_or(0xdeadbeef);break;
            case 28:case 29:case 30:case 31:v=clear[i-28].value_or(0xdeadbeef);break;
            default:v=i<11?table_slot(tables[i-8]).value_or(0xdeadbeef):motions[i-13]->value_or(0xdeadbeef);break;}
            std::memcpy(out+o,&v,kScalars[39+i].width);});
    blocks([&](auto& b,std::size_t o){for(std::size_t i=0;i<b.size;++i)out[o+i]=b.bytes[i].value_or(0xef);});
    result[0]=r.outcome==GameModeLeafOutcome::completed;result[1]=calls;result[2]=r.eax.value_or(0xdeadbeef);result[3]=r.eax.has_value();result[4]=r.source_boundary;
    for(int k=0;k<7;++k)result[8+k]=rect[k];result[15]=fp.exception_flags()&0x3f;
    result[5]=release_walk||r.outcome==GameModeLeafOutcome::nonterminating; // one native owner for the source's 403290 loop; a source loop that never exits
    return 1;
}
