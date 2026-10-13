#pragma once
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace sadx {
// Load-time facts of a PE32 image: bytes at a virtual address as the Windows
// loader maps them (file bytes, zeros past a section's raw data within its
// virtual size). Writes made before the game loop (WinMain, mod Init) are not
// modelled here; nullopt means "not part of the image".
class ExecutableImage {
public:
    static std::optional<ExecutableImage> parse(std::vector<std::uint8_t> bytes,std::string* error=nullptr);
    std::uint32_t image_base()const noexcept{return base_;}
    std::optional<std::uint8_t> byte(std::uint32_t va)const noexcept;
    std::optional<std::uint32_t> u32(std::uint32_t va)const noexcept;
    // Export directory entry by exact (case-sensitive) name, as GetProcAddress
    // looks it up: the image VA, or the forwarder string it names instead.
    struct Export {std::uint32_t va=0;std::optional<std::string> forwarder;};
    std::optional<Export> export_named(const std::string& name)const;
    std::uint32_t size_of_image()const noexcept{return size_of_image_;}
    // The image as the loader maps it at `base`: every section laid out over
    // SizeOfImage and the base relocations applied (HIGHLOW; ABSOLUTE is
    // padding). Other relocation types are rejected rather than guessed.
    std::optional<ExecutableImage> rebased(std::uint32_t base,std::string* error=nullptr)const;
private:
    struct Section{std::uint32_t rva,virtual_size,raw_size,raw_offset;};
    std::vector<std::uint8_t> bytes_;std::uint32_t base_=0;std::vector<Section> sections_;std::map<std::string,Export> exports_;std::uint32_t size_of_image_=0,relocation_rva_=0,relocation_size_=0;
};
} // namespace sadx
