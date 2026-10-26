#include "loader/executable_image.hpp"
#include "runtime/native_display.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <string>

namespace {
// Link-time receipts for the lower callees, not alternate product implementations.
std::vector<sadx::object_draw::ModelId> model_calls;
bool fail_model = false, fail_late = false;
unsigned late_calls = 0;
} // namespace
namespace sadx::object_draw {
void draw_model(Context &, ModelId id) {
    model_calls.push_back(id);
    if (fail_model)
        throw std::runtime_error("model unavailable");
}
void draw_late_object(Context &, ObjectId, std::uint32_t) {
    ++late_calls;
    if (fail_late)
        throw std::runtime_error("late object unavailable");
}
} // namespace sadx::object_draw

namespace sadx::native {
struct DisplayObjectProbe {
    static void verify(Display &display, character_pack::ObjectState &objects,
                       character_pack::ResourceId id, std::uint8_t source_simple_flag) {
        assert(display.simple_model_enabled_ == source_simple_flag);
        auto &memory = display.texture_pool_[0];
        memory.global_index = 17;
        memory.references = 1;
        auto image = std::make_shared<graphics::Texture>();
        image->width = image->height = 1;
        image->bgra = {255, 255, 255, 255};
        memory.surface =
            std::make_shared<texture_lifetime::Surface>(texture_lifetime::Surface{image, {}});
        texture_lifetime::List textures{{&memory}, false, {}, {}};
        display.output().select_texture_list(textures);
        assert(display.queued_texture_list_ == &textures);
        display.frame_mode_.pending_logic_ticks = 0;
        display.fog_.special_flags = display.fog_.active = 0;
        display.queue_state_.values.lighting_palette = 2;

        constexpr std::uint32_t kUnitTransform = 7, kHidden = 8, kBreak = 16;
        auto &object = objects.object(id);
        object.evaluation_flags = kUnitTransform | kHidden | kBreak;
        object.child = std::numeric_limits<character_pack::ResourceId>::max();
        object.sibling = 0;
        display.draw_object_graph(id, objects); // hidden + Break never resolves the invalid child
        assert(model_calls.empty());
        object.evaluation_flags &= ~kHidden;
        const auto depth = display.matrices_.depth();
        display.draw_object_graph(id, objects);
        assert(model_calls == std::vector<object_draw::ModelId>{object.model});
        fail_model = true;
        bool model_failed = false;
        try {
            display.draw_object_graph(id, objects);
        } catch (const std::runtime_error &error) {
            model_failed = std::string(error.what()) == "model unavailable";
        }
        fail_model = false;
        assert(model_failed && model_calls.size() == 2);
        assert(display.matrices_.depth() == depth);

        display.queued_texture_list_ = nullptr;
        display.draw_object_graph(id, objects); // source not-ready route is intentionally undrawn
        assert(model_calls.size() == 2);
        display.output().select_texture_list(textures);
        display.frame_mode_.pending_logic_ticks = 1;
        display.draw_object_graph(std::numeric_limits<character_pack::ResourceId>::max(), objects);
        display.frame_mode_.pending_logic_ticks = 0;

        constexpr std::uint8_t kFogEnabled = 1, kFogEmulated = 2;
        constexpr float kLateDepth = -47952.0f;
        display.fog_.special_flags = kFogEnabled | kFogEmulated;
        display.queued_depth_ = 7;
        fail_late = true;
        bool late_called = false;
        try {
            display.draw_object_graph(id, objects);
        } catch (const std::runtime_error &error) {
            late_called = std::string(error.what()) == "late object unavailable";
        }
        assert(late_called && late_calls == 1 && display.fog_.special_flags == kFogEnabled);
        assert(display.queued_depth_ ==
               kLateDepth); // no fabricated rollback after the missing callee
    }
};
} // namespace sadx::native

int main(int argc, char **argv) {
    assert(argc == 3);
    auto pack = sadx::character_pack::CharacterPack::load(argv[1]);
    sadx::character_pack::ResourceId root = 0;
    for (sadx::character_pack::ResourceId id = 1; id <= pack.record_count(); ++id)
        if (pack.record(id).kind == sadx::character_pack::ResourceKind::Object &&
            pack.object(id).model != 0) {
            root = id;
            break;
        }
    assert(root);
    std::ifstream input(argv[2], std::ios::binary);
    assert(input);
    auto source = sadx::ExecutableImage::parse({std::istreambuf_iterator<char>(input), {}});
    assert(source);
    auto simple_flag = source->byte(0x90a0a0);
    assert(simple_flag);
    sadx::character_pack::ObjectState objects(pack);
    sadx::frame::ModeState timing{};
    sadx::native::Display display({}, timing);
    sadx::native::DisplayObjectProbe::verify(display, objects, root, *simple_flag);
    std::cout
        << "Object binding: shared graph, actual texture gate, live mutations, source initial "
           "byte and explicit model/late boundaries pass\n";
}
