#include "ninja/ninja_texture.hpp"
#include <algorithm>
#include <cstring>
extern "C" int sadx_native_ninja_texture(const std::uint8_t* bytes,std::size_t size,
    std::size_t index,std::size_t level,std::uint16_t* pixels,std::size_t capacity,
    std::uint8_t* rgba,std::size_t rgba_capacity,std::uint64_t* metadata){
    if(!bytes||!metadata)return -1;
    auto parsed=sadx::parse_ninja_texture_resource({bytes,bytes+size});
    metadata[0]=static_cast<unsigned>(parsed.failure);metadata[1]=parsed.source_offset;
    metadata[2]=parsed.archive.records.size();if(!parsed)return 1;
    if(index>=parsed.archive.records.size())return -2;
    const auto& record=parsed.archive.records[index];auto result=sadx::decode_ninja_texture(parsed.archive,index);
    metadata[3]=static_cast<unsigned>(result.failure);metadata[4]=result.source_offset;
    metadata[5]=record.chunk.offset;metadata[6]=record.chunk.size;metadata[7]=record.payload.offset;
    metadata[8]=record.payload.size;metadata[9]=record.pixel_format;metadata[10]=record.data_format;
    metadata[11]=record.width;metadata[12]=record.height;metadata[13]=record.reserved;
    metadata[14]=record.table_global_index?*record.table_global_index:0x100000000ULL;
    metadata[15]=record.table_index?*record.table_index:0x100000000ULL;
    metadata[16]=result.levels.size();if(!result)return 2;if(level>=result.levels.size())return -3;
    const auto& decoded=result.levels[level];metadata[17]=decoded.width;metadata[18]=decoded.height;
    metadata[19]=decoded.pixels.offset;metadata[20]=decoded.pixels.size;
    metadata[21]=result.tail->offset;metadata[22]=result.tail->size;
    if(capacity<decoded.packed16.size()||rgba_capacity<decoded.rgba8.size())return -4;
    if(!pixels||!rgba)return -5;
    std::copy(decoded.packed16.begin(),decoded.packed16.end(),pixels);
    std::copy(decoded.rgba8.begin(),decoded.rgba8.end(),rgba);return 0;
}
extern "C" const char* sadx_native_ninja_texture_compiler(){return "Clang " __clang_version__ "; ARM64 C++17 O2 read-only PVM/PVRT source16-bit layouts; RGBA8 floor255/max policy";}
