#include "runtime/game_runtime.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
// Invariants of the native composition that hold as more owners are ported:
// stop policy halts exactly at the first owner the survey reports; a survey
// completes startup and the requested frames without a non-owner failure.
namespace {
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
std::shared_ptr<const sadx::ExecutableImage> load(const char* path){std::ifstream f(path,std::ios::binary);auto image=sadx::ExecutableImage::parse({std::istreambuf_iterator<char>(f),{}});
    require(image.has_value(),"PE image");return std::make_shared<const sadx::ExecutableImage>(std::move(*image));}
}
int main(int argc,char** argv){try{
    require(argc==3,"usage: game-runtime-test SONIC.EXE INPUT_MOD.DLL");
    const auto exe=load(argv[1]),dll=load(argv[2]);
    require(exe->u32(0x7dce58)==0x42720000u&&exe->byte(0x3abdc7c)==0&&!exe->byte(0x3d14000+0x2000),"load image: .rdata 60.5f at 7dce58, zero-fill .data, outside the image");
    sadx::NativeGameConfig config;config.executable=exe;config.input_mod_dll=dll;
    const auto f=[&](std::uint32_t va){return sadx::Sse32{*dll->u32(va)};};
    config.input_constants={{f(0x1000bb6c),f(0x1000bbb0),f(0x1000bba0),f(0x1000bba4),f(0x1000bb7c)},f(0x1000bba8),f(0x1000bb78)};
    for(auto& s:config.pad_settings)s={false,7849,8689,true,false,30};config.pad_settings[0].allow_keyboard=true;
    std::int64_t now=0;config.clock={[]{return std::optional<std::int64_t>{10'000'000};},[&]{now+=200'000;return std::optional<std::int64_t>{now};}};
    config.policy=sadx::OwnerPolicy::survey_unported_as_noop;config.max_frames=3;
    const auto survey=sadx::NativeGame(config).run();
    require(survey.failure.empty()&&survey.startup_completed&&survey.frames_completed==3&&!survey.unported.empty(),"survey completes startup and three frames");
    config.policy=sadx::OwnerPolicy::stop_at_unported;
    sadx::NativeGame game(config);const auto stopped=game.run();
    require(stopped.stopped_at&&stopped.stopped_at->callee==survey.unported.front().callee&&stopped.failure.empty(),"stop policy halts at the first surveyed owner");
    // 794ec0 then 40bf40>411af0>4119b0's update: zeroed fog table, colour 0,
    // linear table fog with the 65534..65535 range, fog switched off.
    const auto& rs=game.d3d8().render_states;const auto state=[&](std::uint32_t s){const auto i=rs.find(s);return i==rs.end()?0xdeadbeefU:i->second;};
    require(state(28)==0&&state(34)==0&&state(35)==3&&state(36)==0x477ffe00&&state(37)==0x477fff00,"fog init and the reset fog update reach the D3D8 state owner");
    std::cout<<"Game runtime: survey ran startup + 3 frames ("<<survey.unported.size()<<" unported owners); native execution stops at "<<stopped.stopped_at->path<<".\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
