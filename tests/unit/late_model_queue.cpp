#include "engine/late_model_queue.hpp"

#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void require(bool value, const std::string &message) {
    if (!value)
        throw std::runtime_error(message);
}
float bits(std::uint32_t word) {
    float value;
    std::memcpy(&value, &word, sizeof(value));
    return value;
}
sadx::NinjaSpriteQueueRuntime runtime(sadx::NinjaRenderState &render, std::uint32_t mode) {
    sadx::NinjaSpriteQueueRuntime facts;
    facts.render_state = &render;
    facts.mode_at_3ab98ac = mode;
    facts.flag_at_3abdcfe = 3;
    facts.captured_word_at_3abd950 = 1;
    facts.captured_word_at_3d0f9c8 = 11;
    facts.captured_word_at_3d0f840 = 22;
    facts.captured_word_at_3d0f9c4 = 33;
    facts.color_at_3abd9a8 = std::array<float, 4>{1, 2, 3, 4};
    facts.color_at_3ab9864 = std::array<float, 4>{5, 6, 7, 8};
    facts.flag_at_3b12208 = 0;
    facts.sort_bias_at_3abd9c0 = -27952.0f;
    facts.bucket_bias_at_7dcb60 = 2048.0f;
    facts.captured_texture_list_at_3abd950 = std::make_shared<sadx::NinjaTextureListOwner>();
    return facts;
}
sadx::LateModelResult draw(std::uint8_t flags, float translation_z, float limit, std::uint32_t mode) {
    auto arena = std::make_shared<sadx::NinjaRenderQueueArena>(std::vector<std::uint8_t>(65536), 0, 65536, 1);
    sadx::NinjaSpriteQueue queue(arena);
    queue.initialize_sort_buckets();
    sadx::NinjaRenderState render;
    render.packed_blend_at_3ab9890 = 0;
    auto facts = runtime(render, mode);
    sadx::NinjaMatrix matrix{};
    matrix[14] = translation_z;
    sadx::X87Context arithmetic(0x007f);
    return sadx::late_draw_model(queue, matrix, 0x8bf374u, flags, limit, [] {}, facts, arithmetic);
}
} // namespace

int main() {
    {
        const auto queued = draw(6, 10.0f, -3000.0f, 0);
        require(static_cast<bool>(queued), "a ready queue accepts the node");
        require(queued.route == sadx::LateModelRoute::queued, "flag 6 always queues");
        require(queued.depth == -10.0f, "depth is the negated translation");
        require(queued.record && queued.record->draw_model, "the mesh owner stays on the node");
    }
    {
        const auto immediate = draw(1, 10.0f, 0.0f, 0);
        require(immediate.route == sadx::LateModelRoute::immediate, "a near node with the low flag draws now");
        require(immediate.record && !immediate.bucket, "the immediate node is not sorted");
    }
    {
        const auto skipped = draw(6, 10.0f, -3000.0f, 1);
        require(skipped.route == sadx::LateModelRoute::immediate && !skipped.record,
                "a closed queue takes the immediate path");
    }
    require(bits(0xc6da6000u) == -27952.0f, "the shadow sort bias is the image constant");
    std::cout << "late model queue ok\n";
    return 0;
}
