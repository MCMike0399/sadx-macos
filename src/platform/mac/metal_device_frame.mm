#include "platform/mac/metal_device_frame.hpp"
#include <cmath>
#include <cstring>
#include <set>
#include <tuple>

namespace sadx {
namespace {
const char *kShader = R"METAL(
#include <metal_stdlib>
using namespace metal;
struct Vertex {
    packed_float4 clip, diffuse, specular;
    packed_float2 uv0, uv1;
    float eye_depth, texture_q, fog_factor;
};
struct Out {float4 position [[position]];float4 diffuse;float4 specular;float2 uv;float eye_depth;float texture_q;float fog_factor;};
struct Mode {
    uint textured, color_op, alpha_op, color_arg1;
    uint color_arg2, alpha_arg1, alpha_arg2, texcoord;
    uint alpha_test, alpha_func, alpha_ref, specular;
    uint fog_mode, fog_vertex, pad0, pad1;
    float4 factor, fog_color;
    float fog_start, fog_end, fog_density, pad2;
};
vertex Out device_vertex(uint id [[vertex_id]],constant Vertex* vertices [[buffer(0)]],
                         constant Mode& mode [[buffer(1)]]) {
    const auto v = vertices[id]; Out o;
    o.position=float4(v.clip); o.diffuse=float4(v.diffuse); o.specular=float4(v.specular);
    o.uv=float2(v.uv0); o.eye_depth=v.eye_depth; o.texture_q=v.texture_q; o.fog_factor=v.fog_factor;
    return o;
}
float4 argument(uint kind,float4 tex,float4 diffuse,float4 specular,float4 factor) {
    uint source=kind&15; float4 c=source==2?tex:source==3?factor:source==4?specular:diffuse;
    if(kind&16)c=1-c; if(kind&32)c=float4(c.a); return c;
}
float4 operation(uint mode,float4 a,float4 b,float4 diffuse) {
    return mode==1?diffuse:mode==2?a:mode==3?b:a*b;
}
bool compare_alpha(uint f,float a,float b) {
    return f==1?false:f==2?a<b:f==3?a==b:f==4?a<=b:f==5?a>b:f==6?a!=b:f==7?a>=b:true;
}
fragment float4 device_fragment(Out in [[stage_in]],texture2d<float> texture [[texture(0)]],
                               sampler sampler0 [[sampler(0)]],constant Mode& m [[buffer(0)]]) {
    float4 t=m.textured?texture.sample(sampler0,in.uv/in.texture_q):float4(1);
    float4 color=in.diffuse;
    if(m.color_op!=1) {
        float4 a=argument(m.color_arg1,t,in.diffuse,in.specular,m.factor);
        float4 b=argument(m.color_arg2,t,in.diffuse,in.specular,m.factor);
        color=operation(m.color_op,a,b,in.diffuse);
        a=argument(m.alpha_arg1,t,in.diffuse,in.specular,m.factor);
        b=argument(m.alpha_arg2,t,in.diffuse,in.specular,m.factor);
        color.a=operation(m.alpha_op,a,b,in.diffuse).a;
    }
    color=clamp(color,0.0f,1.0f);
    // D3D compares 8-bit alpha, after the texture-stage calculation.
    if(m.alpha_test&&!compare_alpha(m.alpha_func,round(color.a*255.0f),float(m.alpha_ref)))discard_fragment();
    if(m.specular)color.rgb=clamp(color.rgb+in.specular.rgb,0.0f,1.0f);
    if(m.fog_mode) {
        float depth=in.eye_depth;
        float fog=m.fog_mode==1?exp(-m.fog_density*depth):m.fog_mode==2?
            exp(-pow(m.fog_density*depth,2.0f)):(m.fog_end-depth)/(m.fog_end-m.fog_start);
        color.rgb=mix(m.fog_color.rgb,color.rgb,clamp(fog,0.0f,1.0f));
    } else if(m.fog_vertex)color.rgb=mix(m.fog_color.rgb,color.rgb,in.fog_factor);
    return color;
}
)METAL";
static_assert(sizeof(graphics::PreparedVertex) == 76, "Metal vertex ABI");
struct Mode {
    std::uint32_t textured, color_op, alpha_op, color_arg1;
    std::uint32_t color_arg2, alpha_arg1, alpha_arg2, texcoord;
    std::uint32_t alpha_test, alpha_func, alpha_ref, specular;
    std::uint32_t fog_mode, fog_vertex, pad0, pad1;
    float factor[4], fog_color[4];
    float fog_start, fog_end, fog_density, pad2;
};
static_assert(sizeof(Mode) == 112, "Metal uniform ABI");
std::uint32_t state(const std::map<std::uint32_t, std::uint32_t> &states, std::uint32_t key,
                    std::uint32_t fallback) {
    const auto found = states.find(key);
    return found == states.end() ? fallback : found->second;
}
float float_word(std::uint32_t word) {
    float result = 0;
    std::memcpy(&result, &word, 4);
    return result;
}
void color(std::uint32_t word, float *output) {
    output[0] = float((word >> 16) & 255) / 255;
    output[1] = float((word >> 8) & 255) / 255;
    output[2] = float(word & 255) / 255;
    output[3] = float(word >> 24) / 255;
}
MTLBlendFactor blend_factor(std::uint32_t value) {
    static const MTLBlendFactor factors[] = {MTLBlendFactorZero,
                                             MTLBlendFactorZero,
                                             MTLBlendFactorOne,
                                             MTLBlendFactorSourceColor,
                                             MTLBlendFactorOneMinusSourceColor,
                                             MTLBlendFactorSourceAlpha,
                                             MTLBlendFactorOneMinusSourceAlpha,
                                             MTLBlendFactorDestinationAlpha,
                                             MTLBlendFactorOneMinusDestinationAlpha,
                                             MTLBlendFactorDestinationColor,
                                             MTLBlendFactorOneMinusDestinationColor,
                                             MTLBlendFactorSourceAlphaSaturated};
    return factors[value];
}
std::string validate(const graphics::Draw &draw) {
    if (!draw.unsupported.empty())
        return draw.unsupported;
    const auto r = [&](unsigned key, unsigned fallback) {
        return state(draw.render_states, key, fallback);
    };
    const auto t = [&](unsigned key, unsigned fallback) {
        return state(draw.stage0_states, key, fallback);
    };
    // Accept implemented states and documented inert states for this triangle pipeline.
    // Unknown API states remain visible rather than falling through to a guessed mode.
    static const std::set<unsigned> render_states = {
        7,   8,   9,   14,  15,  16,  19,  20,  22,  23,  24,  25,  26,  27,
        28,  29,  34,  35,  36,  37,  38,  48,  52,  60,  128, 136, 137, 139,
        140, 141, 142, 143, 145, 146, 147, 148, 151, 152, 161, 162, 168, 171};
    static const std::set<unsigned> stage_states = {1,  2,  3,  4,  5,  6,  7,  8,  9,
                                                    10, 11, 13, 14, 15, 16, 17, 18, 19,
                                                    20, 21, 22, 23, 24, 26, 27, 28};
    for (const auto &entry : draw.render_states)
        if (!render_states.count(entry.first))
            return "render state " + std::to_string(entry.first);
    for (const auto &entry : draw.stage0_states)
        if (!stage_states.count(entry.first))
            return "texture stage state " + std::to_string(entry.first);
    if (!r(136, 1))
        return "disabled clipping";
    if (r(162, 0xffffffffU) != 0xffffffffU)
        return "multisample mask";
    for (auto key : {145U, 146U, 147U, 148U})
        if (r(key, key == 145 ? 1 : key == 146 ? 2 : 0) > 2)
            return "material color source";
    for (auto key : {26U, 52U, 128U, 151U, 152U})
        if (r(key, 0))
            return "render state " + std::to_string(key);
    if (r(9, 2) != 2)
        return "flat shading";
    if (r(8, 3) != 3)
        return "fill mode";
    if (r(168, 15) != 15)
        return "color write mask";
    if (t(19, 0) || t(20, 0))
        return "texture LOD bias/maximum mip level";
    if (t(28, 1) != 1)
        return "texture result argument";
    if (r(7, 1) > 1 || r(23, 4) < 1 || r(23, 4) > 8)
        return "depth mode";
    if (r(22, 3) < 1 || r(22, 3) > 3)
        return "cull mode";
    if (r(27, 0) && (r(19, 2) < 1 || r(19, 2) > 11 || r(20, 1) < 1 || r(20, 1) > 10))
        return "blend factor";
    if (r(27, 0) && r(171, 1) != 1)
        return "blend operation";
    if (r(15, 0) && (r(25, 8) < 1 || r(25, 8) > 8 || r(24, 0) > 255))
        return "alpha test";
    for (auto key : {1U, 4U})
        if (t(key, key == 1 ? 4 : 2) < 1 || t(key, key == 1 ? 4 : 2) > 4)
            return "texture operation";
    for (auto key : {2U, 3U, 5U, 6U}) {
        const auto argument = t(key, (key == 2 || key == 5) ? 2 : 1);
        if ((argument & 15) > 4 || (argument & ~63U))
            return "texture argument";
    }
    if ((draw.texture_coordinates || draw.texture) &&
        ((t(11, 0) >> 16) > 2 || (!(t(11, 0) >> 16) && (t(11, 0) & 65535) > 1)))
        return "texture coordinate generation/channel";
    if (t(13, 1) < 1 || t(13, 1) > 3 || t(14, 1) < 1 || t(14, 1) > 3)
        return "texture addressing";
    if (t(16, 1) < 1 || t(16, 1) > 2 || t(17, 1) < 1 || t(17, 1) > 2 || t(18, 0) > 2)
        return "texture filter";
    if (r(28, 0) && r(35, 0) > 3)
        return "fog mode";
    if (r(28, 0) && r(35, 0) && draw.space == graphics::PositionSpace::Object &&
        (draw.projection[11] != 1 || draw.projection[15] != 0))
        return "non-W table fog";
    return {};
}
} // namespace
struct MetalDeviceFrameRenderer::Impl {
    id<MTLDevice> device;
    id<MTLLibrary> library;
    std::map<std::tuple<unsigned, unsigned, bool, unsigned, unsigned>, id<MTLRenderPipelineState>>
        pipelines;
    std::map<std::tuple<unsigned, unsigned, unsigned, unsigned, unsigned>, id<MTLSamplerState>>
        samplers;
    std::map<std::tuple<bool, bool, unsigned>, id<MTLDepthStencilState>> depths;
    id<MTLRenderPipelineState> pipeline(MTLPixelFormat color_format, MTLPixelFormat depth_format,
                                        bool blend, unsigned src, unsigned dst) {
        const auto key = std::make_tuple(unsigned(color_format), unsigned(depth_format), blend,
                                         blend ? src : 0, blend ? dst : 0);
        if (auto found = pipelines.find(key); found != pipelines.end())
            return found->second;
        auto descriptor = [MTLRenderPipelineDescriptor new];
        descriptor.vertexFunction = [library newFunctionWithName:@"device_vertex"];
        descriptor.fragmentFunction = [library newFunctionWithName:@"device_fragment"];
        descriptor.colorAttachments[0].pixelFormat = color_format;
        descriptor.depthAttachmentPixelFormat = depth_format;
        descriptor.colorAttachments[0].blendingEnabled = blend;
        if (blend) {
            descriptor.colorAttachments[0].sourceRGBBlendFactor = blend_factor(src);
            descriptor.colorAttachments[0].sourceAlphaBlendFactor = blend_factor(src);
            descriptor.colorAttachments[0].destinationRGBBlendFactor = blend_factor(dst);
            descriptor.colorAttachments[0].destinationAlphaBlendFactor = blend_factor(dst);
        }
        NSError *error = nil;
        auto result = [device newRenderPipelineStateWithDescriptor:descriptor error:&error];
        if (result)
            pipelines[key] = result;
        return result;
    }
    id<MTLDepthStencilState> depth(bool enable, bool write, unsigned func) {
        const auto key = std::make_tuple(enable, write, func);
        if (auto found = depths.find(key); found != depths.end())
            return found->second;
        auto descriptor = [MTLDepthStencilDescriptor new];
        descriptor.depthCompareFunction =
            enable ? static_cast<MTLCompareFunction>(func - 1) : MTLCompareFunctionAlways;
        descriptor.depthWriteEnabled = enable && write;
        auto result = [device newDepthStencilStateWithDescriptor:descriptor];
        depths[key] = result;
        return result;
    }
    id<MTLSamplerState> sampler(unsigned min, unsigned mag, unsigned mip, unsigned u, unsigned v) {
        const auto key = std::make_tuple(min, mag, mip, u, v);
        if (auto found = samplers.find(key); found != samplers.end())
            return found->second;
        auto descriptor = [MTLSamplerDescriptor new];
        descriptor.minFilter =
            min == 2 ? MTLSamplerMinMagFilterLinear : MTLSamplerMinMagFilterNearest;
        descriptor.magFilter =
            mag == 2 ? MTLSamplerMinMagFilterLinear : MTLSamplerMinMagFilterNearest;
        descriptor.mipFilter = mip == 2   ? MTLSamplerMipFilterLinear
                               : mip == 1 ? MTLSamplerMipFilterNearest
                                          : MTLSamplerMipFilterNotMipmapped;
        const MTLSamplerAddressMode address[] = {
            MTLSamplerAddressModeRepeat, MTLSamplerAddressModeRepeat,
            MTLSamplerAddressModeMirrorRepeat, MTLSamplerAddressModeClampToEdge};
        descriptor.sAddressMode = address[u];
        descriptor.tAddressMode = address[v];
        auto result = [device newSamplerStateWithDescriptor:descriptor];
        samplers[key] = result;
        return result;
    }
    id<MTLTexture> texture(const graphics::Texture &image) {
        auto descriptor = [MTLTextureDescriptor
            texture2DDescriptorWithPixelFormat:image.format == graphics::TextureFormat::Bgra8
                                                   ? MTLPixelFormatBGRA8Unorm
                                                   : MTLPixelFormatRGBA32Float
                                         width:image.width
                                        height:image.height
                                     mipmapped:!image.mipmaps.empty()];
        descriptor.mipmapLevelCount = 1 + image.mipmaps.size();
        auto result = [device newTextureWithDescriptor:descriptor];
        if (!result)
            return nil;
        const auto upload = [&](const graphics::Texture &level, unsigned mip) {
            const bool bytes = level.format == graphics::TextureFormat::Bgra8;
            [result replaceRegion:MTLRegionMake2D(0, 0, level.width, level.height)
                      mipmapLevel:mip
                        withBytes:bytes ? static_cast<const void *>(level.bgra.data())
                                        : static_cast<const void *>(level.rgba.data())
                      bytesPerRow:level.width * (bytes ? 4 : 16)];
        };
        upload(image, 0);
        for (unsigned mip = 0; mip < image.mipmaps.size(); ++mip)
            upload(image.mipmaps[mip], mip + 1);
        return result;
    }
};
MetalDeviceFrameRenderer::MetalDeviceFrameRenderer(std::unique_ptr<Impl> value)
    : impl(std::move(value)) {}
MetalDeviceFrameRenderer::~MetalDeviceFrameRenderer() = default;
std::unique_ptr<MetalDeviceFrameRenderer> MetalDeviceFrameRenderer::create(id<MTLDevice> device,
                                                                           std::string *error) {
    auto value = std::make_unique<Impl>();
    value->device = device;
    NSError *failure = nil;
    value->library = [device newLibraryWithSource:[NSString stringWithUTF8String:kShader]
                                          options:nil
                                            error:&failure];
    if (!value->library) {
        if (error)
            *error = failure.localizedDescription.UTF8String;
        return nullptr;
    }
    return std::unique_ptr<MetalDeviceFrameRenderer>(
        new MetalDeviceFrameRenderer(std::move(value)));
}
MetalFrameStats MetalDeviceFrameRenderer::encode(const graphics::Frame &frame,
                                                 id<MTLCommandBuffer> commands,
                                                 MTLRenderPassDescriptor *pass) {
    MetalFrameStats stats;
    id<MTLRenderCommandEncoder> encoder = nil;
    std::map<const graphics::Texture *, id<MTLTexture>> textures;
    for (const auto &command : frame.commands) {
        if (const auto *clear = std::get_if<graphics::Clear>(&command)) {
            if (encoder)
                [encoder endEncoding];
            ++stats.clears;
            if (clear->flags & ~3U)
                ++stats.unsupported["stencil/unknown clear flags"];
            pass.colorAttachments[0].loadAction =
                (clear->flags & 1) ? MTLLoadActionClear : MTLLoadActionLoad;
            pass.depthAttachment.loadAction =
                (clear->flags & 2) ? MTLLoadActionClear : MTLLoadActionLoad;
            const auto color = clear->color;
            pass.colorAttachments[0].clearColor =
                MTLClearColorMake(((color >> 16) & 255) / 255.0, ((color >> 8) & 255) / 255.0,
                                  (color & 255) / 255.0, (color >> 24) / 255.0);
            pass.depthAttachment.clearDepth = float_word(clear->z_bits);
            encoder = [commands renderCommandEncoderWithDescriptor:pass];
            continue;
        }
        const auto &draw = std::get<graphics::Draw>(command);
        auto problem = validate(draw);
        if (!problem.empty()) {
            ++stats.unsupported[problem];
            continue;
        }
        auto prepared = graphics::prepare_draw(draw);
        if (!prepared.unsupported.empty()) {
            ++stats.unsupported[prepared.unsupported];
            continue;
        }
        if (prepared.indices.empty()) {
            ++stats.draws;
            continue;
        }
        const auto r = [&](unsigned key, unsigned fallback) {
            return state(draw.render_states, key, fallback);
        };
        const auto t = [&](unsigned key, unsigned fallback) {
            return state(draw.stage0_states, key, fallback);
        };
        Mode mode{};
        mode.textured = draw.texture && (draw.texture_coordinates || (t(11, 0) >> 16));
        mode.color_op = t(1, 4);
        mode.alpha_op = t(4, 2);
        mode.color_arg1 = t(2, 2);
        mode.color_arg2 = t(3, 1);
        mode.alpha_arg1 = t(5, 2);
        mode.alpha_arg2 = t(6, 1);
        mode.texcoord = t(11, 0);
        mode.alpha_test = r(15, 0);
        mode.alpha_func = r(25, 8);
        mode.alpha_ref = r(24, 0);
        mode.specular = r(29, 0);
        if (r(28, 0)) {
            mode.fog_mode = r(35, 0);
            mode.fog_vertex = !mode.fog_mode;
        }
        color(r(60, 0xffffffffU), mode.factor);
        color(r(34, 0), mode.fog_color);
        mode.fog_start = float_word(r(36, 0));
        mode.fog_end = float_word(r(37, 0x3f800000U));
        mode.fog_density = float_word(r(38, 0x3f800000U));
        if (mode.fog_mode == 3 && mode.fog_start == mode.fog_end) {
            ++stats.unsupported["zero fog interval"];
            continue;
        }
        id<MTLTexture> texture = nil;
        if (mode.textured) {
            if (draw.texture->format == graphics::TextureFormat::Rgba32Float &&
                !impl->device.supports32BitFloatFiltering &&
                (t(16, 1) == 2 || t(17, 1) == 2 || t(18, 0) == 2)) {
                ++stats.unsupported["float texture filtering"];
                continue;
            }
            const auto *source = draw.texture.get();
            if (!textures.count(source))
                textures[source] = impl->texture(*source);
            texture = textures[source];
            if (!texture) {
                ++stats.unsupported["texture allocation"];
                continue;
            }
        }
        const auto depth_format = pass.depthAttachment.texture
                                      ? pass.depthAttachment.texture.pixelFormat
                                      : MTLPixelFormatInvalid;
        if (r(7, 1) && depth_format == MTLPixelFormatInvalid) {
            ++stats.unsupported["missing depth target"];
            continue;
        }
        auto pipeline = impl->pipeline(pass.colorAttachments[0].texture.pixelFormat, depth_format,
                                       r(27, 0) != 0, r(19, 2), r(20, 1));
        if (!pipeline) {
            ++stats.unsupported["pipeline allocation"];
            continue;
        }
        if (!encoder) {
            pass.colorAttachments[0].loadAction = MTLLoadActionLoad;
            pass.depthAttachment.loadAction = MTLLoadActionLoad;
            encoder = [commands renderCommandEncoderWithDescriptor:pass];
        }
        [encoder setRenderPipelineState:pipeline];
        [encoder setDepthStencilState:impl->depth(r(7, 1) != 0, r(14, 1) != 0, r(23, 4))];
        [encoder setCullMode:r(22, 3) == 1 ? MTLCullModeNone : MTLCullModeBack];
        [encoder
            setFrontFacingWinding:r(22, 3) == 2 ? MTLWindingCounterClockwise : MTLWindingClockwise];
        const auto &v = draw.viewport;
        [encoder setViewport:MTLViewport{v.x, v.y, v.width, v.height, v.near_depth, v.far_depth}];
        auto vertices = [impl->device
            newBufferWithBytes:prepared.vertices.data()
                        length:prepared.vertices.size() * sizeof(graphics::PreparedVertex)
                       options:MTLResourceStorageModeShared];
        auto indices = [impl->device newBufferWithBytes:prepared.indices.data()
                                                 length:prepared.indices.size() * 4
                                                options:MTLResourceStorageModeShared];
        if (!vertices || !indices) {
            ++stats.unsupported["vertex/index allocation"];
            continue;
        }
        [encoder setVertexBuffer:vertices offset:0 atIndex:0];
        [encoder setVertexBytes:&mode length:sizeof mode atIndex:1];
        [encoder setFragmentBytes:&mode length:sizeof mode atIndex:0];
        [encoder setFragmentTexture:texture atIndex:0];
        [encoder
            setFragmentSamplerState:impl->sampler(t(16, 1), t(17, 1), t(18, 0), t(13, 1), t(14, 1))
                            atIndex:0];
        [encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                            indexCount:prepared.indices.size()
                             indexType:MTLIndexTypeUInt32
                           indexBuffer:indices
                     indexBufferOffset:0];
        ++stats.draws;
        if (mode.textured)
            ++stats.textured;
    }
    if (encoder)
        [encoder endEncoding];
    return stats;
}
} // namespace sadx
