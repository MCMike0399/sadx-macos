#pragma once
#ifndef __OBJC__
#error "Metal device boundary requires Objective-C++"
#endif
#include "engine/graphics_frame.hpp"
#import <Metal/Metal.h>
#include <memory>

namespace sadx {
struct MetalFrameStats {
    std::size_t clears = 0, draws = 0, textured = 0;
    std::map<std::string, std::size_t> unsupported;
};
// [platform:metal] Shared fixed-function device backend; no guest memory, EXE or HLE dependency.
class MetalDeviceFrameRenderer {
  public:
    static std::unique_ptr<MetalDeviceFrameRenderer> create(id<MTLDevice>, std::string *error);
    MetalFrameStats encode(const graphics::Frame &, id<MTLCommandBuffer>,
                           MTLRenderPassDescriptor *);
    ~MetalDeviceFrameRenderer();

  private:
    struct Impl;
    std::unique_ptr<Impl> impl;
    explicit MetalDeviceFrameRenderer(std::unique_ptr<Impl>);
};
} // namespace sadx
