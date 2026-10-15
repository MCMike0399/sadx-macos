#include "hle/hle_device_bridge.hpp"
#include "hle/hle_process.hpp"
#include "guest/fpu_shims.hpp"
#include <unicorn/unicorn.h>
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {
using namespace sadx;
void require(bool ok,const std::string& why){if(!ok)throw std::runtime_error(why);}
struct Range{std::uint32_t address,size;};
// Every written region in verify_original_screen_init.py, including its seeded
// sentinels, plus the live viewport/projection/blend inputs callbacks may change.
constexpr Range outputs[]={
    {0x3d0f804,4},{0x3d0f964,4},{0x3d0f814,4},{0x3d0f9e4,8},
    {0x3d0fa20,4},{0x3d0f838,4},{0x3d0fa04,16},{0x3ce7160,4},
    {0x3d0f820,16},{0x3ce7164,0x40},{0x3d0fd80,64},{0x3d0fd20,16},
    {0x389d7dc,4},{0x3d084a8,8},{0x3ce712c,4},{0x3d0f840,4},
    {0x3d0f9c4,4},{0x3ce71b0,4},{0x3ce71a4,4},{0x3d0fd1c,4},
    {0x3d12780,24},{0x3d129c0,64},{0x389d318,64},{0x88ad64,88}};
std::vector<std::uint32_t> snapshot(X86Guest& g){
    std::vector<std::uint32_t> result;
    for(const auto r:outputs){const auto old=result.size();result.resize(old+r.size/4);require(g.read(r.address,result.data()+old,r.size),"snapshot read");}
    return result;
}
struct Observation{
    std::string method;std::vector<std::uint32_t> globals,frame,matrix;
    std::array<std::uint32_t,7> registers{};
};
struct Fixture{
    FpuShimStats shims;std::unique_ptr<HleProcess> process;
    std::array<std::uint32_t,2> devices{};
    std::array<std::uint32_t,4> targets{};
    std::uint32_t entry=0;unsigned bodies=0,calls=0,mutation=0,stop_at=0,fallback=0,mutant=0;
    std::vector<Observation> observations;
    explicit Fixture(const ExecutableImage& image,bool native){
        std::string error;process=HleProcess::create(".","build/screen-init-bridge-overlay",&error);require(bool(process),error);
        require(process->map_main(image,&error),error);auto& g=process->guest();
        require(install_fpu_shims(g,load_fpu_sites("reference/inventory/fpu-sites.json"),shims,&error),error);
        require(g.map(0x10000000,4096,true,&error),error);
        require(g.map(0x10002000,4096,false,&error),error);
        const std::uint8_t ret=0xc3;require(g.write(0x10000000,&ret,1),"foreign method code");
        const auto create=process->hle().trap(g,"d3d8.dll","Direct3DCreate8",&error);require(create.has_value(),error);
        const auto d3d=g.call(*create,{0xdc});require(bool(d3d),d3d.error);
        const auto params=process->memory().allocate(52),out=process->memory().allocate(4);require(params&&out,"device memory");
        const std::uint32_t pp[13]={1,1,22,1,0,1,0,1,0,0,0,0,0};require(g.write(*params,pp,sizeof pp),"present parameters");
        const auto table=g.u32(d3d.eax),method=table?g.u32(*table+60):std::nullopt;require(method.has_value(),"CreateDevice target");
        for(auto& device:devices){const auto r=g.call(*method,{d3d.eax,0,1,0,0x20,*params,*out});require(bool(r),r.error);device=g.u32(*out).value_or(0);require(device!=0,"device");}
        const std::array<const char*,4> names={"SetViewport","SetTransform","MultiplyTransform","SetRenderState"};
        const unsigned offsets[]={0xa0,0x94,0x9c,0xc8};
        for(unsigned i=0;i<names.size();++i){
            targets[i]=*g.u32(*g.u32(devices[0])+offsets[i]);
            auto hle=*process->hle().find("com:IDirect3DDevice8",names[i]);const auto body=hle.body;const std::string name=names[i];
            hle.body=[this,body,name](HleCall& c){
                auto& guest=c.guest;Observation o;o.method=name;o.globals=snapshot(guest);
                // Compare actual call arguments, source return, and the surviving
                // enclosing source frame through entry ESP (saved ESI/EDI included).
                const auto count=(entry-guest.esp())/4+1;
                require(count<=16,"COM frame outside source stack");o.frame.resize(count);
                require(guest.read(guest.esp(),o.frame.data(),count*4),"COM frame read");
                const int regs[]={UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EFLAGS,UC_X86_REG_ESP};
                for(unsigned k=0;k<7;++k)o.registers[k]=guest.reg(regs[k]);o.registers[5]&=0x8d5U;
                if(name!="SetRenderState"){
                    const auto at=c.argument(name=="SetViewport"?1:2);require(at.has_value(),"COM data pointer");
                    o.matrix.resize(name=="SetViewport"?6:16);require(guest.read(*at,o.matrix.data(),o.matrix.size()*4),"COM data read");
                }
                observations.push_back(std::move(o));const auto result=body(c);++calls;
                if(calls==1){
                    if(mutation==1)guest.set_u32(0x3d128b0,devices[1]);
                    if(mutation==2){guest.set_u32(0x3d0fa04,0x43de8000);guest.set_u32(0x3d0fa08,0x435e0000);}
                    if(mutation==3)guest.set_u32(0x3d129c0,*guest.u32(0x3d129c0)^0x100000U);
                    if(mutation==7)guest.set_u32(entry-4,0x98765432);
                    if(mutation==8){guest.set_u32(0x3d12788,321);guest.set_u32(0x3d1278c,123);}
                }
                if(calls==2&&mutation==4)guest.set_u32(0x3d0fd80,*guest.u32(0x3d0fd80)^0x100000U);
                if(calls==3){
                    if(mutation==5){guest.set_u32(0x88ad64,0x113);guest.set_u32(0x88ad68,0x114);guest.set_u32(0x88ad8c,0x55);guest.set_u32(0x88ad84,0x66);}
                    if(mutation==6||mutation==9){guest.set_u32(entry-20,0x12345678);guest.set_u32(entry-16,0x87654321);}
                }
                if(stop_at==calls)guest.stop("screen backend unavailable");
                // Every source call ignores HRESULT, including failed receipts.
                return calls%2?0x80004005U:result;
            };process->hle().add("com:IDirect3DDevice8",name,hle);
        }
        require(g.add_instruction_hook(0x780c39,[this](X86Guest&,std::uint32_t){++bodies;return false;},&error),error);
        require(g.add_instruction_hook(0x10000000,[](X86Guest& guest,std::uint32_t){guest.stop("foreign source method");return false;},&error),error);
        if(native){
            bool installed=false;
            for(const auto& override:device_overrides(*process))if(override.address==0x780c30){
                const auto bridge=override.bridge;
                require(g.add_override(override.address,[this,bridge](X86Guest& guest){
                    enter(guest);
                    if(mutant==1)guest.set_u32(0x3d12788,*guest.u32(0x3d12788)^0x40U);
                    if(mutant>=3){const auto at=entry+4*(mutant-2);guest.set_u32(at,*guest.u32(at)^(mutant==3?0x10U:1U));}
                    // An always-original impostor must fail the body-path assertion.
                    if(mutant==2){guest.set_eax(*guest.u32(entry+8));guest.tail_redirect(0x780c34);return 0U;}
                    return bridge(guest);
                },&error),error);installed=true;
            }
            require(installed,"screen initializer override missing");
        }else require(g.add_instruction_hook(0x780c30,[this](X86Guest& guest,std::uint32_t){enter(guest);return false;},&error),error);
    }
    void enter(X86Guest& g){
        entry=g.esp();std::array<std::uint32_t,64> scratch{};
        for(unsigned i=0;i<scratch.size();++i)scratch[i]=0xaabb0000U+i;
        require(g.write(entry-256,scratch.data(),sizeof scratch),"initial scratch");
        g.set_reg(UC_X86_REG_EAX,0x11223344);g.set_reg(UC_X86_REG_EBX,0x22334455);
        g.set_reg(UC_X86_REG_ECX,0x33445566);g.set_reg(UC_X86_REG_EDX,0x44556677);
        g.set_reg(UC_X86_REG_ESI,0x55667788);g.set_reg(UC_X86_REG_EDI,mutation==9?1U:0x66778899U);
        g.set_reg(UC_X86_REG_EFLAGS,0x202U|(fallback==1?0x400U:0));
        if(fallback==2)g.return_x87(0x8000000000000000ULL,0x3fff);
        if(fallback==3)g.set_reg(UC_X86_REG_FPCW,0x27e);
        if(fallback>=4&&fallback<=6){ // Initial source arguments overlap its first output.
            const std::uint32_t args[]={g.stop_address(),*g.u32(entry+4),*g.u32(entry+8),*g.u32(entry+12)};
            entry=fallback==4?0x3d0f800:fallback==5?0x10002000:0x10002ff4;
            require(g.write(entry,args,fallback==6?12:sizeof args),"fallback arguments");
            if(fallback==6)require(!g.u32(entry+12),"later caller argument must be unavailable");g.set_reg(UC_X86_REG_ESP,entry);
        }
    }
    void seed(const std::uint32_t* c,unsigned change=0,unsigned fail=0,unsigned branch=0,unsigned corrupt=0){
        auto& g=process->guest();entry=0;bodies=calls=0;mutation=change;stop_at=fail;fallback=branch;mutant=corrupt;observations.clear();
        const Range zeros[]={{0x3d0f804,4},{0x3d0f964,4},{0x3d0f814,4},{0x3d0f9e4,8},{0x3d0fa20,4},{0x3d0f838,4},{0x3d0fa04,16},{0x3ce7160,4},{0x3d0f820,16},{0x3ce7164,0x40},{0x3d0fd80,64},{0x3d0fd20,16},{0x389d7dc,4},{0x3d084a8,8}};
        std::array<std::uint8_t,64> zero{};for(const auto r:zeros)require(g.write(r.address,zero.data(),r.size),"seed outputs");
        for(const auto [at,value]:std::array<std::pair<std::uint32_t,std::uint32_t>,5>{{{0x3ce712c,5},{0x3d0f840,0x1234},{0x3d0f9c4,0x5678},{0x3ce71b0,0x11},{0x3ce71a4,0x22}}})g.set_u32(at,value);
        const std::uint32_t colors[]={1,2,3,4};g.write(0x3ce7174,colors,sizeof colors);
        g.write(0x3d12780,c+4,24);g.set_u32(0x3d0fd1c,c[10]);g.write(0x389d318,c+11,64);g.write(0x3d129c0,c+27,64);
        g.write(0x3d08538,c+43,8);g.set_u32(0x3d128b0,c[45]?devices[0]:0);g.write(0x88ad64,c+46,88);
        process->d3d8.transforms.clear();process->d3d8.render_states.clear();process->d3d8.render_calls=0;process->hle().clear_log();
    }
    GuestCallResult call(const std::uint32_t* c){return process->guest().call(0x780c30,{c[1],c[2],c[3]},std::nullopt,std::nullopt,static_cast<std::uint16_t>(c[0]));}
};
void compare(Fixture& source,Fixture& native,const std::uint32_t* c,bool foreign=false){
    const auto a=source.call(c),b=native.call(c);auto& x=source.process->guest();auto& y=native.process->guest();
    require(a.completed==b.completed,"completion: source "+a.error+" native "+b.error);
    require(source.bodies==1,"source main body absent");require(native.bodies==(native.fallback?1U:0U),"native/source path classification");
    require(snapshot(x)==snapshot(y),"written globals");
    require(x.u32(source.entry+4)==y.u32(native.entry+4),"changed caller argument");
    if(!foreign)require(a.esp==b.esp,"returned/stopped caller ESP source "+std::to_string(a.esp)+" native "+std::to_string(b.esp)+" source error "+a.error+" native error "+b.error);
    if(a.completed){
        const int regs[]={UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBX};
        for(const auto reg:regs)require(x.reg(reg)==y.reg(reg),"final register "+std::to_string(reg));
        require((x.reg(UC_X86_REG_EFLAGS)&0x8d5U)==(y.reg(UC_X86_REG_EFLAGS)&0x8d5U),"final integer flags");
    }
    require(source.observations.size()==native.observations.size(),"COM observation count");
    for(unsigned k=0;k<source.observations.size();++k){const auto& l=source.observations[k];const auto& r=native.observations[k];
        require(l.method==r.method,"COM method");require(l.globals==r.globals,"COM globals phase "+std::to_string(k));
        for(unsigned j=0;j<l.frame.size()&&j<r.frame.size();++j)require(l.frame[j]==r.frame[j],"COM source stack phase "+std::to_string(k)+" word "+std::to_string(j)+" source "+std::to_string(l.frame[j])+" native "+std::to_string(r.frame[j]));require(l.frame.size()==r.frame.size(),"COM frame length");require(l.matrix==r.matrix,"COM matrix/viewport phase "+std::to_string(k));
        for(unsigned j=0;j<l.registers.size();++j)require(l.registers[j]==r.registers[j],"COM register phase "+std::to_string(k)+" field "+std::to_string(j)+" source "+std::to_string(l.registers[j])+" native "+std::to_string(r.registers[j]));
    }
    const auto& l=source.process->hle().log();const auto& r=native.process->hle().log();require(l.size()==r.size(),"COM trace count");
    for(unsigned k=0;k<l.size();++k)require(l[k].dll==r[k].dll&&l[k].symbol==r[k].symbol&&l[k].return_address==r[k].return_address&&l[k].arguments==r[k].arguments&&l[k].result==r[k].result,"COM arguments/return/HRESULT");
    require(source.process->d3d8.render_states==native.process->d3d8.render_states,"backend render states");
    const auto& st=source.process->d3d8.transforms;const auto& nt=native.process->d3d8.transforms;require(st.size()==nt.size(),"backend transform count");
    for(const auto& [key,matrix]:st){const auto it=nt.find(key);require(it!=nt.end()&&std::memcmp(matrix.data(),it->second.data(),64)==0,"backend transform bits");}
    if(foreign)require(!b&&b.error.find("non-HLE")!=std::string::npos,"foreign target rejection");
}
}
int main(int argc,char** argv){try{
    require(argc==3,"usage: hle-screen-init-bridge-test SONIC.EXE SCREEN_INIT.CASES.BIN");
    std::ifstream exe(argv[1],std::ios::binary);std::string error;const auto image=ExecutableImage::parse({std::istreambuf_iterator<char>(exe),{}},&error);require(image.has_value(),error);
    std::ifstream file(argv[2],std::ios::binary);const std::string bytes((std::istreambuf_iterator<char>(file)),{});
    require(!bytes.empty()&&bytes.size()%(68*4)==0,"screen corpus layout");std::vector<std::uint32_t> corpus(bytes.size()/4);std::memcpy(corpus.data(),bytes.data(),bytes.size());
    require(corpus.size()==3000*68,"complete 3000-case screen corpus required");
    Fixture source(*image,false),native(*image,true);std::size_t count=0;
    for(std::size_t i=0;i<corpus.size();i+=68){source.seed(corpus.data()+i);native.seed(corpus.data()+i);try{compare(source,native,corpus.data()+i);}catch(const std::exception& e){throw std::runtime_error("corpus case "+std::to_string(count)+": "+e.what());}++count;}
    std::array<std::uint32_t,68> live{};std::memcpy(live.data(),corpus.data(),sizeof live);live[0]=0x27f;live[1]=0x30;live[3]=1;live[6]=640;live[7]=480;live[10]=live[45]=1;
    unsigned mutations=0,stops=0,branches=0,foreign=0,caught=0;
    for(unsigned m=1;m<=9;++m){source.seed(live.data(),m);native.seed(live.data(),m);try{compare(source,native,live.data());}catch(const std::exception& e){throw std::runtime_error("mutation "+std::to_string(m)+": "+e.what());}++mutations;}
    for(unsigned phase=1;phase<=5;++phase){source.seed(live.data(),0,phase);native.seed(live.data(),0,phase);compare(source,native,live.data());require(source.calls==phase&&native.calls==phase,"backend stop phase");++stops;}
    for(unsigned branch=1;branch<=6;++branch){source.seed(live.data(),0,0,branch);native.seed(live.data(),0,0,branch);compare(source,native,live.data());++branches;}
    const unsigned offsets[]={0xa0,0x94,0x9c,0xc8};
    for(unsigned k=0;k<4;++k){
        source.seed(live.data());native.seed(live.data());
        for(auto* f:{&source,&native})f->process->guest().set_u32(*f->process->guest().u32(f->devices[0])+offsets[k],0x10000000);
        compare(source,native,live.data(),true);
        for(auto* f:{&source,&native})f->process->guest().set_u32(*f->process->guest().u32(f->devices[0])+offsets[k],f->targets[k]);++foreign;
    }
    for(unsigned mutant=1;mutant<=5;++mutant){source.seed(live.data());native.seed(live.data(),0,0,0,mutant);try{compare(source,native,live.data());}catch(const std::exception&){++caught;}}
    require(caught==5,"bridge data/path mutants survived");
    std::cout<<"Screen initialization bridge: "<<count<<" corpus cases, "<<mutations<<" callback mutations, "<<stops<<" backend stops, "<<branches<<" source fallbacks, "<<foreign<<" foreign methods and "<<caught<<" caught mutants; globals, COM frames/arguments/HRESULTs, callback registers and caller stack agree\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
