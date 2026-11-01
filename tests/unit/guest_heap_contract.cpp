#include "loader/guest_heap.hpp"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {
struct Generator {
    std::uint32_t state = 1234567;
    std::uint32_t next() { state = state * 1664525U + 1013904223U; return state; }
};
void scenario(std::uint32_t handle, std::uint32_t first) {
    sadx::GuestHeap heap(handle, first);
    Generator random;
    std::vector<std::uint32_t> addresses{first};
    // Allocation does not require a preceding create; optional zero is a valid token.
    const auto initial = heap.alloc(handle, 0);
    std::cout << "initial " << bool(initial) << ' ' << initial.value_or(0) << '\n';
    if (initial) {
        assert(!heap.read(*initial));
        assert(heap.write(*initial, 0) && heap.read(*initial) == 0);
        assert(!heap.write(*initial + 1, 1));
        addresses.push_back(*initial);
    }
    for (unsigned step = 0; step < 4096; ++step) {
        const auto action = random.next() % 5;
        const auto selected = random.next() & 3U ? handle : handle + 1U;
        const auto address = addresses[random.next() % addresses.size()] + (random.next() % 48) - 1U;
        const auto requested = step % 127 == 0 ? 0xFFFFFFFFU : random.next() % 33;
        std::cout << step << ' ' << action << ' ' << selected << ' ' << address << ' ' << requested << ' ';
        if (action == 0) {
            std::cout << heap.create();
        } else if (action == 1) {
            const auto result = heap.alloc(selected, requested);
            std::cout << bool(result) << ':' << result.value_or(0);
            if (result) addresses.push_back(*result);
        } else if (action == 2) {
            std::cout << heap.free(selected, address);
        } else if (action == 3) {
            const auto result = heap.read(address);
            std::cout << bool(result) << ':' << unsigned(result.value_or(0));
        } else {
            const auto value = static_cast<std::uint8_t>(random.next());
            std::cout << unsigned(value) << ':' << heap.write(address, value);
        }
        std::cout << ' ' << heap.live_blocks() << '\n';
    }
    for (const auto &call : heap.trace)
        std::cout << "trace " << call.kind << ' ' << call.a << ' ' << call.b << '\n';
}
} // namespace
int main() {
    for (const auto &setting : std::array<std::array<std::uint32_t, 2>, 5>{{
             {0x510000, 0x50000000}, {17, 0}, {99, 0xFFFFFFEF}, {99, 0xFFFFFFE0}, {0, 5}}})
        scenario(setting[0], setting[1]);
}
