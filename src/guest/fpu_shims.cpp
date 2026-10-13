#include "guest/fpu_shims.hpp"
#include "cpu/x87_transcendental.hpp"
#include <unicorn/unicorn.h>
#include <fstream>
#include <iterator>
namespace sadx {
std::vector<FpuSite> load_fpu_sites(const std::string& path,bool confirmed_only){
    std::ifstream f(path);std::string all((std::istreambuf_iterator<char>(f)),{});std::vector<FpuSite> out;
    for(std::size_t at=all.find("\"address\":");at!=std::string::npos;at=all.find("\"address\":",at+10)){
        const auto end=all.find('}',at);const auto obj=all.substr(at,end-at);
        const auto value=[&](const std::string& key){const auto k=obj.find("\""+key+"\":");if(k==std::string::npos)return std::string();auto v=k+key.size()+3;while(obj[v]==' ')++v;
            if(obj[v]=='"'){const auto e=obj.find('"',v+1);return obj.substr(v+1,e-v-1);}auto e=v;while(e<obj.size()&&obj[e]!=','&&obj[e]!='\n')++e;return obj.substr(v,e-v);};
        if(confirmed_only&&value("confirmed")!="true")continue;
        out.push_back({static_cast<std::uint32_t>(std::stoul(value("address"),nullptr,16)),value("instruction")});}
    return out;
}
namespace {
constexpr std::uint16_t C2=0x0400;
std::uint16_t cw_of(X86Guest& g){return static_cast<std::uint16_t>(g.reg(UC_X86_REG_FPCW));}
}
bool install_fpu_shims(X86Guest& g,const std::vector<FpuSite>& sites,FpuShimStats& stats,std::string* error){
    for(const auto& site:sites){
        const auto kind=site.instruction;
        X86Guest::InstructionHook hook;
        if(kind=="FPATAN"||kind=="FSINCOS"||kind=="FPTAN"||kind=="FSIN"||kind=="FCOS"){
            hook=[kind,&stats](X86Guest& guest,std::uint32_t)->bool{
                X87Context fp(cw_of(guest));
                try{
                    const auto a=guest.st(0);const auto x=fp.from_canonical_ext80(a.second,a.first);
                    auto status=guest.fpu_status();
                    const auto finish=[&](){status=static_cast<std::uint16_t>(status|(fp.exception_flags()&0x3f));guest.set_fpu_status(status);++stats.shimmed[kind];return true;};
                    if(kind=="FPATAN"){const auto b=guest.st(1);const auto y=fp.from_canonical_ext80(b.second,b.first);
                        const auto r=X87Transcendental::fpatan_bochs_p5_p6(fp,y,x);guest.set_st(1,r.significand(),r.sign_exponent());guest.pop_x87();status=guest.fpu_status();return finish();}
                    status=static_cast<std::uint16_t>(status&~C2);
                    if(kind=="FSINCOS"){const auto r=X87Transcendental::fsincos_bochs(fp,x);
                        if(!r){guest.set_fpu_status(static_cast<std::uint16_t>(status|C2));++stats.shimmed[kind];return true;}
                        guest.set_st(0,r->sine.significand(),r->sine.sign_exponent());guest.return_x87(r->cosine.significand(),r->cosine.sign_exponent());
                        status=static_cast<std::uint16_t>(guest.fpu_status()&~C2);return finish();}
                    if(kind=="FPTAN"){const auto r=X87Transcendental::fptan_bochs(fp,x);
                        if(!r){guest.set_fpu_status(static_cast<std::uint16_t>(status|C2));++stats.shimmed[kind];return true;}
                        guest.set_st(0,r->significand(),r->sign_exponent());guest.return_x87(1ULL<<63,0x3fff);status=static_cast<std::uint16_t>(guest.fpu_status()&~C2);return finish();}
                    const auto r=kind=="FSIN"?X87Transcendental::fsin_bochs(fp,x):X87Transcendental::fcos_bochs(fp,x);
                    if(!r){guest.set_fpu_status(static_cast<std::uint16_t>(status|C2));++stats.shimmed[kind];return true;}
                    guest.set_st(0,r->significand(),r->sign_exponent());guest.set_fpu_status(status);return finish();
                }catch(const X87Error& e){
                    if(e.failure()==X87Failure::noncanonical_ext80_source){++stats.fallback[kind];return false;} // Unicorn executes it
                    guest.stop(kind+" shim: an unmasked x87 exception (the guest's handler is not modelled)");return true;}
            };
        }else hook=[kind,&stats](X86Guest&,std::uint32_t)->bool{++stats.unshimmed[kind];return false;};
        if(!g.add_instruction_hook(site.address,std::move(hook),error))return false;
    }
    return true;
}
} // namespace sadx
