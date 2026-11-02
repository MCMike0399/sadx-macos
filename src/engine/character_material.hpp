#pragma once
#include <array>
#include <cstdint>
#include <cstring>

namespace sadx {
// NOTSA: lossless NJS_MATERIAL storage. The legacy word view remains for reference
// adapters; native game code names the fields without aliasing imported bytes.
struct SceneMaterial {
    std::array<std::uint32_t, 5> words;
    std::uint32_t &diffuse() { return words[0]; }
    std::uint32_t diffuse() const { return words[0]; }
    std::uint32_t specular() const { return words[1]; }
    float power() const {
        float value;
        std::memcpy(&value, &words[2], sizeof(value));
        return value;
    }
    std::uint32_t texture() const { return words[3]; }
    std::uint32_t &attributes() { return words[4]; }
    std::uint32_t attributes() const { return words[4]; }
};
} // namespace sadx
