#include "hle/hle_com.hpp"
#include <cstdio>
namespace sadx {
HleComInterface::HleComInterface(HleProcess& p,std::string name,std::vector<HleComMethod> methods):p_(p),name_(std::move(name)){
    vtable_=p.memory().allocate(static_cast<std::uint32_t>(4*methods.size())).value_or(0);
    std::uint32_t i=0;
    for(auto& m:methods){
        const auto dll="com:"+name_;
        p.hle().add(dll,m.name,{m.argument_bytes,m.argument_bytes/4,std::move(m.body)});
        const auto trap=p.hle().trap(p.guest(),dll,m.name);
        p.guest().set_u32(vtable_+4*i++,trap.value_or(0));}
}
std::optional<std::uint32_t> HleComInterface::create(std::uint32_t size){
    if(!vtable_)return std::nullopt;const auto at=p_.memory().allocate(std::max<std::uint32_t>(size,8));if(!at)return std::nullopt;
    p_.guest().set_u32(*at,vtable_);p_.guest().set_u32(*at+4,1);return at;}
HleComMethod com_add_ref(){return {"AddRef",4,[](HleCall& c)->std::uint32_t{const auto o=c.argument(0).value_or(0);const auto n=c.guest.u32(o+4).value_or(0)+1;c.guest.set_u32(o+4,n);return n;}};}
HleComMethod com_release(){return {"Release",4,[](HleCall& c)->std::uint32_t{const auto o=c.argument(0).value_or(0);auto n=c.guest.u32(o+4).value_or(1);if(n)--n;c.guest.set_u32(o+4,n);return n;}};}
HleComMethod com_query_interface_stop(const std::string& name){return {"QueryInterface",12,[name](HleCall& c)->std::uint32_t{
    c.guest.stop(name+"::QueryInterface("+guid_text(c.guest,c.argument(1).value_or(0))+") is not modelled");return 0x80004002U;}};} // E_NOINTERFACE
std::string guid_text(const X86Guest& g,std::uint32_t at){
    std::uint8_t b[16]={};g.read(at,b,16);char s[48];
    std::snprintf(s,sizeof s,"%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",b[3],b[2],b[1],b[0],b[5],b[4],b[7],b[6],b[8],b[9],b[10],b[11],b[12],b[13],b[14],b[15]);return s;}
} // namespace sadx
