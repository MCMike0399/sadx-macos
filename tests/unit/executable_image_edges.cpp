// Boundary cases independent of loader implementation and external assets.
#include "loader/executable_image.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using Bytes = std::vector<std::uint8_t>;
unsigned checks = 0;
void require(bool value, const char *why) {
    ++checks;
    if (!value)
        throw std::runtime_error(why);
}
void put(Bytes &bytes, unsigned at, std::uint32_t value, unsigned width = 4) {
    for (unsigned i = 0; i < width; ++i)
        bytes.at(at + i) = value >> (8 * i);
}
Bytes fixture() {
    Bytes bytes(0x2000, 0);
    put(bytes, 0, 0x5a4d, 2);
    put(bytes, 0x3c, 0x80);
    put(bytes, 0x80, 0x4550);
    put(bytes, 0x84, 0x14c, 2);
    put(bytes, 0x86, 1, 2);
    put(bytes, 0x94, 0xe0, 2);
    put(bytes, 0x98, 0x10b, 2);
    put(bytes, 0xb4, 0x400000);
    put(bytes, 0xd0, 0x2000);
    put(bytes, 0xd4, 0x200);
    put(bytes, 0x180, 0x1e00);
    put(bytes, 0x184, 0x200);
    put(bytes, 0x188, 0x1e00);
    put(bytes, 0x18c, 0x200);
    return bytes;
}
sadx::ExecutableImage parse(const Bytes &bytes) {
    auto image = sadx::ExecutableImage::parse(bytes);
    require(bool(image), "valid fixture");
    return *image;
}
void valid() {
    auto bytes = fixture();
    put(bytes, 0xb4, 0xffffe000);
    bytes[0x1ffc] = 0x12;
    bytes[0x1fff] = 0x34;
    auto image = parse(bytes);
    require(image.byte(0xffffffff) == 0x34 && image.u32(0xfffffffc) == 0x34000012,
            "last address and complete word at top of VA space");
    auto mapped = image.rebased(0xffffe000);
    require(mapped && mapped->byte(0xffffffff) == 0x34, "exact top-aligned rebased image");
    bytes = fixture();
    put(bytes, 0x86, 2, 2);
    put(bytes, 0x1a8, 0x200);
    put(bytes, 0x1ac, 0x200);
    put(bytes, 0x1b0, 0x200);
    put(bytes, 0x1b4, 0x600);
    bytes[0x200] = 0x11;
    bytes[0x600] = 0x22;
    image = parse(bytes);
    require(image.byte(0x400200) == 0x11, "overlap lookup uses first section");
    require(image.rebased(0x500000)->byte(0x500200) == 0x22,
            "overlap flat mapping copies later section last");
    bytes = fixture();
    put(bytes, 0x184, 0x1000);
    put(bytes, 0x180, 0x1000);
    put(bytes, 0x188, 0x1000);
    image = parse(bytes);
    require(!image.byte(0x400300) && image.rebased(0x500000)->byte(0x500300) == 0,
            "flat rebase zeroes unmapped image gaps");
    bytes = fixture();
    put(bytes, 0x120, 0x300); // relocation directory
    put(bytes, 0x124, 10);
    put(bytes, 0x300, 0);
    put(bytes, 0x304, 10);
    put(bytes, 0x308, 0x3400, 2);
    put(bytes, 0x400, 0x400500);
    image = parse(bytes);
    auto once = image.rebased(0x500000);
    auto twice = once->rebased(0x600000);
    require(twice && twice->u32(0x600400) == 0x500500,
            "existing flat snapshot does not reapply relocation metadata");
    std::string error = "caller text";
    require(sadx::ExecutableImage::parse(bytes, &error).has_value() && error == "caller text",
            "success preserves caller error string");
    std::cout << "valid edge checks " << checks << '\n';
}
void malformed() {
    auto bytes = fixture();
    for (std::size_t size = 0; size < bytes.size(); ++size) {
        auto truncated = bytes;
        truncated.resize(size);
        std::string error;
        require(!sadx::ExecutableImage::parse(std::move(truncated), &error) && !error.empty(),
                "every truncated raw file rejected");
    }
    put(bytes, 0xb4, 0xffffe000);
    auto image = parse(bytes);
    require(!image.u32(0xfffffffd) && !image.u32(0xfffffffe) && !image.u32(0xffffffff),
            "word read must not wrap VA space");
    require(!image.rebased(0xffffe001), "rebased image range overflow");
    bytes = fixture();
    put(bytes, 0xd4, 0x2001);
    require(!sadx::ExecutableImage::parse(bytes), "declared headers past file");
    bytes = fixture();
    put(bytes, 0xa8, 0x2000);
    require(!sadx::ExecutableImage::parse(bytes), "entry point past image");
    bytes = fixture();
    put(bytes, 0xf8, 0x300); // export directory
    put(bytes, 0xfc, 0xfffffff0);
    require(!sadx::ExecutableImage::parse(bytes), "export directory extent overflow");
    bytes = fixture();
    put(bytes, 0xf8, 0x300);
    put(bytes, 0xfc, 0x80);
    put(bytes, 0x314, 1);
    put(bytes, 0x318, 1);
    put(bytes, 0x31c, 0x380);
    put(bytes, 0x320, 0x390);
    put(bytes, 0x324, 0x3a0);
    put(bytes, 0x380, 0x280);
    put(bytes, 0x390, 0x1000);
    std::fill(bytes.begin() + 0x1000, bytes.end(), 'A');
    require(!sadx::ExecutableImage::parse(bytes), "unterminated 4096-byte export name");
    bytes[0x1fff] = 0;
    require(parse(bytes).export_named(std::string(4095, 'A')).has_value(),
            "last permitted string terminator");
    put(bytes, 0x380, 0x1000);
    put(bytes, 0xfc, 0x1800);
    put(bytes, 0x390, 0x3b0);
    std::fill(bytes.begin() + 0x1000, bytes.end(), 'B');
    std::string error;
    require(!sadx::ExecutableImage::parse(bytes, &error) && error == "invalid export forwarder",
            "invalid forwarder string");
    bytes = fixture();
    put(bytes, 0x100, 0x300); // import directory
    put(bytes, 0x104, 40);
    put(bytes, 0x300, 0x400);
    put(bytes, 0x30c, 0x1fff);
    put(bytes, 0x310, 0x500);
    bytes[0x1fff] = 'X';
    require(!parse(bytes).imports(), "unterminated DLL name");
    put(bytes, 0x30c, 0x600);
    put(bytes, 0x400, 0x1fff);
    require(!parse(bytes).imports(), "truncated hint/name");
    put(bytes, 0x400, 0x80000001);
    put(bytes, 0x310, 0x1ffe);
    require(!parse(bytes).imports(), "truncated IAT slot");
    bytes = fixture();
    put(bytes, 0x120, 0x300);
    put(bytes, 0x124, 9);
    put(bytes, 0x304, 8);
    require(!parse(bytes).rebased(0x500000), "trailing partial relocation header");
    std::cout << "malformed edge checks " << checks << '\n';
}
} // namespace
int main(int argc, char **argv) {
    try {
        const std::string mode = argc == 2 ? argv[1] : "--all";
        if (mode == "--all" || mode == "--valid")
            valid();
        if (mode == "--all" || mode == "--malformed")
            malformed();
        require(mode == "--all" || mode == "--valid" || mode == "--malformed",
                "usage: executable-image-edges-test [--valid|--malformed|--all]");
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
