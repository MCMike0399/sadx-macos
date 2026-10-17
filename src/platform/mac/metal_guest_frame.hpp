#pragma once
#ifndef __OBJC__
#error "metal_guest_frame.hpp is the Objective-C++ Metal boundary for guest frames"
#endif
#include "hle/hle_process.hpp"
#include "platform/mac/metal_device_frame.hpp"
#import <Metal/Metal.h>
#include <memory>
#include <string>
namespace sadx {
// Reference adapter: decode guest resources while paused at Present, then feed the shared
// device backend. Textures must remain stable between their draw and that Present.
using MetalGuestFrameStats = MetalFrameStats;
class MetalGuestFrameRenderer {
  public:
    static std::unique_ptr<MetalGuestFrameRenderer> create(id<MTLDevice>, std::string *error);
    // Encodes the frame into `pass` (its colour attachment is the target; the frame's first Clear
    // sets the load action). Returns what was drawn.
    MetalGuestFrameStats encode(const HleProcess::D3D8Frame &, const X86Guest &memory,
                                id<MTLCommandBuffer>, MTLRenderPassDescriptor *);
    ~MetalGuestFrameRenderer();

  private:
    struct Impl;
    std::unique_ptr<Impl> impl;
    explicit MetalGuestFrameRenderer(std::unique_ptr<Impl>);
};
} // namespace sadx
