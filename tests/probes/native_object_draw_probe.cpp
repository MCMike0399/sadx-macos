#include "engine/object_draw.hpp"
#include "engine/ninja_sprite_queue.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>
namespace d = sadx::object_draw;
namespace {
float number(std::uint32_t word) { float value; std::memcpy(&value, &word, 4); return value; }
std::uint32_t bits(float value) { std::uint32_t word; std::memcpy(&word, &value, 4); return word; }
constexpr std::uint32_t model_words[]{0, 1, 1048575, 1048576, 285237248, 4294967295U, 1048574, 17661820};
std::uint32_t model_id(std::uint32_t code) { return model_words[code] >= 1048576 ? code : 0; }
struct Calls final : d::Calls {
    const std::uint32_t *input;
    std::uint32_t *out;
    std::array<d::Object, 5> nodes{};
    std::uint8_t fog, active, simple;
    float depth;
    std::int32_t pending, palette;
    unsigned gate, current = 0, count = 0;
    bool push_changed = false, model_changed = false;
    Calls(const std::uint32_t *i, std::uint32_t *o)
        : input(i), out(o), fog(i[2]), active(i[3]), simple(i[6]), depth(number(i[5])),
          pending(i[4]), palette(i[7]), gate(i[8]) {
        for (unsigned n = 0; n < nodes.size(); ++n) {
            const auto *v = i + 16 + n * 13;
            nodes[n] = {v[0], model_id(v[1]), {number(v[2]), number(v[3]), number(v[4])},
                        {static_cast<std::int32_t>(v[5]), static_cast<std::int32_t>(v[6]), static_cast<std::int32_t>(v[7])},
                        {number(v[8]), number(v[9]), number(v[10])}, v[11], v[12]};
        }
    }
    d::State state() { return {fog, active, depth, pending, simple, palette}; }
    void receipt(std::initializer_list<std::uint32_t> row) {
        if (count >= 247) throw std::runtime_error("probe trace bound");
        std::copy(row.begin(), row.end(), out + 8 + count++ * 8); out[0] = count;
    }
    const d::Object &object(d::ObjectId id) override { current = id; return nodes.at(id - 1); }
    void push_matrix_ex() override {
        receipt({3});
        if (!push_changed) {
            auto &node = nodes.at(current - 1);
            if (input[10] & 1U) node.evaluation_flags ^= 16U;
            if (input[10] & 2U) node.model = 4;
            push_changed = true;
        }
    }
    void translate_current(const std::array<float, 3> &p) override { receipt({4, bits(p[0]), bits(p[1]), bits(p[2])}); }
    void rotate_current(d::RotationOrder order, const std::array<std::int32_t, 3> &a) override {
        receipt({order == d::RotationOrder::Default ? 5U : 6U, static_cast<std::uint32_t>(a[0]), static_cast<std::uint32_t>(a[1]), static_cast<std::uint32_t>(a[2])});
    }
    void scale_current(const std::array<float, 3> &s) override { receipt({7, bits(s[0]), bits(s[1]), bits(s[2])}); }
    void pop_matrix_ex() override { receipt({8}); }
    void draw_model(d::ModelId model) override {
        receipt({9, model});
        if (!model_changed && current != 0) {
            auto &node = nodes.at(current - 1);
            if (input[10] & 4U) node.sibling = current < 5 ? 5 : 0;
            if (input[10] & 8U) node.child = current < 5 ? 5 : 0;
            model_changed = true;
        }
    }
    void draw_late_object(d::ObjectId object, std::uint32_t flags) override {
        receipt({1, object, flags, fog, bits(depth)});
        if (input[9] & 1U) fog = static_cast<std::uint8_t>(input[12]);
        if (input[9] & 2U) pending = static_cast<std::int32_t>(input[13]);
        if (input[9] & 4U) gate = input[14];
        if (input[9] & 8U) depth = number(input[15]);
    }
    bool texture_list_not_ready() override {
        receipt({2, gate});
        std::optional<std::shared_ptr<sadx::NinjaTextureListOwner>> list{std::shared_ptr<sadx::NinjaTextureListOwner>{}};
        if (gate != 1) {
            auto owner = std::make_shared<sadx::NinjaTextureListOwner>();
            owner->count = gate == 2 ? 0 : gate >= 5 ? 2 : 1;
            auto entries = std::make_shared<sadx::NinjaTextureListOwner::Entries>();
            for (unsigned n = 0; n < *owner->count; ++n) {
                std::shared_ptr<const sadx::NinjaTextureMemoryReference> memory;
                if (!(gate == 3 || (gate == 5 && n == 1)))
                    memory = std::make_shared<sadx::NinjaTextureMemoryReference>();
                entries->push_back(memory);
            }
            owner->entries = entries;
            owner->first_filename_is_invalid_sentinel = gate == 4;
            list = owner;
        }
        const auto result = sadx::ninja_texture_list_gate(list);
        if (!result || !result.source_not_ready)
            throw std::runtime_error("explicit unknown texture gate boundary");
        return *result.source_not_ready;
    }
    void finish() {
        out[1] = fog; out[2] = bits(depth); out[3] = static_cast<std::uint32_t>(pending); out[4] = gate;
        for (unsigned n = 0; n < nodes.size(); ++n) {
            out[1984 + n * 4] = nodes[n].evaluation_flags;
            out[1985 + n * 4] = nodes[n].model;
            out[1986 + n * 4] = nodes[n].child;
            out[1987 + n * 4] = nodes[n].sibling;
        }
    }
};
}
extern "C" void sadx_native_object_draw_probe(const std::uint32_t *input, std::uint32_t *out) {
    std::fill_n(out, 2048, 0); Calls calls(input, out);
    try {
        if (input[0] == 0) d::draw(input[1], calls.state(), calls);
        else if (input[0] == 1) d::draw_objects(input[1], calls.state(), calls);
        else out[5] = d::draw_simple_model(model_id(input[1]), calls);
    } catch (const std::exception &) { out[6] = 1; }
    calls.finish();
}
