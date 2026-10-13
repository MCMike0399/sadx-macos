#include "game/game_winmain.hpp"
#include <algorithm>
#include <cstring>
#include <string>
#include <vector>
// Native 401a20/401990 + 780f60/780fb0 with the layout of tools/verify_original_cd_drive_scan.py.
// in: [0] 389d628 [1] HeapAlloc fails (0/1) [2] drive count [3..10] drive types [11..18] disc file present
//     drives: NUL-separated drive strings; root: the 0x104 bytes of 3a8af78 before.
// out: [0] completed [1] 389d628 [2] live heap blocks; root: 3a8af78 after;
//      text: "D|drive" and "F|pattern" queries, then "H|kind|a|b" heap calls, in order.
extern "C" int sadx_native_cd_drive_scan(const std::uint32_t* in,const char* drives,const std::uint8_t* root_in,std::uint32_t* out,std::uint8_t* root_out,char* text,std::size_t text_size){
    using namespace sadx;
    std::vector<std::string> list;{const char* p=drives;for(std::uint32_t i=0;i<in[2];++i){list.emplace_back(p);p+=list.back().size()+1;}}
    CdDriveState s;for(std::size_t i=0;i<0x104;++i)s.root_at_3a8af78.bytes[i]=root_in[i];
    NinjaMemoryState memory;memory.heap_at_389d628=in[0];
    GuestHeap heap;
    CdDriveHost host;
    host.logical_drive_strings=[&](std::uint32_t size)->std::optional<std::pair<std::uint32_t,std::vector<std::uint8_t>>>{
        std::vector<std::uint8_t> bytes;for(const auto& d:list){bytes.insert(bytes.end(),d.begin(),d.end());bytes.push_back(0);}bytes.push_back(0);
        const auto total=static_cast<std::uint32_t>(bytes.size());
        if(size<total)return std::pair<std::uint32_t,std::vector<std::uint8_t>>{total,{}};
        return std::pair<std::uint32_t,std::vector<std::uint8_t>>{total-1,bytes};};
    host.drive_type=[&](const std::string& d)->std::optional<std::uint32_t>{for(std::size_t i=0;i<list.size();++i)if(list[i]==d)return in[3+i];return 1;};
    host.file_exists=[&](const std::string& p)->std::optional<bool>{for(std::size_t i=0;i<list.size();++i)if(p.rfind(list[i],0)==0)return in[11+i]!=0;return false;};
    GuestHeap failing(0x00510000u,0xfffffff0u); // every HeapAlloc fails
    GuestHeap& used=in[1]?failing:heap;
    const auto r=run_cd_drive_scan_401a20(s,memory,used,host);
    std::fill(out,out+16,0);out[0]=r?1U:0U;out[1]=memory.heap_at_389d628.value_or(0xdeadbeef);
    for(std::size_t i=0;i<0x104;++i)root_out[i]=s.root_at_3a8af78.bytes[i].value_or(0xef);
    std::string t;for(const auto& q:r.queries)t+=std::string(1,q.kind)+"|"+q.text+"\n";
    for(const auto& c:used.trace)t+=std::string("H|")+c.kind+"|"+std::to_string(c.a)+"|"+std::to_string(c.b)+"\n";
    std::snprintf(text,text_size,"%s",t.c_str());
    out[2]=static_cast<std::uint32_t>(used.live_blocks()); // blocks still live (0 after the source's njFree)
    return 1;
}
