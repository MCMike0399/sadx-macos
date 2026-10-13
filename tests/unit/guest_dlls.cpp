// M1 step 4b (in progress): the mod loader and the mods load and initialise inside the guest. sonic.exe's
// static initializer 401ac0 (this = 3ab9170) runs from the original bytes; its LoadLibraryA("SYSTEM\chrmodels.dll")
// maps the pinned SADXModLoader (as CHRMODELS.dll), whose real DllMain patches 401ae1/401ae2 to call
// LoadChrmodels, which loads CHRMODELS_orig and the garden DLLs; the loader then reads its profile and the
// mods' mod.ini and loads sadx-input-mod and sadx-speedrun-mod. The MSVC C++ runtime runs as real code
// (reference/windows-system, see its PROVENANCE.md); every Win32/UCRT function below it is HLE.
//
// The run must end at an explicit stop naming the next unimplemented import (the current frontier is the
// window system SDL2 needs: USER32!RegisterClassW). Patch sites: every site the guest wrote is compared with
// tools/gate/profile_patches.json; sites written although the inventory marks every entry inactive must be
// exactly the known list below (empty: none so far), unless an active entry's range covers them. Entries whose
// original bytes are unknown (null: .bss) are not judged.
//   guest-dlls-test GAME_DIR WINDOWS_SYSTEM_DIR
#include "hle/hle_process.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <cstdlib>
#include <set>
#include <sstream>
namespace {
using namespace sadx;
void require(bool ok,const std::string& why){if(!ok)throw std::runtime_error(why);}
struct Site {std::uint32_t va=0;std::vector<std::pair<std::uint32_t,std::string>> ranges;bool active=false;std::uint32_t active_size=0;}; // every entry's (size, original bytes)
// The few fields this test needs from profile_patches.json (one object per patch, keys in a fixed order).
std::map<std::uint32_t,Site> patch_sites(const std::string& path){
    std::ifstream f(path);std::string all((std::istreambuf_iterator<char>(f)),{});
    std::map<std::uint32_t,Site> sites;std::size_t at=0;
    const auto field=[&](const std::string& key,std::size_t from,std::size_t limit)->std::string{
        const auto k=all.find("\""+key+"\"",from);if(k==std::string::npos||k>limit)return "";
        auto v=all.find(':',k)+1;while(all[v]==' ')++v;
        if(all[v]=='"'){const auto e=all.find('"',v+1);return all.substr(v+1,e-v-1);}
        auto e=v;while(e<all.size()&&all[e]!=','&&all[e]!='}'&&all[e]!='\n')++e;return all.substr(v,e-v);};
    while((at=all.find("\"va\":",at))!=std::string::npos){
        const auto next=all.find("\"va\":",at+5);const auto end=next==std::string::npos?all.size():next; // targets may contain '}'
        const auto va=static_cast<std::uint32_t>(std::stoul(field("va",at,end),nullptr,16));
        auto& s=sites[va];s.va=va;
        const auto original=field("original_bytes",at,end);
        if(original!="null"&&!original.empty())s.ranges.push_back({static_cast<std::uint32_t>(std::stoul(field("size",at,end))),original}); // null: unknown load-time bytes
        if(field("active",at,end)=="true"){s.active=true;s.active_size=std::max(s.active_size,static_cast<std::uint32_t>(std::stoul(field("size",at,end))));}
        at=end;}
    return sites;
}
std::string hex_bytes(const X86Guest& g,std::uint32_t at,std::uint32_t n){
    std::ostringstream o;for(std::uint32_t i=0;i<n;++i){std::uint8_t b=0;g.read(at+i,&b,1);char h[3];std::snprintf(h,3,"%02x",b);o<<h;}return o.str();}
}
int main(int argc,char** argv){try{
    require(argc==3,"usage: guest-dlls-test GAME_DIR WINDOWS_SYSTEM_DIR");
    const std::string dir=argv[1];
    std::ifstream f(dir+"/sonic.exe",std::ios::binary);require(bool(f),"cannot read "+dir+"/sonic.exe");
    std::string error;const auto image=ExecutableImage::parse({std::istreambuf_iterator<char>(f),{}},&error);require(bool(image),error);
    const auto overlay=std::filesystem::path("build/guest-dlls-overlay");std::filesystem::remove_all(overlay);
    auto p=HleProcess::create(dir,overlay,&error);require(p!=nullptr,error);
    p->use_real_cpp_runtime(argv[2]);
    require(p->map_main(*image,&error),"map sonic.exe: "+error);

    const auto r=p->guest().call(0x401ac0,{},0x3ab9170);
    require(!r&&r.error.find("unbound import USER32.dll!RegisterClassW")!=std::string::npos,"the run ends at the documented frontier, got: "+(r?std::string("completed"):r.error));

    // The loader's DllMain: 401ae1 = NOP, 401ae2 = CALL LoadChrmodels inside CHRMODELS.dll (the loader).
    auto* loader=p->modules().by_name("CHRMODELS.dll");require(loader&&loader->entry_called,"the loader (CHRMODELS.dll) ran its entry point");
    std::uint8_t patch[6]={};p->guest().read(0x401ae1,patch,6);std::uint32_t rel=0;std::memcpy(&rel,patch+2,4);
    const auto target=0x401ae7+rel;
    require(patch[0]==0x90&&patch[1]==0xe8&&target>=loader->base&&target<loader->base+loader->size,"401ae1/401ae2 patched to call into the loader");
    for(const char* name:{"CHRMODELS_orig.dll","CHAOSTGGARDEN02MR_DAYTIME.DLL","CHAOSTGGARDEN02MR_EVENING.DLL","CHAOSTGGARDEN02MR_NIGHT.DLL",
                          "ADV00MODELS.DLL","msvcp140.dll","vcruntime140.dll","sadx-input-mod.dll","sadx-speedrun-mod.dll","SDL2.dll"})
        require(p->modules().by_name(name)!=nullptr,std::string("module loaded: ")+name);
    for(const char* name:{"sadx-input-mod.dll","sadx-speedrun-mod.dll"})require(p->modules().by_name(name)->entry_called,std::string("DllMain ran: ")+name);
    std::set<std::string> seen;for(const auto& t:p->file_trace)seen.insert(t.substr(0,t.find(" ->")));
    for(const char* request:{"fopen[r](C:\\SADX\\mods\\.modloader\\profiles\\Profiles.json)","fopen[r](C:\\SADX\\mods\\.modloader\\profiles\\updated-speedrun.json)",
                             "fopen[r](mods\\sadx-input-mod\\mod.ini)","fopen[r](mods\\sadx-speedrun-mod\\mod.ini)"})
        require(seen.count(request)!=0,std::string("the loader read ")+request);

    // Patch sites.
    const auto sites=patch_sites("tools/gate/profile_patches.json");
    std::size_t active=0,active_written=0;std::set<std::uint32_t> unexpected;
    for(const auto& [va,s]:sites){
        bool written=false;for(const auto& [size,original]:s.ranges)written=written||hex_bytes(p->guest(),va,size)!=original;
        if(s.active){++active;active_written+=written;continue;}
        if(!written)continue;
        // A write inside an active entry's range is that patch's (e.g. 78b074 lies in the active 9-byte 78b070 trampoline).
        bool covered=false;for(const auto& [other,o]:sites)if(o.active&&other<va&&va<other+o.active_size)covered=true;
        if(!covered)unexpected.insert(va);}
    const std::set<std::uint32_t> known={};
    std::ostringstream list;for(const auto va:unexpected){list<<std::hex<<va<<" ";if(std::getenv("GUEST_DLLS_DEBUG"))for(const auto& [size,original]:sites.at(va).ranges)std::cerr<<std::hex<<va<<" size "<<std::dec<<size<<" guest "<<hex_bytes(p->guest(),va,size)<<" original "<<original<<"\n";}
    require(unexpected==known,"sites written with no active inventory entry: "+list.str());
    require(active_written>=67,"active patch sites written so far: "+std::to_string(active_written));
    std::cout<<"Guest DLLs: 401ac0 ran the real loader DllMain (401ae1 patched), LoadChrmodels, the profile and both mods' DllMains in the guest; "
             <<active_written<<" of "<<active<<" active patch sites written; "<<"no write outside the inventory's active patches; frontier: "<<r.error<<"\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
