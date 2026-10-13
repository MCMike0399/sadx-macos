#include "cpu/crt_rand.hpp"
namespace sadx {
void crt_srand_6443b5(CrtRandState& s,std::uint32_t seed) noexcept{s.seed_at_2bcbdf8=seed;}
std::optional<std::uint32_t> crt_rand_6443bf(CrtRandState& s) noexcept{
    if(!s.seed_at_2bcbdf8)return std::nullopt;
    const std::uint32_t next=*s.seed_at_2bcbdf8*0x343fdU+0x269ec3U;s.seed_at_2bcbdf8=next;return (next>>16)&0x7fffU;
}
} // namespace sadx
