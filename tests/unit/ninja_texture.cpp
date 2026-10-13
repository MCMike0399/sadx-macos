#include "engine/ninja_texture.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace {
void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
std::vector<std::uint8_t> read(const char* path){std::ifstream f(path,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
void word(std::vector<std::uint8_t>& b,std::size_t at,std::uint32_t value){for(std::size_t i=0;i<4;++i)b[at+i]=static_cast<std::uint8_t>(value>>(i*8));}
void test(int argc,char** argv){
    require(argc==9,"usage: texture-test EFF_REGULAR BEACH01 BEACH02 BEACH03 OBJ_BEACH BEACH_SEA BG_BEACH EC_CLOUDS");
    std::size_t count=0,levels=0;
    for(int p=1;p<argc;++p){auto parsed=sadx::parse_ninja_texture_resource(read(argv[p]));require(static_cast<bool>(parsed),"Actual archive parses");require(parsed.archive.pvm_count&&*parsed.archive.pvm_count==parsed.archive.records.size(),"All archive records remain owned");
        for(std::size_t n=0;n<parsed.archive.records.size();++n){auto decoded=sadx::decode_ninja_texture(parsed.archive,n);require(static_cast<bool>(decoded),"Actual needed format decodes");++count;levels+=decoded.levels.size();for(const auto& level:decoded.levels)require(level.rgba8.size()==std::size_t(level.width)*level.height*4&&level.packed16.size()*4==level.rgba8.size(),"Expansion preserves exact packed samples");}
        if(p==1){const auto& b=parsed.archive.records[130];require(b.table_index==130&&b.table_global_index==0x184f&&b.chunk.offset==0xda730&&b.chunk.size==8224&&b.payload.offset==0xda740&&b.payload.size==8208&&b.pixel_format==0&&b.data_format==9&&b.width==64&&b.height==64,"Pinned bubble metadata and original16-byte pixel offset");
            auto decoded=sadx::decode_ninja_texture(parsed.archive,130);require(decoded.tail->size==16&&decoded.levels[0].pixels.size==8192,"Tail is retained rather than counted as pixels");
            auto broken=parsed.archive.bytes;broken.pop_back();auto truncated=sadx::parse_ninja_texture_resource(std::move(broken));require(truncated.failure==sadx::TextureFailure::truncated_chunk&&truncated.archive.records.size()==135,"Truncated final chunk retains parsed prefix");
            broken=parsed.archive.bytes;word(broken,4,4);require(sadx::parse_ninja_texture_resource(std::move(broken)).failure==sadx::TextureFailure::truncated_metadata,"Header count is checked before table reads");
            broken=parsed.archive.bytes;broken[b.chunk.offset+8]=7;auto unsupported=sadx::parse_ninja_texture_resource(std::move(broken));require(unsupported&&unsupported.archive.records[130].pixel_format==7&&sadx::decode_ninja_texture(unsupported.archive,130).failure==sadx::TextureFailure::unsupported_pixel_format,"Unknown pixel format retained, not guessed");
            broken=parsed.archive.bytes;broken[b.chunk.offset+9]=8;unsupported=sadx::parse_ninja_texture_resource(std::move(broken));require(unsupported&&sadx::decode_ninja_texture(unsupported.archive,130).failure==sadx::TextureFailure::unsupported_data_format,"Unknown data layout retained");
            auto tiny=std::vector<std::uint8_t>(parsed.archive.bytes.begin()+b.chunk.offset,parsed.archive.bytes.begin()+b.chunk.offset+18);word(tiny,4,10);auto short_pixels=sadx::parse_ninja_texture_resource(tiny);require(short_pixels&&sadx::decode_ninja_texture(short_pixels.archive,0).failure==sadx::TextureFailure::truncated_pixels,"Valid chunk can still lack its declared pixels");
            tiny.resize(16);word(tiny,4,8);tiny[12]=0;tiny[13]=0;auto zero=sadx::parse_ninja_texture_resource(std::move(tiny));require(zero&&sadx::decode_ninja_texture(zero.archive,0).failure==sadx::TextureFailure::invalid_dimensions,"Zero dimensions never synthesize an empty success");
            // A synthetic standalone GBIX does not overwrite the separate PVM
            // table identity. Unknown chunks and nonzero GBIX suffix remain raw.
            std::vector<std::uint8_t> single(16);single[0]='G';single[1]='B';single[2]='I';single[3]='X';word(single,4,8);word(single,8,0x12345678);word(single,12,0xabcdef01);
            single.insert(single.end(),parsed.archive.bytes.begin()+b.chunk.offset,parsed.archive.bytes.begin()+b.chunk.offset+b.chunk.size);
            auto standalone=sadx::parse_ninja_texture_resource(std::move(single));require(standalone&&standalone.archive.records[0].gbix_global_index==0x12345678&&!standalone.archive.records[0].table_global_index&&standalone.archive.records[0].gbix_chunks.size()==1&&standalone.archive.chunks[0].bytes.size==16,"GBIX identity and raw suffix are preserved without invented table metadata");
            std::vector<std::uint8_t> low_flags(18);low_flags[0]='P';low_flags[1]='V';low_flags[2]='M';low_flags[3]='H';word(low_flags,4,10);low_flags[8]=8;low_flags[10]=1;low_flags[12]=17;word(low_flags,14,0x76543210);
            low_flags.insert(low_flags.end(),parsed.archive.bytes.begin()+b.chunk.offset,parsed.archive.bytes.begin()+b.chunk.offset+b.chunk.size);
            auto source_flags=sadx::parse_ninja_texture_resource(std::move(low_flags));require(source_flags&&source_flags.archive.records[0].table_index==17&&source_flags.archive.records[0].table_global_index==0x76543210&&!source_flags.archive.records[0].name,"Original bit8 global-index stride is independent of bit1 filenames");
        }
    }
    std::cout<<"Texture assets: "<<count<<" actual records and "<<levels<<" levels, bubble ranges, partial parse and unsupported/truncated cases passed.\n";
}
}
int main(int argc,char** argv){try{test(argc,argv);return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
