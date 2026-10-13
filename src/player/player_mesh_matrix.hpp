#pragma once
#include "player/player_coordinates.hpp"

namespace sadx {
X87Value mesh_matrix_determinant(const NinjaMatrix&,X87Context&); // actual795000
// Actual787630: singular-zero determinant stores source finite sentinel
// 0x7f7fc99e into all16elements and returnsfalse. NaN comparison proceeds with
// arithmetic, as in source FUCOM. The source caller decides whether to use the
// boolean; this owner never substitutes identity or rolls back partial stores.
bool invert_mesh_matrix(NinjaMatrix&,const CoordinateAxisConstants&,X87Context&);
// Actual788a50: retain X/Y FLDs before stores; MOV-copy rawZ once and FLD it
// for each row2component. Translation row is not modified.
void scale_mesh_matrix_vector(NinjaMatrix&,const NinjaVector&,X87Context&);
} // namespace sadx
