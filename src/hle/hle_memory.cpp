#include "hle/hle_memory.hpp"
#include <vector>
namespace sadx {
HleMemory::HleMemory(X86Guest& guest,std::uint32_t base,std::uint32_t limit):guest_(guest),base_(base),limit_(limit),top_(base),mapped_(base){}
std::optional<std::uint32_t> HleMemory::allocate(std::uint32_t size,std::uint32_t alignment){
    const std::uint32_t need=(std::max<std::uint32_t>(size,1)+15U)&~15U;
    if(alignment<16)alignment=16;
    for(auto i=free_.begin();i!=free_.end();++i){
        const auto at=(i->first+alignment-1)&~(alignment-1);
        if(at-i->first+need<=i->second){
            const auto block=i->first,space=i->second;free_.erase(i);
            if(at>block)free_[block]=at-block;
            if(block+space>at+need)free_[at+need]=block+space-(at+need);
            live_[at]=need;const std::vector<std::uint8_t> zero(need,0);guest_.write(at,zero.data(),need);return at;}
    }
    const auto at=(top_+alignment-1)&~(alignment-1);
    if(at<top_||need>limit_-at)return std::nullopt;
    // RWX: VirtualAlloc(PAGE_EXECUTE_*) blocks hold generated code (the loader's trampolines); DEP is not modelled.
    while(mapped_<at+need){if(!guest_.map(mapped_,0x100000,true,nullptr))return std::nullopt;mapped_+=0x100000;}
    if(at>top_)free_[top_]=at-top_;
    top_=at+need;live_[at]=need;
    const std::vector<std::uint8_t> zero(need,0);guest_.write(at,zero.data(),need);
    return at;
}
bool HleMemory::free(std::uint32_t address){
    const auto i=live_.find(address);if(i==live_.end())return false;
    free_[address]=i->second;live_.erase(i);return true;
}
std::optional<std::uint32_t> HleMemory::size(std::uint32_t address)const{const auto i=live_.find(address);if(i==live_.end())return std::nullopt;return i->second;}
std::optional<std::uint32_t> HleMemory::reallocate(std::uint32_t address,std::uint32_t size){
    const auto old=this->size(address);if(!old)return std::nullopt;
    const auto fresh=allocate(size);if(!fresh)return std::nullopt;
    std::vector<std::uint8_t> bytes(std::min(*old,size));guest_.read(address,bytes.data(),bytes.size());guest_.write(*fresh,bytes.data(),bytes.size());
    free(address);return fresh;
}
} // namespace sadx
