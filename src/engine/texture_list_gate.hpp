#pragma once

namespace sadx::texture_lifetime {
struct List;
}

namespace sadx::texture_selection {
// The imported List's entries are the complete source count/array. Missing import
// facts must be rejected by the importer before constructing this native view.
// True means not ready; a binding need not own a live graphics surface.
// FUNCTION: SONIC 0x403250 isTextureNG
bool list_not_ready(const texture_lifetime::List *);
} // namespace sadx::texture_selection
