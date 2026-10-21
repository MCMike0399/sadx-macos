#pragma once
#include "engine/native_texture_lifetime.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace sadx::regular_textures {
using List = texture_lifetime::List;
using Filename = texture_lifetime::Filename;
struct Binding {
    std::optional<std::string> archive;
    List *textures;
};
struct Group {
    Binding common;
    std::vector<Binding> regular;
};
using Catalog = std::array<const Group *, 3>;
struct Localization {
    std::int32_t language;
    std::string source;
    Filename replacement;
};
using Localizations = std::array<Localization, 1>;
// NOTSA: native ownership of parsed replacement-index resources. Raw PVM loads
// populate neither cache; their lifetimes still follow the pinned loader clear.
struct CacheState {
    bool was_loading = false;
    std::vector<std::shared_ptr<const void>> directories, archives;
};
struct State {
    std::uint8_t &resync;
    const std::int32_t &language;
    List *&common;
    std::array<List *, 32> &regular;
    const Localizations &localizations;
    // The source's shared 300-entry TexNameBuffer starts in zero-filled storage.
    // Its attributes survive ordinary PVM loads; that path replaces only handles.
    std::array<std::uint32_t, 300> scratch_attributes{};
    CacheState cache{};
};
struct ArchiveResult {
    std::int32_t status;
    std::vector<texture_lifetime::Memory *> entries;
};
struct Services {
    std::function<ArchiveResult(std::string_view)> load_archive;
    std::function<std::int32_t(List &)> load_individual;
    std::function<void(std::string_view)> diagnostic;
};
enum class Boundary { GroupIndex, RegularCapacity, Filename, FilenameStorage, ScratchCapacity };
class Error : public std::runtime_error {
  public:
    Error(Boundary boundary, std::string text)
        : std::runtime_error(std::move(text)), boundary_(boundary) {}
    Boundary boundary() const { return boundary_; }
  private:
    Boundary boundary_;
};
// FUNCTION: LOADER 0x100a75e0 check_cache
void check_cache(State &);
// Pinned loader hook replaces the original named-archive loading wrapper.
// FUNCTION: LOADER 0x100a9dc0 LoadPVM_r
void load_archive(std::string_view name, List *, State &, Services &);
// Pinned loader hook replaces the original individual-texture loading wrapper.
// FUNCTION: LOADER 0x100ab2d0 njLoadTexture_Wrapper_r
std::int32_t load_individual(List &, State &, Services &);
// FUNCTION: SONIC 0x420d80 LoadNoNamePVM
void load_no_name(List &, State &, Services &);
// FUNCTION: SONIC 0x4212e0 LoadRegObjTextures
void load_regular(std::uint32_t index, const Catalog &, State &, Services &);
} // namespace sadx::regular_textures
