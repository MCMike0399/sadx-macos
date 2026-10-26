// This TU is compiled with Root's source tree first: these are its actual complete types.
#include "runtime/model_device.hpp"
#include "engine/polybuff_initialization.hpp"
#include <type_traits>

static_assert(sizeof(sadx::polybuff::Context) == sizeof(void *));
static_assert(std::is_same_v<decltype(sadx::polybuff::Context::display), sadx::native::Display &>);

// Instantiate the real native Context specialization in the combined executable.
// Startup remains explicitly unbound in this test; no fabricated count owner is supplied.
void polybuff_native_initializer_link(sadx::polybuff::Context &context,
                                      sadx::polybuff::Buffers &buffers,
                                      sadx::polybuff::RetainedBase &retained,
                                      sadx::polybuff::Scratch &scratch, void *base) {
    sadx::polybuff::initialize_buffers(context, buffers, retained, scratch, 32, 24576, base);
}
