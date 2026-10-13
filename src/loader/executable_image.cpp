#include "loader/executable_image.hpp"
#include <algorithm>
namespace sadx {
namespace {
std::optional<std::uint32_t> le(const std::vector<std::uint8_t>& b,std::size_t at,unsigned n){
    if(at>b.size()||n>b.size()-at)return std::nullopt;std::uint32_t v=0;for(unsigned i=0;i<n;++i)v|=std::uint32_t(b[at+i])<<(8*i);return v;}
}
std::optional<ExecutableImage> ExecutableImage::parse(std::vector<std::uint8_t> bytes,std::string* error){
    const auto bad=[&](const char* why)->std::optional<ExecutableImage>{if(error)*error=why;return std::nullopt;};
    const auto mz=le(bytes,0,2),pe=le(bytes,0x3c,4);if(mz!=0x5a4du||!pe)return bad("not an MZ image");
    if(le(bytes,*pe,4)!=0x4550u)return bad("missing PE signature");
    const auto machine=le(bytes,*pe+4,2),count=le(bytes,*pe+6,2),optional_size=le(bytes,*pe+20,2),magic=le(bytes,*pe+24,2);
    if(machine!=0x14cu||magic!=0x10bu||!count||!optional_size)return bad("not a PE32 i386 image");
    const auto base=le(bytes,*pe+24+28,4);if(!base)return bad("truncated optional header");
    ExecutableImage image;image.base_=*base;const std::size_t table=*pe+24+*optional_size;
    image.size_of_image_=le(bytes,*pe+24+56,4).value_or(0);
    if(*optional_size>=96+8*6){image.relocation_rva_=le(bytes,*pe+24+96+40,4).value_or(0);image.relocation_size_=le(bytes,*pe+24+96+44,4).value_or(0);}
    for(std::uint32_t n=0;n<*count;++n){const auto at=table+40*n;const auto vsize=le(bytes,at+8,4),rva=le(bytes,at+12,4),rsize=le(bytes,at+16,4),roff=le(bytes,at+20,4);
        if(!vsize||!rva||!rsize||!roff)return bad("truncated section table");
        if(*rsize&&(*roff>bytes.size()||*rsize>bytes.size()-*roff))return bad("section raw data outside the file");
        image.sections_.push_back({*rva,std::max(*vsize,*rsize),*rsize,*roff});}
    image.bytes_=std::move(bytes);
    // Export directory (data directory 0).
    if(*optional_size>=96+8){
        const auto dir=le(image.bytes_,*pe+24+96,4),dir_size=le(image.bytes_,*pe+24+100,4);
        if(dir&&dir_size&&*dir){
            const auto rd=[&](std::uint32_t rva,unsigned n)->std::optional<std::uint32_t>{std::uint32_t v=0;for(unsigned i=0;i<n;++i){const auto b=image.byte(image.base_+rva+i);if(!b)return std::nullopt;v|=std::uint32_t(*b)<<(8*i);}return v;};
            const auto str=[&](std::uint32_t rva)->std::optional<std::string>{std::string s;for(std::uint32_t i=0;i<4096;++i){const auto b=image.byte(image.base_+rva+i);if(!b)return std::nullopt;if(!*b)return s;s.push_back(static_cast<char>(*b));}return std::nullopt;};
            const auto functions=rd(*dir+20,4),names=rd(*dir+24,4),address_table=rd(*dir+28,4),name_table=rd(*dir+32,4),ordinal_table=rd(*dir+36,4);
            if(!functions||!names||!address_table||!name_table||!ordinal_table)return bad("truncated export directory");
            for(std::uint32_t i=0;i<*names;++i){
                const auto name_rva=rd(*name_table+4*i,4),ordinal=rd(*ordinal_table+2*i,2);if(!name_rva||!ordinal||*ordinal>=*functions)return bad("invalid export name entry");
                const auto name=str(*name_rva);const auto function=rd(*address_table+4*(*ordinal),4);if(!name||!function)return bad("invalid export entry");
                Export e;if(*function>=*dir&&*function-*dir<*dir_size){e.forwarder=str(*function);if(!e.forwarder)return bad("invalid export forwarder");}else e.va=image.base_+*function;
                image.exports_.emplace(*name,std::move(e));}
        }
    }
    return image;
}
std::optional<std::uint8_t> ExecutableImage::byte(std::uint32_t va)const noexcept{
    if(va<base_)return std::nullopt;const auto rva=va-base_;
    for(const auto& s:sections_)if(rva>=s.rva&&rva-s.rva<s.virtual_size){const auto off=rva-s.rva;return off<s.raw_size?bytes_[s.raw_offset+off]:std::uint8_t(0);}
    return std::nullopt;
}
std::optional<ExecutableImage> ExecutableImage::rebased(std::uint32_t base,std::string* error)const{
    const auto bad=[&](const char* why)->std::optional<ExecutableImage>{if(error)*error=why;return std::nullopt;};
    if(!size_of_image_)return bad("missing SizeOfImage");
    std::vector<std::uint8_t> mapped(size_of_image_,0);
    for(const auto& s:sections_){
        if(s.raw_size&&s.rva>=size_of_image_)return bad("section outside SizeOfImage");
        const auto n=s.raw_size?std::min<std::size_t>(s.raw_size,size_of_image_-s.rva):0;
        std::copy_n(bytes_.begin()+s.raw_offset,n,mapped.begin()+s.rva);}
    const std::uint32_t delta=base-base_;
    for(std::uint32_t at=relocation_rva_;relocation_size_&&at+8<=relocation_rva_+relocation_size_;){
        const auto page=le(mapped,at,4),block=le(mapped,at+4,4);if(!page||!block||*block<8||*block>relocation_rva_+relocation_size_-at)return bad("invalid relocation block");
        for(std::uint32_t e=at+8;e+2<=at+*block;e+=2){
            const auto entry=*le(mapped,e,2);const auto type=entry>>12,offset=entry&0xfff;
            if(type==0)continue;
            if(type!=3)return bad("unsupported relocation type");
            const auto target=*page+offset;const auto v=le(mapped,target,4);if(!v)return bad("relocation outside the image");
            const auto r=*v+delta;for(unsigned i=0;i<4;++i)mapped[target+i]=static_cast<std::uint8_t>(r>>(8*i));}
        at+=*block;}
    ExecutableImage image;image.base_=base;image.size_of_image_=size_of_image_;image.sections_={{0,size_of_image_,size_of_image_,0}};image.bytes_=std::move(mapped);
    for(const auto& [name,e]:exports_){Export x=e;if(!x.forwarder)x.va=e.va-base_+base;image.exports_.emplace(name,std::move(x));}
    return image;
}
std::optional<ExecutableImage::Export> ExecutableImage::export_named(const std::string& name)const{const auto i=exports_.find(name);if(i==exports_.end())return std::nullopt;return i->second;}
std::optional<std::uint32_t> ExecutableImage::u32(std::uint32_t va)const noexcept{
    std::uint32_t v=0;for(unsigned i=0;i<4;++i){const auto b=byte(va+i);if(!b)return std::nullopt;v|=std::uint32_t(*b)<<(8*i);}return v;
}
} // namespace sadx
