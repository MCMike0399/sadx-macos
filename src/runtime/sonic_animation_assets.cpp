#include "runtime/sonic_animation_assets.hpp"
#include "runtime/animation_resources.hpp"
#include <stdexcept>

namespace sadx::native {
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
                import_animation_action(pack, ActionHandle{action.identity}, resources_);
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
        import_animation_action(pack, action, resources_);
}

} // namespace sadx::native
