// M1 step 5: host input in the guest, and the determinism test with an input script. The host's keyboard reaches
// sonic.exe as WM_KEYDOWN / WM_KEYUP posted to the focus window (HleProcess::host_key); the Input mod's window
// subclass maps them to the controller (its config.ini: Enter = Start). The script presses Enter for 5 frames at
// Present 450 (during the intro movie: 5130a0's skip check ends it) and at Present 800 (the title screen: the
// game opens "Select a Saved File", whose texture list ava_filesel_e (10d7c28 -> 3c5ee08) gets loaded).
// Checked: with the script the title (GameMode 12) comes before frame 500 and the file-select textures are loaded
// by frame 900 but not at frame 799; without input the movie is still playing at frame 500 (GameMode 8); two boots
// with the same script produce identical per-frame digests (D3D8 state, vertex bytes, HLE call count, virtual clock).
//   guest-input-test GAME_DIR WINDOWS_SYSTEM_DIR
#include "guest/fpu_shims.hpp"
#include "hle/hle_process.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
namespace {
using namespace sadx;
void require(bool ok,const std::string& why){if(!ok)throw std::runtime_error(why);}
struct Key {std::uint64_t frame;std::uint32_t vk,hold;};
struct Run {std::vector<std::uint64_t> digests;std::vector<std::uint32_t> modes;std::uint32_t filesel_799=0,filesel_last=0;std::string stop;};
Run run(const std::string& dir,const std::string& system,const std::vector<Key>& script,std::size_t frames){
    std::ifstream f(dir+"/sonic.exe",std::ios::binary);require(bool(f),"cannot read "+dir+"/sonic.exe");
    std::string error;const auto image=ExecutableImage::parse({std::istreambuf_iterator<char>(f),{}},&error);require(bool(image),error);
    std::filesystem::remove_all("build/guest-input-overlay");
    auto p=HleProcess::create(dir,"build/guest-input-overlay",&error);require(p!=nullptr,error);
    p->use_real_cpp_runtime(system);p->hle().set_logging(false);
    require(p->map_main(*image,&error),"map sonic.exe: "+error);
    FpuShimStats shims;require(install_fpu_shims(p->guest(),load_fpu_sites("reference/inventory/fpu-sites.json"),shims,&error),error);
    Run r;std::size_t polygons_seen=0;
    p->on_present=[&](X86Guest& g){
        auto& d=p->d3d8;std::uint64_t h=1469598103934665603ULL;
        const auto mix=[&h](const void* at,std::size_t n){const auto* b=static_cast<const std::uint8_t*>(at);for(std::size_t i=0;i<n;++i){h^=b[i];h*=1099511628211ULL;}};
        for(const auto& [k,v]:d.render_states){mix(&k,sizeof k);mix(&v,sizeof v);}
        for(const auto& [k,v]:d.stage0_states){mix(&k,sizeof k);mix(&v,sizeof v);}
        for(;polygons_seen<d.polygons.size();++polygons_seen)mix(d.polygons[polygons_seen].vertex_bytes.data(),d.polygons[polygons_seen].vertex_bytes.size());
        const auto calls=p->hle().calls();mix(&calls,sizeof calls);mix(&p->now_100ns,sizeof p->now_100ns);
        r.digests.push_back(h);r.modes.push_back(g.u32(0x3abdc7c).value_or(0xffffffffU));
        const auto frame=r.digests.size();
        if(frame==799)r.filesel_799=g.u32(0x3c5ee10).value_or(0);
        r.filesel_last=g.u32(0x3c5ee10).value_or(0); // ava_filesel_e texture 0's texaddr (0 until the texture list is loaded)
        for(const auto& k:script){if(frame==k.frame)p->host_key(k.vk,true);if(frame==k.frame+k.hold)p->host_key(k.vk,false);}
        if(frame>=frames)g.stop("frames done");};
    const auto result=p->guest().call(0x643f96,{},std::nullopt,std::nullopt,0x27f,0);
    r.stop=result?std::string("returned"):result.error;return r;
}
std::size_t first_frame_of(const Run& r,std::uint32_t mode){for(std::size_t i=0;i<r.modes.size();++i)if(r.modes[i]==mode)return i+1;return 0;}
}
int main(int argc,char** argv){try{
    require(argc==3,"usage: guest-input-test GAME_DIR WINDOWS_SYSTEM_DIR");
    const std::vector<Key> script={{450,0x0d,5},{800,0x0d,5}};
    const auto a=run(argv[1],argv[2],script,900);
    require(a.stop=="frames done",std::string("the scripted run stopped after ")+std::to_string(a.digests.size())+" frames: "+a.stop);
    const auto title=first_frame_of(a,12);
    require(title&&title<500,"Enter during the intro movie skips it (title at frame "+std::to_string(title)+")");
    require(a.filesel_799==0&&a.filesel_last!=0,"Enter on the title opens the file select (its texture list is loaded after frame 800, not before)");
    const auto b=run(argv[1],argv[2],script,900);
    require(b.stop=="frames done"&&b.digests==a.digests,"a second boot with the same input script produces different frame digests");
    const auto none=run(argv[1],argv[2],{},500);
    require(none.stop=="frames done"&&none.modes.back()==8,"without input the intro movie still plays at frame 500");
    std::size_t same=0;while(same<none.digests.size()&&none.digests[same]==a.digests[same])++same;
    std::cout<<"Guest input: Enter at frame 450 skips the intro movie (title at frame "<<title<<" instead of ~7168), Enter at frame 800 opens the file select; "
             <<"two boots with the script give identical digests over 900 frames; without input the movie still plays at 500 (digests equal up to frame "<<same<<")\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
