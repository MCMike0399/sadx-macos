#include "hle/hle_modules.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <unicorn/unicorn.h>
namespace sadx {
namespace {
std::string lower(std::string s){std::transform(s.begin(),s.end(),s.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});return s;}
bool iequal(const std::string& a,const std::string& b){return lower(a)==lower(b);}
std::string stem(const std::string& file){const auto lowered=lower(file);const auto dot=lowered.rfind('.');return dot==std::string::npos?lowered:lowered.substr(0,dot);}
std::string file_part(const std::string& path){const auto at=path.find_last_of("\\/");return at==std::string::npos?path:path.substr(at+1);}
// The DLLs a Windows install provides (resolved by HLE, never from the game directory). Any other bare
// name that is not a file in the game directory makes LoadLibrary fail, as it would on Windows.
bool system_dll(const std::string& name){
    const auto s=stem(file_part(name));
    static const char* known[]={"kernel32","kernelbase","ntdll","user32","gdi32","advapi32","shell32","shlwapi","ole32","oleaut32",
        "winmm","dsound","dinput8","dinput","d3d8","d3d9","dbghelp","gdiplus","msvcp140","vcruntime140","ucrtbase","msvcrt","imm32",
        "setupapi","version","comctl32","comdlg32","ws2_32","xinput1_3","xinput1_4","xinput9_1_0","dxgi","dwmapi","uxtheme","winhttp","hid",
        // more of a Windows 10 22H2 System32 (present there, so LoadLibrary must succeed; their functions are HLE or stop)
        "cfgmgr32","combase","rpcrt4","sechost","bcrypt","bcryptprimitives","crypt32","shcore","powrprof","winusb","d3d11","d3d12","dcomp","propsys",
        "mmdevapi","avrt","ksuser","dwrite","d2d1","usp10","msimg32","psapi","secur32","userenv","wtsapi32","iphlpapi","winspool","opengl32","glu32",
        "windowscodecs","wintrust","ntmarta","imagehlp","msacm32","dsound","xaudio2_9","x3daudio1_7","mfplat","mf","mfreadwrite","dbgcore","apphelp","win32u",
        "wmvcore","wmasf","quartz","msdmo","dmoguids"}; // wmvcore: Windows Media Format runtime of a Windows 10 with Media Player (not an N edition)
    for(const auto* k:known)if(s==k)return true;
    return s.rfind("api-ms-win-",0)==0;
}
}
std::optional<std::string> guest_string(const X86Guest& g,std::uint32_t at,std::size_t limit){
    std::string s;for(std::size_t i=0;i<limit;++i){char c;if(!g.read(at+static_cast<std::uint32_t>(i),&c,1))return std::nullopt;if(!c)return s;s.push_back(c);}return std::nullopt;}
std::optional<std::string> guest_wide_string(const X86Guest& g,std::uint32_t at,std::size_t limit){
    std::string s;for(std::size_t i=0;i<limit;++i){std::uint16_t c;if(!g.read(at+static_cast<std::uint32_t>(2*i),&c,2))return std::nullopt;if(!c)return s;if(c>0x7f)return std::nullopt;s.push_back(static_cast<char>(c));}return std::nullopt;}
HleModules::HleModules(X86Guest& g,HleRegistry& r,HleMemory& m,std::filesystem::path dir,std::string wdir):guest_(g),hle_(r),memory_(m),game_dir_(std::move(dir)),windows_dir_(std::move(wdir)){}
std::optional<std::filesystem::path> HleModules::host_path(const std::string& windows_path)const{
    std::string p=windows_path;std::replace(p.begin(),p.end(),'/','\\');
    const auto root=lower(windows_dir_)+"\\";
    if(lower(p).rfind(root,0)==0)p=p.substr(root.size());
    else if(p.size()>=2&&p[1]==':')return std::nullopt; // another drive or outside the install
    std::filesystem::path at=game_dir_;std::size_t start=0;
    while(start<=p.size()){
        const auto end=p.find('\\',start);const auto part=p.substr(start,end==std::string::npos?std::string::npos:end-start);
        start=end==std::string::npos?p.size()+1:end+1;
        if(part.empty()||part==".")continue;
        if(part==".."){if(at==game_dir_)return std::nullopt;at=at.parent_path();continue;}
        std::error_code ec;bool found=false;
        for(const auto& e:std::filesystem::directory_iterator(at,ec))if(iequal(e.path().filename().string(),part)){at=e.path();found=true;break;}
        if(!found)return std::nullopt;}
    std::error_code ec;if(!std::filesystem::exists(at,ec))return std::nullopt;
    return at;
}
std::optional<std::uint32_t> HleModules::choose_base(std::uint32_t preferred,std::uint32_t size)const{
    const auto free=[&](std::uint32_t base){
        for(const auto& m:modules_)if(!m->system&&base<m->base+m->size&&m->base<base+size)return false;
        return !memory_.contains(base)&&!memory_.contains(base+size-1);};
    if(free(preferred))return preferred;
    for(std::uint32_t b=next_base_;b<0x40000000U;b+=0x10000U)if(free(b))return b;
    return std::nullopt;
}
GuestModule* HleModules::by_base(std::uint32_t base){for(auto& m:modules_)if(m->base==base)return m.get();return nullptr;}
GuestModule* HleModules::by_name(const std::string& name){
    const auto want=file_part(name);
    for(auto& m:modules_)if(iequal(m->name,want)||stem(m->name)==stem(want))return m.get();
    return nullptr;
}
bool HleModules::map_main(const ExecutableImage& image,const std::string& file_name,std::string* error){
    if(!guest_.map_image(image,error))return false;
    auto m=std::make_unique<GuestModule>();m->name=file_name;m->windows_path=windows_dir_+"\\"+file_name;m->host_path=game_dir_/file_name;
    m->base=image.image_base();m->size=image.size_of_image();m->entry=image.entry_point();m->image=std::make_shared<ExecutableImage>(image);
    m->references=1;m->entry_called=true;
    auto* raw=m.get();modules_.push_back(std::move(m));
    return link(*raw,nullptr,error).has_value();
}
std::optional<std::uint32_t> HleModules::map_library(const std::string& requested,std::vector<GuestModule*>* pending,std::string* error){
    const bool has_dir=requested.find_first_of("\\/")!=std::string::npos;
    if(auto* loaded=by_name(requested);loaded&&(!has_dir||loaded->system||iequal(loaded->windows_path,requested)||(host_path(requested)&&*host_path(requested)==loaded->host_path))){++loaded->references;return loaded->base;}
    auto host=host_path(requested);
    std::string windows_path;
    if(!host&&!has_dir&&system_dll(requested)){
        const auto file=file_part(requested);std::string wanted=lower(file);if(wanted.find('.')==std::string::npos)wanted+=".dll";
        const bool real=std::any_of(real_system_.begin(),real_system_.end(),[&](const std::string& n){return lower(n)==wanted;});
        std::error_code ec;
        if(real&&!system_dir_.empty())for(const auto& e:std::filesystem::directory_iterator(system_dir_,ec))if(lower(e.path().filename().string())==wanted){host=e.path();windows_path="C:\\Windows\\SysWOW64\\"+e.path().filename().string();break;}
        if(!host){
            auto m=std::make_unique<GuestModule>();m->name=file;m->windows_path="C:\\Windows\\System32\\"+m->name;m->system=true;
            m->base=next_system_;next_system_+=0x10000U;m->references=1;m->entry_called=true;const auto base=m->base;modules_.push_back(std::move(m));return base;}
    }
    if(!host){if(error)*error="file not found: "+requested;return std::nullopt;}
    for(auto& m:modules_)if(!m->system&&m->host_path==*host){++m->references;return m->base;}
    if(windows_path.empty()){windows_path=windows_dir_+"\\"+std::filesystem::relative(*host,game_dir_).string();std::replace(windows_path.begin(),windows_path.end(),'/','\\');}
    return map_file(*host,windows_path,pending,error);
}
std::optional<std::uint32_t> HleModules::map_file(const std::filesystem::path& host,const std::string& windows_path,std::vector<GuestModule*>* pending,std::string* error){
    const auto requested=host.filename().string();
    std::ifstream f(host,std::ios::binary);if(!f){if(error)*error="cannot read "+host.string();return std::nullopt;}
    std::string why;auto image=ExecutableImage::parse({std::istreambuf_iterator<char>(f),{}},&why);
    if(!image){if(error)*error=requested+": "+why;return std::nullopt;}
    const auto base=choose_base(image->image_base(),image->size_of_image());
    if(!base){if(error)*error="no address space for "+requested;return std::nullopt;}
    if(*base!=image->image_base()){image=image->rebased(*base,&why);if(!image){if(error)*error=requested+": "+why;return std::nullopt;}next_base_=*base+((image->size_of_image()+0xffffU)&~0xffffU);}
    if(!guest_.map_image(*image,&why)){if(error)*error=requested+": "+why;return std::nullopt;}
    auto m=std::make_unique<GuestModule>();m->name=host.filename().string();m->windows_path=windows_path;
    m->host_path=host;m->base=*base;m->size=image->size_of_image();m->entry=image->entry_point();m->image=std::make_shared<ExecutableImage>(std::move(*image));m->references=1;
    auto* raw=m.get();modules_.push_back(std::move(m));
    if(!link(*raw,pending,error))return std::nullopt;
    if(pending&&raw->entry)pending->push_back(raw);else raw->entry_called=true;
    return raw->base;
}
std::optional<std::uint32_t> HleModules::link(GuestModule& m,std::vector<GuestModule*>* pending,std::string* error){
    const ImportResolver resolve=[&](const std::string& dll,const ExecutableImage::ImportedSymbol& s,std::string* e)->std::optional<std::uint32_t>{
        const auto base=map_library(dll,pending,e);
        if(!base)return std::nullopt;
        auto* dep=by_base(*base);
        if(dep->system)return std::nullopt; // HLE trap
        if(!s.name){if(e)*e="ordinal import from a game DLL is not supported: "+dll;return std::nullopt;}
        const auto at=export_address(*base,*s.name);
        if(!at&&e)*e="missing export "+dll+"!"+*s.name;
        return at;};
    const auto r=bind_imports(guest_,*m.image,hle_,resolve);
    if(!r){if(error)*error=m.name+": "+r.error;return std::nullopt;}
    return m.base;
}
std::optional<std::uint32_t> HleModules::export_address(std::uint32_t base,const std::string& name){
    auto* m=by_base(base);if(!m)return std::nullopt;
    if(m->system){if(const auto d=hle_.data(m->name,name))return d;if(!hle_.find(m->name,name))return std::nullopt;return hle_.trap(guest_,m->name,name);}
    const auto e=m->image->export_named(name);if(!e)return std::nullopt;
    if(!e->forwarder)return e->va;
    const auto dot=e->forwarder->find('.');if(dot==std::string::npos)return std::nullopt;
    const auto target=map_library(e->forwarder->substr(0,dot)+".dll",nullptr,nullptr);if(!target)return std::nullopt;
    return export_address(*target,e->forwarder->substr(dot+1));
}
void HleModules::run_entries(X86Guest& g,std::vector<GuestModule*> pending,std::uint32_t pops,std::function<void(X86Guest&,bool)> done){
    auto state=std::make_shared<std::pair<std::vector<GuestModule*>,std::size_t>>(std::move(pending),0);
    auto ok=std::make_shared<bool>(true);
    auto step=std::make_shared<std::function<void(X86Guest&,std::uint32_t)>>();
    *step=[state,ok,step,done](X86Guest& gg,std::uint32_t pop){
        while(state->second<state->first.size()&&!state->first[state->second]->entry)++state->second;
        if(state->second>=state->first.size()){done(gg,*ok);return;}
        auto* m=state->first[state->second++];
        gg.redirect(m->entry,{m->base,1U,0U},pop,[state,ok,step,m](X86Guest& g2){
            m->entry_called=true;if(!g2.reg(UC_X86_REG_EAX))*ok=false;
            (*step)(g2,0);});};
    (*step)(g,pops);
}
void HleModules::register_functions(){
    const auto load=[this](bool wide){return [this,wide](HleCall& c)->std::uint32_t{
        const auto at=c.argument(0).value_or(0);
        const auto name=wide?guest_wide_string(c.guest,at):guest_string(c.guest,at);
        if(!name){c.guest.stop(std::string("LoadLibrary")+(wide?"W":"A")+": unreadable or non-ASCII file name");return 0;}
        std::vector<GuestModule*> pending;std::string error;
        const auto base=map_library(*name,&pending,&error);
        if(trace_)trace_(wide?"LoadLibraryW":"LoadLibraryA",*name,base?"loaded":error);
        if(!base)return 0;
        if(pending.empty())return *base;
        const auto b=*base;
        run_entries(c.guest,std::move(pending),4,[b](X86Guest& g,bool ok){g.set_eax(ok?b:0);});
        return b;};};
    hle_.add("KERNEL32.dll","LoadLibraryA",{4,1,load(false)});
    hle_.add("KERNEL32.dll","LoadLibraryW",{4,1,load(true)});
    const auto load_ex=[this](bool wide){return [this,wide](HleCall& c)->std::uint32_t{
        // Search-path flags only: LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR 0x100, _APPLICATION_DIR 0x200, _USER_DIRS 0x400,
        // _SYSTEM32 0x800, _DEFAULT_DIRS 0x1000. With SYSTEM32 alone a game-directory DLL is not found.
        const auto flags=c.argument(2).value_or(0);
        if(c.argument(1).value_or(1)!=0||(flags&~0x1f00U)){c.guest.stop("LoadLibraryEx with a file handle or flags "+std::to_string(flags)+" is not implemented");return 0;}
        const auto at=c.argument(0).value_or(0);
        const auto name=wide?guest_wide_string(c.guest,at):guest_string(c.guest,at);
        if(!name){c.guest.stop("LoadLibraryEx: unreadable file name");return 0;}
        if(flags==0x800&&!system_dll(*name)){if(trace_)trace_(wide?"LoadLibraryExW":"LoadLibraryExA",*name,"not in System32");return 0;}
        std::vector<GuestModule*> pending;std::string error;const auto base=map_library(*name,&pending,&error);
        if(!base)return 0;if(pending.empty())return *base;const auto b=*base;
        run_entries(c.guest,std::move(pending),12,[b](X86Guest& g,bool ok){g.set_eax(ok?b:0);});return b;};};
    hle_.add("KERNEL32.dll","LoadLibraryExA",{12,3,load_ex(false)});
    hle_.add("KERNEL32.dll","LoadLibraryExW",{12,3,load_ex(true)});
    hle_.add("KERNEL32.dll","GetProcAddress",{8,2,[this](HleCall& c)->std::uint32_t{
        const auto module=c.argument(0).value_or(0),at=c.argument(1).value_or(0);
        const auto name=at<0x10000?std::optional<std::string>("#"+std::to_string(at)):guest_string(c.guest,at);
        if(!name){c.guest.stop("GetProcAddress: unreadable name");return 0;}
        if(at<0x10000&&by_base(module)&&!by_base(module)->system){c.guest.stop("GetProcAddress by ordinal on a guest DLL is not implemented");return 0;}
        auto* m=by_base(module);if(!m)return 0;
        if(m->system&&hle_.data(m->name,*name))return *hle_.data(m->name,*name);
        if(m->system&&!hle_.find(m->name,*name)&&hle_.absent(m->name,*name))return 0; // not on the modelled Windows version
        // A system function without an HLE body: its trap (the function exists on the modelled Windows 10 22H2;
        // calling it stops with its name). Functions newer than that are registered absent (NULL above).
        if(m->system&&!hle_.find(m->name,*name)){if(on_unmodelled_lookup_)on_unmodelled_lookup_(m->name,*name);return hle_.trap(guest_,m->name,*name).value_or(0);}
        return export_address(module,*name).value_or(0);}});
    const auto handle=[this](bool wide){return [this,wide](HleCall& c)->std::uint32_t{
        const auto at=c.argument(0).value_or(0);
        if(!at)return modules_.empty()?0:modules_.front()->base;
        const auto name=wide?guest_wide_string(c.guest,at):guest_string(c.guest,at);if(!name)return 0;
        if(auto* m=by_name(*name))return m->base;
        if(system_dll(*name)){std::string e;return map_library(file_part(*name),nullptr,&e).value_or(0);} // loaded in every process the exe imports from
        return 0;};};
    hle_.add("KERNEL32.dll","GetModuleHandleA",{4,1,handle(false)});
    hle_.add("KERNEL32.dll","GetModuleHandleW",{4,1,handle(true)});
    const auto file_name=[this](bool wide){return [this,wide](HleCall& c)->std::uint32_t{
        const auto module=c.argument(0).value_or(0),buffer=c.argument(1).value_or(0),size=c.argument(2).value_or(0);
        const GuestModule* m=module?by_base(module):(modules_.empty()?nullptr:modules_.front().get());
        if(!m||!size)return 0;
        const auto& path=m->windows_path;const auto n=static_cast<std::uint32_t>(std::min<std::size_t>(path.size(),size-1));
        for(std::uint32_t i=0;i<n;++i){if(wide){const std::uint16_t ch=static_cast<unsigned char>(path[i]);c.guest.write(buffer+2*i,&ch,2);}else c.guest.write(buffer+i,&path[i],1);}
        if(wide){const std::uint16_t z=0;c.guest.write(buffer+2*n,&z,2);}else{const char z=0;c.guest.write(buffer+n,&z,1);}
        return path.size()<size?static_cast<std::uint32_t>(path.size()):size;};};
    hle_.add("KERNEL32.dll","GetModuleFileNameA",{12,3,file_name(false)});
    hle_.add("KERNEL32.dll","GetModuleFileNameW",{12,3,file_name(true)});
    hle_.add("KERNEL32.dll","FreeLibrary",{4,1,[this](HleCall& c)->std::uint32_t{
        auto* m=by_base(c.argument(0).value_or(0));if(!m)return 0;if(m->references)--m->references;return 1;}});
    hle_.add("KERNEL32.dll","DisableThreadLibraryCalls",{4,1,[](HleCall&)->std::uint32_t{return 1;}});
}
} // namespace sadx
