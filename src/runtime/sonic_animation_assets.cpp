#include "runtime/sonic_animation_assets.hpp"
#include <stdexcept>

namespace sadx::native {
namespace {
// NOTSA: SDXACT03 words are little endian; no source pointers enter the runtime.
std::uint32_t word(const std::vector<std::uint8_t> &bytes, std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < 4)
        throw std::runtime_error("truncated Sonic animation resource");
    const auto *p = bytes.data() + offset;
    return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) | (std::uint32_t(p[2]) << 16) |
           (std::uint32_t(p[3]) << 24);
}
} // namespace

SonicAnimationAssets::SonicAnimationAssets(const character_pack::CharacterPack &pack,
                                         Declarations declarations)
    : declarations_(std::move(declarations)) {
    animation::SonicExports exports;
    for (std::size_t index = 0; index < declarations_.size(); ++index) {
        const auto &declaration = declarations_[index];
        const auto &action = declaration.action;
        if (action.status == animation::ResourceStatus::KnownNull) {
            if (declaration.identity || action.identity || !action.unavailable_reason.empty())
                throw std::runtime_error("invalid Sonic known-null declaration");
        } else {
            if (!declaration.identity || declaration.identity > index + 1)
                throw std::runtime_error("invalid Sonic declaration alias");
            const auto &first = declarations_[declaration.identity - 1];
            if (first.identity != declaration.identity || first.action.status != action.status ||
                first.action.identity != action.identity ||
                first.action.unavailable_reason != action.unavailable_reason)
                throw std::runtime_error("inconsistent Sonic declaration alias");
            if (action.status == animation::ResourceStatus::Loaded) {
                if (!action.identity || !action.unavailable_reason.empty())
                    throw std::runtime_error("invalid loaded Sonic declaration");
                import_action(pack, ActionHandle{action.identity});
            } else if (action.status != animation::ResourceStatus::Unavailable || action.identity ||
                       action.unavailable_reason.empty()) {
                throw std::runtime_error("invalid unavailable Sonic declaration");
            }
        }
        exports.push_back(action);
    }
    auto table = std::make_shared<animation::Table>(animation::initial_sonic_table());
    const auto initialized = animation::initialize_sonic_table(*table, exports);
    if (initialized.failure != animation::TableImportFailure::None ||
        initialized.completed_records != 147)
        throw std::runtime_error("incomplete Sonic animation table");
    table_ = std::move(table);
    using Selection = character_pack::SelectionAction;
    preview_ = {pack.action(Selection::SonicIdle), pack.action(Selection::SonicSelected),
                pack.action(Selection::SonicSettled)};
    for (const auto action : preview_)
        import_action(pack, action);
}

void SonicAnimationAssets::import_action(const character_pack::CharacterPack &pack,
                                        ActionHandle handle) {
    const auto action = pack.resolve(handle);
    resources_.actions.emplace(handle.identity,
                               animation::Action{{action.object}, {{action.motion}}});
    if (resources_.motions.count(action.motion))
        return;
    const auto motion = pack.motion(action.motion);
    resources_.motions.emplace(action.motion,
        animation::Motion{motion.frame_count, motion.channels, motion.interpolation, {{motion.tracks}}});
    const auto &tracks = pack.record(motion.tracks).payload;
    std::vector<animation::Node> nodes;
    for (std::uint32_t index = 0; index < word(tracks, 0); ++index) {
        animation::Node node{};
        for (unsigned channel = 0; channel < 2; ++channel) {
            const auto key = word(tracks, 4 + index * 16 + channel * 4);
            node.counts[channel] = word(tracks, 12 + index * 16 + channel * 4);
            if (!key)
                continue;
            node.keys[channel].id = key;
            if (resources_.key_arrays.count(key))
                continue;
            auto backing = std::make_shared<const std::vector<std::uint8_t>>(pack.record(key).payload);
            // Pack arrays have a four-byte count followed by unchanged 16-byte source keys.
            resources_.key_arrays.emplace(key, animation::KeyBytes{backing, 4, backing->size() - 4});
        }
        nodes.push_back(std::move(node));
    }
    resources_.node_arrays.emplace(motion.tracks, std::move(nodes));
}
} // namespace sadx::native
