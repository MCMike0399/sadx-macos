#include "hle/hle_heap_bridge.hpp"
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
struct Fixture {
    std::unique_ptr<HleProcess> p;bool fixture=true;std::array<std::uint32_t,6> values{};std::vector<std::uint32_t> observed;unsigned stop_at=0,calls=0;bool mutant=false;
    Fixture(const ExecutableImage& image,bool native){
        std::string error;p=HleProcess::create(".","build/heap-bridge-overlay",&error);require(p!=nullptr,error);require(p->map_main(image,&error),error);
        for(const auto* name:{"HeapCreate","HeapAlloc","HeapFree"}){
            auto method=*p->hle().find("KERNEL32.dll",name);const auto body=method.body;const auto index=std::string(name)=="HeapCreate"?3U:std::string(name)=="HeapAlloc"?4U:5U;
            method.body=[this,body,index](HleCall& c){observed.push_back(c.guest.u32(0x389d628).value_or(0));const auto result=fixture?values[index]:body(c);if(++calls==stop_at)c.guest.stop("heap backend unavailable");return result;};
            p->hle().add("KERNEL32.dll",name,method);
        }
        if(native)for(auto entry:heap_overrides(*p)){const auto bridge=entry.bridge;require(p->guest().add_override(entry.address,[this,bridge](X86Guest& g){if(mutant)g.set_u32(g.esp()+4,g.u32(g.esp()+4).value_or(0)^1U);return bridge(g);},&error),error);}
    }
    void reset(const std::uint32_t* c,unsigned stop=0){std::memcpy(values.data(),c,sizeof values);fixture=true;calls=0;stop_at=stop;mutant=false;observed.clear();p->hle().clear_log();p->guest().set_u32(0x389d628,c[1]);}
};
void compare(Fixture& a,Fixture& b,std::uint32_t entry,std::uint32_t arg){
    const auto x=a.p->guest().call(entry,{arg}),y=b.p->guest().call(entry,{arg});require(x.completed==y.completed,"heap completion");
    if(x.completed)require(x.eax==y.eax,"heap EAX receipt");require(a.p->guest().u32(0x389d628)==b.p->guest().u32(0x389d628),"heap global");require(a.observed==b.observed,"heap global stores before calls");
    const auto& l=a.p->hle().log();const auto& r=b.p->hle().log();require(l.size()==r.size(),"heap call count");
    for(unsigned k=0;k<l.size();++k)require(l[k].dll==r[k].dll&&l[k].symbol==r[k].symbol&&l[k].return_address==r[k].return_address&&l[k].arguments==r[k].arguments&&l[k].result==r[k].result,"heap trace/arguments/receipt");
    require(a.p->memory().live_blocks()==b.p->memory().live_blocks(),"live HLE blocks");
}
}
int main(int argc,char** argv){try{
    require(argc==3,"usage: hle-heap-bridge-test SONIC.EXE CORPUS");std::string error;std::ifstream f(argv[1],std::ios::binary);const auto image=ExecutableImage::parse({std::istreambuf_iterator<char>(f),{}},&error);require(image.has_value(),error);
    std::ifstream input(argv[2],std::ios::binary);const std::string bytes((std::istreambuf_iterator<char>(input)),{});require(!bytes.empty()&&bytes.size()%24==0,"heap corpus");std::vector<std::uint32_t> words(bytes.size()/4);std::memcpy(words.data(),bytes.data(),bytes.size());
    Fixture original(*image,false),native(*image,true);unsigned cases=0,stops=0;
    for(std::size_t i=0;i<words.size();i+=6){const auto* c=words.data()+i;for(auto* p:{&original,&native})p->reset(c);compare(original,native,c[0]?0x780fb0:0x780f60,c[2]);++cases;
        if(i%120==0)for(const auto at:{1U,2U}){for(auto* p:{&original,&native})p->reset(c,at);compare(original,native,c[0]?0x780fb0:0x780f60,c[2]);if(original.calls>=at)++stops;}
    }
    const std::uint32_t initial[6]={0,0xffffffffU,32,0,0,0};for(auto* p:{&original,&native}){p->reset(initial);p->fixture=false;}
    compare(original,native,0x780f60,32);const auto first=original.p->hle().log().back().result;require(first!=0,"live allocation");
    std::array<std::uint8_t,32> pattern{};pattern.fill(0x7a);for(auto* p:{&original,&native})require(p->p->guest().write(first,pattern.data(),pattern.size()),"live write");
    compare(original,native,0x780f60,48);compare(original,native,0x780fb0,first);require(!original.p->memory().size(first)&&!native.p->memory().size(first),"freed block");
    compare(original,native,0x780f60,16);const auto reused=original.p->hle().log().back().result;require(reused==first,"first fit reuse");std::array<std::uint8_t,16> a{},b{};original.p->guest().read(reused,a.data(),16);native.p->guest().read(reused,b.data(),16);require(a==b&&a==std::array<std::uint8_t,16>{},"same zeroed HLE contents assumption");
    compare(original,native,0x780f60,0x20000000);require(original.p->hle().log().back().result==0,"live allocation exhaustion");compare(original,native,0x780fb0,0x12345678);compare(original,native,0x780fb0,0);
    unsigned caught=0;
    for(const auto entry:{0x780f60U,0x780fb0U}){for(auto* p:{&original,&native})p->reset(initial);native.mutant=true;try{compare(original,native,entry,32);}catch(const std::exception&){++caught;}}
    require(caught==2,"heap bridge input mutants");
    for(const auto slot:{0x7db078U,0x7db074U,0x7db0f8U}){
        native.reset(initial);auto& g=native.p->guest();const auto target=g.u32(slot);require(target.has_value(),"IAT target");g.set_u32(slot,0);
        const auto r=g.call(slot==0x7db0f8?0x780fb0:0x780f60,{32});require(!r&&r.error.find("non-HLE")!=std::string::npos,"foreign heap IAT rejection");g.set_u32(slot,*target);
    }
    auto& g=native.p->guest();std::array<std::uint8_t,4096> saved{};require(g.read(0x389d000,saved.data(),saved.size()),"state page");require(g.unmap(0x389d000,4096,&error),error);
    const auto null_free=g.call(0x780fb0,{0});require(bool(null_free)&&null_free.eax==0,"null free skips unknown state");
    const auto missing_alloc=g.call(0x780f60,{32}),missing_free=g.call(0x780fb0,{32});
    require(!missing_alloc&&missing_alloc.error.find("780f60")!=std::string::npos&&!missing_free&&missing_free.error.find("780fb9")!=std::string::npos,"unknown heap boundaries");
    require(g.map(0x389d000,4096,false,&error),error);require(g.write(0x389d000,saved.data(),saved.size()),"restore state page");
    std::cout<<"Heap bridge: "<<cases<<" oracle cases and "<<stops<<" backend-stop cases; identical EAX/global/HLE traces and store phases; live allocation/free/reuse/exhaustion/contents; 2 input mutants caught; 3 foreign IAT targets rejected\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
