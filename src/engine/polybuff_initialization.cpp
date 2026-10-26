#include "engine/polybuff_initialization.hpp"

namespace sadx::polybuff {
const std::array<BufferDescription, kBufferCount> &buffer_descriptions() {
    static const std::array<BufferDescription, kBufferCount> descriptions{{
        {28, 324, "PolyBuff_QUAD"},
        {12, 2, "PolyBuff_CS"},
        {24, 18, "PolyBuff_CS_N"},
        {20, 258, "PolyBuff_CS_UVN"},
        {32, 274, "PolyBuff_CS_UVN_N"},
        {16, 66, "PolyBuff_CS_D8"},
        {28, 82, "PolyBuff_CS_N_D8"},
        {24, 322, "PolyBuff_CS_UVN_D8"},
        {36, 338, "PolyBuff_CS_UVN_N_D8"},
    }};
    return descriptions;
}

} // namespace sadx::polybuff
