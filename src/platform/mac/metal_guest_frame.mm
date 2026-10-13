#include "platform/mac/metal_guest_frame.hpp"
#include <cstring>
#include <map>
#include <tuple>
namespace sadx {
namespace {
const char* kGuestShader=R"METAL(
#include <metal_stdlib>
using namespace metal;
struct Vertex {packed_float3 position;packed_float4 color;packed_float2 uv;}; // 36 bytes, as the C++ side packs them
struct Out {float4 position [[position]];float4 color;float2 uv;};
struct Mode {uint textured,color_op,alpha_op,pad;};
vertex Out guest_vertex(uint id [[vertex_id]],constant Vertex* v [[buffer(0)]],constant float4& viewport [[buffer(1)]]){
    float3 p=float3(v[id].position);float2 xy=(p.xy+0.5f-viewport.xy)/viewport.zw;Out o;o.position=float4(xy.x*2.0f-1.0f,1.0f-xy.y*2.0f,p.z,1.0f);o.color=float4(v[id].color);o.uv=float2(v[id].uv);return o;}
// D3DTOP: 2 SELECTARG1 (texture), 3 SELECTARG2 (diffuse), 4 MODULATE.
float4 op(uint mode,float4 t,float4 d){return mode==2?t:mode==3?d:t*d;}
fragment float4 guest_fragment(Out in [[stage_in]],texture2d<float> tex [[texture(0)]],sampler s [[sampler(0)]],constant Mode& m [[buffer(0)]]){
    if(m.textured==0)return in.color;
    float4 t=tex.sample(s,in.uv);float4 c=op(m.color_op,t,in.color);float4 a=op(m.alpha_op,t,in.color);return float4(c.rgb,a.a);}
)METAL";
struct Vertex {float x,y,z;float r,g,b,a;float u,v;};
MTLBlendFactor blend_factor(std::uint32_t d3d){
    switch(d3d){case 1:return MTLBlendFactorZero;case 2:return MTLBlendFactorOne;case 3:return MTLBlendFactorSourceColor;case 4:return MTLBlendFactorOneMinusSourceColor;
        case 5:return MTLBlendFactorSourceAlpha;case 6:return MTLBlendFactorOneMinusSourceAlpha;case 7:return MTLBlendFactorDestinationAlpha;case 8:return MTLBlendFactorOneMinusDestinationAlpha;
        case 9:return MTLBlendFactorDestinationColor;case 10:return MTLBlendFactorOneMinusDestinationColor;default:return MTLBlendFactorOne;}}
std::uint32_t state(const std::map<std::uint32_t,std::uint32_t>& m,std::uint32_t k,std::uint32_t fallback){const auto i=m.find(k);return i==m.end()?fallback:i->second;}
// Guest texture object words (hle_d3d8.cpp): +36 levels, +40 level table; surfaces: +8 w, +12 h, +16 format, +20 pitch, +24 bits.
std::optional<std::vector<std::uint8_t>> decode_level0(const X86Guest& g,std::uint32_t texture,std::uint32_t& w,std::uint32_t& h,std::string& why){
    const auto table=g.u32(texture+40);if(!table){why="texture without levels";return std::nullopt;}
    const auto surface=g.u32(*table);if(!surface){why="texture level 0 missing";return std::nullopt;}
    w=g.u32(*surface+8).value_or(0);h=g.u32(*surface+12).value_or(0);const auto format=g.u32(*surface+16).value_or(0),pitch=g.u32(*surface+20).value_or(0),bits=g.u32(*surface+24).value_or(0);
    if(!w||!h||!pitch){why="empty texture";return std::nullopt;}
    std::vector<std::uint8_t> raw(static_cast<std::size_t>(pitch)*h);g.read(bits,raw.data(),raw.size());
    std::vector<std::uint8_t> bgra(static_cast<std::size_t>(w)*h*4);
    const auto expand=[](std::uint32_t v,unsigned bitsn){return static_cast<std::uint8_t>(bitsn==8?v:bitsn==1?(v?255:0):(v*255+((1u<<bitsn)-1)/2)/((1u<<bitsn)-1));};
    for(std::uint32_t y=0;y<h;++y)for(std::uint32_t x=0;x<w;++x){
        std::uint8_t* o=&bgra[(static_cast<std::size_t>(y)*w+x)*4];const std::uint8_t* s=&raw[static_cast<std::size_t>(y)*pitch];
        switch(format){
        case 21:case 22:{std::memcpy(o,s+4*x,4);if(format==22)o[3]=255;break;}
        case 23:{std::uint16_t p;std::memcpy(&p,s+2*x,2);o[0]=expand(p&31,5);o[1]=expand((p>>5)&63,6);o[2]=expand(p>>11,5);o[3]=255;break;}
        case 24:case 25:{std::uint16_t p;std::memcpy(&p,s+2*x,2);o[0]=expand(p&31,5);o[1]=expand((p>>5)&31,5);o[2]=expand((p>>10)&31,5);o[3]=format==25?expand(p>>15,1):255;break;}
        case 26:{std::uint16_t p;std::memcpy(&p,s+2*x,2);o[0]=expand(p&15,4);o[1]=expand((p>>4)&15,4);o[2]=expand((p>>8)&15,4);o[3]=expand(p>>12,4);break;}
        default:why="texture format "+std::to_string(format);return std::nullopt;}}
    return bgra;
}
}
struct MetalGuestFrameRenderer::Impl {
    id<MTLDevice> device;id<MTLLibrary> library;std::map<std::tuple<bool,std::uint32_t,std::uint32_t>,id<MTLRenderPipelineState>> pipelines;
    std::map<std::pair<bool,bool>,id<MTLSamplerState>> samplers;std::map<std::tuple<bool,bool,std::uint32_t>,id<MTLDepthStencilState>> depths;
    id<MTLDepthStencilState> depth(bool enable,bool write,std::uint32_t func){
        const auto key=std::make_tuple(enable,write,func);if(auto i=depths.find(key);i!=depths.end())return i->second;
        static const MTLCompareFunction f[]={MTLCompareFunctionAlways,MTLCompareFunctionNever,MTLCompareFunctionLess,MTLCompareFunctionEqual,MTLCompareFunctionLessEqual,
            MTLCompareFunctionGreater,MTLCompareFunctionNotEqual,MTLCompareFunctionGreaterEqual,MTLCompareFunctionAlways};
        MTLDepthStencilDescriptor* d=[MTLDepthStencilDescriptor new];d.depthCompareFunction=enable?f[func<9?func:0]:MTLCompareFunctionAlways;d.depthWriteEnabled=enable&&write;
        auto s=[device newDepthStencilStateWithDescriptor:d];depths[key]=s;return s;}
    id<MTLRenderPipelineState> pipeline(bool blend,std::uint32_t src,std::uint32_t dst){
        const auto key=std::make_tuple(blend,blend?src:0,blend?dst:0);if(auto i=pipelines.find(key);i!=pipelines.end())return i->second;
        MTLRenderPipelineDescriptor* d=[MTLRenderPipelineDescriptor new];d.vertexFunction=[library newFunctionWithName:@"guest_vertex"];d.fragmentFunction=[library newFunctionWithName:@"guest_fragment"];
        d.colorAttachments[0].pixelFormat=MTLPixelFormatBGRA8Unorm;d.depthAttachmentPixelFormat=MTLPixelFormatDepth32Float;
        if(blend){d.colorAttachments[0].blendingEnabled=YES;d.colorAttachments[0].sourceRGBBlendFactor=d.colorAttachments[0].sourceAlphaBlendFactor=blend_factor(src);
            d.colorAttachments[0].destinationRGBBlendFactor=d.colorAttachments[0].destinationAlphaBlendFactor=blend_factor(dst);}
        NSError* e=nil;auto p=[device newRenderPipelineStateWithDescriptor:d error:&e];pipelines[key]=p;return p;}
    id<MTLSamplerState> sampler(bool linear,bool clamp){
        const auto key=std::make_pair(linear,clamp);if(auto i=samplers.find(key);i!=samplers.end())return i->second;
        MTLSamplerDescriptor* s=[MTLSamplerDescriptor new];s.minFilter=s.magFilter=linear?MTLSamplerMinMagFilterLinear:MTLSamplerMinMagFilterNearest;
        s.sAddressMode=s.tAddressMode=clamp?MTLSamplerAddressModeClampToEdge:MTLSamplerAddressModeRepeat;auto r=[device newSamplerStateWithDescriptor:s];samplers[key]=r;return r;}
};
MetalGuestFrameRenderer::MetalGuestFrameRenderer(std::unique_ptr<Impl> i):impl(std::move(i)){}
MetalGuestFrameRenderer::~MetalGuestFrameRenderer()=default;
std::unique_ptr<MetalGuestFrameRenderer> MetalGuestFrameRenderer::create(id<MTLDevice> device,std::string* error){
    auto i=std::make_unique<Impl>();i->device=device;NSError* e=nil;
    i->library=[device newLibraryWithSource:[NSString stringWithUTF8String:kGuestShader] options:nil error:&e];
    if(!i->library){if(error)*error=e.localizedDescription.UTF8String;return nullptr;}
    return std::unique_ptr<MetalGuestFrameRenderer>(new MetalGuestFrameRenderer(std::move(i)));
}
MetalGuestFrameStats MetalGuestFrameRenderer::encode(const HleProcess::D3D8Frame& frame,const X86Guest& g,id<MTLCommandBuffer> commands,MTLRenderPassDescriptor* pass){
    MetalGuestFrameStats stats;
    // The first colour Clear of the frame becomes the load action (D3DCLEAR_TARGET = 1; colour ARGB).
    pass.colorAttachments[0].loadAction=MTLLoadActionLoad;
    pass.depthAttachment.loadAction=MTLLoadActionLoad;
    for(const auto& clear:frame.clears){++stats.clears;const auto color=clear.color;
        if(clear.flags&1){pass.colorAttachments[0].loadAction=MTLLoadActionClear;
            pass.colorAttachments[0].clearColor=MTLClearColorMake(((color>>16)&255)/255.0,((color>>8)&255)/255.0,(color&255)/255.0,(color>>24)/255.0);}
        if(clear.flags&2){float z;std::memcpy(&z,&clear.z_bits,4);pass.depthAttachment.loadAction=MTLLoadActionClear;pass.depthAttachment.clearDepth=z;}}
    id<MTLRenderCommandEncoder> encoder=[commands renderCommandEncoderWithDescriptor:pass];
    float viewport[4]={0,0,640,480};
    if(frame.viewport){viewport[0]=static_cast<float>((*frame.viewport)[0]);viewport[1]=static_cast<float>((*frame.viewport)[1]);viewport[2]=static_cast<float>((*frame.viewport)[2]);viewport[3]=static_cast<float>((*frame.viewport)[3]);}
    for(const auto& d:frame.draws){
        // FVF: XYZRHW 0x004, DIFFUSE 0x040, SPECULAR 0x080, TEX1 0x100
        if((d.fvf&0x00e)!=0x004||!(d.fvf&0x040)||(d.fvf&~0x1c4U)){++stats.unsupported["FVF "+std::to_string(d.fvf)];continue;}
        const bool tex=(d.fvf&0x100)!=0;std::uint32_t off_specular=20,off_uv=(d.fvf&0x080)?24:20;(void)off_specular;
        const auto count=d.stride?d.vertices.size()/d.stride:0;if(count<3){++stats.unsupported["fewer than 3 vertices"];continue;}
        std::vector<Vertex> vs(count);
        for(std::size_t k=0;k<count;++k){const auto* b=&d.vertices[k*d.stride];float x,y;std::uint32_t c;std::memcpy(&x,b,4);std::memcpy(&y,b+4,4);std::memcpy(&c,b+16,4);
            float u=0,v=0,z=0;std::memcpy(&z,b+8,4);if(tex){std::memcpy(&u,b+off_uv,4);std::memcpy(&v,b+off_uv+4,4);}
            vs[k]={x,y,z,((c>>16)&255)/255.0f,((c>>8)&255)/255.0f,(c&255)/255.0f,(c>>24)/255.0f,u,v};}
        std::vector<std::uint32_t> idx;
        if(d.primitive_type==4)for(std::uint32_t k=0;k+2<count;k+=3){idx.push_back(k);idx.push_back(k+1);idx.push_back(k+2);}
        else if(d.primitive_type==5)for(std::uint32_t k=0;k+2<count;++k){if(k&1){idx.push_back(k+1);idx.push_back(k);idx.push_back(k+2);}else{idx.push_back(k);idx.push_back(k+1);idx.push_back(k+2);}}
        else if(d.primitive_type==6)for(std::uint32_t k=1;k+1<count;++k){idx.push_back(0);idx.push_back(k);idx.push_back(k+1);}
        else{++stats.unsupported["primitive type "+std::to_string(d.primitive_type)];continue;}
        const bool blend=state(d.render_states,27,0)!=0; // D3DRS_ALPHABLENDENABLE
        // Depth: D3DRS_ZENABLE (7, default TRUE with the auto depth surface), ZWRITEENABLE (14, default TRUE), ZFUNC (23, default LESSEQUAL).
        [encoder setDepthStencilState:impl->depth(state(d.render_states,7,1)!=0,state(d.render_states,14,1)!=0,state(d.render_states,23,4))];
        [encoder setRenderPipelineState:impl->pipeline(blend,state(d.render_states,19,2),state(d.render_states,20,1))];
        std::uint32_t mode[4]={0,state(d.stage0_states,1,4),state(d.stage0_states,4,2),0};
        id<MTLTexture> texture=nil;
        if(tex&&d.texture){std::uint32_t w=0,h=0;std::string why;const auto pixels=decode_level0(g,d.texture,w,h,why);
            if(!pixels){++stats.unsupported[why];continue;}
            MTLTextureDescriptor* td=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:w height:h mipmapped:NO];
            texture=[impl->device newTextureWithDescriptor:td];[texture replaceRegion:MTLRegionMake2D(0,0,w,h) mipmapLevel:0 withBytes:pixels->data() bytesPerRow:w*4];
            mode[0]=1;++stats.textured;}
        if(tex&&!d.texture)mode[0]=0; // no texture bound: the stage's texture argument reads as white; the diffuse colour remains
        [encoder setVertexBytes:vs.data() length:vs.size()*sizeof(Vertex) atIndex:0];[encoder setVertexBytes:viewport length:sizeof viewport atIndex:1];
        [encoder setFragmentBytes:mode length:sizeof mode atIndex:0];
        if(texture){[encoder setFragmentTexture:texture atIndex:0];
            [encoder setFragmentSamplerState:impl->sampler(state(d.stage0_states,17,2)==2||state(d.stage0_states,16,2)==2,state(d.stage0_states,13,1)==3) atIndex:0];}
        id<MTLBuffer> ib=[impl->device newBufferWithBytes:idx.data() length:idx.size()*4 options:MTLResourceStorageModeShared];
        [encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:idx.size() indexType:MTLIndexTypeUInt32 indexBuffer:ib indexBufferOffset:0];
        ++stats.draws;}
    [encoder endEncoding];
    return stats;
}
} // namespace sadx
