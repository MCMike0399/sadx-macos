// M1 4d: override-equivalence. For every entry of the override table, two guests with the same sonic.exe image
// and the same globals run the same call: one executes the original bytes, the other the native port through the
// override bridge. The ranges the original may write (and EAX where the function returns a value) must be
// byte-identical.
// Inputs: the frame-loop entries replay their oracles' corpora (build/oracles/<slug>.cases.bin, written by the
// verify scripts): 7899a0 the game_frame_control cases of entry 0, 7815e0 the projection_range calls, 7815c0 the
// fov cases (every control word), 781570 the ninja_view mode-0 cases; the D3DX leaves 7716df the d3dx_perspective
// cases (minus the |fovy/2| >= 2^63 ones, the native port's declared boundary) and 7757e0 the d3dx_matrix_inverse
// cases (MXCSR rounding, with/without pDeterminant, aligned/misaligned). The startup owners (789930, 40a8a0,
// 77ffc0, 7959c0) and 774310/774150 have no corpus of their own (their oracles run them nested inside larger
// owners), so their input classes are enumerated here: empty/full/duplicate slots, counts, zero/non-zero flag
// bytes; random and special-valued matrices, aliased pOut, misaligned matrices, every MXCSR rounding mode.
// 772615 replays the d3dx_project cases (slots 3/10 set to the SSE table; NULL arguments, misaligned matrices) and
// checks that unset slots stop; 402f40 the ninja_queue_color cases plus one input overlapping 3ab9864 per four,
// comparing ECX/EDX and the x87 condition bits as well; 78dbe0 the ninja_projected_depth cases, comparing the
// returned ST(0), TOP and the tag word (its mutant flips the returned significand's lowest bit).
// 40f350 replays the game_startup input pages and compares EAX, ECX/EDX, arithmetic flags and every page byte.
// 40f3a0 replays controller_init pages with DF=0/1 and RET/JMP suffixes, recording suffix-entry state.
// 419030 replays mode0 of collision_shapes under all oracle CWs; matrix/metadata/absolute aliases and DF=1 retain original execution, asserted by a source-body hook.
// The original side runs with the FPU shims installed, as every guest boot does (7716df's FSINCOS is a shim site).
// Mutants: every entry is also run with a bridge that flips one bit of its first store; the comparison must
// catch it, or the test fails (an insensitive comparison would pass anything).
//   guest-overrides-test SONIC.EXE ORACLE_DIR
#include "guest/fpu_shims.hpp"
#include "guest/override_table.hpp"
#include "math/d3dx_math.hpp"
#include <unicorn/unicorn.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <map>
#include <random>
#include <stdexcept>
namespace {
using namespace sadx;
void require(bool ok,const std::string& why){if(!ok)throw std::runtime_error(why);}
struct Case {std::vector<std::pair<std::uint32_t,std::uint32_t>> memory;std::vector<std::uint32_t> arguments;std::optional<std::uint32_t> ecx;std::uint16_t cw=0x27f;std::uint32_t mxcsr=0x1f80;std::uint32_t eflags=2;bool source_body=false;};
constexpr std::uint32_t scratch=0x10000000,out_at=scratch+0x100,det_at=scratch+0x180,matrix_at=scratch+0x200,second_at=scratch+0x300, // a page mapped in both guests
    project_v_at=scratch+0x40,project_viewport_at=scratch+0x60,color_at=scratch+0x80,project_matrix_at=scratch+0x400,collision_task_at=scratch+0x1100,collision_shape_at=scratch+0x1200,collision_out_at=scratch+0x1300,collision_matrix_at=scratch+0x2000;
std::vector<std::uint32_t> words(const std::string& path){
    std::ifstream f(path,std::ios::binary);require(bool(f),"cannot read the oracle corpus "+path+" (run its oracle)");
    std::string all((std::istreambuf_iterator<char>(f)),{});require(all.size()%4==0&&!all.empty(),path+": not a word corpus");
    std::vector<std::uint32_t> w(all.size()/4);std::memcpy(w.data(),all.data(),all.size());return w;}
std::vector<Case> corpus_cases(std::uint32_t entry,const std::string& dir){
    std::vector<Case> out;
    if(entry==0x40f350){ // game_startup: the complete 3b0e000 input page (1024 words); compare all neighbours too
        const auto w=words(dir+"/game_startup.cases.bin");require(w.size()%1024==0,"game_startup corpus size");
        for(std::size_t i=0;i<w.size();i+=1024){
            Case c;for(std::uint32_t k=0;k<1024;++k)c.memory.push_back({0x3b0e000+4*k,w[i+k]});
            out.push_back(c);}}
    else if(entry==0x40f3a0){
        const auto w=words(dir+"/controller_init.cases.bin");require(w.size()%1024==0,"controller_init corpus size");
        for(std::size_t i=0;i<w.size();i+=1024){
            Case c;c.memory.push_back({scratch+0x900,0x909090c3U});
            for(std::uint32_t k=0;k<1024;++k)c.memory.push_back({0x3b0e000+4*k,w[i+k]});
            out.push_back(c);Case backwards=c;backwards.eflags|=0x400U;out.push_back(backwards);
            // The RET may be a profile jump. This fixture records the suffix-entry
            // registers, flags, ESP and caller return word, then repoints the pads.
            constexpr std::uint32_t hook=scratch+0x800,observed=scratch+0x600;
            std::vector<std::uint8_t> code;
            const auto imm=[&](std::uint32_t value){for(unsigned b=0;b<4;++b)code.push_back(static_cast<std::uint8_t>(value>>(8*b)));};
            const auto record=[&](std::uint8_t modrm,std::uint32_t offset){code.push_back(0x89);code.push_back(modrm);imm(observed+offset);};
            record(0x05,0);record(0x0d,4);record(0x15,8);record(0x25,12);record(0x35,16);record(0x3d,20);
            code.insert(code.end(),{0x9c,0x8f,0x05});imm(observed+24); // PUSHFD; POP [observed+24]
            code.insert(code.end(),{0x8b,0x04,0x24,0xa3});imm(observed+28); // MOV EAX,[ESP]; MOV [observed+28],EAX
            for(std::uint32_t pad=0;pad<4;++pad){code.insert(code.end(),{0xc7,0x05});imm(0x3b0e77c+4*pad);imm(scratch+0xc00+0x34*pad);}
            code.push_back(0xc3);while(code.size()%4)code.push_back(0x90);
            for(std::uint32_t offset=0;offset<code.size();offset+=4){std::uint32_t value;std::memcpy(&value,code.data()+offset,4);c.memory.push_back({hook+offset,value});}
            const auto relative=hook-(0x40f451+5);
            c.memory.push_back({0x40f451,0xe9U|((relative&0xffffffU)<<8)});
            c.memory.push_back({0x40f455,0x90909000U|(relative>>24)});
            out.push_back(c);c.eflags|=0x400U;out.push_back(c);
        }}

    else if(entry==0x419030){ // collision_shapes: 136 words; replay mode0 under all twelve oracle CWs
        const auto w=words(dir+"/collision_shapes.cases.bin");require(w.size()%136==0,"collision_shapes corpus size");
        for(std::size_t i=0;i<w.size();i+=136)if(w[i]==0)
            for(const std::uint16_t pc:{0U,0x200U,0x300U})for(const std::uint16_t rc:{0U,0x400U,0x800U,0xc00U}){
                Case c;c.cw=static_cast<std::uint16_t>(pc|rc|0x7f);c.ecx=0x12345678;
                c.memory={{collision_shape_at+4,w[i+8]},{0x3d0f818,collision_matrix_at+64*(w[i+32]-1)},
                    {0x3d0fa28,w[i+135]},{0x3d0fa2c,w[i+32]},{0x3d0f7ac,w[i+33]}};
                for(std::uint32_t k=0;k<3;++k){
                    c.memory.push_back({collision_task_at+0x14+4*k,w[i+2+k]});
                    c.memory.push_back({collision_task_at+0x20+4*k,w[i+5+k]});
                    c.memory.push_back({collision_shape_at+8+4*k,w[i+9+k]});
                    c.memory.push_back({collision_out_at+4*k,w[i+130+k]});
                }
                for(std::uint32_t k=0;k<96;++k)c.memory.push_back({collision_matrix_at+4*k,w[i+34+k]});
                c.arguments={w[i+26]?collision_task_at:0U,collision_shape_at,collision_out_at};out.push_back(c);
                if((w[i+8]&0x20)!=0){Case alias=c;alias.arguments[2]=collision_shape_at+12;alias.source_body=true;out.push_back(alias);}
                else if((w[i+8]&0x8000)!=0){Case alias=c;alias.arguments[2]=collision_task_at+0x24;out.push_back(alias);}
                else {
                    const auto changed=collision_matrix_at+64*(w[i+32]-1+(w[i+32]<w[i+135]?1:0));
                    Case alias=c;alias.arguments[2]=changed+8;alias.source_body=true;out.push_back(alias);
                    alias=c;alias.arguments[2]=0x3d0fa28;alias.source_body=true;out.push_back(alias);
                    alias=c;alias.eflags|=0x400U;alias.source_body=true;out.push_back(alias);
                }
            }}
    else if(entry==0x7899a0){ // game_frame_control: 19 words; [0] entry index (0 = 7899a0), [2] argument, [4..9] 3d0853c 3d08538 389d7dc 89295c 3d084a8 3d084ac
        const auto w=words(dir+"/game_frame_control.cases.bin");require(w.size()%19==0,"game_frame_control corpus size");
        for(std::size_t i=0;i<w.size();i+=19)if(w[i]==0)
            out.push_back({{{0x3d0853c,w[i+4]},{0x3d08538,w[i+5]},{0x389d7dc,w[i+6]},{0x89295c,w[i+7]},{0x3d084a8,w[i+8]},{0x3d084ac,w[i+9]}},{w[i+2]},std::nullopt,0x7f});}
    else if(entry==0x7815e0){ // projection_range: 23 words per call; [0] near, [1] far, [2] 3d0f808, [3] 3d0f968, [4] control word
        const auto w=words(dir+"/projection_range.cases.bin");require(w.size()%23==0,"projection_range corpus size");
        for(std::size_t i=0;i<w.size();i+=23)out.push_back({{{0x3d0f808,w[i+2]},{0x3d0f968,w[i+3]}},{w[i],w[i+1]},std::nullopt,static_cast<std::uint16_t>(w[i+4])});}
    else if(entry==0x7815c0){ // fov: (control word, angle, width at 3d0f9e4); 3d0f9e0 starts as deadbeef
        const auto w=words(dir+"/fov.cases.bin");require(w.size()%3==0,"fov corpus size");
        for(std::size_t i=0;i<w.size();i+=3)out.push_back({{{0x3d0f9e4,w[i+2]},{0x3d0f9e0,0xdeadbeefU}},{w[i+1]},std::nullopt,static_cast<std::uint16_t>(w[i])});}
    else if(entry==0x781570){ // ninja_view: 37 words; [0] control word, [2] mode (0 runs 781570), [3..12] the camera block, the rest sentinels
        const auto w=words(dir+"/ninja_view.cases.bin");require(w.size()%37==0,"ninja_view corpus size");
        for(std::size_t i=0;i<w.size();i+=37)if(w[i+2]==0){
            Case c;c.cw=static_cast<std::uint16_t>(w[i]);c.ecx=0x3aad0a0;
            for(std::uint32_t k=0;k<10;++k)c.memory.push_back({0x3aad0a0+4*k,w[i+3+k]});
            for(std::uint32_t k=10;k<16;++k)c.memory.push_back({0x3aad0a0+4*k,0x5eeded5eU});
            out.push_back(c);}}
    else if(entry==0x7716df){ // d3dx_perspective: (cw, fovy, aspect, near, far); pOut = out_at
        const auto w=words(dir+"/d3dx_perspective.cases.bin");require(w.size()%5==0,"d3dx_perspective corpus size");
        for(std::size_t i=0;i<w.size();i+=5){
            float v[4];std::memcpy(v,&w[i+1],16);X87Context fp(static_cast<std::uint16_t>(w[i]));
            if(!d3dx_perspective_7716df(v[0],v[1],v[2],v[3],fp))continue; // the declared boundary: the override stops there
            out.push_back({{},{out_at,w[i+1],w[i+2],w[i+3],w[i+4]},std::nullopt,static_cast<std::uint16_t>(w[i])});}}
    else if(entry==0x7757e0){ // d3dx_matrix_inverse: mxcsr, with pDeterminant, aligned, the matrix
        const auto w=words(dir+"/d3dx_matrix_inverse.cases.bin");require(w.size()%19==0,"d3dx_matrix_inverse corpus size");
        for(std::size_t i=0;i<w.size();i+=19){
            Case c;c.mxcsr=w[i];const auto at=matrix_at+(w[i+2]?0U:8U);
            for(std::uint32_t k=0;k<16;++k)c.memory.push_back({at+4*k,w[i+3+k]});
            c.arguments={out_at,w[i+1]?det_at:0U,at};c.cw=0x7f;out.push_back(c);}}
    else if(entry==0x772615){ // d3dx_project: the probe's 61 words; [0] cw [1] mxcsr [2] present bits [3] aligned bits [4..6] v
        // [7..12] viewport [13..60] projection/view/world. Slots 3/10 hold the SSE table, as 77350c leaves them.
        const auto w=words(dir+"/d3dx_project.cases.bin");require(w.size()%61==0,"d3dx_project corpus size");
        for(std::size_t i=0;i<w.size();i+=61){
            Case c;c.cw=static_cast<std::uint16_t>(w[i]);c.mxcsr=w[i+1];c.memory={{0x389b784,0x774310},{0x389b7a0,0x774150}};
            for(std::uint32_t k=0;k<3;++k)c.memory.push_back({project_v_at+4*k,w[i+4+k]});
            for(std::uint32_t k=0;k<6;++k)c.memory.push_back({project_viewport_at+4*k,w[i+7+k]});
            std::vector<std::uint32_t> matrices;
            for(std::uint32_t m=0;m<3;++m){const auto at=project_matrix_at+0x100*m+((w[i+3]>>m&1)?0U:8U);matrices.push_back(w[i+2]>>m&1?at:0U);
                for(std::uint32_t k=0;k<16;++k)c.memory.push_back({at+4*k,w[i+13+16*m+k]});}
            c.arguments={out_at,project_v_at,w[i+2]&8?project_viewport_at:0U,matrices[0],matrices[1],matrices[2]};out.push_back(c);}}
    else if(entry==0x78dbe0){ // ninja_projected_depth: 41 words; [0] cw [1] mxcsr [2] w [3..8] 3d12780 [9..24] 3d129c0 [25..40] 389d318
        const auto w=words(dir+"/ninja_projected_depth.cases.bin");require(w.size()%41==0,"ninja_projected_depth corpus size");
        for(std::size_t i=0;i<w.size();i+=41){
            Case c;c.cw=static_cast<std::uint16_t>(w[i]);c.mxcsr=w[i+1];c.memory={{0x389b784,0x774310},{0x389b7a0,0x774150},{0x389b7b0,0x772615}};
            for(std::uint32_t k=0;k<6;++k)c.memory.push_back({0x3d12780+4*k,w[i+3+k]});
            for(std::uint32_t k=0;k<16;++k){c.memory.push_back({0x3d129c0+4*k,w[i+9+k]});c.memory.push_back({0x389d318+4*k,w[i+25+k]});}
            c.arguments={w[i+2]};out.push_back(c);}}
    else if(entry==0x402f40){ // ninja_queue_color: 9 words; [0] cw [1..4] the input colour [5..8] the prior 3ab9864
        const auto w=words(dir+"/ninja_queue_color.cases.bin");require(w.size()%9==0,"ninja_queue_color corpus size");
        for(std::size_t i=0;i<w.size();i+=9){
            Case c;c.cw=static_cast<std::uint16_t>(w[i]);
            for(std::uint32_t k=0;k<4;++k){c.memory.push_back({color_at+4*k,w[i+1+k]});c.memory.push_back({0x3ab9864+4*k,w[i+5+k]});}
            c.arguments={color_at};out.push_back(c);
            if(i%36==0){Case alias=c;alias.arguments={0x3ab9864-4};alias.memory.push_back({0x3ab9864-4,w[i+1]});out.push_back(alias);}}} // overlaps the copy
    return out;
}
std::vector<Case> enumerated_cases(std::uint32_t entry,std::mt19937& rng){
    const auto u32=[&](){return static_cast<std::uint32_t>(rng());};
    std::vector<Case> out;
    for(int n=0;n<2000;++n){
        Case c;
        if(entry==0x789930){ // 32 slots: empty ones at random places, or none (all taken)
            const auto free_slot=n%5==0?32U:u32()%32;
            for(std::uint32_t i=0;i<32;++i)c.memory.push_back({0x3d084b0+4*i,i<free_slot?(u32()|1):(i==free_slot?0U:u32()%3)});
            c.arguments={u32()};}
        else if(entry==0x40a8a0){ // 900e88 first byte zero or not; the other block, flag bytes random
            for(std::uint32_t i=0;i<12;++i){c.memory.push_back({0x900e88+4*i,i==0&&n%2?u32()&0xffffff00U:u32()});c.memory.push_back({0x9008b4+4*i,u32()});}
            c.memory.push_back({0x3abdbc0,u32()});c.memory.push_back({0x903e58,u32()});}
        else if(entry==0x77ffc0){ // count 0..63 (the comparison covers 64 slots)
            c.memory.push_back({0x3ce712c,u32()%64});for(std::uint32_t i=0;i<64;++i)c.memory.push_back({0x3d0fa40+4*i,u32()});c.arguments={u32()};}
        else if(entry==0x7959c0){ // 16 hooks: zeros, the argument already present, or full
            const auto hook=u32()|1;
            for(std::uint32_t i=0;i<16;++i){const auto k=u32()%6;c.memory.push_back({0x3d08af0+4*i,k==0?0U:k==1&&n%3==0?hook:(u32()|1)});}
            if(n%7==0)for(auto& [at,v]:c.memory)v=u32()|1; // full, no duplicate (almost surely)
            c.arguments={hook};}
        else if(entry==0x774310||entry==0x774150){ // random/special matrices, aligned or not, pOut aliasing the first operand
            const auto value=[&]()->std::uint32_t{float x;switch(u32()%10){case 0:return 0;case 1:return 0x80000000U;case 2:return 0x7f800000U;case 3:return u32()&0x807fffffU;
                case 4:x=std::uniform_real_distribution<float>(-1e20F,1e20F)(rng);break;default:x=std::uniform_real_distribution<float>(-3,3)(rng);}std::uint32_t b;std::memcpy(&b,&x,4);return b;};
            const auto a=matrix_at+(n%2?8U:0U),b=second_at+(n%3?0U:4U);
            for(std::uint32_t k=0;k<16;++k){c.memory.push_back({a+4*k,value()});c.memory.push_back({b+4*k,value()});}
            c.mxcsr=0x1f80U|((u32()%4)<<13);c.cw=0x7f;
            if(entry==0x774310)c.arguments={n%5==0?a:out_at,a,b};else c.arguments={out_at,b,a};}
        else break;
        out.push_back(c);}
    return out;
}
// Runs `cases` on an original guest and on one with `bridge` installed; returns the number compared, or the
// first mismatch's description in `mismatch` (nullopt mismatch = all equal).
// The ranges a case's call may store to: the entry's fixed ranges and those addressed by its pointer arguments.
std::vector<std::pair<std::uint32_t,std::uint32_t>> ranges(const OverrideEntry& entry,const Case& c){
    auto r=entry.writes;if(entry.address==0x419030)r.push_back({collision_matrix_at,384});for(const auto& [index,size]:entry.pointer_writes)if(index<c.arguments.size()&&c.arguments[index])r.push_back({c.arguments[index],size});return r;}
std::size_t compare(const ExecutableImage& image,const OverrideEntry& entry,const X86Guest::Override& bridge,const std::vector<Case>& cases,std::optional<std::string>& mismatch){
    std::string error;auto original=X86Guest::create(&error);auto native=X86Guest::create(&error);require(original&&native,error);
    require(original->map_image(image,&error)&&native->map_image(image,&error),error);
    require(original->map(scratch,0x4000,true,&error)&&native->map(scratch,0x4000,true,&error),error);
    FpuShimStats shims;require(install_fpu_shims(*original,load_fpu_sites("reference/inventory/fpu-sites.json"),shims,&error),error);
    require(native->add_override(entry.address,bridge,&error),error);
    std::size_t source_bodies=0;
    if(entry.address==0x419030)require(native->add_instruction_hook(0x419031,[&](X86Guest&,std::uint32_t){++source_bodies;return false;},&error),error);
    std::size_t compared=0;
    for(const auto& c:cases){
        auto out=ranges(entry,c);
        if(entry.address==0x40f3a0)out.push_back({scratch+0x600,32}); // suffix-entry observations
        // A prior case's profile fixture must not leak into the next plain case.
        if(entry.address==0x40f3a0){const std::uint8_t tail[]={0xc3,0x90,0x90,0x90,0x90,0x90,0x90,0x90};
            original->write(0x40f451,tail,sizeof tail);native->write(0x40f451,tail,sizeof tail);}
        for(auto* g:{original.get(),native.get()}){g->set_reg(UC_X86_REG_EFLAGS,c.eflags);
            if(entry.address==0x40f3a0||entry.address==0x419030){g->set_reg(UC_X86_REG_ESI,0x13579bdf);g->set_reg(UC_X86_REG_EDI,0x2468ace0);g->set_reg(UC_X86_REG_EBX,0xdeadbeef);}
            for(const auto& [at,size]:out){const std::vector<std::uint8_t> z(size,0xa5);g->write(at,z.data(),size);}
            for(const auto& [at,v]:c.memory)g->set_u32(at,v);}
        const auto before_source_bodies=source_bodies;
        const auto ra=original->call(entry.address,c.arguments,c.ecx,std::nullopt,c.cw,600'000'000,c.mxcsr);
        const auto rb=native->call(entry.address,c.arguments,c.ecx,std::nullopt,c.cw,600'000'000,c.mxcsr);
        require(bool(ra),entry.name+": original failed: "+ra.error);
        if(!rb){mismatch=entry.name+": override failed: "+rb.error;return compared;}
        if((entry.address==0x40f350||entry.address==0x40f3a0)&&((original->reg(UC_X86_REG_EFLAGS)^native->reg(UC_X86_REG_EFLAGS))&0xcd5U))
            {mismatch=entry.name+": arithmetic flags differ";return compared;}
        if(entry.address==0x419030){
            if(source_bodies-before_source_bodies!=(c.source_body?1U:0U)){mismatch=entry.name+": native/original execution path differs";return compared;}
            if(ra.esp!=rb.esp){mismatch=entry.name+": ESP differs";return compared;}
            if(original->reg(UC_X86_REG_FPCW)!=native->reg(UC_X86_REG_FPCW)
                ||original->reg(UC_X86_REG_FPTAG)!=native->reg(UC_X86_REG_FPTAG)
                ||((original->fpu_status()^native->fpu_status())&0x3800U)){mismatch=entry.name+": x87 control/stack differs";return compared;}
            // Translation-only overwrites the caller's second argument.
            if(original->u32(ra.esp+4)!=native->u32(rb.esp+4)){mismatch=entry.name+": caller argument store differs";return compared;}
            if((original->reg(UC_X86_REG_EFLAGS)^native->reg(UC_X86_REG_EFLAGS))&0x8d5U){mismatch=entry.name+": arithmetic flags differ";return compared;}
        }
        if(entry.address==0x40f3a0||entry.address==0x419030){
            if(ra.esp!=rb.esp){mismatch=entry.name+": caller stack differs";return compared;}
            for(const auto reg:{UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP})
                if(original->reg(reg)!=native->reg(reg)){mismatch=entry.name+": preserved register differs";return compared;}
        }
        if(entry.returns_eax&&ra.eax!=rb.eax){mismatch=entry.name+": EAX differs";return compared;}
        if(entry.returns_x87&&(original->st(0)!=native->st(0)||((original->fpu_status()^native->fpu_status())&0x3800)||original->reg(UC_X86_REG_FPTAG)!=native->reg(UC_X86_REG_FPTAG)))
            {mismatch=entry.name+": ST(0), TOP or the tag word differ (case "+std::to_string(compared)+")";return compared;}
        if(entry.returns_scratch&&(original->reg(UC_X86_REG_ECX)!=native->reg(UC_X86_REG_ECX)||original->reg(UC_X86_REG_EDX)!=native->reg(UC_X86_REG_EDX)
            ||((original->fpu_status()^native->fpu_status())&0x4700))){mismatch=entry.name+": ECX/EDX or the x87 condition bits differ (case "+std::to_string(compared)+")";return compared;}
        for(const auto& [at,size]:out){
            std::vector<std::uint8_t> x(size),y(size);original->read(at,x.data(),size);native->read(at,y.data(),size);
            if(x!=y){mismatch=entry.name+": the stores differ at "+std::to_string(at)+" (case "+std::to_string(compared)+")";return compared;}}
        ++compared;}
    return compared;
}
}
int main(int argc,char** argv){try{
    require(argc==3,"usage: guest-overrides-test SONIC.EXE ORACLE_DIR");
    std::ifstream f(argv[1],std::ios::binary);std::string error;
    const auto image=ExecutableImage::parse({std::istreambuf_iterator<char>(f),{}},&error);require(bool(image),error);
    const auto table=all_overrides(*image);
    std::mt19937 rng(0x0d1d);std::size_t total=0,from_corpora=0;
    for(const auto& entry:table){
        auto cases=corpus_cases(entry.address,argv[2]);const auto corpus=cases.size();
        if(!corpus)cases=enumerated_cases(entry.address,rng);
        require(!cases.empty(),entry.name+": no cases");
        std::optional<std::string> mismatch;const auto compared=compare(*image,entry,entry.bridge,cases,mismatch);
        require(!mismatch,mismatch.value_or(""));
        // The mutant: the real bridge, then one bit of the first store range flipped (a pOut range: through the argument).
        const auto fixed=entry.writes.empty()?std::optional<std::uint32_t>():entry.writes.front().first;const auto pointer=entry.pointer_writes.empty()?0U:entry.pointer_writes.front().first;
        const auto bridge=entry.bridge;std::optional<std::string> caught;
        if(entry.returns_x87&&entry.writes.empty()&&entry.pointer_writes.empty()) // no store range: flip the returned ST(0)'s lowest significand bit
            compare(*image,entry,[bridge](X86Guest& g)->std::uint32_t{const auto pops=bridge(g);if(g.stop_reason())return pops;const auto [s,e]=g.st(0);g.set_st(0,s^1,e);return pops;},cases,caught);
        else compare(*image,entry,[bridge,fixed,pointer](X86Guest& g)->std::uint32_t{
            const auto at=fixed?*fixed:g.u32(g.esp()+4+4*pointer).value_or(0);const auto pops=bridge(g);
            std::uint8_t b=0;g.read(at,&b,1);b^=1;g.write(at,&b,1);return pops;},cases,caught);
        require(bool(caught),entry.name+": the mutant (one flipped store bit) was not detected");
        if(entry.address==0x40f350){
            const std::vector<std::pair<std::string,X86Guest::Override>> mutants={
                {"wrong EAX",[bridge](X86Guest& g){const auto pops=bridge(g);g.set_eax(0);return pops;}},
                {"wrong ECX",[bridge](X86Guest& g){const auto pops=bridge(g);g.set_reg(UC_X86_REG_ECX,1);return pops;}},
                {"wrong CMP flags",[bridge](X86Guest& g){const auto pops=bridge(g);g.set_reg(UC_X86_REG_EFLAGS,g.reg(UC_X86_REG_EFLAGS)^0x40U);return pops;}},
                {"cleared trailing bytes",[bridge](X86Guest& g){const auto pops=bridge(g);const std::uint16_t zero=0;g.write(0x3b0e6e0,&zero,2);return pops;}},
                {"omitted last controller word",[bridge](X86Guest& g){const auto prior=g.u32(0x3b0e994);const auto pops=bridge(g);g.set_u32(0x3b0e994,*prior);return pops;}}
            };
            for(const auto& [name,mutant]:mutants){
                std::optional<std::string> failure;compare(*image,entry,mutant,cases,failure);
                require(bool(failure),"40f350 bridge mutant survived: "+name);
            }
            std::cout<<"  clear_input_edges: 5/5 register/flag/store-boundary bridge mutants caught\n";
        }
        if(entry.address==0x40f3a0){
            const std::vector<std::pair<std::string,X86Guest::Override>> mutants={
                {"wrong EAX",[bridge](X86Guest& g){const auto pops=bridge(g);g.set_eax(0);return pops;}},
                {"wrong ECX",[bridge](X86Guest& g){const auto pops=bridge(g);g.set_reg(UC_X86_REG_ECX,0);return pops;}},
                {"wrong CMP flags",[bridge](X86Guest& g){const auto pops=bridge(g);g.set_reg(UC_X86_REG_EFLAGS,g.reg(UC_X86_REG_EFLAGS)^0x40U);return pops;}},
                {"wrong raw record",[bridge](X86Guest& g){const auto pops=bridge(g);g.set_u32(0x3b0e9c8,*g.u32(0x3b0e9c8)^1U);return pops;}},
                {"wrong controller identity",[bridge](X86Guest& g){const auto pops=bridge(g);g.set_u32(0x3b0e788,0x3b0e9c8);return pops;}},
                {"cleared neighbour",[bridge](X86Guest& g){const auto pops=bridge(g);const std::uint8_t zero=0;g.write(0x3b0e9a8,&zero,1);return pops;}},
                {"skipped patched suffix",[bridge](X86Guest& g){const auto pops=bridge(g);if(!(g.reg(UC_X86_REG_EFLAGS)&0x400U))g.tail_redirect(scratch+0x900);return pops;}}
            };
            for(const auto& [name,mutant]:mutants){
                std::optional<std::string> failure;compare(*image,entry,mutant,cases,failure);
                require(bool(failure),"40f3a0 bridge mutant survived: "+name);
            }
            std::cout<<"  init_controllers: 7/7 register/store/profile-suffix bridge mutants caught\n";
        }
        if(entry.address==0x419030){
            const std::vector<std::pair<std::string,X86Guest::Override>> mutants={
                {"wrong task position",[bridge](X86Guest& g){g.set_u32(g.esp()+4,*g.u32(g.esp()+4)+4);return bridge(g);}},
                {"wrong rotation order",[bridge](X86Guest& g){const auto shape=*g.u32(g.esp()+8);g.set_u32(shape+4,*g.u32(shape+4)^0x200U);return bridge(g);}},
                {"wrong capacity",[bridge](X86Guest& g){g.set_u32(0x3d0fa28,0);return bridge(g);}},
                {"wrong ECX",[bridge](X86Guest& g){const auto pops=bridge(g);g.set_reg(UC_X86_REG_ECX,1);return pops;}},
                {"wrong EDX",[bridge](X86Guest& g){const auto pops=bridge(g);g.set_edx(1);return pops;}},
                {"wrong FP control",[bridge](X86Guest& g){g.set_reg(UC_X86_REG_FPCW,0x27f);return bridge(g);}},
                {"original-only override",[](X86Guest& g){const auto esp=g.esp();g.set_u32(esp-4,g.reg(UC_X86_REG_ECX));g.set_reg(UC_X86_REG_ESP,esp-4);g.tail_redirect(0x419031);return 0U;}}
            };
            for(const auto& [name,mutant]:mutants){
                std::optional<std::string> failure;compare(*image,entry,mutant,cases,failure);
                require(bool(failure),"419030 bridge mutant survived: "+name);
            }
            std::size_t original_paths=0;for(const auto& c:cases)original_paths+=c.source_body;
            std::cout<<"  collision_shape_center: "<<cases.size()-original_paths<<" native / "<<original_paths
                <<" original-body cases; 7/7 bridge mutants caught\n";
        }
        total+=compared;from_corpora+=corpus?compared:0;
        std::cout<<"  "<<entry.name<<" ("<<std::hex<<entry.address<<std::dec<<"): "<<compared<<(corpus?" oracle-corpus":" enumerated")<<" cases equivalent; mutant caught\n";}
    { // 772615 binds only the SSE dispatch table: with the image's slot stubs (77350c not yet run) it stops, never guesses
        const auto e=std::find_if(table.begin(),table.end(),[](const OverrideEntry& x){return x.address==0x772615;});require(e!=table.end(),"772615 not bound");
        auto native=X86Guest::create(&error);require(native&&native->map_image(*image,&error)&&native->map(scratch,0x1000,true,&error)&&native->add_override(0x772615,e->bridge,&error),error);
        const auto r=native->call(0x772615,{out_at,project_v_at,0,0,0,0});
        require(!r&&r.error.find("dispatch slots")!=std::string::npos,"772615: unset dispatch slots did not stop explicitly");
        std::cout<<"  d3dx_vec3_project (772615): unset dispatch slots stop explicitly\n";}
    { // 78dbe0 likewise: slot 14 still the 770284 stub (and slots 3/10 set) stops by name
        const auto e=std::find_if(table.begin(),table.end(),[](const OverrideEntry& x){return x.address==0x78dbe0;});require(e!=table.end(),"78dbe0 not bound");
        auto native=X86Guest::create(&error);require(native&&native->map_image(*image,&error)&&native->add_override(0x78dbe0,e->bridge,&error),error);
        native->set_u32(0x389b784,0x774310);native->set_u32(0x389b7a0,0x774150);
        const auto r=native->call(0x78dbe0,{0x41200000U});
        require(!r&&r.error.find("dispatch slots")!=std::string::npos,"78dbe0: unset dispatch slot 14 did not stop explicitly");
        std::cout<<"  ninja_projected_depth (78dbe0): unset dispatch slot 14 stops explicitly\n";}
    std::cout<<"Guest overrides: "<<table.size()<<" overrides equivalent to the original bytes on "<<total<<" cases ("<<from_corpora<<" from oracle corpora); every mutant caught\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
