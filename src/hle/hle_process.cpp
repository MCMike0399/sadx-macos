#include "hle/hle_process.hpp"
#include <algorithm>
#include <cctype>
namespace sadx {
std::unique_ptr<HleProcess> HleProcess::create(std::filesystem::path game_dir,std::filesystem::path overlay,std::string* error){
    std::unique_ptr<HleProcess> p(new HleProcess);p->overlay_dir=std::move(overlay);
    p->guest_=X86Guest::create(error);if(!p->guest_)return nullptr;
    p->memory_=std::make_unique<HleMemory>(*p->guest_);
    p->modules_=std::make_unique<HleModules>(*p->guest_,p->hle_,*p->memory_,std::move(game_dir));
    p->modules_->register_functions();
    p->modules_->trace_=[raw=p.get()](const std::string& f,const std::string& path,const std::string& outcome){raw->trace_file(f,path,outcome);};
    register_hle_kernel32(*p);register_hle_crt(*p);register_hle_dbghelp(*p);register_hle_user32(*p);
    p->assume("the install is C:\\SADX (the guest's module paths and current directory); the reference install path was not captured");
    return p;
}
bool HleProcess::map_main(const ExecutableImage& image,std::string* error){
    if(!modules_->map_main(image,"sonic.exe",error))return false;
    // The main thread's TEB and the PEB (fields the CRT and SEH read): ExceptionList = end of chain,
    // the stack bounds, Self, the client ids, the PEB with BeingDebugged 0 and ImageBaseAddress.
    teb=memory_->allocate(0x1000,0x1000).value_or(0);peb=memory_->allocate(0x1000,0x1000).value_or(0);
    if(!teb||!peb){if(error)*error="TEB/PEB allocation";return false;}
    const auto top=guest_->stack_base()+0x100000U;
    const std::uint32_t tib[13]={0xffffffffU,top,guest_->stack_base(),0,0,0,teb,0,0x1000,0x1004,0,0,peb};
    guest_->write(teb,tib,sizeof tib);
    const std::uint32_t image_base=image.image_base();guest_->write(peb+8,&image_base,4);
    assume("main thread TEB/PEB: only ExceptionList, the stack bounds, Self, the client ids and PEB.ImageBaseAddress are filled");
    return guest_->set_fs_base(teb,0x7fe00000U,error);
}
namespace {
// The path below the install for a guest Windows path (C:\SADX\x or a relative x), split into parts.
std::optional<std::vector<std::string>> install_relative(const std::string& windows_dir,std::string p){
    std::replace(p.begin(),p.end(),'/','\\');
    auto lower=[](std::string s){for(auto& ch:s)ch=static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));return s;};
    const auto root=lower(windows_dir)+"\\";
    if(lower(p).rfind(root,0)==0)p=p.substr(root.size());else if(p.size()>=2&&p[1]==':')return std::nullopt;
    std::vector<std::string> parts;std::size_t start=0;
    while(start<=p.size()){const auto end=p.find('\\',start);const auto part=p.substr(start,end==std::string::npos?std::string::npos:end-start);
        start=end==std::string::npos?p.size()+1:end+1;if(part.empty()||part==".")continue;if(part==".."){if(parts.empty())return std::nullopt;parts.pop_back();continue;}parts.push_back(part);}
    return parts;}
std::optional<std::filesystem::path> resolve_under(const std::filesystem::path& root,const std::vector<std::string>& parts){
    std::filesystem::path at=root;
    for(const auto& part:parts){std::error_code ec;bool found=false;
        for(const auto& e:std::filesystem::directory_iterator(at,ec)){auto a=e.path().filename().string(),b=part;
            for(auto* s:{&a,&b})for(auto& ch:*s)ch=static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            if(a==b){at=e.path();found=true;break;}}
        if(!found)return std::nullopt;}
    return at;}
}
std::optional<std::filesystem::path> HleProcess::host_path(const std::string& windows_path)const{
    const auto parts=install_relative(modules_->windows_dir(),windows_path);if(!parts)return std::nullopt;
    if(!overlay_dir.empty())if(const auto o=resolve_under(overlay_dir,*parts))return o;
    return modules_->host_path(windows_path);
}
std::optional<std::filesystem::path> HleProcess::write_path(const std::string& windows_path){
    const auto parts=install_relative(modules_->windows_dir(),windows_path);if(!parts||parts->empty()||overlay_dir.empty())return std::nullopt;
    std::filesystem::path at=overlay_dir;for(const auto& part:*parts)at/=part;
    std::error_code ec;std::filesystem::create_directories(at.parent_path(),ec);
    // Copy-on-write: a file that exists only in the install is copied into the overlay first.
    if(!std::filesystem::exists(at,ec))if(const auto original=modules_->host_path(windows_path);original&&std::filesystem::is_regular_file(*original,ec))std::filesystem::copy_file(*original,at,ec);
    return at;
}
void HleProcess::use_real_cpp_runtime(std::filesystem::path directory){
    modules_->set_real_system_dlls(std::move(directory),{"msvcp140.dll","vcruntime140.dll"});
    assume("MSVCP140/VCRUNTIME140 14.51.36247.0 run as real code (a local VC++ redistributable); the reference machine's runtime version was not captured");
}
bool HleProcess::put_string(std::uint32_t at,const std::string& s){return guest_->write(at,s.c_str(),s.size()+1);}
bool HleProcess::put_wide(std::uint32_t at,const std::string& s){
    std::vector<std::uint16_t> w(s.begin(),s.end());w.push_back(0);
    for(auto& c:w)c=static_cast<std::uint16_t>(static_cast<unsigned char>(c));
    return guest_->write(at,w.data(),w.size()*2);}
std::optional<std::uint32_t> HleProcess::allocate_string(const std::string& s){
    const auto at=memory_->allocate(static_cast<std::uint32_t>(s.size()+1));if(!at)return std::nullopt;put_string(*at,s);return at;}
} // namespace sadx
