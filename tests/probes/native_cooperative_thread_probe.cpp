#include "engine/cooperative_thread_native.hpp"
#include <algorithm>
#include <cstdint>
extern "C" void sadx_native_cooperative_thread_probe(const std::uint32_t *i, std::uint32_t *out) {
    std::fill_n(out, 8, 0);
    std::shared_ptr<void> current;
    if (i[0]) current = std::make_shared<std::uint32_t>(i[0]);
    out[0] = static_cast<std::uint32_t>(sadx::cooperative_threads::wait(current));
    out[1] = current ? 1 : 0;
    if (i[1]) current.reset();
    out[2] = static_cast<std::uint32_t>(sadx::cooperative_threads::wait(current));
    out[3] = current ? 1 : 0;
}
