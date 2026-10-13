#pragma once
#include <cstdint>
#include <optional>
namespace sadx {
// The executable's single-threaded MSVC CRT RNG: one 32-bit seed at 2bcbdf8.
// srand 6443b5 stores it; rand 6443bf advances seed*0x343fd+0x269ec3 (low 32
// bits) and returns bits 16..30 (the word at 2bcbdfa masked with 0x7fff).
struct CrtRandState {std::optional<std::uint32_t> seed_at_2bcbdf8;};
void crt_srand_6443b5(CrtRandState&,std::uint32_t seed) noexcept;
std::optional<std::uint32_t> crt_rand_6443bf(CrtRandState&) noexcept; // nullopt: unknown seed, nothing stored
} // namespace sadx
