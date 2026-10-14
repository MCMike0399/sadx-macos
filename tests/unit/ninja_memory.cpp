#include "game/game_winmain.hpp"
#include <iostream>
#include <stdexcept>
// njMalloc/njFree: lazy HeapCreate into 389d628, unknown handle, free of 0;
// 401a20 with no CD-ROM leaves 3a8af78 empty and frees its buffer.
namespace {void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}}
int main(){try{
    using namespace sadx;
    GuestHeap heap;NinjaMemoryState s;s.heap_at_389d628=0xffffffffu;
    const auto a=nj_malloc_780f60(s,heap,16);require(a&&a.created&&s.heap_at_389d628==0x00510000u&&a.eax!=0,"first njMalloc creates the heap");
    const auto b=nj_malloc_780f60(s,heap,16);require(b&&!b.created&&b.eax!=a.eax,"later calls reuse it");
    require(bool(nj_free_780fb0(s,heap,0))&&bool(nj_free_780fb0(s,heap,a.eax))&&heap.live_blocks()==1,"free of 0 and of a block");
    NinjaMemoryState unknown;require(nj_malloc_780f60(unknown,heap,4).source_boundary==0x780f60u,"unknown 389d628");
    NinjaMemoryState external{0xffffffffU};unsigned allocations=0;
    NinjaMemoryMethods methods{[]{return std::optional<std::uint32_t>(0);},[&](std::uint32_t h,std::uint32_t n){require(external.heap_at_389d628==h&&h==0&&n==7,"created zero handle is stored before allocation");++allocations;return std::optional<std::uint32_t>(0);},[](std::uint32_t,std::uint32_t){return std::optional<std::uint32_t>(0x80004005U);}};
    const auto failed_alloc=nj_malloc_780f60(external,methods,7);require(failed_alloc&&failed_alloc.created&&failed_alloc.eax==0&&allocations==1,"ordinary zero allocation is a completed call");
    require(nj_free_780fb0(external,methods,1).eax==0x80004005U,"free retains raw platform receipt");
    methods.allocate={};require(nj_malloc_780f60(external,methods,7).failure==NinjaMemoryFailure::host_unavailable,"unavailable allocator is distinct from zero result");
    require(bool(nj_free_780fb0(unknown,NinjaMemoryMethods{},0)),"null free requires no known heap or methods");
    CdDriveState cd;for(auto& v:cd.root_at_3a8af78.bytes)v=0x41;
    CdDriveHost host{[](std::uint32_t size)->std::optional<std::pair<std::uint32_t,std::vector<std::uint8_t>>>{
            std::vector<std::uint8_t> bytes{'C',':','\\',0,0};if(size<bytes.size())return std::pair<std::uint32_t,std::vector<std::uint8_t>>{5,{}};return std::pair<std::uint32_t,std::vector<std::uint8_t>>{4,bytes};},
        [](const std::string&)->std::optional<std::uint32_t>{return 3;},[](const std::string&)->std::optional<bool>{return false;}};
    const auto live=heap.live_blocks();const auto r=run_cd_drive_scan_401a20(cd,s,heap,host);
    require(r&&cd.root_at_3a8af78.bytes[0]==0&&cd.root_at_3a8af78.bytes[1]==0x41&&r.queries.size()==1&&r.queries[0].kind=='D'&&heap.live_blocks()==live,"no CD-ROM: empty root, buffer freed");
    std::cout<<"njMalloc/njFree and 401a20: lazy heap, reuse, frees, unknown handle and the no-CD scan passed.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
