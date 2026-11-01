// Synthetic PE32 records. No executable assets or production parsing helpers.
#include "loader/executable_image.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using Bytes = std::vector<std::uint8_t>;
constexpr std::uint32_t kBase = 0x400000;
constexpr std::size_t kOptional = 0x98;
constexpr std::size_t kSection = 0x178;
unsigned checks = 0;
void require(bool ok, const std::string& why) {
    ++checks;
    if (!ok) throw std::runtime_error(why);
}
void put(Bytes& b, std::size_t at, std::uint32_t value, unsigned width = 4) {
    for (unsigned i = 0; i < width; ++i) b.at(at + i) = value >> (i * 8);
}
void text(Bytes& b, std::size_t at, const std::string& value) {
    for (char c : value) b.at(at++) = static_cast<std::uint8_t>(c);
    b.at(at) = 0;
}
Bytes pe() {
    Bytes b(0x1000, 0);
    put(b, 0, 0x5a4d, 2);
    put(b, 0x3c, 0x80);
    put(b, 0x80, 0x4550);
    put(b, 0x84, 0x14c, 2);
    put(b, 0x86, 1, 2);
    put(b, 0x94, 0xe0, 2);
    put(b, kOptional, 0x10b, 2);
    put(b, kOptional + 16, 0x280);
    put(b, kOptional + 28, kBase);
    put(b, kOptional + 56, 0x1000);
    put(b, kOptional + 60, 0x200);
    put(b, kOptional + 92, 16);
    put(b, kSection + 8, 0xe00);
    put(b, kSection + 12, 0x200);
    put(b, kSection + 16, 0xe00);
    put(b, kSection + 20, 0x200);
    return b;
}
void directory(Bytes& b, unsigned index, std::uint32_t rva, std::uint32_t size) {
    put(b, kOptional + 96 + index * 8, rva);
    put(b, kOptional + 100 + index * 8, size);
}
Bytes with_tables() {
    auto b = pe();
    directory(b, 0, 0x300, 0x80);
    put(b, 0x314, 2); // NumberOfFunctions
    put(b, 0x318, 2); // NumberOfNames
    put(b, 0x31c, 0x380);
    put(b, 0x320, 0x390);
    put(b, 0x324, 0x3a0);
    put(b, 0x380, 0x280);
    put(b, 0x384, 0x350);
    put(b, 0x390, 0x3b0);
    put(b, 0x394, 0x3c0);
    put(b, 0x3a0, 0, 2);
    put(b, 0x3a2, 1, 2);
    text(b, 0x350, "other.Target");
    text(b, 0x3b0, "Direct");
    text(b, 0x3c0, "Forward");
    directory(b, 1, 0x400, 40);
    put(b, 0x400, 0x480);
    put(b, 0x40c, 0x460);
    put(b, 0x410, 0x4a0);
    text(b, 0x460, "EXAMPLE.dll");
    put(b, 0x480, 0x4c0);
    put(b, 0x484, 0x8000000b);
    put(b, 0x4a0, 0x4c0);
    put(b, 0x4a4, 0x8000000b);
    put(b, 0x4c0, 7, 2);
    text(b, 0x4c2, "Named");
    directory(b, 5, 0x500, 12);
    put(b, 0x500, 0);
    put(b, 0x504, 12);
    put(b, 0x508, 0x3280, 2); // HIGHLOW at RVA 0x280
    put(b, 0x50a, 0, 2); // ABSOLUTE padding
    put(b, 0x280, kBase + 0x777);
    return b;
}
sadx::ExecutableImage parse(const Bytes& b) {
    std::string error;
    auto image = sadx::ExecutableImage::parse(b, &error);
    require(bool(image), "parse: " + error);
    return *image;
}
void valid() {
    for (unsigned variant = 0; variant < 4; ++variant) {
        auto b = with_tables();
        if (variant & 1) put(b, 0x400, 0); // OriginalFirstThunk fallback
        if (variant & 2) directory(b, 1, 0x400, 0); // mapped terminator limit
        auto image = parse(b);
        require(image.image_base() == kBase && image.size_of_image() == 0x1000 &&
                    image.entry_point() == kBase + 0x280, "image facts");
        require(!image.byte(kBase - 1) && !image.byte(kBase + 0x1000), "mapping edges");
        for (unsigned i = 0; i < b.size(); ++i)
            require(image.byte(kBase + i) == b[i], "mapped file byte");
        require(!image.export_named("direct") && !image.export_named("Missing"), "exact names");
        const auto direct = image.export_named("Direct");
        const auto forward = image.export_named("Forward");
        require(direct && direct->va == kBase + 0x280 && !direct->forwarder, "direct export");
        require(forward && forward->va == 0 && forward->forwarder == "other.Target", "forwarder");
        auto imports = image.imports();
        require(imports && imports->size() == 1 && (*imports)[0].dll == "EXAMPLE.dll", "DLL");
        const auto& symbols = (*imports)[0].symbols;
        require(symbols.size() == 2 && symbols[0].name == "Named" && !symbols[0].ordinal &&
                    symbols[0].hint == 7 && symbols[0].iat_va == kBase + 0x4a0, "named import");
        require(!symbols[1].name && symbols[1].ordinal == 11 && symbols[1].hint == 0 &&
                    symbols[1].iat_va == kBase + 0x4a4, "ordinal import");
        for (auto base : {kBase, 0x200000u, 0x10000000u}) {
            auto mapped = image.rebased(base);
            require(bool(mapped), "valid rebase");
            const auto value = base + 0x777;
            for (unsigned i = 0; i < b.size(); ++i) {
                const auto expected = i >= 0x280 && i < 0x284 ?
                    std::uint8_t(value >> (8 * (i - 0x280))) : b[i];
                require(mapped->byte(base + i) == expected, "rebased byte");
            }
            require(mapped->export_named("Direct")->va == base + 0x280 &&
                        mapped->export_named("Forward")->forwarder == "other.Target", "rebased exports");
            require(mapped->imports()->at(0).symbols[0].iat_va == base + 0x4a0, "rebased import");
            require(image.u32(kBase + 0x280) == kBase + 0x777, "immutable original");
        }
    }
    // A complete final block, multiple blocks and modulo-32-bit relocated values.
    auto tail = with_tables();
    directory(tail, 5, 0xfe8, 24);
    put(tail, 0xfe8, 0);
    put(tail, 0xfec, 12);
    put(tail, 0xff0, 0x3280, 2);
    put(tail, 0xff4, 0);
    put(tail, 0xff8, 12);
    put(tail, 0xffc, 0x3284, 2);
    put(tail, 0x284, 0xfffffff0);
    auto end = parse(tail).rebased(kBase + 0x10000);
    require(end && end->u32(kBase + 0x10280) == kBase + 0x10777 &&
                end->u32(kBase + 0x10284) == 0xfff0, "final complete blocks and modulo delta");
    auto short_virtual = pe();
    put(short_virtual, kSection + 8, 0x100);
    require(parse(short_virtual).byte(kBase + 0xfff) == 0, "raw size extends virtual size");
    auto b = pe();
    b.resize(0x300);
    put(b, kSection + 16, 0x100);
    auto sparse = parse(b);
    require(sparse.byte(kBase + 0x300) == 0 && sparse.u32(kBase + 0xffc) == 0 &&
                !sparse.u32(kBase + 0xffd), "zero tail and word boundary");
    put(b, kOptional + 16, 0);
    require(parse(b).entry_point() == 0, "absent entry point");
    put(b, kOptional + 56, 0);
    std::string error;
    require(!parse(b).rebased(kBase, &error) && error == "missing SizeOfImage", "missing size");
    std::cout << "valid checks " << checks << '\n';
}
void p2() {
    auto b = pe();
    directory(b, 5, 0xff8, 10);
    put(b, 0xff8, 0);
    put(b, 0xffc, 10);
    std::string error;
    auto image = parse(b);
    require(!image.rebased(kBase + 0x10000, &error) && !error.empty(),
            "LB-1: final eight-byte header declaring ten-byte block must fail explicitly");
    std::cout << "LB-1 explicit malformed-image failure\n";
}
void malformed() {
    p2();
    auto reject_parse = [](Bytes b, const char* why) {
        std::string error;
        require(!sadx::ExecutableImage::parse(std::move(b), &error) && !error.empty(), why);
    };
    auto reject_rebase = [](Bytes b, const char* why) {
        std::string error;
        require(!parse(b).rebased(kBase + 0x10000, &error) && !error.empty(), why);
    };
    auto reject_imports = [](Bytes b, const char* why) {
        std::string error;
        require(!parse(b).imports(&error) && !error.empty(), why);
    };
    for (unsigned size : {0u, 1u, 63u, 128u, 151u, 375u, 390u}) {
        auto b = pe(); b.resize(size); reject_parse(b, "truncated headers/table");
    }
    auto b = pe(); put(b, 0x3c, 0xfffffff0); reject_parse(b, "PE offset overflow");
    b = pe(); put(b, 0x94, 32, 2); reject_parse(b, "short declared optional header");
    b = pe(); put(b, kSection + 20, 0xfffffff0); reject_parse(b, "raw range overflow");
    b = pe(); put(b, kSection + 12, 0xfffffff0); reject_parse(b, "section RVA overflow");
    b = pe(); put(b, kSection + 8, 0xf00); reject_parse(b, "section exceeds image");
    b = pe(); put(b, kOptional + 28, 0xfffff800); reject_parse(b, "image VA overflow");
    b = with_tables(); directory(b, 5, 0xfffffff8, 16); reject_rebase(b, "directory wraps");
    b = with_tables(); directory(b, 5, 0x500, 7); reject_rebase(b, "partial block header");
    b = with_tables(); put(b, 0x504, 6); reject_rebase(b, "undersized block");
    b = with_tables(); put(b, 0x504, 14); reject_rebase(b, "block exceeds directory");
    b = with_tables(); put(b, 0x504, 11); reject_rebase(b, "partial entry");
    b = with_tables(); put(b, 0x508, 0x1280, 2); reject_rebase(b, "unsupported relocation");
    b = with_tables(); put(b, 0x500, 0xffffff00); reject_rebase(b, "target wraps");
    b = with_tables(); put(b, 0x508, 0x3ffe, 2); reject_rebase(b, "truncated target word");
    b = with_tables(); put(b, 0x320, 0xfffffffc); reject_parse(b, "export name table wraps");
    b = with_tables(); put(b, 0x3a0, 2, 2); reject_parse(b, "invalid export ordinal");
    b = with_tables(); put(b, 0x380, 0xffffff00); reject_parse(b, "export VA overflow");
    b = with_tables(); directory(b, 1, 0x400, 20); reject_imports(b, "missing import terminator");
    b = with_tables(); directory(b, 1, 0xfffffff0, 40); reject_imports(b, "import directory wraps");
    b = with_tables(); put(b, 0x400, 0xfffffffc); reject_imports(b, "lookup table wraps");
    b = with_tables(); put(b, 0x410, 0xfffffffc); reject_imports(b, "IAT VA wraps");
    b = with_tables(); put(b, 0x480, 0x7fffffff); reject_imports(b, "import name unmapped");
    std::cout << "malformed checks " << checks << '\n';
}
}
int main(int argc, char** argv) {
    try {
        const std::string mode = argc == 2 ? argv[1] : "--all";
        if (mode == "--valid" || mode == "--all") valid();
        if (mode == "--malformed" || mode == "--all") malformed();
        if (mode == "--p2") p2();
        require(mode == "--valid" || mode == "--malformed" || mode == "--p2" || mode == "--all",
                "usage: executable-image-test [--valid|--malformed|--p2|--all]");
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
