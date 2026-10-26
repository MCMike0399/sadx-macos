#pragma once
#include <cstdint>
constexpr unsigned kStripProbeInputWords = 10784;
constexpr unsigned kStripProbeOutputWords = 262144;
constexpr unsigned kMeshInput = 32, kUvInput = 2080, kPointsInput = 6176;
constexpr unsigned kNormalsInput = 8224, kArgumentsInput = 10272;
extern "C" unsigned sadx_native_normal_uv_strip_probe(const std::uint32_t *, std::uint32_t *);
