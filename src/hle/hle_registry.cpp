#include "hle/hle_registry.hpp"
#include <algorithm>
#include <cctype>
#include <cstdio>
namespace sadx {
namespace {
std::string lower(std::string s){std::transform(s.begin(),s.end(),s.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});return s;}
std::string key(const std::string& dll,const std::string& symbol){return lower(dll)+"!"+symbol;}
std::string hex(std::uint32_t v){char b[16];std::snprintf(b,sizeof b,"%x",v);return b;}
}
void HleRegistry::add(const std::string& dll,const std::string& symbol,HleFunction f){functions_[key(dll,symbol)]=std::move(f);}
void HleRegistry::add_data(const std::string& dll,const std::string& symbol,std::function<std::optional<std::uint32_t>()> provide){data_providers_[key(dll,symbol)]=std::move(provide);}
std::optional<std::uint32_t> HleRegistry::data(const std::string& dll,const std::string& symbol){
    const auto k=key(dll,symbol);if(const auto i=data_.find(k);i!=data_.end())return i->second;
    const auto p=data_providers_.find(k);if(p==data_providers_.end())return std::nullopt;
    const auto at=p->second();if(at)data_[k]=*at;return at;}
bool HleRegistry::alias(const std::string& dll,const std::string& symbol,const std::string& from_dll,const std::string& from_symbol){
    const auto* f=find(from_dll,from_symbol);if(!f)return false;functions_[key(dll,symbol)]=*f;return true;}
void HleRegistry::add_absent(const std::string& dll,const std::string& symbol){absent_.insert(key(dll,symbol));}
bool HleRegistry::absent(const std::string& dll,const std::string& symbol)const{return absent_.count(key(dll,symbol))!=0;}
const HleFunction* HleRegistry::find(const std::string& dll,const std::string& symbol)const{
    const auto i=functions_.find(key(dll,symbol));return i==functions_.end()?nullptr:&i->second;}
std::optional<std::uint32_t> HleRegistry::trap(X86Guest& guest,const std::string& dll,const std::string& symbol,std::string* error){
    const auto k=key(dll,symbol);const auto found=traps_.find(k);if(found!=traps_.end())return found->second;
    if(!next_trap_)next_trap_=mapped_trap_end_=hle_trap_base;
    if(next_trap_+16>mapped_trap_end_){
        if(!guest.map(mapped_trap_end_,0x1000,true,error))return std::nullopt;
        const std::vector<std::uint8_t> halt(0x1000,0xf4);guest.write(mapped_trap_end_,halt.data(),halt.size());mapped_trap_end_+=0x1000;}
    const auto at=next_trap_;next_trap_+=16;
    if(!guest.add_override(at,[this,dll,symbol](X86Guest& g)->std::uint32_t{
            const auto esp=g.esp();const auto back=g.u32(esp).value_or(0);
            const auto* f=find(dll,symbol);
            if(!f||!f->body){g.stop("unbound import "+dll+"!"+symbol+" called (return address "+hex(back)+")");return 0;}
            HleCall call(g,esp);HleCallRecord rec{dll,symbol,back,{},0};
            for(unsigned i=0;i<f->logged_arguments;++i)rec.arguments.push_back(call.argument(i).value_or(0));
            rec.result=f->body(call);g.set_eax(rec.result);record(std::move(rec));
            return f->argument_bytes;},error))return std::nullopt;
    traps_[k]=at;return at;
}
ImportBindingResult bind_imports(X86Guest& guest,const ExecutableImage& image,HleRegistry& registry,const ImportResolver& resolve){
    ImportBindingResult r;std::string error;
    const auto modules=image.imports(&error);
    if(!modules){r.error="import table: "+error;return r;}
    for(const auto& m:*modules)for(const auto& s:m.symbols){
        const std::string symbol=s.name?*s.name:"#"+std::to_string(*s.ordinal);
        std::optional<std::uint32_t> target;
        if(resolve){target=resolve(m.dll,s,&error);if(!target&&!error.empty()){r.error=m.dll+"!"+symbol+": "+error;return r;}}
        ImportBinding b{m.dll,symbol,s.iat_va,0,false};
        if(!target)target=registry.data(m.dll,symbol);
        if(target){b.trap=*target;b.bound=true;}
        else{const auto trap=registry.trap(guest,m.dll,symbol,&error);if(!trap){r.error="trap "+m.dll+"!"+symbol+": "+error;return r;}
            b.trap=*trap;b.bound=registry.find(m.dll,symbol)!=nullptr;}
        if(!guest.set_u32(s.iat_va,b.trap)){r.error="IAT slot "+hex(s.iat_va)+" not mapped";return r;}
        (b.bound?r.bound:r.unbound)+=1;r.bindings.push_back(std::move(b));}
    return r;
}
} // namespace sadx
