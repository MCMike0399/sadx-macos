#pragma once
#include "game/saved_fov.hpp"
#include <optional>
namespace sadx::reference {
// Reference transport owns availability only, never a second source word.
// Unknown import leaves native bits untouched; export remains unknown until an
// observed import or actual source store publishes a known word.
class SavedFovReference {
  public:
    void import_value(view::SavedFovState &state, std::optional<std::uint32_t> source) {
        known_ = source.has_value();
        if (source)
            view::set_saved_fov_bits(state, *source);
    }
    std::optional<std::uint32_t> export_value(const view::SavedFovState &state) const {
        if (!known_)
            return std::nullopt;
        return view::saved_fov_bits(state);
    }
    void observe_write() noexcept { known_ = true; }

  private:
    bool known_ = false;
};
} // namespace sadx::reference
