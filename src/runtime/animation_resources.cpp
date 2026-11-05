#include "runtime/animation_resources.hpp"
#include <stdexcept>

namespace sadx::native {
namespace {
// NOTSA: SDXACT03 words are little endian; no source pointers enter the runtime.
std::uint32_t word(const std::vector<std::uint8_t> &bytes, std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < 4)
        throw std::runtime_error("truncated animation resource");
    const auto *p = bytes.data() + offset;
    return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) | (std::uint32_t(p[2]) << 16) |
           (std::uint32_t(p[3]) << 24);
}
} // namespace

void import_animation_action(const character_pack::CharacterPack &pack, ActionHandle handle,
                             animation::Resources &resources) {
    const auto action = pack.resolve(handle);
    resources.actions.emplace(handle.identity,
                              animation::Action{{action.object}, {{action.motion}}});
    if (resources.motions.count(action.motion))
        return;
    const auto motion = pack.motion(action.motion);
    resources.motions.emplace(action.motion, animation::Motion{motion.frame_count,
                                                               motion.channels,
                                                               motion.interpolation,
                                                               {{motion.tracks}}});
    const auto &tracks = pack.record(motion.tracks).payload;
    const unsigned channels = (motion.interpolation & 15) == 3 ? 3 : 2;
    std::vector<animation::Node> nodes;
    for (std::uint32_t index = 0; index < word(tracks, 0); ++index) {
        animation::Node node{};
        for (unsigned channel = 0; channel < channels; ++channel) {
            const auto key = word(tracks, 4 + index * channels * 8 + channel * 4);
            node.counts[channel] = word(tracks, 4 + index * channels * 8 + channels * 4 + channel * 4);
            if (!key)
                continue;
            node.keys[channel].id = key;
            if (resources.key_arrays.count(key))
                continue;
            auto backing =
                std::make_shared<const std::vector<std::uint8_t>>(pack.record(key).payload);
            // Pack arrays have a four-byte count followed by unchanged 16-byte source keys.
            resources.key_arrays.emplace(key, animation::KeyBytes{backing, 4, backing->size() - 4});
        }
        nodes.push_back(std::move(node));
    }
    resources.node_arrays.emplace(motion.tracks, std::move(nodes));
}
} // namespace sadx::native
