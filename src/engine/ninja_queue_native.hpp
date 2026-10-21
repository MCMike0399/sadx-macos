#pragma once
#include "cpu/x87_math.hpp"
#include "engine/ninja_queue_initial_state.hpp"
#include <array>
#include <functional>
#include <memory>
#include <stdexcept>

namespace sadx {
class NinjaRenderQueueArena;
class NinjaSpriteQueue;
namespace graphics {
struct Draw;
}
namespace queue {
inline constexpr std::size_t kBufferBytes = 557056;
// NOTSA: an actual retained queue allocation and its physical alignment.
// The reference allocator supplies the source allocation's alignment residue;
// the native allocator derives it from the real owned byte storage.
struct Buffer {
    std::shared_ptr<NinjaRenderQueueArena> arena;
    std::uint32_t alignment_remainder = 0;
};
Buffer allocate_buffer();
// NOTSA: storage reuses the maintained arena generations, records and buckets.
class Storage {
  public:
    Storage();
    ~Storage();
    Storage(const Storage &) = delete;
    Storage &operator=(const Storage &) = delete;
    NinjaSpriteQueue &records();
    const NinjaSpriteQueue &records() const;
    const Buffer &buffer() const;
    void release_buffer();
    void bind_buffer(Buffer);
    void reset_buffer();

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
struct State {
    NinjaQueueInitialState values{};
    std::array<float, 4> current_material{};
};
struct Owners {
    Storage &storage;
    State &state;
    std::array<float, 4> &reset_material;
    std::int32_t &loop_count;
};
// Callbacks perform actual device writes. A false return means unavailable,
// never a negative HRESULT (the original ignores device HRESULT values).
struct Services {
    std::function<bool(std::uint32_t)> set_depth_comparison;
    std::function<bool(bool)> set_depth_write;
    std::function<bool(bool)> set_alpha;
    std::function<void(const std::array<float, 4> &)> set_constant_material;
    std::function<bool(std::uint32_t, std::uint32_t)> set_blend;
};
enum class Boundary {
    Buffer,
    DepthComparison,
    DepthWrite,
    Alpha,
    Material,
    Blend,
    Device,
    BlendSelector,
    BlendValue
};
class Unavailable : public std::runtime_error {
  public:
    explicit Unavailable(Boundary);
    Boundary boundary() const {
        return boundary_;
    }

  private:
    Boundary boundary_;
};
// FUNCTION: SONIC 0x402f40 ___njSetConstantMaterial
void set_material(const std::array<float, 4> &, State &, const Services &, X87Context &);
// FUNCTION: SONIC 0x4128f0 ResetMaterial
void reset_material(Owners &, const Services &, X87Context &);
// FUNCTION: SONIC 0x407330 ResetDrawQueue
void reset_draw_queue(Storage &, State &, const Services &);
// FUNCTION: SONIC 0x407ab0 late_alloca_reset
void reset(Owners &, const Services &, X87Context &);
// FUNCTION: SONIC 0x408630 late_alloca_init
void initialize(Owners &, const Services &, X87Context &,
                const std::function<Buffer()> &allocator = allocate_buffer);
// NOTSA: immutable contiguous blend-table values, including the four depth-table
// entries reached by original modes12..15. No executable reads at runtime.
struct BlendTables {
    std::array<std::uint32_t, 2> targets;
    std::array<std::uint32_t, 12> modes;
    std::array<std::uint32_t, 4> following_depth_modes;
};
using RenderStateWriter = std::function<bool(std::uint32_t, std::uint32_t)>;
// Each acquisition borrows the current device again; source may replace it after
// the first of the special-mode writes. Missing acquisition is explicit failure.
using DeviceAccess = std::function<RenderStateWriter()>;
// FUNCTION: SONIC 0x77ec60 njColorBlendingMode_
void set_device_blend(std::uint32_t selector, std::uint32_t mode, const BlendTables &,
                      const DeviceAccess &);
// NOTSA: native command-state adapter, borrowing the actual maintained draw owner.
void set_device_blend(std::uint32_t selector, std::uint32_t mode, const BlendTables &,
                      graphics::Draw &);
// FUNCTION: SONIC 0x4030d0 njColorBlendingMode
void set_blend(std::uint32_t selector, std::uint32_t value, State &, const Services &);
} // namespace queue
} // namespace sadx
