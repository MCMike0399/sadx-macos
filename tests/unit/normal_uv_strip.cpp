#include "engine/normal_uv_strip.hpp"
#include "normal_uv_strip_probe.hpp"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {
using Input = std::array<std::uint32_t, kStripProbeInputWords>;
using Row = std::array<std::uint32_t, 6>;
struct Result {
    std::vector<std::uint32_t> words;
    std::vector<Row> trace;
    explicit Result(const Input &v) : words(kStripProbeOutputWords) {
        const auto n = sadx_native_normal_uv_strip_probe(v.data(), words.data());
        words.resize(n);
        for (unsigned row = 0; row != words[11]; ++row) {
            Row receipt{};
            for (unsigned field = 0; field != 6; ++field)
                receipt[field] = words[12 + row * 6 + field];
            trace.push_back(receipt);
        }
    }
    std::vector<std::uint32_t> arguments() const {
        auto cursor = 12 + words[11] * 6;
        cursor += words[cursor] * 2 + 1;
        const auto size = words[cursor++];
        return {words.begin() + cursor, words.begin() + cursor + size};
    }
    std::vector<Row> of(unsigned kind, unsigned field = ~0U) const {
        std::vector<Row> selected;
        for (auto row : trace)
            if (row[0] == kind && (field == ~0U || row[1] == field)) selected.push_back(row);
        return selected;
    }
};
Input input(unsigned mode = 0) {
    Input v{};
    v[0] = mode; v[1] = 1; v[2] = 36; v[3] = 3600; v[4] = 3600;
    v[6] = 0x152; v[7] = 3; v[8] = 0xb1c2d3e4; v[9] = 0x7f;
    v[11] = 1000; v[12] = 4; v[13] = 6; v[15] = 12; v[16] = 12;
    v[17] = 512; v[18] = 4; v[19] = 9;
    v[kMeshInput] = 3; v[kMeshInput+1] = 0; v[kMeshInput+2] = 1; v[kMeshInput+3] = 2;
    for (unsigned n = 0; n != 12; ++n) {
        v[kPointsInput+n] = 0x3f000000 + n;
        v[kNormalsInput+n] = 0xbf000000 + n;
    }
    v[kUvInput] = 255; v[kUvInput+1] = 0xffff;
    v[kUvInput+2] = 0x8000; v[kUvInput+3] = 0x7fff;
    for (unsigned n = 0; n != 512; ++n) v[kArgumentsInput+n] = 0x6a000000 + n;
    return v;
}
}
int main() {
    using namespace sadx::polybuff;
    unsigned checks = 0;
    {
        auto v = input(); Result r(v); auto bytes = r.of(3); auto draw = r.of(4, 6);
        assert(bytes.size() == 45 && draw.size() == 1);
        assert(draw[0][2] == kTriangleStripPrimitive && draw[0][4] == 3);
        assert(bytes[0][2] == bytes[9][2]);
        assert(bytes[27][2] == bytes[36][2]);
        assert(bytes[7][2] == 0x3f800000); // 255 * source m32 constant rounds to 1.
        assert((bytes[8][2] & 0x80000000) != 0);
        assert(r.words[2] == 3420 && r.words[6] == 0);
        ++checks;
        // Winding/header bits are ignored, while UV signs are retained.
        for (auto flags : {0x4000U, 0x8000U, 0xc000U}) {
            auto other = v; other[kMeshInput] |= flags; Result q(other);
            assert(q.words == r.words); ++checks;
        }
    }
    {
        auto v = input(1); v[4] = 144; Result equal(v);
        assert(equal.of(4, 3)[0][4] == kDiscard);
        v[4] = 145; Result extra(v);
        assert(extra.of(4, 3)[0][4] == kNoOverwrite);
        auto stores = extra.of(2);
        assert(stores[0][1] == 1 && stores[1][1] == 0 && stores[2][1] == 2);
        ++checks;
    }
    {
        auto v = input(1); v[18] = 0; Result r(v);
        assert(r.of(2)[0][2] == 0xfffffffeU);
        v[2] = 0; Result divide(v);
        assert(divide.of(1).size() == 1 && divide.of(2).empty());
        assert(divide.of(5)[0][1] == 7); ++checks;
    }
    {
        auto v = input(); v[10] = 1; Result r(v);
        assert(r.of(3).empty() && r.words[6] == 1);
        assert(r.of(5)[0][1] == 5 && r.of(5)[0][2] == 36);
        v[10] = 2; Result ignored_hresult(v);
        assert(ignored_hresult.of(3).size() == 45); ++checks;
    }
    {
        auto v = input(); v[16] = 1; Result r(v);
        assert(r.of(3).size() == 4); // All XYZ and normal X precede missing normal Y.
        assert(r.of(5)[0][1] == static_cast<unsigned>(DataKind::Normals));
        v = input(); v[11] = 8; Result output_fault(v);
        assert(output_fault.of(3).size() == 8 && output_fault.words[6] == 1); ++checks;
    }
    {
        auto v = input(); v[kMeshInput+1] = 0xffff; Result r(v);
        assert(r.of(5)[0][1] == static_cast<unsigned>(DataKind::Points));
        assert(r.of(5)[0][2] == 0xfffffffdU); ++checks;
    }
    {
        auto v = input(3); v[18] = 2; Result r(v);
        auto stores = r.of(2);
        assert(stores.size() == 4 && stores[2][1] == 4 && stores[3][1] == 5);
        assert(r.words[1] == 72 && r.words[2] == 72 && r.words[4] == 0x152);
        const auto arguments = r.arguments();
        assert(arguments[0] == 0 && arguments[1] == 0);
        assert(arguments[2] == 0x6a000002 && arguments[3] == 0x6a000003);
        assert(arguments[6] == 0x6a000006 && arguments[7] == 0x6a000007);
        assert(arguments[8] == 0x6a000008);
        v[10] = 8; Result failed(v);
        assert(failed.words[5] == 0 && failed.words[6] == 0);
        assert(failed.of(5)[0][1] == static_cast<unsigned>(DataKind::RenderArguments)); ++checks;
    }
    {
        auto v = input(2); v[5] = 2; Result r(v);
        assert(r.of(4, 5).size() == 2 && r.of(4, 6).size() == 2 && r.words[6] == 0);
        v[20] = 1; Result stopped(v);
        assert(stopped.words[6] == 2 && stopped.of(1).empty()); ++checks;
    }
    {
        auto v = input(5); Result r(v);
        assert(r.words[2] == 3564 && r.of(4, 3)[0][3] == 36);
        assert(r.of(4, 4).size() == 1);
        v[10] = 16; Result no_resource(v); assert(no_resource.trace.empty()); ++checks;
    }
    std::cout << "normal_uv_strip: " << checks << " checks passed\n";
}
