// M1 step 4c: sonic.exe from its real entry point (643f96: the CRT startup, its static initializers including
// 401ac0's mod loader and mods, then WinMain) runs in the guest up to the first IDirect3DDevice8::Present, with
// the Win32/D3D8/DirectInput/DirectSound imports HLE'd (src/hle) and the D3D8 state recorded in a D3D8StateOwner.
// Checked: the run reaches Present; the windows the loader creates (borderless wrapper + 640x480 game child);
// the D3D8 device came from CreateDevice with the profile's 640x480 back buffer; every Input/Speedrun and
// WinMain-time loader patch is written except the 8 HUD-scaling sites applied after the first frame; nothing
// outside the inventory's active ranges is written; and a second boot is identical (call log, D3D8 state).
//   guest-boot-test GAME_DIR WINDOWS_SYSTEM_DIR
#include "guest/override_table.hpp"
#include "hle/hle_process.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
#include <sstream>
namespace {
using namespace sadx;
void require(bool ok,const std::string& why){if(!ok)throw std::runtime_error(why);}
struct Boot {std::unique_ptr<HleProcess> process;GuestCallResult result;};
Boot boot(const std::string& dir,const std::string& system,const std::string& overlay,std::map<std::string,std::size_t>* fired=nullptr){
    std::ifstream f(dir+"/sonic.exe",std::ios::binary);require(bool(f),"cannot read "+dir+"/sonic.exe");
    std::string error;const auto image=ExecutableImage::parse({std::istreambuf_iterator<char>(f),{}},&error);require(bool(image),error);
    std::filesystem::remove_all(overlay);
    Boot b;b.process=HleProcess::create(dir,overlay,&error);require(b.process!=nullptr,error);
    b.process->use_real_cpp_runtime(system);
    require(b.process->map_main(*image,&error),"map sonic.exe: "+error);
    b.process->on_present=[](X86Guest& g){g.stop("first Present");};
    if(fired)for(auto entry:frame_loop_overrides(*image)){ // the override table, counting how often each fires
        auto bridge=entry.bridge;const auto name=entry.name;
        require(b.process->guest().add_override(entry.address,[bridge,name,fired](X86Guest& g){++(*fired)[name];return bridge(g);},&error),error);}
    b.result=b.process->guest().call(0x643f96,{});
    return b;
}
std::string hex_bytes(const X86Guest& g,std::uint32_t at,std::uint32_t n){
    std::ostringstream o;for(std::uint32_t i=0;i<n;++i){std::uint8_t v=0;g.read(at+i,&v,1);char h[3];std::snprintf(h,3,"%02x",v);o<<h;}return o.str();}
// (va, size, original bytes, active, owner) per patch entry of tools/gate/profile_patches.json.
struct Entry {std::uint32_t va,size;std::string original;bool active;};
std::vector<Entry> entries(const std::string& path){
    std::ifstream f(path);std::string all((std::istreambuf_iterator<char>(f)),{});std::vector<Entry> out;std::size_t at=0;
    const auto field=[&](const std::string& key,std::size_t from,std::size_t limit)->std::string{
        const auto k=all.find("\""+key+"\"",from);if(k==std::string::npos||k>limit)return "";
        auto v=all.find(':',k)+1;while(all[v]==' ')++v;
        if(all[v]=='"'){const auto e=all.find('"',v+1);return all.substr(v+1,e-v-1);}
        auto e=v;while(e<all.size()&&all[e]!=','&&all[e]!='}'&&all[e]!='\n')++e;return all.substr(v,e-v);};
    while((at=all.find("\"va\":",at))!=std::string::npos){
        const auto next=all.find("\"va\":",at+5);const auto end=next==std::string::npos?all.size():next;
        out.push_back({static_cast<std::uint32_t>(std::stoul(field("va",at,end),nullptr,16)),static_cast<std::uint32_t>(std::stoul(field("size",at,end))),
                       field("original_bytes",at,end),field("active",at,end)=="true"});
        at=end;}
    return out;
}
}
int main(int argc,char** argv){try{
    require(argc==3,"usage: guest-boot-test GAME_DIR WINDOWS_SYSTEM_DIR");
    auto first=boot(argv[1],argv[2],"build/guest-boot-overlay");
    auto& p=*first.process;
    require(!first.result&&first.result.error=="first Present","the boot reaches the first Present, got: "+(first.result?std::string("returned"):first.result.error));
    require(p.d3d8.presents==1&&p.d3d8.clears==1,"one Clear and the Present");
    require(p.d3d8_presentation.size()==13&&p.d3d8_presentation[0]==640&&p.d3d8_presentation[1]==480,"CreateDevice with the profile's 640x480 back buffer");
    // The loader's borderless window: a top-level wrapper with a 640x480 client and the visible game child.
    const HleWindow* wrapper=nullptr;const HleWindow* game=nullptr;
    for(const auto& [h,w]:p.windows.windows){if(w.text=="SonicAdventureDXPC"&&!w.parent)wrapper=&w;if(w.text=="SonicAdventureDXPC"&&w.parent)game=&w;}
    require(wrapper&&game&&game->parent==wrapper->handle&&wrapper->visible&&game->visible,"wrapper and game windows exist and are visible");
    require(game->width==640&&game->height==480&&wrapper->width==646&&wrapper->height==509,"640x480 client in a 646x509 captioned wrapper (fixed frame 3, caption 23)");
    // Patch sites.
    const std::set<std::uint32_t> after_first_frame={0x628951,0x62895b,0x7261cf,0x726211,0x727513,0x727546,0x72767b,0x72773b};
    std::map<std::uint32_t,std::pair<bool,bool>> site; // va -> (active, written)
    std::map<std::uint32_t,std::uint32_t> active_size;
    for(const auto& e:entries("tools/gate/profile_patches.json")){
        auto& s=site[e.va];if(e.active){s.first=true;active_size[e.va]=std::max(active_size[e.va],e.size);}
        if(e.original!="null"&&!e.original.empty()&&hex_bytes(p.guest(),e.va,e.size)!=e.original)s.second=true;}
    std::set<std::uint32_t> pending,unexpected;std::size_t active=0;
    for(const auto& [va,s]:site){
        if(s.first){++active;if(!s.second)pending.insert(va);continue;}
        if(!s.second)continue;bool covered=false;for(const auto& [other,size]:active_size)if(other<va&&va<other+size)covered=true;
        if(!covered)unexpected.insert(va);}
    std::ostringstream still;for(const auto va:pending)still<<std::hex<<va<<" ";
    require(pending==after_first_frame,"active sites still original at the first Present: "+still.str());
    require(unexpected.empty(),"a site outside the inventory's active ranges was written");
    // Determinism: a second boot issues the same HLE calls with the same results and records the same D3D8 state.
    auto second=boot(argv[1],argv[2],"build/guest-boot-overlay");
    auto& q=*second.process;
    require(q.hle().log().size()==p.hle().log().size(),"the same number of HLE calls");
    for(std::size_t i=0;i<p.hle().log().size();++i){const auto& a=p.hle().log()[i];const auto& b=q.hle().log()[i];
        require(a.dll==b.dll&&a.symbol==b.symbol&&a.arguments==b.arguments&&a.result==b.result&&a.return_address==b.return_address,"HLE call "+std::to_string(i)+" differs between boots ("+a.symbol+")");}
    require(q.d3d8.render_states==p.d3d8.render_states&&q.d3d8.stage0_states==p.d3d8.stage0_states&&q.d3d8.presents==p.d3d8.presents,"the same D3D8 state");
    // In-game override equivalence: a boot with the override table installed issues the same HLE calls and leaves
    // the same bytes in every override's write ranges at the first Present.
    std::map<std::string,std::size_t> fired;
    auto third=boot(argv[1],argv[2],"build/guest-boot-overlay",&fired);auto& o=*third.process;
    require(!third.result&&third.result.error=="first Present","the boot with overrides reaches the first Present: "+third.result.error);
    require(o.hle().log().size()==p.hle().log().size(),"the same number of HLE calls with the overrides installed");
    for(std::size_t i=0;i<p.hle().log().size();++i){const auto& a=p.hle().log()[i];const auto& b=o.hle().log()[i];
        require(a.symbol==b.symbol&&a.arguments==b.arguments&&a.result==b.result,"HLE call "+std::to_string(i)+" differs with the overrides ("+a.symbol+")");}
    std::ifstream exe(std::string(argv[1])+"/sonic.exe",std::ios::binary);std::string why;
    const auto image=ExecutableImage::parse({std::istreambuf_iterator<char>(exe),{}},&why);
    std::ostringstream counts;
    for(const auto& entry:frame_loop_overrides(*image)){
        for(const auto& [at,size]:entry.writes)require(hex_bytes(p.guest(),at,size)==hex_bytes(o.guest(),at,size),entry.name+": different bytes at the first Present");
        counts<<entry.name<<" x"<<fired[entry.name]<<" ";}
    std::cout<<"Guest boot: entry point to the first Present in the guest ("<<p.hle().log().size()<<" HLE calls, identical on a second boot); windows "
             <<p.windows.windows.size()<<" (borderless 646x509 wrapper, 640x480 game); "<<(active-pending.size())<<" of "<<active
             <<" active patch sites written, the 8 HUD-scaling ones pending; "<<p.assumptions.size()<<" HLE assumptions reported; with the override table installed the boot is identical (fired: "<<counts.str()<<")\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
