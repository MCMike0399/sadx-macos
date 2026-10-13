#include "hle/hle_device_bridge.hpp"
#include "hle/hle_process.hpp"
#include "guest/fpu_shims.hpp"
#include <array>
#include <algorithm>
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
    FpuShimStats shims;std::unique_ptr<HleProcess> p;std::uint32_t devices[2]={};std::uint32_t boundary=0;
    bool mapped_blend=true,mapped_device=true;unsigned calls=0,mutation=0,fail_at=0;std::vector<std::array<std::uint32_t,32>> dispatch_observed;std::vector<std::array<std::uint32_t,396>> fog_observed;
    std::vector<std::pair<std::optional<std::uint32_t>,std::optional<std::uint32_t>>> observed;
    std::vector<std::array<std::uint32_t,6>> range_observed;
    std::uint32_t receipts[2]={};std::uint16_t cw=0x27f;bool argument_mutant=false;
    Fixture(const ExecutableImage& image,bool native){
        std::string error;p=HleProcess::create(".","build/device-bridge-overlay",&error);require(p!=nullptr,error);require(p->map_main(image,&error),error);
        auto& g=p->guest();auto sites=load_fpu_sites("reference/inventory/fpu-sites.json");

        require(install_fpu_shims(g,sites,shims,&error),error);require(g.map(0x10000000,4096,false,&error),error);const auto create=p->hle().trap(g,"d3d8.dll","Direct3DCreate8",&error);require(create.has_value(),error);
        const auto d3d=g.call(*create,{0xdc});require(bool(d3d),d3d.error);
        const auto params=p->memory().allocate(52),out=p->memory().allocate(4);require(params&&out,"memory");
        const std::uint32_t pp[13]={1,1,22,1,0,1,0,1,0,0,0,0,0};g.write(*params,pp,sizeof pp);
        const auto table=g.u32(d3d.eax),method=table?g.u32(*table+60):std::nullopt;require(method.has_value(),"CreateDevice method");
        for(auto& device:devices){const auto r=g.call(*method,{d3d.eax,0,1,0,0x20,*params,*out});require(bool(r),r.error);device=g.u32(*out).value_or(0);require(device!=0,"device");}
        auto registry=*p->hle().find("ADVAPI32.dll","RegOpenKeyA");const auto registry_body=registry.body;
        registry.body=[this,registry_body](HleCall& c){std::array<std::uint32_t,32> snapshot{};c.guest.read(0x3d12960,snapshot.data(),64);c.guest.read(0x3d129c0,snapshot.data()+16,64);dispatch_observed.push_back(snapshot);return registry_body(c);};
        p->hle().add("ADVAPI32.dll","RegOpenKeyA",registry);
        auto state=*p->hle().find("com:IDirect3DDevice8","SetRenderState");const auto body=state.body;
        state.body=[this,body](HleCall& c)->std::uint32_t{
            observed.push_back({c.guest.u32(0x3ab9890),c.guest.u32(0x3d12898)});
            std::array<std::uint32_t,396> snapshot{};
            const std::uint32_t addresses[8]={0x3d12798,0x3d128a4,0x3d128a0,0x3d12898,0x3d0fce0,0x3d12950,0x3d0fac0,0x3d12894};
            for(unsigned k=0;k<8;++k)snapshot[k]=c.guest.u32(addresses[k]).value_or(0xdeadbeef);
            c.guest.read(0x3d12a00,snapshot.data()+8,512);c.guest.read(0x3d12c00,snapshot.data()+136,512);c.guest.read(0x3b10f78,snapshot.data()+264,512);
            snapshot[392]=c.guest.u32(0x3d0849c).value_or(0);snapshot[393]=c.guest.u32(0x90a098).value_or(0);snapshot[394]=c.guest.u32(0x90a09c).value_or(0);
            std::uint8_t flag=0;c.guest.read(0x3abdcfe,&flag,1);snapshot[395]=flag;fog_observed.push_back(snapshot);
            const auto result=body(c);const auto index=calls++;
            if(index==0){if(mutation==1)c.guest.set_u32(0x3d128b0,devices[1]);if(mutation==2){std::string e;require(c.guest.unmap(0x3d12000,4096,&e),e);mapped_device=false;}}
            if(fail_at&&calls==fail_at)c.guest.stop("backend unavailable");
            return index<2?receipts[index]:result;
        };
        p->hle().add("com:IDirect3DDevice8","SetRenderState",state);
        for(const auto* name:{"GetTransform","SetTransform"}){
            auto method=*p->hle().find("com:IDirect3DDevice8",name);const auto transform_body=method.body;
            method.body=[this,transform_body,name](HleCall& c){
                std::array<std::uint32_t,6> snapshot{};const std::uint32_t addresses[6]={0x3d0f808,0x3d0f968,0x90082c,0x900830,0x3aad0c0,0x3aad0c4};
                for(unsigned k=0;k<6;++k)snapshot[k]=c.guest.u32(addresses[k]).value_or(0);range_observed.push_back(snapshot);
                const auto result=transform_body(c);
                if(std::string(name)=="GetTransform"){
                    if(mutation==1)c.guest.set_u32(0x3d128b0,devices[1]);
                    if(fail_at==1)c.guest.stop("backend unavailable");
                    return receipts[0];
                }
                return result;
            };p->hle().add("com:IDirect3DDevice8",name,method);
        }
        for(const auto at:{0x77ecb7U,0x77eccaU,0x78753aU})
            require(g.add_instruction_hook(at,[this](X86Guest& gg,std::uint32_t pc){if(boundary==pc)gg.stop("declared port boundary");return false;},&error),error);
        if(native)for(auto entry:device_overrides(*p)){
            const auto bridge=entry.bridge;const auto index=entry.address==0x7949b0||entry.address==0x781250||entry.address==0x791170||entry.address==0x794e40?0U:1U;
            const auto initialize=entry.address==0x794ec0;const auto disable=entry.address==0x411b40,enable=entry.address==0x411af0;
            require(g.add_override(entry.address,[this,bridge,index,initialize,disable,enable](X86Guest& guest){
                if(argument_mutant){if(disable)guest.set_u32(0x3d12798,0);else if(enable)guest.set_u32(0x3b1117c,1);else if(initialize)guest.set_u32(0x3d127e4,guest.u32(0x3d127e4).value_or(0)^0x100000);else{const auto at=guest.esp()+4+4*index;guest.set_u32(at,guest.u32(at).value_or(0)^1);}}
                return bridge(guest);},&error),error);
        }
    }
    void reset(bool blend_known=true,bool device_known=true){
        auto& g=p->guest();std::string e;
        if(!mapped_blend){require(g.map(0x3ab9000,4096,false,&e),e);mapped_blend=true;}
        if(!mapped_device){require(g.map(0x3d12000,4096,false,&e),e);mapped_device=true;}
        if(!blend_known){require(g.unmap(0x3ab9000,4096,&e),e);mapped_blend=false;}
        if(!device_known){require(g.unmap(0x3d12000,4096,&e),e);mapped_device=false;}
        p->d3d8.transforms.clear();p->d3d8.render_states.clear();p->d3d8.render_calls=0;p->hle().clear_log();calls=0;observed.clear();range_observed.clear();dispatch_observed.clear();fog_observed.clear();boundary=mutation=fail_at=0;argument_mutant=false;cw=0x27f;receipts[0]=receipts[1]=0;
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
    std::size_t fog_init_cases=0,fog_table_cases=0,fog_generate_cases=0,fog_switch_cases=0,fog_explicit=0;
    const std::array<std::pair<std::uint32_t,unsigned>,10> fog_outputs={{{0x3d12798,4},{0x3d128a4,4},{0x3d128a0,4},{0x3d12898,4},{0x3d0fce0,4},{0x3d12950,4},{0x3d0fac0,4},{0x3d12894,4},{0x3d12a00,512},{0x3d12c00,512}}};
    for(std::size_t i=0;i<fog.size();i+=416)if(fog[i]==0||fog[i]==2||fog[i]==3||fog[i]==4){
        const auto* c=fog.data()+i;const auto entry=c[0]==0?0x794ec0U:c[0]==2?0x787260U:c[0]==3?0x794e40U:c[26]?0x411af0U:0x411b40U;
        for(auto* f:{&original,&native}){
            f->reset();auto& g=f->p->guest();f->fail_at=c[2];
            g.set_u32(0x3d128b0,f->devices[0]);
            const std::uint32_t at[11]={0x3d12798,0x3d128a4,0x3d128a0,0x3d12898,0x3d0fce0,0x3d12950,0x3d0fac0,0x3d12894,0x3d0849c,0x3d127e4,0x3d08ae0};
            for(unsigned k=0;k<11;++k)g.set_u32(at[k],c[4+k]);
            g.set_u32(0x3b1117c,c[17]);const auto profile=static_cast<std::uint8_t>(c[18]),special=static_cast<std::uint8_t>(c[19]),flag=static_cast<std::uint8_t>(c[25]);
            g.write(0x3abdc6c,&profile,1);g.write(0x909eb4,&special,1);g.write(0x3abdcfe,&flag,1);
            g.set_u32(0x90a098,c[20]);g.set_u32(0x90a09c,c[21]);g.set_u32(0x3abdc60,c[22]);g.set_u32(0x3abdc64,c[23]);g.set_u32(0x3abdc68,c[24]);g.set_u32(0x3d0f400,c[27]);
            g.write(0x3d12a00,c+32,512);g.write(0x3d12c00,c+160,512);g.write(0x3b10f78,c+288,512);
        }
        const std::vector<std::uint32_t> args=c[0]==2?std::vector<std::uint32_t>{0x3b10f78,c[15],c[16]}:c[0]==3?std::vector<std::uint32_t>{0x3b10f78}:std::vector<std::uint32_t>{};
        const auto b=native.p->guest().call(entry,args,std::nullopt,std::nullopt,static_cast<std::uint16_t>(c[1]));
        if(!b&&b.error.find("78753a")!=std::string::npos){original.boundary=0x78753a;++fog_explicit;}
        const auto a=original.p->guest().call(entry,args,std::nullopt,std::nullopt,static_cast<std::uint16_t>(c[1]));
        require(a.completed==b.completed,"fog init/table completion");
        const auto& l=original.p->hle().log();const auto& r=native.p->hle().log();require(l.size()==r.size(),"fog init/table trace count");
        for(unsigned k=0;k<l.size();++k)require(l[k].dll==r[k].dll&&l[k].symbol==r[k].symbol&&l[k].return_address==r[k].return_address&&l[k].arguments==r[k].arguments&&l[k].result==r[k].result,"fog init/table COM trace");
        require(original.p->d3d8.render_states==native.p->d3d8.render_states,"fog init/table device state");
        require(original.fog_observed==native.fog_observed,"fog scalar/table order at COM calls");
        for(const auto& [at,size]:fog_outputs){std::vector<std::uint8_t> x(size),y(size);original.p->guest().read(at,x.data(),size);native.p->guest().read(at,y.data(),size);require(x==y,"fog stores "+std::to_string(at)+" case "+std::to_string(i/416));}
        for(const auto at:{0x3d0849cU,0x90a098U,0x90a09cU})require(original.p->guest().u32(at)==native.p->guest().u32(at),"fog switch globals");
        std::uint8_t original_flag=0,native_flag=0;original.p->guest().read(0x3abdcfe,&original_flag,1);native.p->guest().read(0x3abdcfe,&native_flag,1);require(original_flag==native_flag,"fog switch byte");
        std::array<std::uint32_t,128> source_table{},native_table{};original.p->guest().read(0x3b10f78,source_table.data(),512);native.p->guest().read(0x3b10f78,native_table.data(),512);require(source_table==native_table,"generated fog table");
        if(c[0]==0)++fog_init_cases;else if(c[0]==3)++fog_table_cases;else if(c[0]==2)++fog_generate_cases;else ++fog_switch_cases;
    }
    const auto view=corpus(std::string(argv[2])+"/ninja_view.cases.bin",37);
    std::size_t view_cases=0,projection_cases=0;
    const std::array<std::pair<std::uint32_t,unsigned>,9> outputs={{{0x3d0f760,64},{0x3d0f980,64},{0x3d0f9e0,4},{0x3d0f808,4},{0x3d0f968,4},{0x3d12960,64},{0x3d129c0,64},{0x389d398,64},{0x389d358,64}}};
    for(const auto entry:{0x781250U,0x791170U})for(std::size_t i=0;i<view.size();i+=37){
        const auto* c=view.data()+i;
        for(auto* f:{&original,&native}){
            f->reset();auto& g=f->p->guest();g.set_u32(0x3d128b0,f->devices[0]);
            g.set_u32(0x389b784,i/37%300==0?image->u32(0x389b784).value_or(0):0x774310);
            g.set_u32(0x389b7c0,i/37%300==0?image->u32(0x389b7c0).value_or(0):0x7757e0);
            g.set_u32(0x389b948,i/37%300==0?0xffff:0);
            for(const auto& [at,size]:outputs){std::vector<std::uint8_t> bytes(size,0xa5);g.write(at,bytes.data(),bytes.size());}
            g.write(0x3aad0a0,c+3,40);g.write(0x3d12780,c+13,24);g.write(0x3d0fd80,c+19,64);
            std::array<std::uint32_t,64> slots;slots.fill(0xa5a5a5a5);g.write(0x10000100,slots.data(),sizeof slots);
            g.set_u32(0x3d0fa1c,0x10000100);g.set_u32(0x3d0f818,0x10000100+64*(c[36]-1));g.set_u32(0x3d0fa2c,c[36]);
            if(c[2]==0){const auto r=g.call(0x781570,{},0x3aad0a0);require(bool(r),r.error);}
            f->p->hle().clear_log();
        }
        std::vector<std::uint32_t> args={0x3aad0a0};
        if(entry==0x791170){
            X87Context fp(static_cast<std::uint16_t>(c[0]));
            const auto angle=static_cast<std::int32_t>(c[2]==0?0x2aaaU:c[10]);
            const auto fov=fp.to_float(fp.multiply_double_memory(fp.from_int64(angle),9.5873779296875e-05));
            std::uint32_t bits;std::memcpy(&bits,&fov,4);args={bits,c[11],c[2]==0?0x476a6000U:c[12]^0x80000000U};
        }
        const auto a=original.p->guest().call(entry,args,std::nullopt,std::nullopt,static_cast<std::uint16_t>(c[0]),600'000'000,c[1]);
        const auto b=native.p->guest().call(entry,args,std::nullopt,std::nullopt,static_cast<std::uint16_t>(c[0]),600'000'000,c[1]);
        require(bool(a),"original view: "+a.error);require(bool(b),"native view: "+b.error);
        for(const auto& [at,size]:outputs){
            std::vector<std::uint8_t> x(size),y(size);original.p->guest().read(at,x.data(),size);native.p->guest().read(at,y.data(),size);
            require(x==y,"view stores at "+std::to_string(at)+" case "+std::to_string(i/37)+" entry "+std::to_string(entry));
        }
        std::array<std::uint32_t,64> x{},y{};original.p->guest().read(0x10000100,x.data(),sizeof x);native.p->guest().read(0x10000100,y.data(),sizeof y);
        require(x==y&&original.p->guest().u32(0x3d0f818)==native.p->guest().u32(0x3d0f818)&&original.p->guest().u32(0x3d0fa2c)==native.p->guest().u32(0x3d0fa2c),"matrix stack");
        const auto& l=original.p->hle().log();const auto& r=native.p->hle().log();require(l.size()==r.size(),"transform trace count");
        require(original.dispatch_observed==native.dispatch_observed,"perspective/projection ordering at dispatch initialization");
        for(unsigned k=0;k<l.size();++k)require(l[k].dll==r[k].dll&&l[k].symbol==r[k].symbol&&l[k].return_address==r[k].return_address&&l[k].arguments==r[k].arguments&&l[k].result==r[k].result,"transform COM trace");
        require(original.p->d3d8.transforms.size()==native.p->d3d8.transforms.size(),"transform matrix count");
        for(const auto& [state,matrix]:original.p->d3d8.transforms){const auto found=native.p->d3d8.transforms.find(state);require(found!=native.p->d3d8.transforms.end()&&std::memcmp(matrix.data(),found->second.data(),64)==0,"transform device matrix bits");}
        if(entry==0x781250)++view_cases;else ++projection_cases;
    }
    const auto range=corpus(std::string(argv[2])+"/projection_range.cases.bin",23);
    std::size_t range_cases=0;
    const auto check_range=[&](const std::uint32_t* c,bool mutant){
        for(auto* f:{&original,&native}){
            f->reset();auto& g=f->p->guest();g.set_u32(0x3d128b0,f->devices[0]);g.set_u32(0x3aad0c0,c[2]);g.set_u32(0x3aad0c4,c[3]);
            g.set_u32(0x7dcb0c,c[5]);g.set_u32(0x7dce50,c[6]);
            NinjaMatrix matrix{};std::memcpy(matrix.data(),c+7,64);f->p->d3d8.transforms[3]=matrix;
            f->mutation=range_cases%100==0?1:0;f->fail_at=range_cases%200==0?1:0;f->receipts[0]=range_cases%50==0?0x80004005U:0;
        }
        native.argument_mutant=mutant;
        const auto a=original.p->guest().call(0x403180,{c[0],c[1]},std::nullopt,std::nullopt,static_cast<std::uint16_t>(c[4]));
        const auto b=native.p->guest().call(0x403180,{c[0],c[1]},std::nullopt,std::nullopt,static_cast<std::uint16_t>(c[4]));
        require(a.completed==b.completed,"range completion");
        for(const auto at:{0x3d0f808U,0x3d0f968U,0x90082cU,0x900830U,0x3aad0c0U,0x3aad0c4U})require(original.p->guest().u32(at)==native.p->guest().u32(at),"range globals case "+std::to_string(range_cases));
        require(original.range_observed==native.range_observed,"range stores before COM");
        const auto& l=original.p->hle().log();const auto& r=native.p->hle().log();require(l.size()==r.size(),"range COM count");
        for(unsigned k=0;k<l.size();++k)require(l[k].dll==r[k].dll&&l[k].symbol==r[k].symbol&&l[k].return_address==r[k].return_address&&l[k].arguments==r[k].arguments&&l[k].result==r[k].result,"range COM pointers/receipt");
        require(std::memcmp(original.p->d3d8.transforms[3].data(),native.p->d3d8.transforms[3].data(),64)==0,"range matrix bits");
    };
    for(std::size_t i=0;i<range.size();i+=23){check_range(range.data()+i,false);++range_cases;}
    unsigned caught=0;
    for(const auto entry:{0x4030d0U,0x77ec60U,0x7949b0U}){
        for(auto* f:{&original,&native}){f->reset();const std::uint8_t zero=0;f->p->guest().write(0x3ab9890,&zero,1);f->p->guest().set_u32(0x3d128b0,f->devices[0]);f->p->guest().set_u32(0x3d12898,0);f->p->guest().set_u32(0x3d128a4,1);}
        native.argument_mutant=true;
        try{compare(original,native,entry,entry==0x7949b0?std::vector<std::uint32_t>{0x123456}:std::vector<std::uint32_t>{0,0});}
        catch(const std::exception&){++caught;}
    }
    require(caught==3,"bridge argument mutants not caught");
    for(const auto entry:{0x781250U,0x791170U}){
        for(auto* f:{&original,&native}){
            f->reset();auto& g=f->p->guest();g.set_u32(0x3d128b0,f->devices[0]);g.set_u32(0x389b784,0x774310);g.set_u32(0x389b7c0,0x7757e0);
            g.set_u32(0x3d0fa1c,0x10000100);g.set_u32(0x3d0f818,0x10000100);g.set_u32(0x3d0fa2c,1);
            const std::uint32_t viewport[6]={0,0,640,480,0,0x3f800000};g.write(0x3d12780,viewport,24);
            std::array<std::uint32_t,16> unit{};unit[0]=unit[5]=unit[10]=unit[15]=0x3f800000;g.write(0x10000100,unit.data(),64);g.write(0x3d0fd80,unit.data(),64);
            require(bool(g.call(0x781570,{},0x3aad0a0)),"camera init");
        }
        native.argument_mutant=true;const std::vector<std::uint32_t> args=entry==0x781250?std::vector<std::uint32_t>{0x3aad0a0}:std::vector<std::uint32_t>{0x3f800000,0x3f800000,0x476a6000};
        const auto a=original.p->guest().call(entry,args),b=native.p->guest().call(entry,args);
        bool differs=a.completed!=b.completed;
        for(const auto& [at,size]:outputs){std::vector<std::uint8_t> x(size),y(size);original.p->guest().read(at,x.data(),size);native.p->guest().read(at,y.data(),size);differs=differs||x!=y;}
        if(differs)++caught;
    }
    require(caught==5,"view/projection argument mutants not caught");
    for(const auto entry:{0x794ec0U,0x794e40U}){
        for(auto* f:{&original,&native}){
            f->reset();auto& g=f->p->guest();g.set_u32(0x3d128b0,f->devices[0]);g.set_u32(0x3d127e4,0x100000);g.set_u32(0x3d08ae0,0);g.set_u32(0x3d128a4,1);
            std::array<std::uint32_t,128> values{};
            for(unsigned k=0;k<128;++k){const auto v=float(k+1);std::memcpy(&values[k],&v,4);}
            g.write(0x3d12a00,values.data(),512);g.write(0x3d12c00,values.data(),512);values.fill(0);values[0]=0x3f800000;values[1]=0x3f000000;g.write(0x3b10f78,values.data(),512);
        }
        native.argument_mutant=true;const auto args=entry==0x794ec0?std::vector<std::uint32_t>{}:std::vector<std::uint32_t>{0x3b10f78};
        const auto a=original.p->guest().call(entry,args),b=native.p->guest().call(entry,args);
        bool differs=a.completed!=b.completed||original.p->d3d8.render_states!=native.p->d3d8.render_states;
        for(const auto& [at,size]:fog_outputs){std::vector<std::uint8_t> x(size),y(size);original.p->guest().read(at,x.data(),size);native.p->guest().read(at,y.data(),size);differs=differs||x!=y;}
        if(differs)++caught;
    }
    require(caught==7,"fog bridge mutants not caught");
    for(const auto entry:{0x787260U,0x411af0U,0x411b40U}){
        for(auto* f:{&original,&native}){
            f->reset();auto& g=f->p->guest();g.set_u32(0x3d128b0,f->devices[0]);g.set_u32(0x3d127e4,0x100000);g.set_u32(0x3d08ae0,0);g.set_u32(0x3d128a4,1);g.set_u32(0x3d12798,1);g.set_u32(0x3b1117c,0);
            const std::uint8_t one=1,zero=0;g.write(0x3abdc6c,&one,1);g.write(0x909eb4,&zero,1);g.write(0x3abdcfe,&one,1);
            g.set_u32(0x3abdc60,0x43960000);g.set_u32(0x3abdc64,0x453b8000);g.set_u32(0x3abdc68,0x123456);
            g.set_u32(0x90a098,0);g.set_u32(0x90a09c,0);
            std::array<std::uint32_t,128> values{};
            for(unsigned k=0;k<128;++k){const auto v=float(k+1);std::memcpy(&values[k],&v,4);}
            g.write(0x3d12a00,values.data(),512);g.write(0x3d12c00,values.data(),512);values.fill(0);g.write(0x3b10f78,values.data(),512);
        }
        native.argument_mutant=true;const auto args=entry==0x787260?std::vector<std::uint32_t>{0x3b10f78,0xc3960000,0xc53b8000}:std::vector<std::uint32_t>{};
        const auto a=original.p->guest().call(entry,args),b=native.p->guest().call(entry,args);
        bool differs=a.completed!=b.completed||original.p->d3d8.render_states!=native.p->d3d8.render_states;
        for(const auto& [at,size]:fog_outputs){std::vector<std::uint8_t> x(size),y(size);original.p->guest().read(at,x.data(),size);native.p->guest().read(at,y.data(),size);differs=differs||x!=y;}
        std::array<std::uint32_t,128> x{},y{};original.p->guest().read(0x3b10f78,x.data(),512);native.p->guest().read(0x3b10f78,y.data(),512);differs=differs||x!=y;
        if(differs)++caught;
    }
    require(caught==10,"fog generation/switch mutants not caught");
    bool range_mutant_caught=false;
    for(std::size_t i=0;i<range.size()&&!range_mutant_caught;i+=23){try{check_range(range.data()+i,true);}catch(const std::exception&){range_mutant_caught=true;}}
    require(range_mutant_caught,"range argument mutant not caught");
    native.reset();native.p->guest().set_u32(0x3d128b0,native.devices[0]);
    const auto vtable=native.p->guest().u32(native.devices[0]);require(vtable.has_value(),"vtable");
    native.p->guest().set_u32(*vtable+0xc8,0x10000000);
    const auto rejected=native.p->guest().call(0x77ec60,{0,2});
    require(!rejected&&rejected.error.find("non-HLE")!=std::string::npos&&native.calls==0,"foreign device target not rejected");
    for(const auto offset:{0x98U,0x94U}){
        native.reset();auto& g=native.p->guest();g.set_u32(0x3d128b0,native.devices[0]);g.set_u32(0x3aad0c0,0);g.set_u32(0x3aad0c4,0);
        const auto target=g.u32(*vtable+offset);require(target.has_value(),"transform target");g.set_u32(*vtable+offset,0x10000000);
        const auto bad=g.call(0x403180,{0xc0000000,0xc2c80000});
        require(!bad&&bad.error.find("non-HLE")!=std::string::npos,"foreign transform not rejected");g.set_u32(*vtable+offset,*target);
    }
    native.reset();auto& g=native.p->guest();g.set_u32(0x3d128b0,0);g.set_u32(0x3aad0c0,0xc0000000);g.set_u32(0x3aad0c4,0xc2c80000);
    require(bool(g.call(0x403180,{0xc0000000,0xc2c80000}))&&native.p->hle().log().empty(),"unchanged range must skip null device");
    g.set_u32(0x3aad0c0,0);
    const auto absent=g.call(0x403180,{0xc0000000,0xc2c80000});
    require(!absent&&absent.error.find("403207")!=std::string::npos&&g.u32(0x3aad0c0)==0xc0000000&&native.p->hle().log().empty(),"null range device boundary/prefix");
    std::cout<<"Device bridge: "<<blend_cases<<" blend wrapper/leaf and "<<fog_cases<<" fog-colour oracle cases equivalent; identical globals, HLE device states and COM traces/HRESULTs; unknown/null/live-changing devices and declared boundaries verified; 11 bridge input mutants caught; foreign vtable rejected; "<<view_cases<<" view and "<<projection_cases<<" projection cases equivalent; "<<fog_init_cases<<" fog init and "<<fog_table_cases<<" table cases ("<<fog_explicit<<" declared table boundaries); "<<fog_generate_cases<<" generate, "<<fog_switch_cases<<" switch cases; "<<range_cases<<" projection range cases and its argument mutant\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
