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
const HleFunction* HleRegistry::find(const std::string& dll,const std::string& symbol)const{
    const auto i=functions_.find(key(dll,symbol));return i==functions_.end()?nullptr:&i->second;}
ImportBindingResult bind_imports(X86Guest& guest,const ExecutableImage& image,HleRegistry& registry,std::uint32_t trap_base){
    ImportBindingResult r;std::string error;
    const auto modules=image.imports(&error);
    if(!modules){r.error="import table: "+error;return r;}
    std::size_t count=0;for(const auto& m:*modules)count+=m.symbols.size();
    const auto size=static_cast<std::uint32_t>(std::max<std::size_t>(count,1)*16);
    if(!guest.map(trap_base,size,true,&error)){r.error="trap page: "+error;return r;}
    const std::vector<std::uint8_t> halt((size+0xfffU)&~0xfffU,0xf4);guest.write(trap_base,halt.data(),halt.size());
    std::uint32_t trap=trap_base;
    for(const auto& m:*modules)for(const auto& s:m.symbols){
        ImportBinding b{m.dll,s.name?*s.name:"#"+std::to_string(*s.ordinal),s.iat_va,trap,registry.find(m.dll,s.name?*s.name:"#"+std::to_string(*s.ordinal))!=nullptr};
        if(!guest.set_u32(s.iat_va,trap)){r.error="IAT slot "+hex(s.iat_va)+" not mapped";return r;}
        const auto dll=b.dll,symbol=b.symbol;
        if(!guest.add_override(trap,[&registry,dll,symbol](X86Guest& g)->std::uint32_t{
                const auto esp=g.esp();const auto back=g.u32(esp).value_or(0);
                const auto* f=registry.find(dll,symbol);
                if(!f||!f->body){g.stop("unbound import "+dll+"!"+symbol+" called (return address "+hex(back)+")");return 0;}
                HleCall call(g,esp);HleCallRecord rec{dll,symbol,back,{},0};
                for(unsigned i=0;i<f->logged_arguments;++i)rec.arguments.push_back(call.argument(i).value_or(0));
                rec.result=f->body(call);g.set_eax(rec.result);registry.record(std::move(rec));
                return f->argument_bytes;},&error)){r.error="trap "+dll+"!"+symbol+": "+error;return r;}
        (b.bound?r.bound:r.unbound)+=1;r.bindings.push_back(std::move(b));trap+=16;}
    return r;
}
} // namespace sadx
