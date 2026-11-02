// Development-only raw fixture adapter. Native game callers borrow existing owners.
#include "player/model_motion_matrices.hpp"
#include "engine/ninja_matrix_composition.hpp"
#include <algorithm>
#include <cstring>

namespace {
constexpr unsigned kNodes = 8, kStack = 8;
struct Case {
    std::uint32_t control[16];
    sadx::NinjaMatrix matrices[128], stack[kStack], base, destination;
    std::uint32_t objects[kNodes][13], present[kNodes][3], keys[kNodes][3][4][4];
};
struct Source final : sadx::model_matrices::MotionSource {
    const Case &fixture;
    std::array<sadx::character_pack::ObjectResource, kNodes> objects{};
    explicit Source(const Case &fixture) : fixture(fixture) {
        for (unsigned n = 0; n < kNodes; ++n) {
            const auto *raw = fixture.objects[n];
            auto &o = objects[n];
            o.evaluation_flags = raw[0];
            std::memcpy(o.position.data(), raw + 2, 12);
            std::memcpy(o.rotation.data(), raw + 5, 12);
            std::memcpy(o.scale.data(), raw + 8, 12);
            o.child = raw[11];
            o.sibling = raw[12];
        }
    }
    std::uint32_t frame_count() override { return fixture.control[5]; }
    std::uint16_t type() override { return fixture.control[4]; }
    sadx::model_matrices::ObjectId root() override { return fixture.control[6]; }
    const sadx::character_pack::ObjectResource &object(sadx::model_matrices::ObjectId id) override {
        if (id == 0 || id > kNodes)
            throw sadx::model_matrices::Error(sadx::model_matrices::Failure::MissingResource);
        return objects.at(id - 1);
    }
    bool has_keys(unsigned node, sadx::model_matrices::Channel c) override {
        if (node >= kNodes)
            throw sadx::model_matrices::Error(sadx::model_matrices::Failure::MissingKeys);
        return fixture.present[node][unsigned(c)] != 0;
    }
    std::uint32_t key_word(unsigned node, sadx::model_matrices::Channel c,
                           unsigned key, unsigned word) override {
        if (node >= kNodes || key >= 4 || word >= 4)
            throw sadx::model_matrices::Error(sadx::model_matrices::Failure::MissingKeys);
        return fixture.keys[node][unsigned(c)][key][word];
    }
};
}
extern "C" unsigned sadx_model_matrix_case_words() { return sizeof(Case) / 4; }
extern "C" unsigned sadx_model_matrix_output_words() { return 6 + (128 + kStack + 2) * 16; }
extern "C" int sadx_native_model_matrices(const void *raw, const void *table_bytes,
                                         const void *unit_bytes, std::uint32_t *output) {
    const auto &fixture = *static_cast<const Case *>(raw);
    sadx::model_matrices::State state;
    state.count = fixture.control[11];
    state.limit = fixture.control[12];
    std::copy_n(fixture.matrices, 128, state.matrices.begin());
    std::vector<sadx::NinjaMatrix> frames(fixture.stack, fixture.stack + kStack);
    frames.resize(fixture.control[10]);
    sadx::NinjaMatrixStack stack(std::move(frames), fixture.control[9], 0);
    sadx::NinjaMath::SineTable table;
    sadx::NinjaMatrix unit, destination = fixture.destination, base = fixture.base;
    std::memcpy(table.data(), table_bytes, sizeof(table));
    std::memcpy(unit.data(), unit_bytes, sizeof(unit));
    sadx::NinjaMatrixPrimitives coordinates(table, unit, stack);
    sadx::X87Context fp(fixture.control[1]);
    Source source(fixture);
    unsigned result = 0, failure = 0;
    float frame;
    std::memcpy(&frame, fixture.control + 3, 4);
    try {
        switch (fixture.control[0]) {
        case 0:
            result = sadx::model_matrices::get(state, fixture.control[2], destination);
            break;
        case 1:
            result = sadx::model_matrices::calculate(state, coordinates, source,
                fixture.control[7] ? &base : nullptr, frame, fixture.control[2],
                fixture.control[8] ? &destination : nullptr, fp);
            break;
        case 2:
            sadx::multiply_ninja_matrix(destination, fixture.control[8] ? destination : base, fp);
            result = 1;
            break;
        case 3: {
            std::array<std::int32_t, 3> angles;
            std::memcpy(angles.data(), fixture.control + 13, 12);
            sadx::rotate_ninja_xyz(coordinates, destination, angles, fp);
            result = 1;
            break;
        }
        default: return 2;
        }
    } catch (const sadx::model_matrices::Error &) {
        failure = 1;
    } catch (const std::out_of_range &) {
        failure = 1;
    }
    output[0] = result;
    output[1] = state.count;
    output[2] = state.limit;
    output[3] = stack.depth();
    output[4] = fp.exception_flags();
    output[5] = failure;
    std::memcpy(output + 6, state.matrices.data(), sizeof(fixture.matrices));
    std::memcpy(output + 6 + 128 * 16, fixture.stack, sizeof(fixture.stack));
    std::memcpy(output + 6 + 128 * 16, stack.slots().data(), stack.slots().size() * 64);
    std::memcpy(output + 6 + (128 + kStack) * 16, base.data(), 64);
    std::memcpy(output + 6 + (129 + kStack) * 16, destination.data(), 64);
    return 0;
}
