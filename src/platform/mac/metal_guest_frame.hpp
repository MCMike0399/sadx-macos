#pragma once
#ifndef __OBJC__
#error "metal_guest_frame.hpp is the Objective-C++ Metal boundary for guest frames"
#endif
#import <Metal/Metal.h>
#include "hle/hle_process.hpp"
#include <memory>
#include <string>
namespace sadx {
// Draws one recorded guest frame (HleProcess::D3D8Frame) with Metal: the frame's Clears, then every
// DrawPrimitiveUP in order — pre-transformed vertices (FVF XYZRHW with DIFFUSE, optional SPECULAR and one set of
// texture coordinates), triangle lists/strips/fans, the stage-0 texture decoded from the guest surface (A8R8G8B8,
// X8R8G8B8, R5G6B5, X1R5G5B5, A1R5G5B5, A4R4G4B4), stage-0 COLOROP/ALPHAOP MODULATE/SELECTARG1/SELECTARG2 and
// alpha blending and depth testing (ZENABLE/ZWRITEENABLE/ZFUNC, the pass needs a Depth32Float attachment) from
// the recorded render states, point or linear filtering and wrap/clamp addressing.
// Anything else (other FVFs, DXT, other blend/stage modes) is not drawn and is counted in `unsupported` with a
// reason — the picture never invents state. The guest's memory is read when the frame is drawn, so draw it while
// the guest is paused at the Present that closed it.
struct MetalGuestFrameStats {std::size_t clears=0,draws=0,textured=0;std::map<std::string,std::size_t> unsupported;};
class MetalGuestFrameRenderer {
public:
    static std::unique_ptr<MetalGuestFrameRenderer> create(id<MTLDevice>,std::string* error);
    // Encodes the frame into `pass` (its colour attachment is the target; the frame's first Clear sets the load
    // action). Returns what was drawn.
    MetalGuestFrameStats encode(const HleProcess::D3D8Frame&,const X86Guest& memory,id<MTLCommandBuffer>,MTLRenderPassDescriptor*);
    ~MetalGuestFrameRenderer();
private:
    struct Impl;std::unique_ptr<Impl> impl;
    explicit MetalGuestFrameRenderer(std::unique_ptr<Impl>);
};
} // namespace sadx
