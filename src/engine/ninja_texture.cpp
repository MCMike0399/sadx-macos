#include "engine/ninja_texture.hpp"
#include <algorithm>
#include <limits>
#include <new>
namespace sadx {
namespace {
bool fits(std::size_t start,std::size_t count,std::size_t limit){return start<=limit&&count<=limit-start;}
std::uint16_t u16(const std::vector<std::uint8_t>& b,std::size_t o){return std::uint16_t(b[o])|(std::uint16_t(b[o+1])<<8);}
std::uint32_t u32(const std::vector<std::uint8_t>& b,std::size_t o){return std::uint32_t(u16(b,o))|(std::uint32_t(u16(b,o+2))<<16);}
bool tag(const std::vector<std::uint8_t>& b,std::size_t p,const char* text){return fits(p,4,b.size())&&std::equal(b.begin()+p,b.begin()+p+4,text);}
bool pow2(std::uint16_t n){return n&&(n&(n-1))==0;}
std::size_t morton(std::size_t x,std::size_t y){std::size_t out=0;for(std::size_t k=0;x||y;++k){out|=(y&1)<<(2*k);out|=(x&1)<<(2*k+1);x>>=1;y>>=1;}return out;}
void rgba(std::uint16_t p,std::uint8_t format,std::uint8_t* dest){
    const auto scale=[](unsigned n,unsigned max){return static_cast<std::uint8_t>(n*255/max);};
    if(format==0){dest[0]=scale((p>>10)&31,31);dest[1]=scale((p>>5)&31,31);dest[2]=scale(p&31,31);dest[3]=(p&0x8000)?255:0;}
    else if(format==1){dest[0]=scale((p>>11)&31,31);dest[1]=scale((p>>5)&63,63);dest[2]=scale(p&31,31);dest[3]=255;}
    else {dest[0]=scale((p>>8)&15,15);dest[1]=scale((p>>4)&15,15);dest[2]=scale(p&15,15);dest[3]=scale((p>>12)&15,15);}
}
}
NinjaTextureParseResult parse_ninja_texture_resource(std::vector<std::uint8_t> bytes){
    NinjaTextureParseResult r;r.archive.bytes=std::move(bytes);auto& a=r.archive;const auto& b=a.bytes;
    const auto fail=[&](TextureFailure f,std::size_t o){r.failure=f;r.source_offset=o;return std::move(r);};
    std::size_t start=0;
    try {
        if(!tag(b,0,"PVMH")&&!tag(b,0,"PVRT")&&!tag(b,0,"GBIX"))return fail(TextureFailure::invalid_magic,0);
        std::size_t entry_bytes=0;bool archive=tag(b,0,"PVMH");
        if(archive){
            if(b.size()<12)return fail(TextureFailure::truncated_chunk,0);
            const auto length=u32(b,4);if(length<4)return fail(TextureFailure::invalid_chunk_length,4);
            if(!fits(8,length,b.size()))return fail(TextureFailure::truncated_chunk,0);
            start=8+length;a.header=TextureByteRange{0,start};a.pvm_flags=u16(b,8);a.pvm_count=u16(b,10);
            // Actual77f3f0: bit1 filename, bit2 formats, bit4 dimensions,
            // bit8 global index. Retain the unconsumed header suffix verbatim.
            entry_bytes=2+((*a.pvm_flags&1)?28:0)+((*a.pvm_flags&2)?2:0)+((*a.pvm_flags&4)?2:0)+((*a.pvm_flags&8)?4:0);
            const auto total=std::size_t(*a.pvm_count)*entry_bytes;
            if(!fits(12,total,start))return fail(TextureFailure::truncated_metadata,12);
            a.header_padding=TextureByteRange{12+total,start-12-total};
            a.chunks.push_back({{{'P','V','M','H'}},{0,start}});
        }
        std::vector<std::size_t> gbix;
        while(start<b.size()){
            if(!fits(start,8,b.size()))return fail(TextureFailure::truncated_chunk,start);
            const auto length=u32(b,start+4);if(!fits(start+8,length,b.size()))return fail(TextureFailure::truncated_chunk,start);
            NinjaTextureChunk chunk;std::copy_n(b.begin()+start,4,chunk.tag.begin());chunk.bytes={start,std::size_t(length)+8};a.chunks.push_back(chunk);
            if(tag(b,start,"GBIX")){
                if(length<4)return fail(TextureFailure::missing_gbix_index,start+8);
                gbix.push_back(a.chunks.size()-1);
            }else if(tag(b,start,"PVRT")){
                if(length<8)return fail(TextureFailure::invalid_chunk_length,start+4);
                NinjaTextureRecord rec;rec.ordinal=a.records.size();rec.chunk=chunk.bytes;rec.payload={start+16,std::size_t(length)-8};
                rec.pixel_format=b[start+8];rec.data_format=b[start+9];rec.reserved=u16(b,start+10);rec.width=u16(b,start+12);rec.height=u16(b,start+14);
                rec.gbix_chunks=std::move(gbix);gbix.clear();
                if(!rec.gbix_chunks.empty())rec.gbix_global_index=u32(b,a.chunks[rec.gbix_chunks.back()].bytes.offset+8);
                if(archive){
                    if(rec.ordinal>=*a.pvm_count)return fail(TextureFailure::texture_count_mismatch,start);
                    std::size_t p=12+rec.ordinal*entry_bytes;rec.table_record=TextureByteRange{p,entry_bytes};rec.table_index=u16(b,p);p+=2;
                    if(*a.pvm_flags&1){std::array<std::uint8_t,28> name;std::copy_n(b.begin()+p,28,name.begin());rec.name=name;p+=28;}
                    if(*a.pvm_flags&2){rec.table_formats=std::array<std::uint8_t,2>{{b[p],b[p+1]}};p+=2;}
                    if(*a.pvm_flags&4){rec.table_dimensions=std::array<std::uint8_t,2>{{b[p],b[p+1]}};p+=2;}
                    if(*a.pvm_flags&8)rec.table_global_index=u32(b,p);
                }
                a.records.push_back(std::move(rec));
            }
            start+=chunk.bytes.size;
        }
        if(archive&&a.records.size()!=*a.pvm_count)return fail(TextureFailure::texture_count_mismatch,start);
        if(!archive&&a.records.size()!=1)return fail(TextureFailure::texture_count_mismatch,start);
        return r;
    }catch(const std::bad_alloc&){return fail(TextureFailure::resource_exhausted,start);}
}
NinjaTextureDecodeResult decode_ninja_texture(const NinjaTextureArchive& a,std::size_t index){
    NinjaTextureDecodeResult out;out.record_index=index;
    const auto fail=[&](TextureFailure f,std::size_t p){out.failure=f;out.source_offset=p;return std::move(out);};
    if(index>=a.records.size())return fail(TextureFailure::invalid_record,0);
    const auto& r=a.records[index];const auto& b=a.bytes;
    if(!fits(r.payload.offset,r.payload.size,b.size()))return fail(TextureFailure::truncated_pixels,r.payload.offset);
    if(r.pixel_format>2)return fail(TextureFailure::unsupported_pixel_format,r.chunk.offset+8);
    if(r.data_format!=1&&r.data_format!=2&&r.data_format!=3&&r.data_format!=9)return fail(TextureFailure::unsupported_data_format,r.chunk.offset+9);
    if(!r.width||!r.height||(r.data_format!=9&&(!pow2(r.width)||r.width!=r.height))||(r.data_format==3&&r.width<2)||(r.data_format==2&&r.width>1024))return fail(TextureFailure::invalid_dimensions,r.chunk.offset+12);
    std::size_t consumed=0;
    const auto level=[&](std::uint16_t width,std::uint16_t height,std::size_t local,std::size_t count){
        NinjaTextureLevel result;result.width=width;result.height=height;result.pixels={r.payload.offset+local,count};
        const auto pixels=std::size_t(width)*height;result.packed16.resize(pixels);result.rgba8.resize(pixels*4);
        for(std::size_t y=0;y<height;++y)for(std::size_t x=0;x<width;++x){
            std::size_t p;
            if(r.data_format==3){const auto code=b[r.payload.offset+2048+morton(x/2,y/2)];p=r.payload.offset+std::size_t(code)*8+((x&1)*2+(y&1))*2;}
            else p=r.payload.offset+local+(r.data_format==9?(y*width+x):morton(x,y))*2;
            const auto raw=u16(b,p);const auto at=y*width+x;result.packed16[at]=raw;rgba(raw,r.pixel_format,result.rgba8.data()+at*4);
        }
        out.levels.push_back(std::move(result));
    };
    try {
        if(r.data_format==3){const auto count=std::size_t(r.width)*r.height/4;if(!fits(2048,count,r.payload.size))return fail(TextureFailure::truncated_pixels,r.payload.offset);
            out.codebook=TextureByteRange{r.payload.offset,2048};level(r.width,r.height,2048,count);consumed=2048+count;
        }else if(r.data_format==2){
            out.prefix_padding=TextureByteRange{r.payload.offset,2};std::vector<std::pair<std::uint16_t,std::size_t>> offsets;
            consumed=2;for(std::uint16_t n=1;;n=static_cast<std::uint16_t>(n*2)){const auto count=std::size_t(n)*n*2;if(!fits(consumed,count,r.payload.size))return fail(TextureFailure::truncated_pixels,r.payload.offset+consumed);offsets.push_back({n,consumed});consumed+=count;if(n==r.width)break;}
            for(auto i=offsets.rbegin();i!=offsets.rend();++i)level(i->first,i->first,i->second,std::size_t(i->first)*i->first*2);
        }else {const auto count=std::size_t(r.width)*r.height*2;if(!fits(0,count,r.payload.size))return fail(TextureFailure::truncated_pixels,r.payload.offset);level(r.width,r.height,0,count);consumed=count;}
        out.tail=TextureByteRange{r.payload.offset+consumed,r.payload.size-consumed};return out;
    }catch(const std::bad_alloc&){return fail(TextureFailure::resource_exhausted,r.payload.offset);}
}
} // namespace sadx
