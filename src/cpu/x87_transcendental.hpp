#pragma once

#include <optional>
#include "cpu/x87_math.hpp"

namespace sadx {

// Explicitly selected, native integer software model from pinned Bochs
// upstream. It models P5/P6 x87 tables/internal rounding. Agreement with the
// user's Intel Ultra hardware has not yet been measured. This is not an
// assumption that mathematical correctly-rounded atan2 equals Intel FPATAN.
class X87Transcendental {
public:
    // FPATAN evaluates atan2(y,x), forces a 64-bit significand independently
    // of CW precision control, and respects CW rounding control. The existing
    // context owns low-six exception flags; unmasked exceptions propagate as
    // X87Error. x87 stack/TOP/tag/C1 and deferred traps are outside this API.
    static X87Value fpatan_bochs_p5_p6(X87Context& context, X87Value y, X87Value x);
    // FSINCOS and FPTAN, same model, PC-independent 64-bit significands, RC
    // respected. nullopt: |x| >= 2^63, where the instruction sets C2 and leaves
    // ST(0) unchanged (FPTAN then pushes nothing).
    struct SineCosine {X87Value sine, cosine;};
    static std::optional<SineCosine> fsincos_bochs(X87Context& context, X87Value x);
    static std::optional<X87Value> fptan_bochs(X87Context& context, X87Value x); // tan; the instruction then pushes 1.0
    static std::optional<X87Value> fsin_bochs(X87Context& context, X87Value x);  // FSIN (nullopt: the C2 path)
    static std::optional<X87Value> fcos_bochs(X87Context& context, X87Value x);  // FCOS
    // F2XM1 (2^x - 1, defined for |x| <= 1), FYL2X (y * log2 x) and FYL2XP1 (y * log2(x + 1)), same model.
    static X87Value f2xm1_bochs(X87Context& context, X87Value x);
    static X87Value fyl2x_bochs(X87Context& context, X87Value x, X87Value y);
    static X87Value fyl2xp1_bochs(X87Context& context, X87Value x, X87Value y);
};

} // namespace sadx
