#include "hle/hle_process.hpp"
#include <iostream>
#include <stdexcept>

namespace {
void require(bool value, const std::string &message) {
    if (!value)
        throw std::runtime_error(message);
}
}
int main() {
    try {
        std::string error;
        auto process = sadx::HleProcess::create(".", "build/compare-string-overlay", &error);
        require(bool(process), error);
        auto &guest = process->guest();
        require(guest.map(0x10000000, 4096, false, &error), error);
        const auto trap = process->hle().trap(guest, "kernel32.dll", "CompareStringA", &error);
        require(trap.has_value(), error);
        const auto compare = [&](const std::string &left, const std::string &right,
                                 std::uint32_t left_length, std::uint32_t right_length) {
            require(guest.write(0x10000000, left.c_str(), left.size() + 1), "left input");
            require(guest.write(0x10000800, right.c_str(), right.size() + 1), "right input");
            const auto result = guest.call(*trap, {9, 1, 0x10000000, left_length,
                                                  0x10000800, right_length});
            require(bool(result), result.error);
            return result.eax;
        };
        require(compare("SonicDX01.snc", "sonicdx01.SNC", -1, -1) == 2,
                "new save must recognize an existing name without case sensitivity");
        require(compare("SonicDX01.snc", "SonicDX02.snc", -1, -1) == 1, "less result is 1");
        require(compare("SonicDX02.snc", "SonicDX01.snc", -1, -1) == 3, "greater result is 3");
        require(compare("ABCx", "abcy", 3, 3) == 2, "explicit lengths exclude following bytes");
        require(compare("abc", "abcd", 3, 4) == 1, "prefix length");
        require(compare("ignored", "ignored", 0, 0) == 2, "empty spans");
        require(compare("a", "A", -2, -1) == 2, "any negative length is null terminated");
        const auto unsupported = guest.call(*trap, {0x41f, 1, 0x10000000, -1U,
                                                    0x10000800, -1U});
        require(!unsupported && unsupported.error.find("CompareStringA") != std::string::npos,
                "unsupported collation must stop explicitly");
        const auto unreadable = guest.call(*trap, {9, 1, 0x20000000, -1U, 0x10000800, -1U});
        require(!unreadable && unreadable.error.find("unreadable string") != std::string::npos,
                "missing memory must not be treated as an empty name");
        require(guest.write(0x10000000, "a.b", 4) && guest.write(0x10000800, "a_b", 4),
                "unsupported punctuation inputs");
        const auto punctuation = guest.call(*trap, {9, 1, 0x10000000, -1U, 0x10000800, -1U});
        require(!punctuation && punctuation.error.find("NLS collation") != std::string::npos,
                "unrecovered punctuation ordering must stop explicitly");
        std::cout << "CompareStringA: save names, ordering, lengths and unsupported locale passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
