#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
namespace sadx {
// Owner of a contiguous run of source globals that has no typed owner yet.
// Each byte is a fact or explicitly unknown; multi-byte accesses are
// little-endian and need every byte. Accesses outside the run are rejected.
template<std::uint32_t Base,std::size_t Size>
struct SourceBytes {
    static constexpr std::uint32_t base=Base;
    static constexpr std::size_t size=Size;
    std::array<std::optional<std::uint8_t>,Size> bytes{};
    static constexpr bool contains(std::uint32_t address,std::size_t width)noexcept{
        return address>=Base&&width<=Size&&address-Base<=Size-width;}
    bool write(std::uint32_t address,std::uint64_t value,std::size_t width){
        if(!contains(address,width))return false;
        for(std::size_t i=0;i<width;++i)bytes[address-Base+i]=static_cast<std::uint8_t>(value>>(8*i));
        return true;}
    bool fill(std::uint32_t address,std::size_t count,std::uint8_t value){
        if(!contains(address,count))return false;
        for(std::size_t i=0;i<count;++i)bytes[address-Base+i]=value;
        return true;}
    std::optional<std::uint64_t> read(std::uint32_t address,std::size_t width)const{
        if(!contains(address,width))return std::nullopt;
        std::uint64_t v=0;
        for(std::size_t i=0;i<width;++i){const auto& b=bytes[address-Base+i];if(!b)return std::nullopt;v|=std::uint64_t{*b}<<(8*i);}
        return v;}
};
} // namespace sadx
