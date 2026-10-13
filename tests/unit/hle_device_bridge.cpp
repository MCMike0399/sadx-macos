#include "hle/hle_device_bridge.hpp"
#include "hle/hle_process.hpp"
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
namespace {
using namespace sadx;
void require(bool ok,const std::string& why){if(!ok)throw std::runtime_error(why);}
std::vector<std::uint32_t> corpus(const std::string& path,unsigned width){
    std::ifstream f(path,std::ios::binary);require(bool(f),path);std::string bytes((std::istreambuf_iterator<char>(f)),{});
    require(!bytes.empty()&&bytes.size()%(width*4)==0,"corpus layout");std::vector<std::uint32_t> out(bytes.size()/4);std::memcpy(out.data(),bytes.data(),bytes.size());return out;
}
struct Fixture {
    std::unique_ptr<HleProcess> p;std::uint32_t devices[2]={};std::uint32_t boundary=0;
    bool mapped_blend=true,mapped_device=true;unsigned calls=0,mutation=0,fail_at=0;
    std::vector<std::pair<std::optional<std::uint32_t>,std::optional<std::uint32_t>>> observed;
    std::uint32_t receipts[2]={};std::uint16_t cw=0x27f;bool argument_mutant=false;
    Fixture(const ExecutableImage& image,bool native){
        std::string error;p=HleProcess::create(".","build/device-bridge-overlay",&error);require(p!=nullptr,error);require(p->map_main(image,&error),error);
        auto& g=p->guest();const auto create=p->hle().trap(g,"d3d8.dll","Direct3DCreate8",&error);require(create.has_value(),error);
        const auto d3d=g.call(*create,{0xdc});require(bool(d3d),d3d.error);
        const auto params=p->memory().allocate(52),out=p->memory().allocate(4);require(params&&out,"memory");
        const std::uint32_t pp[13]={1,1,22,1,0,1,0,1,0,0,0,0,0};g.write(*params,pp,sizeof pp);
        const auto table=g.u32(d3d.eax),method=table?g.u32(*table+60):std::nullopt;require(method.has_value(),"CreateDevice method");
        for(auto& device:devices){const auto r=g.call(*method,{d3d.eax,0,1,0,0x20,*params,*out});require(bool(r),r.error);device=g.u32(*out).value_or(0);require(device!=0,"device");}
        auto state=*p->hle().find("com:IDirect3DDevice8","SetRenderState");const auto body=state.body;
        state.body=[this,body](HleCall& c)->std::uint32_t{
            observed.push_back({c.guest.u32(0x3ab9890),c.guest.u32(0x3d12898)});
            const auto result=body(c);const auto index=calls++;
            if(index==0){if(mutation==1)c.guest.set_u32(0x3d128b0,devices[1]);if(mutation==2){std::string e;require(c.guest.unmap(0x3d12000,4096,&e),e);mapped_device=false;}}
            if(fail_at&&calls==fail_at)c.guest.stop("backend unavailable");
            return index<2?receipts[index]:result;
        };
        p->hle().add("com:IDirect3DDevice8","SetRenderState",state);
        for(const auto at:{0x77ecb7U,0x77eccaU})
            require(g.add_instruction_hook(at,[this](X86Guest& gg,std::uint32_t pc){if(boundary==pc)gg.stop("declared port boundary");return false;},&error),error);
        if(native)for(auto entry:device_overrides(*p)){
            const auto bridge=entry.bridge;const auto index=entry.address==0x7949b0?0U:1U;
            require(g.add_override(entry.address,[this,bridge,index](X86Guest& guest){
                if(argument_mutant){const auto at=guest.esp()+4+4*index;guest.set_u32(at,guest.u32(at).value_or(0)^1);}
                return bridge(guest);},&error),error);
        }
    }
    void reset(bool blend_known=true,bool device_known=true){
        auto& g=p->guest();std::string e;
        if(!mapped_blend){require(g.map(0x3ab9000,4096,false,&e),e);mapped_blend=true;}
        if(!mapped_device){require(g.map(0x3d12000,4096,false,&e),e);mapped_device=true;}
        if(!blend_known){require(g.unmap(0x3ab9000,4096,&e),e);mapped_blend=false;}
        if(!device_known){require(g.unmap(0x3d12000,4096,&e),e);mapped_device=false;}
        p->d3d8.render_states.clear();p->d3d8.render_calls=0;p->hle().clear_log();calls=0;observed.clear();boundary=mutation=fail_at=0;argument_mutant=false;cw=0x27f;receipts[0]=receipts[1]=0;
    }
};
void compare(Fixture& a,Fixture& b,std::uint32_t entry,const std::vector<std::uint32_t>& args,bool mutant=false){
    const auto x=a.p->guest().call(entry,args,std::nullopt,std::nullopt,a.cw),y=b.p->guest().call(entry,args,std::nullopt,std::nullopt,b.cw);
    require(x.completed==y.completed,"completion mismatch at "+std::to_string(entry));
    if(a.boundary)require(y.error.find(a.boundary==0x77ecb7?"77ecb7":"77ecca")!=std::string::npos,"declared boundary missing");
    require(a.p->guest().fpu_status()==b.p->guest().fpu_status()&&a.p->guest().mxcsr()==b.p->guest().mxcsr(),"FP state mismatch");
    require(a.calls==b.calls,"render call count");
    require(a.observed==b.observed,"global store ordering at COM calls");
    require(a.p->d3d8.render_states==b.p->d3d8.render_states,"device states");
    const auto& l=a.p->hle().log();const auto& r=b.p->hle().log();require(l.size()==r.size(),"COM trace length");
    for(std::size_t i=0;i<l.size();++i)require(l[i].dll==r[i].dll&&l[i].symbol==r[i].symbol&&l[i].return_address==r[i].return_address&&l[i].arguments==r[i].arguments&&l[i].result==r[i].result,"COM trace details");
    std::uint8_t old=0,now=0;const auto has=a.p->guest().read(0x3ab9890,&old,1),got=b.p->guest().read(0x3ab9890,&now,1);
    require(has==got&&(!has||old==now),"packed blend");
    if(entry==0x7949b0)require(a.p->guest().u32(0x3d12898)==b.p->guest().u32(0x3d12898),"fog colour");
    (void)mutant;
}
}
int main(int argc,char** argv){try{
    require(argc==3,"usage: hle-device-bridge-test SONIC.EXE ORACLE_DIR");
    std::ifstream file(argv[1],std::ios::binary);std::string error;const auto image=ExecutableImage::parse({std::istreambuf_iterator<char>(file),{}},&error);require(image.has_value(),error);
    Fixture original(*image,false),native(*image,true);std::size_t blend_cases=0,fog_cases=0;
    const auto blend=corpus(std::string(argv[2])+"/ninja_render.cases.bin",8);
    for(const auto entry:{0x4030d0U,0x77ec60U})for(std::size_t i=0;i<blend.size();i+=8){
        const auto* c=blend.data()+i;
        for(auto* f:{&original,&native}){
            f->reset(entry==0x77ec60||c[3]!=0,c[4]!=0);auto& g=f->p->guest();
            if(f->mapped_blend){const auto byte=static_cast<std::uint8_t>(c[2]);g.write(0x3ab9890,&byte,1);}
            if(f->mapped_device)g.set_u32(0x3d128b0,c[4]==1?f->devices[0]:0);
            f->receipts[0]=c[5];f->receipts[1]=c[6];f->mutation=c[7];
            if(c[1]>=16)f->boundary=0x77ecb7;else if(c[0]>=2&&c[1]>=2)f->boundary=0x77ecca;
        }
        compare(original,native,entry,{c[0],c[1]});++blend_cases;
    }
    const auto fog=corpus(std::string(argv[2])+"/fog.cases.bin",416);
    for(std::size_t i=0;i<fog.size();i+=416)if(fog[i]==1){
        const auto* c=fog.data()+i;
        for(auto* f:{&original,&native}){f->reset();f->cw=static_cast<std::uint16_t>(c[1]);f->fail_at=c[2];f->p->guest().set_u32(0x3d128b0,f->devices[0]);f->p->guest().set_u32(0x3d12898,c[7]);f->p->guest().set_u32(0x3d128a4,c[5]);}
        compare(original,native,0x7949b0,{c[15]});++fog_cases;
    }
    unsigned caught=0;
    for(const auto entry:{0x4030d0U,0x77ec60U,0x7949b0U}){
        for(auto* f:{&original,&native}){f->reset();const std::uint8_t zero=0;f->p->guest().write(0x3ab9890,&zero,1);f->p->guest().set_u32(0x3d128b0,f->devices[0]);f->p->guest().set_u32(0x3d12898,0);f->p->guest().set_u32(0x3d128a4,1);}
        native.argument_mutant=true;
        try{compare(original,native,entry,entry==0x7949b0?std::vector<std::uint32_t>{0x123456}:std::vector<std::uint32_t>{0,0});}
        catch(const std::exception&){++caught;}
    }
    require(caught==3,"bridge argument mutants not caught");
    native.reset();native.p->guest().set_u32(0x3d128b0,native.devices[0]);
    const auto vtable=native.p->guest().u32(native.devices[0]);require(vtable.has_value(),"vtable");
    native.p->guest().set_u32(*vtable+0xc8,0x10000000);
    const auto rejected=native.p->guest().call(0x77ec60,{0,2});
    require(!rejected&&rejected.error.find("non-HLE")!=std::string::npos&&native.calls==0,"foreign device target not rejected");
    std::cout<<"Device bridge: "<<blend_cases<<" blend wrapper/leaf and "<<fog_cases<<" fog-colour oracle cases equivalent; identical globals, HLE device states and COM traces/HRESULTs; unknown/null/live-changing devices and declared boundaries verified; 3 bridge argument mutants caught; foreign vtable rejected\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
