#include "loader/guest_heap.hpp"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string_view>
#include <vector>

namespace {
struct Generator {
    std::uint32_t state = 1234567;
    std::uint32_t next() {
        state = state * 1664525U + 1013904223U;
        return state;
    }
};
void scenario(std::uint32_t handle, std::uint32_t first, std::ostream &out) {
    sadx::GuestHeap heap(handle, first);
    Generator random;
    std::vector<std::uint32_t> addresses{first};
    // Allocation does not require a preceding create; optional zero is a valid token.
    const auto initial = heap.alloc(handle, 0);
    out << "initial " << bool(initial) << ' ' << initial.value_or(0) << '\n';
    if (initial) {
        assert(!heap.read(*initial));
        assert(heap.write(*initial, 0) && heap.read(*initial) == 0);
        assert(!heap.write(*initial + 1, 1));
        assert(!heap.free(handle + 1U, *initial));
        assert(heap.live_blocks() == 1 && heap.read(*initial) == 0);
        addresses.push_back(*initial);
    }
    for (unsigned step = 0; step < 4096; ++step) {
        const auto action = random.next() % 5;
        const auto selected = random.next() & 3U ? handle : handle + 1U;
        const auto address = addresses[random.next() % addresses.size()] + (random.next() % 48) - 1U;
        const auto requested = step % 127 == 0 ? 0xFFFFFFFFU : random.next() % 33;
        out << step << ' ' << action << ' ' << selected << ' ' << address << ' ' << requested << ' ';
        if (action == 0) {
            out << heap.create();
        } else if (action == 1) {
            const auto result = heap.alloc(selected, requested);
            out << bool(result) << ':' << result.value_or(0);
            if (result) addresses.push_back(*result);
        } else if (action == 2) {
            out << heap.free(selected, address);
        } else if (action == 3) {
            const auto result = heap.read(address);
            out << bool(result) << ':' << unsigned(result.value_or(0));
        } else {
            const auto value = static_cast<std::uint8_t>(random.next());
            out << unsigned(value) << ':' << heap.write(address, value);
        }
        out << ' ' << heap.live_blocks() << '\n';
    }
    for (const auto &call : heap.trace)
        out << "trace " << call.kind << ' ' << call.a << ' ' << call.b << '\n';
}
} // namespace
int main(int argc, char **argv) {
    std::ostringstream out;
    for (const auto &setting : std::array<std::array<std::uint32_t, 2>, 5>{{
             {0x510000, 0x50000000}, {17, 0}, {99, 0xFFFFFFEF}, {99, 0xFFFFFFE0}, {0, 5}}})
        scenario(setting[0], setting[1], out);
    const auto captured = out.str();
    std::uint64_t digest = 14695981039346656037ULL;
    for (const unsigned char byte : captured)
        digest = (digest ^ byte) * 1099511628211ULL;
    // Frozen from immutable1300815, including all results and C/A/F trace records.
    assert(digest == 0xe8974e7380706609ULL && captured.size() == 755791);
    if (argc == 2 && std::string_view(argv[1]) == "--capture")
        std::cout << captured;
    else {
        assert(argc == 1);
        std::cout << "Guest heap: 20,480 mixed operations and complete trace match the frozen contract.\n";
    }
}
