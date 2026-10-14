#include "hle/hle_task_bridge.hpp"
#include "hle/hle_process.hpp"
#include "engine/task_work.hpp"
#include <array>
#include <algorithm>
#include <unicorn/unicorn.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
namespace {
using namespace sadx;
void require(bool ok,const std::string& why){if(!ok)throw std::runtime_error(why);}
struct Fixture {
    std::unique_ptr<HleProcess> p;bool mutant=false;
    std::vector<std::array<std::uint32_t,9>> callbacks;
    Fixture(const ExecutableImage& image,bool native){std::string error;p=HleProcess::create(".","build/task-bridge-overlay",&error);require(p!=nullptr,error);require(p->map_main(image,&error),error);
        require(p->guest().add_override(0x51a740,[](X86Guest&){return 0U;},&error),error);
        require(p->guest().map(0x10000000,4096,true,&error),error);
        const auto put=[&](std::uint32_t at,const std::vector<std::uint8_t>& code){require(p->guest().write(at,code.data(),code.size()),"callback code");};
        const auto call_bytes=[](std::uint32_t at,std::uint32_t target){std::vector<std::uint8_t> bytes{0xe8};const auto displacement=target-at-5;for(unsigned k=0;k<4;++k)bytes.push_back(static_cast<std::uint8_t>(displacement>>(8*k)));return bytes;};
        put(0x10000000,{0xc3});put(0x10000040,{0x8b,0x54,0x24,0x04,0x8b,0x02,0x85,0xc0,0x74,0x07,0xc7,0x40,0x10,0,0,0,0x10,0xc3});
        for(const auto at:{0x10000080U,0x100000c0U,0x10000100U,0x10000140U}){std::vector<std::uint8_t> code;
            if(at==0x10000080)code={0x6a,0x01};else if(at==0x100000c0)code={0xff,0x74,0x24,0x04};else code={0x68,0,0,0,0x10,0x6a,0x00,0x6a,0x00};
            const auto invoke=call_bytes(at+code.size(),at==0x10000080?0x40b0c0:at==0x100000c0?0x40b570:0x40b860);code.insert(code.end(),invoke.begin(),invoke.end());code.insert(code.end(),{0x83,0xc4,static_cast<std::uint8_t>(at==0x10000100?12:4),0xc3});put(at,code);
        }
        std::vector<std::uint8_t> reused={0x8b,0x44,0x24,0x04,0xff,0x30};const auto remove=call_bytes(0x10000140+reused.size(),0x40b570);reused.insert(reused.end(),remove.begin(),remove.end());reused.insert(reused.end(),{0x83,0xc4,0x04,0x68,0,0,0,0x10,0x6a,0x00,0x6a,0x00});const auto replace=call_bytes(0x10000140+reused.size(),0x40b860);reused.insert(reused.end(),replace.begin(),replace.end());reused.insert(reused.end(),{0x83,0xc4,0x0c,0xc3});put(0x10000140,reused);
        for(const auto at:{0x10000000U,0x10000040U,0x10000080U,0x100000c0U,0x10000100U,0x10000140U})require(p->guest().add_instruction_hook(at,[this](X86Guest& g,std::uint32_t pc){callbacks.push_back({pc,g.u32(g.esp()+4).value_or(0),g.u32(0x3abdbf4).value_or(0),g.u32(0x3abdbf8).value_or(0),g.u32(g.esp()).value_or(0),g.esp(),g.reg(UC_X86_REG_ESI),g.reg(UC_X86_REG_EBX),g.reg(UC_X86_REG_EDI)});return false;},&error),error);
        if(native)for(auto entry:task_overrides(*p)){const auto bridge=entry.bridge;const auto at=entry.address;require(p->guest().add_override(at,[this,bridge,at](X86Guest& g){if(mutant&&at!=0x40b1a0){if(at==0x40b030)g.set_reg(UC_X86_REG_EDX,g.reg(UC_X86_REG_EDX)^1U);else if(at==0x40b170)g.set_u32(0x3abdbe0,g.u32(0x3abdbe0).value_or(0)^1U);else if(at==0x40b460)g.set_u32(0x3abdbe8,g.u32(0x3abdbe8).value_or(0)^1U);else g.set_u32(g.esp()+4,g.u32(g.esp()+4).value_or(0)^1U);}return bridge(g);},&error),error);}}
};
void compare(Fixture& a,Fixture& b,std::uint32_t entry,const std::vector<std::uint32_t>& args){const auto x=a.p->guest().call(entry,args),y=b.p->guest().call(entry,args);require(bool(x),"original: "+x.error);require(bool(y),"native: "+y.error);
    if(entry!=0x40b310&&entry!=0x40b570&&entry!=0x40b0c0&&entry!=0x40b7e0&&entry!=0x40b170&&entry!=0x40b420)require(x.eax==y.eax,"task-work EAX");
    for(const auto at:{0x3abdbe8U,0x3abdbf0U,0x3c839fcU,0x3c83a00U,0x3abdbe4U,0x3abdbecU,0x3abdbf4U,0x3abdbf8U})require(a.p->guest().u32(at)==b.p->guest().u32(at),"task-work globals at "+std::to_string(at)+" entry "+std::to_string(entry));
    require(a.callbacks==b.callbacks,"callback source return/frame/context/register trace");
    for(unsigned k=0;k<8;++k)require(a.p->guest().u32(0x3abdbc4+4*k)==b.p->guest().u32(0x3abdbc4+4*k),"task list heads entry "+std::to_string(entry));
    const auto& l=a.p->hle().log();const auto& r=b.p->hle().log();require(l.size()==r.size(),"task-work trace count");
    for(unsigned k=0;k<l.size();++k)require(l[k].dll==r[k].dll&&l[k].symbol==r[k].symbol&&l[k].return_address==r[k].return_address&&l[k].arguments==r[k].arguments&&l[k].result==r[k].result,"task-work exact HLE trace");
}
void memory(Fixture& a,Fixture& b){const auto base=a.p->guest().u32(0x3abdbf0).value_or(0);std::vector<std::uint8_t> x(TaskWorkArena::usable_bytes+32),y(x.size());require(a.p->guest().read(base,x.data(),x.size())&&b.p->guest().read(base,y.data(),y.size()),"arena memory");require(x==y,"task-work full arena bytes");}
}
int main(int argc,char** argv){try{
    require(argc==4,"usage: hle-task-bridge-test SONIC.EXE WORK_CORPUS GRAPH_CORPUS");std::string error;std::ifstream f(argv[1],std::ios::binary);const auto image=ExecutableImage::parse({std::istreambuf_iterator<char>(f),{}},&error);require(image.has_value(),error);
    std::ifstream input(argv[2],std::ios::binary);const std::string bytes((std::istreambuf_iterator<char>(input)),{});require(!bytes.empty()&&bytes.size()%4==0,"task corpus");std::vector<std::uint32_t> words(bytes.size()/4);std::memcpy(words.data(),bytes.data(),bytes.size());
    Fixture original(*image,false),native(*image,true);unsigned sequences=0,operations=0;
    for(std::size_t offset=0;offset<words.size();){const auto count=words[offset++];require(count<=(words.size()-offset)/2,"sequence bounds");
        for(auto* p:{&original,&native}){p->p->hle().clear_log();const auto base=p->p->guest().u32(0x3abdbf0).value_or(0);if(base)p->p->guest().set_u32(base+TaskWorkArena::usable_bytes+16,0x12345678U+sequences);}
        compare(original,native,0x40b1a0,{});memory(original,native);std::vector<std::uint32_t> allocations;
        for(unsigned step=0;step<count;++step){const auto op=words[offset++],value=words[offset++];require(op<2,"operation");require(!op||value<allocations.size(),"allocation index");const auto argument=op?allocations[value]:value;
            compare(original,native,op?0x40b310:0x40b220,{argument});
            if(!op){const auto addr=original.p->guest().reg(UC_X86_REG_EAX);allocations.push_back(addr);
                // Game code owns payload writes. Allocation/free must preserve them,
                // including bytes subsequently exposed by first-fit reuse/coalescing.
                if(addr){const auto next=original.p->guest().u32(addr-24);require(next&&*next>=addr,"physical next");const auto capacity=*next-addr;if(capacity){const auto n=std::min<std::uint32_t>(capacity,16);std::array<std::uint8_t,16> pattern{};for(unsigned k=0;k<n;++k)pattern[k]=static_cast<std::uint8_t>(step+k+sequences);for(auto* p:{&original,&native}){p->p->guest().write(addr,pattern.data(),n);p->p->guest().write(addr+capacity-n,pattern.data(),n);}}}
            }++operations;
        }memory(original,native);++sequences;
    }
    std::ifstream graph_input(argv[3],std::ios::binary);const std::string graph_bytes((std::istreambuf_iterator<char>(graph_input)),{});require(!graph_bytes.empty()&&graph_bytes.size()%4==0,"graph corpus");std::vector<std::uint32_t> graph_words(graph_bytes.size()/4);std::memcpy(graph_words.data(),graph_bytes.data(),graph_bytes.size());
    unsigned graph_sequences=0,graph_operations=0;
    for(std::size_t offset=0;offset<graph_words.size();){const auto count=graph_words[offset++];require(count<=(graph_words.size()-offset)/4,"graph sequence bounds");
        compare(original,native,0x40b1a0,{});compare(original,native,0x40b460,{});for(auto* p:{&original,&native})p->p->guest().set_u32(0x38f6e08,0x30000000);std::vector<std::uint32_t> tasks;
        for(unsigned step=0;step<count;++step){const auto op=graph_words[offset++],a=graph_words[offset++],b=graph_words[offset++],c=graph_words[offset++];
            if(op==0){for(auto* p:{&original,&native}){p->p->guest().set_reg(UC_X86_REG_EDX,a);p->p->guest().set_reg(UC_X86_REG_EDI,0);}compare(original,native,0x40b030,{});}
            else if(op==1)compare(original,native,0x40b860,{a,b,0});
            else if(op==2){require(b<tasks.size(),"parent index");compare(original,native,0x40b940,{a,0,tasks[b]});}
            else if(op==3)compare(original,native,0x40b570,{tasks.at(a)});
            else if(op==4)compare(original,native,0x40b6c0,{tasks.at(a)});
            else if(op==5)compare(original,native,0x40b0c0,{a});
            else if(op==6)compare(original,native,0x40b7e0,{tasks.at(a)});
            else if(op==7)for(auto* p:{&original,&native})p->p->guest().set_u32(tasks.at(a)+16,0x40b7e0);
            else if(op==8)for(auto* p:{&original,&native}){const auto work=p->p->guest().u32(tasks.at(a)+32);require(work&&*work,"primary work");p->p->guest().set_u32(*work+b,c);}
            else if(op==9){std::vector<std::uint32_t> blocks;for(const auto n:{32U,16U,80U,20U,20U,8U,16U}){compare(original,native,0x40b220,{n});blocks.push_back(original.p->guest().reg(UC_X86_REG_EAX));}
                for(auto* p:{&original,&native}){auto& g=p->p->guest();const auto work=g.u32(tasks.at(a)+32).value_or(0);const std::uint32_t lengths[7]={32,0,80,20,20,0,16};for(unsigned k=0;k<7;++k)if(lengths[k]){std::vector<std::uint8_t> zero(lengths[k],0);g.write(blocks[k],zero.data(),zero.size());}const std::uint8_t flag=0x80;g.write(blocks[0]+5,&flag,1);
                    const std::pair<std::uint32_t,std::uint32_t> links[]={{blocks[0]+12,blocks[1]},{work+56,blocks[0]},{work+60,blocks[2]},{blocks[3]+16,blocks[4]},{blocks[2]+64,blocks[3]},{blocks[2]+68,blocks[5]},{blocks[2]+76,blocks[6]}};for(const auto& [at,value]:links)g.set_u32(at,value);}
            }else require(false,"graph operation");
            if(op<=2)tasks.push_back(original.p->guest().reg(UC_X86_REG_EAX));++graph_operations;
        }memory(original,native);++graph_sequences;
    }
    unsigned dispatch_cases=0;
    for(const auto owner:{0x40b0c0U,0x40b170U,0x40b420U})for(const auto cb:{0x10000000U,0x10000040U,0x10000080U,0x100000c0U,0x10000100U,0x10000140U}){
        original.callbacks.clear();native.callbacks.clear();compare(original,native,0x40b1a0,{});compare(original,native,0x40b460,{});
        compare(original,native,0x40b860,{0,0,0});const auto parent=original.p->guest().reg(UC_X86_REG_EAX);
        if(owner==0x40b420){compare(original,native,0x40b940,{0,0x10000000,parent});compare(original,native,0x40b940,{0,cb,parent});}
        else {compare(original,native,0x40b860,{0,0,0x10000000});compare(original,native,0x40b860,{0,0,cb});}
        compare(original,native,0x40b860,{0,1,0x10000000});compare(original,native,0x40b860,{0,8,0x10000000});
        const auto base=original.p->guest().u32(0x3abdbe4).value_or(0);for(auto* f:{&original,&native})for(unsigned k=0;k<8;++k)f->p->guest().set_u32(base+52*k+48,0x30000000+4*k);
        compare(original,native,owner,owner==0x40b170?std::vector<std::uint32_t>{}:std::vector<std::uint32_t>{owner==0x40b420?parent:0});memory(original,native);++dispatch_cases;
    }
    unsigned dispatch_mutants=0;
    for(const auto owner:{0x40b0c0U,0x40b170U,0x40b420U}){
        original.callbacks.clear();native.callbacks.clear();compare(original,native,0x40b1a0,{});compare(original,native,0x40b460,{});compare(original,native,0x40b860,{0,0,0x10000000});const auto parent=original.p->guest().reg(UC_X86_REG_EAX);compare(original,native,0x40b940,{0,0x10000000,parent});compare(original,native,0x40b860,{0,8,0x10000000});native.mutant=true;
        try{compare(original,native,owner,owner==0x40b170?std::vector<std::uint32_t>{}:std::vector<std::uint32_t>{owner==0x40b420?parent:0});}catch(const std::exception&){++dispatch_mutants;}native.mutant=false;
        original.callbacks.clear();native.callbacks.clear();for(auto* p:{&original,&native})for(const auto at:{0x3abdbe4U,0x3abdbecU,0x3abdbf4U,0x3abdbf8U,0x3abdbc4U,0x3abdbc8U,0x3abdbccU,0x3abdbd0U,0x3abdbd4U,0x3abdbd8U,0x3abdbdcU,0x3abdbe0U})p->p->guest().set_u32(at,0);
    }require(dispatch_mutants==3,"dispatch bridge mutants");
    unsigned caught=0;
    for(const auto entry:{0x40b220U,0x40b310U}){
        compare(original,native,0x40b1a0,{});compare(original,native,0x40b220,{64});const auto address=original.p->guest().reg(UC_X86_REG_EAX);native.mutant=true;
        try{compare(original,native,entry,{entry==0x40b220?64U:address});}catch(const std::exception&){++caught;}native.mutant=false;
    }native.mutant=false;require(caught==2,"task-work bridge mutants");
    const auto clear_graph=[&]{for(auto* p:{&original,&native})for(const auto at:{0x3abdbe4U,0x3abdbecU,0x3abdbf4U,0x3abdbf8U,0x3abdbc4U,0x3abdbc8U,0x3abdbccU,0x3abdbd0U,0x3abdbd4U,0x3abdbd8U,0x3abdbdcU,0x3abdbe0U})p->p->guest().set_u32(at,0);};
    clear_graph();
    unsigned graph_mutants=0;
    for(const auto entry:{0x40b460U,0x40b030U,0x40b860U,0x40b940U,0x40b6c0U}){
        clear_graph();
        compare(original,native,0x40b1a0,{});compare(original,native,0x40b460,{});compare(original,native,0x40b860,{2,0,0x51f680});const auto parent=original.p->guest().reg(UC_X86_REG_EAX);
        for(auto* p:{&original,&native}){p->p->guest().set_reg(UC_X86_REG_EDX,0);p->p->guest().set_reg(UC_X86_REG_EDI,0x51f680);}native.mutant=true;
        const auto args=entry==0x40b860?std::vector<std::uint32_t>{2,1,0x51f680}:entry==0x40b940?std::vector<std::uint32_t>{2,0x51f680,parent}:entry==0x40b6c0?std::vector<std::uint32_t>{parent}:std::vector<std::uint32_t>{};
        try{compare(original,native,entry,args);}catch(const std::exception&){++graph_mutants;}native.mutant=false;
    }require(graph_mutants==5,"task graph bridge mutants");
    clear_graph();compare(original,native,0x40b1a0,{});compare(original,native,0x40b460,{});
    compare(original,native,0x40b860,{0,7,0});const auto detached=original.p->guest().reg(UC_X86_REG_EAX);compare(original,native,0x40b6c0,{detached});
    compare(original,native,0x40b6c0,{0});
    const auto foreign=native.p->guest().call(0x40b6c0,{0x12345678});require(!foreign&&foreign.error.find("40b6c6")!=std::string::npos,"foreign task identity boundary");
    for(auto* p:{&original,&native}){p->p->guest().set_reg(UC_X86_REG_EDX,0xffffffffU);p->p->guest().set_reg(UC_X86_REG_EDI,0);}compare(original,native,0x40b030,{});compare(original,native,0x40b860,{0,9,0});const auto source_null=original.p->guest().call(0x40b940,{2,0,0}),native_null=native.p->guest().call(0x40b940,{2,0,0});require(!source_null&&!native_null&&native_null.error.find("40b95c")!=std::string::npos,"null parent source boundary");memory(original,native);
    clear_graph();compare(original,native,0x40b1a0,{});compare(original,native,0x40b220,{TaskWorkArena::usable_bytes-32});compare(original,native,0x40b460,{});memory(original,native);
    clear_graph();compare(original,native,0x40b1a0,{});compare(original,native,0x40b460,{});compare(original,native,0x40b220,{TaskWorkArena::usable_bytes-0x34020-32});compare(original,native,0x40b860,{2,0,0});memory(original,native);
    auto& g=native.p->guest();require(bool(g.call(0x40b1a0,{})),"native reset");const auto base=g.u32(0x3abdbf0).value_or(0);g.set_u32(base+8,base+64);
    const auto corrupt=g.call(0x40b220,{32});require(!corrupt&&corrupt.error.find("changed metadata")!=std::string::npos,"external metadata corruption boundary");
    require(bool(g.call(0x40b1a0,{})),"native reset after corruption");const auto wrapped=g.call(0x40b220,{0xffffffffU});require(!wrapped&&wrapped.error.find("40b220")!=std::string::npos,"declared size-wrap boundary");
    const auto block=g.call(0x40b220,{32});require(bool(block)&&block.eax!=0,"native allocation");require(bool(g.call(0x40b310,{block.eax})),"native release");const auto twice=g.call(0x40b310,{block.eax});require(!twice&&twice.error.find("40b31c")!=std::string::npos,"double/foreign free boundary");
    std::cout<<"Task bridge: "<<sequences<<" complete oracle sequences, "<<operations<<" operations; "<<graph_sequences<<" graph sequences/"<<graph_operations<<" operations; "<<dispatch_cases<<" dispatch scenarios; actual CRT/heap acquisition and repeated initialization; EAX allocation/global/exact HLE traces and full 16 MiB+32 arena bytes match with guest payload writes and sentinel padding; 2 work, 5 graph and 3 dispatch input mutants caught; corrupt metadata, wrap and unowned free stop explicitly\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
