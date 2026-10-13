// USER32 / GDI32 / SHELL32 / SHLWAPI / ADVAPI32 for the guest, headless: the window system is not
// modelled yet (the app front end will supply it), so functions that would show or query real UI either
// report the documented "nothing to do" result or stop with their name.
#include "hle/hle_process.hpp"
namespace sadx {
void register_hle_user32(HleProcess& p){
    auto& r=p.hle();auto* P=&p;
    const auto U=[&](const char* name,std::uint32_t pops,unsigned logged,std::function<std::uint32_t(HleCall&)> body){r.add("USER32.dll",name,{pops,logged,std::move(body)});};
    // SHLWAPI path helpers on guest strings (pointers into the argument).
    r.add("SHLWAPI.dll","PathFindFileNameA",{4,1,[P](HleCall& c)->std::uint32_t{
        const auto at=c.argument(0).value_or(0);const auto s=P->string_at(at);if(!s)return at;
        std::size_t start=0;for(std::size_t i=0;i<s->size();++i)if(((*s)[i]=='\\'||(*s)[i]=='/'||(*s)[i]==':')&&i+1<s->size())start=i+1;
        return at+static_cast<std::uint32_t>(start);}});
    r.add("SHLWAPI.dll","PathFindExtensionA",{4,1,[P](HleCall& c)->std::uint32_t{
        const auto at=c.argument(0).value_or(0);const auto s=P->string_at(at);if(!s)return at;
        std::size_t dot=s->size();for(std::size_t i=0;i<s->size();++i){const char ch=(*s)[i];if(ch=='.')dot=i;else if(ch=='\\'||ch=='/'||ch==' ')dot=s->size();}
        return at+static_cast<std::uint32_t>(dot);}});
    U("SetProcessDPIAware",0,0,[P](HleCall&)->std::uint32_t{P->assume("SetProcessDPIAware succeeds and has no effect (no host DPI scaling is modelled)");return 1;});
}
} // namespace sadx
