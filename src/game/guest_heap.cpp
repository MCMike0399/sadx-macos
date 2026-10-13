#include "game/guest_heap.hpp"
namespace sadx {
GuestHeap::GuestHeap(std::uint32_t handle_token,std::uint32_t first_address):handle_(handle_token),next_(first_address){}
std::uint32_t GuestHeap::create(){trace.push_back({'C',handle_,0});return handle_;}
std::optional<std::uint32_t> GuestHeap::alloc(std::uint32_t handle,std::uint32_t size){
    if(handle!=handle_){trace.push_back({'A',size,0});return std::nullopt;}
    const std::uint64_t span=(static_cast<std::uint64_t>(size==0?1:size)+7U)&~std::uint64_t{7};
    if(static_cast<std::uint64_t>(next_)+span+8U>0xffffffffULL){trace.push_back({'A',size,0});return std::nullopt;}
    const auto address=next_;blocks_[address]=Block{size,std::vector<std::optional<std::uint8_t>>(size==0?1:size)};
    next_=static_cast<std::uint32_t>(next_+span+8U); // an 8-byte gap keeps blocks distinct, as heap headers do
    trace.push_back({'A',size,address});return address;
}
bool GuestHeap::free(std::uint32_t handle,std::uint32_t address){
    trace.push_back({'F',address,0});
    if(handle!=handle_)return false;
    return blocks_.erase(address)==1;
}
const GuestHeap::Block* GuestHeap::find(std::uint32_t address,std::uint32_t& offset)const{
    auto it=blocks_.upper_bound(address);if(it==blocks_.begin())return nullptr;--it;
    offset=address-it->first;if(offset>=it->second.bytes.size())return nullptr;
    return &it->second;
}
std::optional<std::uint8_t> GuestHeap::read(std::uint32_t address)const{
    std::uint32_t offset=0;const auto* b=find(address,offset);if(!b)return std::nullopt;return b->bytes[offset];
}
bool GuestHeap::write(std::uint32_t address,std::uint8_t value){
    auto it=blocks_.upper_bound(address);if(it==blocks_.begin())return false;--it;
    const auto offset=address-it->first;if(offset>=it->second.bytes.size())return false;
    it->second.bytes[offset]=value;return true;
}
} // namespace sadx
