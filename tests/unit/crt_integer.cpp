#include "cpu/crt_integer.hpp"
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>

static void require(bool value,const char* message) {
    if (!value) throw std::runtime_error(message);
}

int main() {
    try {
        for (std::uint16_t precision : {std::uint16_t{0},std::uint16_t{0x200},std::uint16_t{0x300}}) {
            for (std::uint16_t rounding : {std::uint16_t{0},std::uint16_t{0x400},std::uint16_t{0x800},std::uint16_t{0xc00}}) {
                sadx::X87Context context(precision | rounding | 0x7f);
                for (double value : {0.,-0.,0.25,-0.25,1.,-1.,1.25,-1.25,1.75,-1.75,
                                     16383.999,-16383.999,16384.,-16384.,65536.,-65536.}) {
                    const auto actual=sadx::crt_ftol(context.from_double(value),context);
                    const auto expected=static_cast<std::uint64_t>(static_cast<std::int64_t>(std::trunc(value)));
                    require(actual==expected,"CRT finite conversion differs from source truncation intent");
                }
                require(sadx::crt_ftol(context.from_double(4294967296.0),context)==0x100000000ULL,
                        "high word of a conversion with zero low word was lost");
                require(sadx::crt_ftol_low32(context.from_double(-16384),context)==0xffffc000U,
                        "negative angle conversion does not preserve low-word bits");
                for (double value : {std::numeric_limits<double>::infinity(),
                                     -std::numeric_limits<double>::infinity(),
                                     std::numeric_limits<double>::quiet_NaN(),0x1p64}) {
                    require(sadx::crt_ftol(context.from_double(value),context)==0x8000000000000000ULL,
                            "masked indefinite FISTP64 result changed");
                }
            }
        }
        std::cout<<"CRT integer conversion: twelve explicit control profiles, sign/carry correction and indefinite values passed.\n"
                    "Original hardware exception delivery remains unverified.\n";
    } catch (const std::exception& error) { std::cerr<<error.what()<<'\n'; return 1; }
}
