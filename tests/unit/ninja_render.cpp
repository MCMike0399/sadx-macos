#include "ninja/ninja_render.hpp"
#include <vector>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace sadx;
namespace {template<class T>void check(const T& v){if(!static_cast<bool>(v))throw std::runtime_error("render assertion");}}
int main(int argc,char** argv){try{
    check(argc==2);std::array<std::uint8_t,56> bytes{};std::ifstream file(argv[1],std::ios::binary);file.read(reinterpret_cast<char*>(bytes.data()),bytes.size());check(file.gcount()==56 && file.peek()==EOF);
    const auto tables=decode_ninja_blend_tables(bytes.data(),bytes.size());check(tables && tables->states[0]==19 && tables->states[1]==20 && tables->values[8]==5 && tables->values[10]==2);
    check(!decode_ninja_blend_tables(bytes.data(),55));
    {   // 77ecb7 reads values 12..15 from the contiguous .rdata words after the table.
        std::vector<std::uint8_t> contiguous(bytes.begin(),bytes.end());for(std::uint32_t i=0;i<8;++i)for(unsigned b=0;b<4;++b)contiguous.push_back(static_cast<std::uint8_t>((0x100+i)>>(8*b)));
        const auto wide=decode_ninja_blend_tables(contiguous.data(),contiguous.size());check(wide&&wide->following_words_at_88ad9c&&(*wide->following_words_at_88ad9c)[3]==0x103);
        NinjaRenderState s;s.blend_tables=&*wide;s.packed_blend_at_3ab9890=0;s.device_at_3d128b0=std::make_shared<NinjaRenderDevice>();
        std::vector<NinjaRenderStateCommand> issued;s.set_render_state=[&](const auto& c){issued.push_back(c);return NinjaRenderCallResult{true,0};};
        for(unsigned value=12;value<16;++value)check(ninja_set_blend(1,value,s)&&issued.back().state==wide->states[1]&&issued.back().value==0x100+value-12);
        check(ninja_set_blend(0,16,s).failure==NinjaRenderFailure::unsupported_value_index);
        s.blend_tables=&*tables;check(ninja_set_blend(0,12,s).failure==NinjaRenderFailure::unsupported_value_index&&(*s.packed_blend_at_3ab9890&15)==12);
    }
    NinjaRenderState state;state.blend_tables=&*tables;auto device=std::make_shared<NinjaRenderDevice>();device->identity=9;state.device_at_3d128b0=device;
    state.set_render_state=[](const auto&){return NinjaRenderCallResult{true,-1};};
    for(unsigned old=0;old<256;++old)for(unsigned selector=0;selector<2;++selector)for(unsigned value=0;value<12;++value){
        state.packed_blend_at_3ab9890=static_cast<std::uint8_t>(old);const auto result=ninja_set_blend(selector,value,state);check(result);
        check(*state.packed_blend_at_3ab9890==(selector==0?((old&240)|value):((old&15)|(value<<4))));
        check(result.commands.size()==(value<2?2:1) && result.ignored_hresults.size()==result.commands.size());
        check(result.commands[0].device==device && result.ignored_hresults[0]==-1);
    }
    state.packed_blend_at_3ab9890.reset();check(ninja_set_blend(0,8,state).failure==NinjaRenderFailure::unknown_packed_blend);
    check(ninja_set_blend(99,0,state)); // source ignores selector for special value0/1
    state.set_render_state=[](const auto&){return NinjaRenderCallResult{true,{}};};
    auto native_receipt=ninja_set_blend(99,0,state);check(native_receipt && !native_receipt.ignored_hresults[0]); // native backend has no original HRESULT
    state.set_render_state=[](const auto&){return NinjaRenderCallResult{false,{}};};
    check(ninja_set_blend(99,0,state).failure==NinjaRenderFailure::renderer_unavailable);
    state.packed_blend_at_3ab9890=0xab;state.device_at_3d128b0.reset();auto unknown=ninja_set_blend(0,8,state);
    check(unknown.failure==NinjaRenderFailure::unknown_device && *state.packed_blend_at_3ab9890==0xa8);
    state.device_at_3d128b0=NinjaRenderDeviceHandle{};check(ninja_set_blend(0,8,state).failure==NinjaRenderFailure::null_device);
    state.device_at_3d128b0=device;state.set_render_state={};auto required=ninja_set_blend(1,10,state);
    check(required.failure==NinjaRenderFailure::missing_renderer && required.pending_command && required.pending_command->value==2 && *state.packed_blend_at_3ab9890==0xa8);
    unsigned calls=0;state.set_render_state=[&](const auto&){++calls;state.device_at_3d128b0.reset();return NinjaRenderCallResult{true,0};};
    state.device_at_3d128b0=device;auto mutated=ninja_set_blend(0,0,state);check(mutated.failure==NinjaRenderFailure::unknown_device && mutated.source_boundary==0x77ec7a && calls==1 && mutated.commands.size()==1);
    std::array<std::uint32_t,8> depth_table{1,2,3,4,5,6,7,8};
    check(ninja_set_depth_comparison(3,nullptr,state).failure==NinjaRenderFailure::unknown_device);
    state.device_at_3d128b0=device;state.set_render_state=[](const auto&){return NinjaRenderCallResult{true,{}};};
    auto depth=ninja_set_depth_comparison(0xffffffff,&depth_table,state);check(depth&&depth.commands[0].state==23&&depth.commands[0].value==8);
    auto write=ninja_set_depth_write(42,state);check(write&&write.commands[0].state==14&&write.commands[0].value==42);
    std::cout<<"Ninja blend state: source masks, ordered commands, ignored HRESULTs and explicit live resource boundaries passed.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
