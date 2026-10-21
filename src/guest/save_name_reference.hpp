#pragma once
#include "game/save_file_name.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>
namespace sadx {
// Reference-only transport: a nonnull source word is an opaque identity, NOT
// observed string bytes. Never dereference the published marker as text. A
// source leaf can clear it; an unchanged word must round-trip exactly, even if
// its numeric value is not a mapped guest address. Unknown observations get a
// separate marker so a later null store becomes known0 rather than unknown.
class SaveNameReference {
  public:
    void import_value(save::SaveNameState &state, std::optional<std::uint32_t> source) {
        if (source && *source == 0) {
            state.value = nullptr;
            return;
        }
        for (const auto &entry : observations_)
            if (entry.source == source) {
                state.value = marker(entry.identity);
                return;
            }
        auto identity = std::make_shared<Identity>();
        state.value = marker(identity);
        observations_.push_back({source, std::move(identity)});
    }
    std::optional<std::uint32_t> export_value(const save::SaveNameState &state) const {
        if (state.value == nullptr)
            return 0;
        for (const auto &entry : observations_)
            if (state.value == marker(entry.identity))
                return entry.source;
        // A newly published native resource has no observed guest identity yet.
        // Its bytes are not guessed, truncated, or reinterpreted as an address.
        return std::nullopt;
    }

  private:
    struct Identity {};
    struct Observation {
        std::optional<std::uint32_t> source;
        std::shared_ptr<Identity> identity;
    };
    // Only reference transport uses a type-erased identity as the source pointer.
    static const char *marker(const std::shared_ptr<Identity> &identity) {
        return reinterpret_cast<const char *>(identity.get());
    }
    // Shared resources keep copied-state snapshots' pointer identities alive.
    std::vector<Observation> observations_;
};
} // namespace sadx
