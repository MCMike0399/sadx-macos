#include "platform/mac/metal_texture.hpp"
#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>
#import <ImageIO/ImageIO.h>
#include <CommonCrypto/CommonDigest.h>
#include <array>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
namespace {
void require(bool ok,const std::string& detail){if(!ok)throw std::runtime_error(detail);}
std::vector<std::uint8_t> read(const char* p){std::ifstream file(p,std::ios::binary);require(static_cast<bool>(file),"Archive open failed");return {std::istreambuf_iterator<char>(file),{}};}
std::uint32_t bits(float value){std::uint32_t result;std::memcpy(&result,&value,4);return result;}
std::array<float,4> expected(std::uint16_t p,std::uint8_t format){
    if(format==0)return {{float((p>>10)&31)/31.0F,float((p>>5)&31)/31.0F,float(p&31)/31.0F,float(p>>15)}};
    if(format==1)return {{float((p>>11)&31)/31.0F,float((p>>5)&63)/63.0F,float(p&31)/31.0F,1.0F}};
    return {{float((p>>8)&15)/15.0F,float((p>>4)&15)/15.0F,float(p&15)/15.0F,float((p>>12)&15)/15.0F}};
}
std::string digest(const void* data,std::size_t count){require(count<=0xffffffffU,"Inspection digest size");std::array<unsigned char,32> d;CC_SHA256(data,static_cast<CC_LONG>(count),d.data());std::ostringstream out;out<<std::hex<<std::setfill('0');for(auto v:d)out<<std::setw(2)<<unsigned(v);return out.str();}
void write(const std::string& path,const void* data,std::size_t count){std::ofstream file(path,std::ios::binary);file.write(static_cast<const char*>(data),count);require(static_cast<bool>(file),"Readback write failed: "+path);}
void png(const std::string& path,const sadx::MetalTextureReadbackResult& pixels){
    std::vector<std::uint8_t> image(pixels.rgba32f.size());for(std::size_t n=0;n<image.size();++n)image[n]=static_cast<std::uint8_t>(std::lround(std::fmin(1.0F,std::fmax(0.0F,pixels.rgba32f[n]))*255));
    CGColorSpaceRef color=CGColorSpaceCreateDeviceRGB();CGDataProviderRef provider=CGDataProviderCreateWithData(nullptr,image.data(),image.size(),nullptr);
    CGImageRef bitmap=color&&provider?CGImageCreate(pixels.width,pixels.height,8,32,std::size_t(pixels.width)*4,color,kCGImageAlphaLast,provider,nullptr,false,kCGRenderingIntentDefault):nullptr;
    NSURL* url=[NSURL fileURLWithPath:[NSString stringWithUTF8String:path.c_str()]];
    CGImageDestinationRef output=bitmap?CGImageDestinationCreateWithURL((__bridge CFURLRef)url,CFSTR("public.png"),1,nullptr):nullptr;
    bool completed=false;if(output){CGImageDestinationAddImage(output,bitmap,nullptr);completed=CGImageDestinationFinalize(output);}
    if(output)CFRelease(output);if(bitmap)CGImageRelease(bitmap);if(provider)CGDataProviderRelease(provider);if(color)CGColorSpaceRelease(color);require(completed,"PNG inspection output failed");
}
int inspect(int argc,char** argv){
    const bool all=argc==4&&std::string(argv[1])=="--all";const bool one=argc==5&&std::string(argv[1])=="--inspect";
    require(all||one,"usage: metal-texture-test --inspect ARCHIVE RECORD OUTPUT_PREFIX | --all ARCHIVE OUTPUT_PREFIX");
    const std::string prefix=argv[all?3:4];auto parsed=sadx::parse_ninja_texture_resource(read(argv[2]));require(static_cast<bool>(parsed),"Archive parse failure at"+std::to_string(parsed.source_offset));
    auto archive=std::make_shared<const sadx::NinjaTextureArchive>(std::move(parsed.archive));
    std::size_t first=0,end=archive->records.size();if(one){std::size_t consumed=0;first=std::stoull(argv[3],&consumed);require(consumed==std::strlen(argv[3])&&first<end,"Record index is outside the parsed archive");end=first+1;}
    id<MTLDevice> device=MTLCreateSystemDefaultDevice();id<MTLCommandQueue> queue=[device newCommandQueue];auto context=sadx::MetalTextureUploader::create(device,queue);
    require(static_cast<bool>(context),"Metal context failure"+std::to_string(static_cast<unsigned>(context.failure))+": "+context.detail);
    auto missing=sadx::MetalTextureUploader::create(nil,queue);require(missing.failure==sadx::MetalTextureFailure::missing_device,"Missing device is explicit");
    require(sadx::MetalTextureUploader::create(device,nil).failure==sadx::MetalTextureFailure::missing_queue,"Missing queue is explicit");
    require(context.uploader->upload({},0).failure==sadx::MetalTextureFailure::missing_archive,"Missing archive never synthesizes a texture");
    auto invalid=context.uploader->upload(archive,archive->records.size());require(invalid.failure==sadx::MetalTextureFailure::decode_failed&&invalid.decode_failure==sadx::TextureFailure::invalid_record&&invalid.resource&&!invalid.resource->ready(),"Bad ordinal preserves failed decoded ownership");
    NSMutableArray* records=[NSMutableArray new];std::uint64_t mismatch_total=0,channels_total=0,levels_total=0;
    for(std::size_t index=first;index<end;++index){auto uploaded=context.uploader->upload(archive,index);require(static_cast<bool>(uploaded),"Upload failure"+std::to_string(static_cast<unsigned>(uploaded.failure))+": "+uploaded.detail);auto resource=uploaded.resource;const auto& record=archive->records[index];
        require(&resource->archive()==archive.get()&&resource->record_index()==index&&resource->ready(),"GPU retains the one archive/record resource owner");
        require(resource->packed_texture().pixelFormat==MTLPixelFormatR16Uint&&resource->normalized_texture().pixelFormat==MTLPixelFormatRGBA32Float,"Real GPU texture formats preserve source bits and FP32 channels");
        NSMutableArray* levels=[NSMutableArray new];std::uint64_t mismatches=0;
        for(std::size_t level=0;level<resource->decoded().levels.size();++level){auto readback=sadx::read_metal_texture(resource,level);require(static_cast<bool>(readback),"GPU readback failed");const auto& source=resource->decoded().levels[level];std::uint64_t different=0,max_ulp=0;
            for(std::size_t p=0;p<source.packed16.size();++p){auto color=expected(source.packed16[p],record.pixel_format);for(std::size_t c=0;c<4;++c){const auto actual=bits(readback.rgba32f[p*4+c]),want=bits(color[c]);if(actual!=want)++different;const auto delta=actual>want?actual-want:want-actual;if(delta>max_ulp)max_ulp=delta;}}
            mismatches+=different;channels_total+=readback.rgba32f.size();++levels_total;
            const auto hash=digest(readback.rgba32f.data(),readback.rgba32f.size()*4);
            [levels addObject:@{@"level":@(level),@"width":@(source.width),@"height":@(source.height),@"encoded_offset":@(source.pixels.offset),@"encoded_bytes":@(source.pixels.size),@"channel_mismatches":@(different),@"maximum_ulp":@(max_ulp),@"gpu_rgba32f_sha256":[NSString stringWithUTF8String:hash.c_str()]}];
            if(one){const auto path=prefix+".mip"+std::to_string(level)+".rgba32f";write(path,readback.rgba32f.data(),readback.rgba32f.size()*4);if(!level)png(prefix+".png",readback);}
        }
        require(sadx::read_metal_texture(resource,resource->decoded().levels.size()).failure==sadx::MetalTextureFailure::invalid_level,"Unknown mip remains explicit");mismatch_total+=mismatches;
        [records addObject:@{@"ordinal":@(index),@"table_index":record.table_index?static_cast<id>(@(*record.table_index)):static_cast<id>([NSNull null]),@"global_index":record.table_global_index?static_cast<id>(@(*record.table_global_index)):static_cast<id>([NSNull null]),@"chunk_offset":@(record.chunk.offset),@"chunk_bytes":@(record.chunk.size),@"pixel_format":@(record.pixel_format),@"data_format":@(record.data_format),@"levels":levels}];
    }
    const std::string archive_digest=digest(archive->bytes.data(),archive->bytes.size()),shader_digest=digest(sadx::metal_texture_shader_source(),std::strlen(sadx::metal_texture_shader_source()));
    NSDictionary* report=@{@"archive":[NSString stringWithUTF8String:argv[2]],@"archive_sha256":[NSString stringWithUTF8String:archive_digest.c_str()],@"shader_sha256":[NSString stringWithUTF8String:shader_digest.c_str()],@"device":device.name,@"registry_id":@(device.registryID),@"unified_memory":@(device.hasUnifiedMemory),@"fp32_filtering":@(context.uploader->supports_float_filtering()),@"os":NSProcessInfo.processInfo.operatingSystemVersionString,@"compiler":@(__clang_version__),@"upload_format":@"R16Uint",@"normalized_format":@"RGBA32Float",@"math_profile":@"safe/precise macOS15+; no fast math on macOS14",@"gpu_commands_completed":@YES,@"records":records,@"levels":@(levels_total),@"channels":@(channels_total),@"channel_mismatches":@(mismatch_total),@"direct_float32_ratios_match":@(mismatch_total==0),@"limits":@[@"Native Apple GPU upload/normalization/readback; not original NVIDIA/D3D equivalence",@"No original runtime texlist/cache ownership or gameplay binding inferred",@"No sampler/filter/blend contract is chosen by upload",@"PNG uses nearest-byte quantization of real FP32 GPU readback as inspection preview"]};
    NSError* error=nil;NSData* json=[NSJSONSerialization dataWithJSONObject:report options:NSJSONWritingPrettyPrinted|NSJSONWritingSortedKeys error:&error];require(json!=nil,error?error.localizedDescription.UTF8String:"JSON encode failed");write(prefix+".json",json.bytes,json.length);
    std::cout<<"Metal texture: "<<(end-first)<<" records, "<<levels_total<<" levels, "<<channels_total<<" channels, "<<mismatch_total<<" float32 ratio mismatches on "<<device.name.UTF8String<<".\n";
    return mismatch_total?1:0;
}
}
int main(int argc,char** argv){@autoreleasepool{try{return inspect(argc,argv);}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}}
