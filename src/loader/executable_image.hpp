#pragma once
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace sadx {
// Immutable PE32 load-time bytes, including zero-filled section tails. Guest
// execution owns a separate mutable copy; nullopt means no mapped image byte.
class ExecutableImage {
  public:
    static std::optional<ExecutableImage> parse(std::vector<std::uint8_t> bytes,
                                                std::string *error = nullptr);
    std::uint32_t image_base() const noexcept { return base_; }
    std::optional<std::uint8_t> byte(std::uint32_t va) const noexcept;
    std::optional<std::uint32_t> u32(std::uint32_t va) const noexcept;

    // Exact, case-sensitive export name: either an image VA or a forwarder.
    struct Export {
        std::uint32_t va = 0;
        std::optional<std::string> forwarder;
    };
    std::optional<Export> export_named(const std::string &name) const;
    std::uint32_t size_of_image() const noexcept { return size_of_image_; }
    // AddressOfEntryPoint as a VA; zero means no entry point.
    std::uint32_t entry_point() const noexcept { return entry_rva_ ? base_ + entry_rva_ : 0; }

    struct ImportedSymbol {
        std::optional<std::string> name;
        std::optional<std::uint16_t> ordinal;
        std::uint16_t hint = 0;
        std::uint32_t iat_va = 0;
    };
    struct ImportedModule {
        std::string dll;
        std::vector<ImportedSymbol> symbols;
    };
    // OriginalFirstThunk (or FirstThunk when absent), in descriptor/thunk order.
    // iat_va is the mapped FirstThunk slot that the guest loader overwrites.
    // A malformed descriptor, thunk or string returns nullopt with an error.
    std::optional<std::vector<ImportedModule>> imports(std::string *error = nullptr) const;

    // Lay sections over SizeOfImage and apply HIGHLOW relocations. ABSOLUTE is
    // padding; other types fail explicitly. The returned flat snapshot retains
    // the existing one-shot behavior: its relocation directory is not reapplied.
    std::optional<ExecutableImage> rebased(std::uint32_t base, std::string *error = nullptr) const;

  private:
    struct Section {
        std::uint32_t rva;
        std::uint32_t virtual_size;
        std::uint32_t raw_size;
        std::uint32_t raw_offset;
    };
    // NOTSA: PE boundary helpers use wide offsets so malformed input cannot wrap
    // into a different, valid part of the image before a bounds check.
    std::optional<std::uint32_t> read_rva(std::uint64_t rva, unsigned width) const noexcept;
    std::optional<std::string> string_rva(std::uint64_t rva) const;
    bool read_sections(std::uint64_t table, std::uint32_t count, std::uint32_t headers,
                       std::string *error);
    bool read_exports(std::uint32_t rva, std::uint32_t size, std::string *error);
    bool apply_relocations(std::vector<std::uint8_t> &mapped, std::uint32_t delta,
                           std::string *error) const;

    std::vector<std::uint8_t> bytes_;
    std::uint32_t base_ = 0;
    std::vector<Section> sections_;
    std::map<std::string, Export> exports_;
    std::uint32_t size_of_image_ = 0;
    std::uint32_t relocation_rva_ = 0;
    std::uint32_t relocation_size_ = 0;
    std::uint32_t import_rva_ = 0;
    std::uint32_t import_size_ = 0;
    std::uint32_t entry_rva_ = 0;
};
} // namespace sadx
