#pragma once
#include <cstdint>

namespace sadx::polybuff {
struct DeviceBuffer;
struct Context;
} // namespace sadx::polybuff
namespace sadx::native {
// NOTSA: stream zero and fixed-function vertex format belong to the existing device.
// [platform:d3d8] A bound stream retains its resource independently of the game handle.
struct ModelVertexStream {
    ModelVertexStream() = default;
    ~ModelVertexStream();
    ModelVertexStream(const ModelVertexStream &) = delete;
    ModelVertexStream &operator=(const ModelVertexStream &) = delete;
    polybuff::DeviceBuffer *resource = nullptr;
    std::uint32_t stride = 0;
    std::uint32_t format = 0;
};
} // namespace sadx::native
