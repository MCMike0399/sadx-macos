#pragma once
#ifndef __OBJC__
#error "metal_texture.hpp is the Objective-C++ Metal consumer boundary"
#endif
#import <Metal/Metal.h>
#include "engine/ninja_texture.hpp"
#include <memory>
#include <string>
namespace sadx {
enum class MetalTextureFailure {none,missing_device,missing_queue,device_mismatch,
    unsupported_memory_profile,shader_compile_failed,pipeline_failed,missing_archive,
    decode_failed,allocation_failed,encoder_failed,command_failed,not_ready,invalid_level};
class MetalTextureResource;
class MetalTextureUploader;
struct MetalTextureUploadResult {
    MetalTextureFailure failure=MetalTextureFailure::none;
    std::string detail;
    std::optional<TextureFailure> decode_failure;
    std::shared_ptr<MetalTextureResource> resource; // preceding allocation/decoded facts retained
    explicit operator bool()const noexcept{return failure==MetalTextureFailure::none&&resource!=nullptr;}
};
struct MetalTextureContextResult {
    MetalTextureFailure failure=MetalTextureFailure::none;std::string detail;
    std::shared_ptr<MetalTextureUploader> uploader;
    explicit operator bool()const noexcept{return failure==MetalTextureFailure::none&&uploader!=nullptr;}
};
class MetalTextureResource {
public:
    ~MetalTextureResource();
    const NinjaTextureArchive& archive()const noexcept;
    const NinjaTextureDecodeResult& decoded()const noexcept;
    std::size_t record_index()const noexcept;
    const std::string& archive_sha256()const noexcept;
    bool ready()const noexcept;
    // Integer source and float channels share one native ownership object. The
    // normalized texture is reusable by a caller's explicit sampler contract.
    id<MTLTexture> packed_texture()const noexcept;
    id<MTLTexture> normalized_texture()const noexcept;
private:
    struct Impl;std::unique_ptr<Impl> impl;
    explicit MetalTextureResource(std::unique_ptr<Impl>);
    friend class MetalTextureUploader;
};
class MetalTextureUploader {
public:
    ~MetalTextureUploader();
    static MetalTextureContextResult create(id<MTLDevice>,id<MTLCommandQueue>);
    MetalTextureUploadResult upload(std::shared_ptr<const NinjaTextureArchive>,std::size_t);
    const std::string& device_name()const noexcept;
    bool supports_float_filtering()const noexcept;
private:
    struct Impl;std::unique_ptr<Impl> impl;
    explicit MetalTextureUploader(std::unique_ptr<Impl>);
};
struct MetalTextureReadbackResult {
    MetalTextureFailure failure=MetalTextureFailure::none;
    std::uint16_t width=0,height=0;std::vector<float> rgba32f;
    explicit operator bool()const noexcept{return failure==MetalTextureFailure::none;}
};
MetalTextureReadbackResult read_metal_texture(const std::shared_ptr<const MetalTextureResource>&,std::size_t level);
// Exact kernel text, retained for shader identity and compiled-profile evidence.
const char* metal_texture_shader_source()noexcept;
} // namespace sadx
