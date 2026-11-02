#include "reset_final_parent.hpp"
#include <charconv>
#include <iostream>
#include <string>

namespace {
unsigned number(const char *text) {
    unsigned value = 0;
    const std::string input(text);
    const auto result = std::from_chars(input.data(), input.data() + input.size(), value);
    if (result.ec != std::errc{} || result.ptr != input.data() + input.size())
        throw std::invalid_argument("invalid fake parent argument");
    return value;
}
} // namespace

int main(int argc, char **argv) {
    if (argc != 3)
        return 64;
    try {
        sadx::test::retain_reset_parent(number(argv[1]), number(argv[2]));
        std::cout << "guarded\n" << std::flush;
        // No game code or reserved input is linked or executed by this process.
        for (;;)
            ::pause();
    } catch (const std::exception &) {
        return 64;
    }
}
