#pragma once
#include "engine/normal_uv_strip.hpp"
#include "runtime/native_display.hpp"

namespace sadx::polybuff {
// [platform:d3d8] CPU storage backing a native vertex-buffer resource.
struct BufferMemory {
    std::vector<std::uint8_t> bytes;
};
struct DeviceBuffer {
    BufferMemory memory;
    std::uint32_t references = 1;
    std::uint32_t usage;
    std::uint32_t format;
    std::uint32_t pool;
};
// NOTSA: the source algorithms borrow the actual shared device and frame owners.
struct Context {
    native::Display &display;
    auto &stream() { return display.model_vertex_stream_; }
    auto &device() { return display.device_state_; }
    auto &frame() { return display.frame_; }
    auto &counts() { return display.graphics_library_.polygon_buffer_counts; }
    auto &buffer(std::size_t index) { return display.polygon_resources_.buffers.records.at(index); }
};
} // namespace sadx::polybuff
