#include "hle/hle_device_bridge.hpp"
#include "hle/hle_process.hpp"
#include "hle/hle_heap_bridge.hpp"
#include <unicorn/unicorn.h>
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
namespace {
using namespace sadx;
constexpr std::uint32_t heap=0x30000000,buckets=0x31000000;
void require(bool ok,const std::string& why){if(!ok)throw std::runtime_error(why);}
struct Fixture {
    std::unique_ptr<HleProcess> p;
    std::uint32_t device=0,allocation=0;unsigned fail=0,calls=0;
    bool mutate=false,mutant=false;
    std::vector<std::vector<std::uint32_t>> observed;
    Fixture(const ExecutableImage& image,bool native,bool live=false){
        std::string e;p=HleProcess::create(".","build/queue-bridge-overlay",&e);require(bool(p),e);require(p->map_main(image,&e),e);auto& g=p->guest();
        require(g.map(heap,0x90000,false,&e),e);require(g.map(buckets,0x4000,false,&e),e);
        const auto create=p->hle().trap(g,"d3d8.dll","Direct3DCreate8",&e);require(bool(create),e);
        const auto d3d=g.call(*create,{0xdc});require(bool(d3d),d3d.error);
        const auto params=p->memory().allocate(52),out=p->memory().allocate(4);require(params&&out,"params");
        const std::uint32_t words[13]={1,1,22,1,0,1,0,1,0,0,0,0,0};g.write(*params,words,sizeof words);
        const auto table=g.u32(d3d.eax);require(bool(table),"table");const auto method=g.u32(*table+60);require(bool(method),"CreateDevice");
        const auto created=g.call(*method,{d3d.eax,0,1,0,0x20,*params,*out});require(bool(created),created.error);device=g.u32(*out).value_or(0);require(device,"device");
        auto state=*p->hle().find("com:IDirect3DDevice8","SetRenderState");const auto body=state.body;
        state.body=[this,body](HleCall& c){
            std::vector<std::uint32_t> snapshot;
            for(const auto at:{0x3abd954U,0x3abd958U,0x3abd960U,0x3ab9874U,0x3abd9c8U,0x3b12208U,0x3ab98acU,0x3b1117cU,0x3abd9d4U,0x3abd9dcU})snapshot.push_back(c.guest.u32(at).value_or(0));
            for(const auto at:{0x3ab9864U,0x3d0f7f0U,0x3ab9898U,0x90084cU})for(unsigned k=0;k<(at==0x3ab9864||at==0x3d0f7f0?4U:3U);++k)snapshot.push_back(c.guest.u32(at+4*k).value_or(0));
            std::uint8_t mode=0;c.guest.read(0x3abdbc0,&mode,1);snapshot.push_back(mode);observed.push_back(snapshot);
            const auto result=body(c);++calls;
            if(mutate&&calls==1){c.guest.set_u32(0x900840,0x13572468);c.guest.set_u32(0x3ab9864,0x3e800000);}
            if(fail&&calls==fail)c.guest.stop("backend unavailable");
            return result;
        };p->hle().add("com:IDirect3DDevice8","SetRenderState",state);
        if(!live)for(const auto at:{0x780f60U,0x780fb0U})require(g.add_override(at,[this,at](X86Guest& guest){if(at==0x780f60)guest.set_eax(allocation);return 0U;},&e),e);
        if(native)for(const auto& entry:device_overrides(*p))if(entry.address==0x407ab0||entry.address==0x408630){const auto body=entry.bridge;require(g.add_override(entry.address,[this,body](X86Guest& guest){if(mutant)guest.set_u32(0x3b18230,guest.u32(0x3b18230).value_or(0)^0x10000000);return body(guest);},&e),e);}
        if(native&&live)for(const auto& entry:heap_overrides(*p))require(g.add_override(entry.address,entry.bridge,&e),e);
    }
    void seed(const std::uint32_t* c){
        auto& g=p->guest();allocation=c[0];fail=c[23];calls=0;observed.clear();p->hle().clear_log();p->d3d8.render_states.clear();
        std::vector<std::uint8_t> bytes(0x90000,0xcd);g.write(heap,bytes.data(),bytes.size());bytes.assign(0x4000,0xef);g.write(buckets,bytes.data(),bytes.size());
        g.set_u32(0x3abd960,c[0]);g.set_u32(0x3abd958,0x88000);g.set_u32(0x3ab9874,c[0]+0x88000);g.set_u32(0x3abd954,c[0]+16);
        g.set_u32(0x3abd9c8,1);g.set_u32(0x90083c,buckets);g.set_u32(0x3d128b0,device);
        g.write(0x3b18230,c+2,16);g.set_u32(0x8928c8,c[6]);g.write(0x900840,c+7,12);g.write(0x900858,c+10,12);
        g.set_u32(0x3b12208,c[13]);g.write(0x3abdbc0,c+14,1);g.set_u32(0x3ab98ac,c[15]);g.set_u32(0x3b1117c,c[16]);g.set_u32(0x3abd9d4,c[17]);g.set_u32(0x3abd9dc,c[18]);g.write(0x3ab9864,c+19,16);
        const std::uint32_t old[4]={0xdeadbeef,0xdeadbeef,0xdeadbeef,0xdeadbeef};g.write(0x3d0f7f0,old,16);g.write(0x3ab9898,old,12);g.write(0x90084c,old,12);
    }
};
void compare(Fixture& a,Fixture& b,const std::uint32_t* c,std::uint32_t entry){
    a.seed(c);b.seed(c);const auto x=a.p->guest().call(entry,{},std::nullopt,std::nullopt,static_cast<std::uint16_t>(c[1])),y=b.p->guest().call(entry,{},std::nullopt,std::nullopt,static_cast<std::uint16_t>(c[1]));
    require(x.completed==y.completed,"completion "+y.error);if(!c[23])require(x.completed,"source "+x.error+" native "+y.error);
    require(a.observed==b.observed,"COM store phases");require(a.p->d3d8.render_states==b.p->d3d8.render_states,"render states");
    const auto& l=a.p->hle().log();const auto& r=b.p->hle().log();require(l.size()==r.size(),"trace size");for(unsigned k=0;k<l.size();++k)require(l[k].dll==r[k].dll&&l[k].symbol==r[k].symbol&&l[k].return_address==r[k].return_address&&l[k].arguments==r[k].arguments&&l[k].result==r[k].result,"trace");
    for(const auto range:{std::pair<std::uint32_t,unsigned>{heap,0x90000},{buckets,0x4000},{0x3ab9864,0x40},{0x3abd950,0x94},{0x3d0f7f0,16},{0x900840,0x24},{0x3b1117c,4},{0x3b12208,4}}){std::vector<std::uint8_t> left(range.second),right(range.second);require(a.p->guest().read(range.first,left.data(),left.size())&&b.p->guest().read(range.first,right.data(),right.size()),"read");require(left==right,"bytes "+std::to_string(range.first));}
}
}
int main(int argc,char** argv){try{
    require(argc==3,"exe corpus");std::ifstream exe(argv[1],std::ios::binary);std::vector<std::uint8_t> binary((std::istreambuf_iterator<char>(exe)),{});const auto parsed=ExecutableImage::parse(std::move(binary));require(bool(parsed),"PE image");const auto& image=*parsed;std::ifstream f(argv[2],std::ios::binary);std::string bytes((std::istreambuf_iterator<char>(f)),{});require(!bytes.empty()&&bytes.size()%96==0,"corpus");std::vector<std::uint32_t> cases(bytes.size()/4);std::memcpy(cases.data(),bytes.data(),bytes.size());
    Fixture original(image,false),native(image,true);
    for(const auto entry:{0x407ab0U,0x408630U})for(unsigned k=0;k<cases.size();k+=24)compare(original,native,cases.data()+k,entry);
    original.mutate=native.mutate=true;auto c=std::array<std::uint32_t,24>{};std::copy(cases.begin(),cases.begin()+24,c.begin());c[23]=0;compare(original,native,c.data(),0x407ab0);
    original.mutate=native.mutate=false;unsigned caught=0;
    for(const auto entry:{0x407ab0U,0x408630U}){native.mutant=true;try{compare(original,native,c.data(),entry);}catch(const std::exception&){++caught;}native.mutant=false;}require(caught==2,"queue input mutants");
    Fixture live_original(image,false,true),live_native(image,true,true);
    for(auto* f:{&live_original,&live_native}){f->seed(c.data());f->p->guest().set_u32(0x3abd960,0);f->p->guest().set_u32(0x389d628,0xffffffff);}
    for(unsigned k=0;k<3;++k){
        const auto a=live_original.p->guest().call(0x408630,{}),b=live_native.p->guest().call(0x408630,{});require(a.completed&&b.completed,"live heap "+a.error+" / "+b.error);
        require(live_original.observed==live_native.observed,"live phases");
        const auto& l=live_original.p->hle().log();const auto& r=live_native.p->hle().log();require(l.size()==r.size(),"live trace");
        for(unsigned i=0;i<l.size();++i)require(l[i].symbol==r[i].symbol&&l[i].return_address==r[i].return_address&&l[i].arguments==r[i].arguments&&l[i].result==r[i].result,"live heap receipt");
        const auto base=live_original.p->guest().u32(0x3abd960);require(base&&base==live_native.p->guest().u32(0x3abd960),"live pointer");require(live_original.p->memory().live_blocks()==live_native.p->memory().live_blocks(),"live block count");
        std::vector<std::uint8_t> left(0x88000),right(0x88000);require(live_original.p->guest().read(*base,left.data(),left.size())&&live_native.p->guest().read(*base,right.data(),right.size())&&left==right,"live buffer");
        const std::uint8_t dirty=0x6a;live_original.p->guest().write(*base+32,&dirty,1);live_native.p->guest().write(*base+32,&dirty,1);
    }
    original.seed(c.data());native.seed(c.data());original.allocation=native.allocation=0;
    const auto failed_source=original.p->guest().call(0x408630,{}),failed_native=native.p->guest().call(0x408630,{});
    require(!failed_source&&!failed_native&&failed_native.error.find("407ad0")!=std::string::npos,"zero allocation fault");
    for(const auto at:{0x3abd960U,0x3abd958U,0x3abd954U,0x3ab9874U,0x90084cU,0x900850U,0x900854U})require(original.p->guest().u32(at)==native.p->guest().u32(at),"allocation fault prefix");
    auto& guest=native.p->guest();native.seed(c.data());guest.set_u32(0x3d128b0,0);
    const auto null_device=guest.call(0x407ab0,{});require(!null_device&&null_device.error.find("77ed09")!=std::string::npos,"null device boundary: "+null_device.error);
    native.seed(c.data());const auto table=guest.u32(native.device),target=table?guest.u32(*table+0xc8):std::nullopt;require(table&&target,"vtable");guest.set_u32(*table+0xc8,0);
    const auto foreign=guest.call(0x407ab0,{});require(!foreign&&foreign.error.find("non-HLE")!=std::string::npos,"foreign device boundary");guest.set_u32(*table+0xc8,*target);
    native.seed(c.data());guest.set_u32(0x3abd958,32);const auto size=guest.call(0x407ab0,{});require(!size&&size.error.find("407ad0")!=std::string::npos,"unobserved buffer-size boundary");
    std::cout<<"Queue bridge: "<<cases.size()/24<<" lifecycle corpus cases at reset and allocation; exact buffer/bucket/global bytes, HLE traces and COM store phases; live colour/capture changes preserved; 3 real heap reallocations, 2 input mutants caught; null/foreign devices and unobserved sizes stopped; FP status remains separate\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
