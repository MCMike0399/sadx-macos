#include "loader/guest_heap.hpp"
#include <iostream>
#include <stdexcept>
// Native heap identities: token, aligned distinct blocks, unknown fresh bytes,
// writes/reads inside blocks only, frees and a foreign handle.
namespace {void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}}
int main(){try{
    sadx::GuestHeap heap;const auto h=heap.create();
    const auto a=heap.alloc(h,5),b=heap.alloc(h,0);
    require(a==0x50000000u&&b==0x50000010u,"8-byte rounded spans plus an 8-byte gap");
    require(!heap.read(*a)&&heap.write(*a+4,7)&&heap.read(*a+4)==7&&!heap.write(*a+5,1),"fresh bytes unknown; writes stay inside the block");
    require(!heap.alloc(h+1,4)&&heap.free(h,*a)&&!heap.free(h,*a)&&heap.live_blocks()==1,"foreign handle, double free");
    require(heap.trace.size()==6&&heap.trace[0].kind=='C'&&heap.trace[3].kind=='A'&&heap.trace[3].b==0,"call trace");
    std::cout<<"Guest heap: identities, alignment, unknown contents, bounds, frees and trace passed.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
