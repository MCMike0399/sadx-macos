#include "loader/executable_image.hpp"
#include <algorithm>

namespace sadx {
namespace {
constexpr std::uint64_t kAddressSpaceSize = std::uint64_t{1} << 32;
constexpr unsigned kStringLimit = 4096;

// NOTSA: bounds checks precede every file/flat-image little-endian read.
bool contains(std::uint64_t size, std::uint64_t at, std::uint64_t width) {
    return at <= size && width <= size - at;
}
std::optional<std::uint32_t> read_le(const std::vector<std::uint8_t> &bytes, std::uint64_t at,
                                     unsigned width) {
    if (!contains(bytes.size(), at, width)) {
        return std::nullopt;
    }
    std::uint32_t value = 0;
    for (unsigned i = 0; i < width; ++i) {
        value |= std::uint32_t(bytes[at + i]) << (8 * i);
    }
    return value;
}
std::nullopt_t malformed(std::string *error, const char *why) {
    if (error) {
        *error = why;
    }
    return std::nullopt;
}
bool reject(std::string *error, const char *why) {
    malformed(error, why);
    return false;
}
} // namespace

std::optional<ExecutableImage> ExecutableImage::parse(std::vector<std::uint8_t> bytes,
                                                      std::string *error) {
    const auto mz = read_le(bytes, 0, 2);
    const auto pe_offset = read_le(bytes, 0x3c, 4);
    if (mz != 0x5a4du || !pe_offset) {
        return malformed(error, "not an MZ image");
    }
    const std::uint64_t pe = *pe_offset;
    if (read_le(bytes, pe, 4) != 0x4550u) {
        return malformed(error, "missing PE signature");
    }
    const auto machine = read_le(bytes, pe + 4, 2);
    const auto count = read_le(bytes, pe + 6, 2);
    const auto optional_size = read_le(bytes, pe + 20, 2);
    const auto optional = pe + 24;
    if (machine != 0x14cu || read_le(bytes, optional, 2) != 0x10bu || !count || !optional_size) {
        return malformed(error, "not a PE32 i386 image");
    }
    // The fields used below end at SizeOfHeaders. Directory fields are read
    // only if the declared optional header includes their complete record.
    if (*optional_size < 64 || !contains(bytes.size(), optional, *optional_size)) {
        return malformed(error, "truncated optional header");
    }
    ExecutableImage image;
    image.base_ = *read_le(bytes, optional + 28, 4);
    image.size_of_image_ = *read_le(bytes, optional + 56, 4);
    image.entry_rva_ = *read_le(bytes, optional + 16, 4);
    if (std::uint64_t(image.base_) + image.size_of_image_ > kAddressSpaceSize ||
        (image.entry_rva_ &&
         (std::uint64_t(image.base_) + image.entry_rva_ >= kAddressSpaceSize ||
          (image.size_of_image_ && image.entry_rva_ >= image.size_of_image_)))) {
        return malformed(error, "image address range overflow");
    }
    if (*optional_size >= 112) {
        image.import_rva_ = *read_le(bytes, optional + 104, 4);
        image.import_size_ = *read_le(bytes, optional + 108, 4);
    }
    if (*optional_size >= 144) {
        image.relocation_rva_ = *read_le(bytes, optional + 136, 4);
        image.relocation_size_ = *read_le(bytes, optional + 140, 4);
    }
    const auto headers = *read_le(bytes, optional + 60, 4);
    image.bytes_ = std::move(bytes);
    if (!image.read_sections(optional + *optional_size, *count, headers, error)) {
        return std::nullopt;
    }
    if (*optional_size >= 104) {
        const auto rva = *read_le(image.bytes_, optional + 96, 4);
        const auto size = *read_le(image.bytes_, optional + 100, 4);
        if (rva && !image.read_exports(rva, size, error)) {
            return std::nullopt;
        }
    }
    return image;
}

bool ExecutableImage::read_sections(std::uint64_t table, std::uint32_t count, std::uint32_t headers,
                                    std::string *error) {
    if (headers) {
        if (!contains(bytes_.size(), 0, headers) || (size_of_image_ && headers > size_of_image_) ||
            std::uint64_t(base_) + headers > kAddressSpaceSize) {
            return reject(error, "headers outside the image or file");
        }
        sections_.push_back({0, headers, headers, 0});
    }
    if (!contains(bytes_.size(), table, std::uint64_t(count) * 40)) {
        return reject(error, "truncated section table");
    }
    for (std::uint32_t n = 0; n < count; ++n) {
        const auto at = table + 40 * std::uint64_t(n);
        const auto virtual_size = *read_le(bytes_, at + 8, 4);
        const auto rva = *read_le(bytes_, at + 12, 4);
        const auto raw_size = *read_le(bytes_, at + 16, 4);
        const auto raw_offset = *read_le(bytes_, at + 20, 4);
        if (raw_size && !contains(bytes_.size(), raw_offset, raw_size)) {
            return reject(error, "section raw data outside the file");
        }
        const auto mapped_size = std::max(virtual_size, raw_size);
        const auto end = std::uint64_t(rva) + mapped_size;
        if (mapped_size &&
            (end > kAddressSpaceSize || std::uint64_t(base_) + end > kAddressSpaceSize ||
             (size_of_image_ && end > size_of_image_))) {
            return reject(error, "section outside SizeOfImage");
        }
        sections_.push_back({rva, mapped_size, raw_size, raw_offset});
    }
    return true;
}

std::optional<std::uint8_t> ExecutableImage::byte(std::uint32_t va) const noexcept {
    if (va < base_) {
        return std::nullopt;
    }
    const auto rva = va - base_;
    for (const auto &section : sections_) {
        if (rva >= section.rva && rva - section.rva < section.virtual_size) {
            const auto offset = rva - section.rva;
            if (offset < section.raw_size) {
                return bytes_[std::size_t(section.raw_offset) + offset];
            }
            return std::uint8_t{0};
        }
    }
    return std::nullopt;
}

std::optional<std::uint32_t> ExecutableImage::read_rva(std::uint64_t rva,
                                                       unsigned width) const noexcept {
    if (!contains(kAddressSpaceSize - base_, rva, width)) {
        return std::nullopt;
    }
    std::uint32_t value = 0;
    for (unsigned i = 0; i < width; ++i) {
        const auto next = byte(static_cast<std::uint32_t>(base_ + rva + i));
        if (!next) {
            return std::nullopt;
        }
        value |= std::uint32_t(*next) << (8 * i);
    }
    return value;
}

std::optional<std::uint32_t> ExecutableImage::u32(std::uint32_t va) const noexcept {
    if (va < base_) {
        return std::nullopt;
    }
    return read_rva(va - base_, 4);
}

std::optional<std::string> ExecutableImage::string_rva(std::uint64_t rva) const {
    std::string result;
    for (unsigned i = 0; i < kStringLimit; ++i) {
        const auto next = read_rva(rva + i, 1);
        if (!next) {
            return std::nullopt;
        }
        if (!*next) {
            return result;
        }
        result.push_back(static_cast<char>(*next));
    }
    return std::nullopt;
}

bool ExecutableImage::read_exports(std::uint32_t rva, std::uint32_t size, std::string *error) {
    if ((size_of_image_ && !contains(size_of_image_, rva, size)) ||
        !contains(kAddressSpaceSize - base_, rva, size)) {
        return reject(error, "export directory outside the image");
    }
    const std::uint64_t directory = rva;
    const auto functions = read_rva(directory + 20, 4);
    const auto names = read_rva(directory + 24, 4);
    const auto addresses = read_rva(directory + 28, 4);
    const auto name_table = read_rva(directory + 32, 4);
    const auto ordinals = read_rva(directory + 36, 4);
    if (!functions || !names || !addresses || !name_table || !ordinals) {
        return reject(error, "truncated export directory");
    }
    for (std::uint64_t i = 0; i < *names; ++i) {
        const auto name_rva = read_rva(*name_table + 4 * i, 4);
        const auto ordinal = read_rva(*ordinals + 2 * i, 2);
        if (!name_rva || !ordinal || *ordinal >= *functions) {
            return reject(error, "invalid export name entry");
        }
        const auto name = string_rva(*name_rva);
        const auto function = read_rva(std::uint64_t(*addresses) + 4 * *ordinal, 4);
        if (!name || !function) {
            return reject(error, "invalid export entry");
        }
        Export entry;
        if (*function >= rva && *function - rva < size) {
            entry.forwarder = string_rva(*function);
            if (!entry.forwarder) {
                return reject(error, "invalid export forwarder");
            }
        } else {
            if (std::uint64_t(base_) + *function >= kAddressSpaceSize) {
                return reject(error, "export address overflow");
            }
            entry.va = base_ + *function;
        }
        exports_.emplace(*name, std::move(entry));
    }
    return true;
}

std::optional<ExecutableImage::Export>
ExecutableImage::export_named(const std::string &name) const {
    const auto found = exports_.find(name);
    if (found == exports_.end()) {
        return std::nullopt;
    }
    return found->second;
}

bool ExecutableImage::apply_relocations(std::vector<std::uint8_t> &mapped, std::uint32_t delta,
                                        std::string *error) const {
    if (!relocation_size_) {
        return true;
    }
    if (!contains(mapped.size(), relocation_rva_, relocation_size_)) {
        return reject(error, "relocation directory outside the image");
    }
    const auto end = std::uint64_t(relocation_rva_) + relocation_size_;
    for (std::uint64_t at = relocation_rva_; at < end;) {
        if (!contains(end, at, 8)) {
            return reject(error, "invalid relocation block");
        }
        const auto page = read_le(mapped, at, 4);
        const auto block = read_le(mapped, at + 4, 4);
        if (!page || !block || *block < 8 || (*block - 8) % 2 || !contains(end, at, *block)) {
            return reject(error, "invalid relocation block");
        }
        for (auto entry_at = at + 8; entry_at < at + *block; entry_at += 2) {
            const auto entry = read_le(mapped, entry_at, 2);
            if (!entry) {
                return reject(error, "relocation entry outside the image");
            }
            const auto type = *entry >> 12;
            if (type == 0) {
                continue; // IMAGE_REL_BASED_ABSOLUTE is padding.
            }
            if (type != 3) {
                return reject(error, "unsupported relocation type");
            }
            const auto target = std::uint64_t(*page) + (*entry & 0xfff);
            const auto value = read_le(mapped, target, 4);
            if (!value) {
                return reject(error, "relocation outside the image");
            }
            const std::uint32_t relocated = *value + delta; // PE32 modular arithmetic.
            for (unsigned i = 0; i < 4; ++i) {
                mapped[target + i] = static_cast<std::uint8_t>(relocated >> (8 * i));
            }
        }
        at += *block;
    }
    return true;
}

std::optional<ExecutableImage> ExecutableImage::rebased(std::uint32_t base,
                                                        std::string *error) const {
    if (!size_of_image_) {
        return malformed(error, "missing SizeOfImage");
    }
    if (std::uint64_t(base) + size_of_image_ > kAddressSpaceSize) {
        return malformed(error, "image address range overflow");
    }
    std::vector<std::uint8_t> mapped(size_of_image_, 0);
    for (const auto &section : sections_) {
        if (!section.raw_size) {
            continue; // No pointer arithmetic for empty sections with arbitrary offsets.
        }
        if (!contains(mapped.size(), section.rva, section.raw_size)) {
            return malformed(error, "section outside SizeOfImage");
        }
        std::copy_n(bytes_.begin() + section.raw_offset, section.raw_size,
                    mapped.begin() + section.rva);
    }
    if (!apply_relocations(mapped, base - base_, error)) {
        return std::nullopt;
    }
    ExecutableImage image;
    image.base_ = base;
    image.size_of_image_ = size_of_image_;
    image.import_rva_ = import_rva_;
    image.import_size_ = import_size_;
    image.entry_rva_ = entry_rva_;
    image.sections_ = {{0, size_of_image_, size_of_image_, 0}};
    image.bytes_ = std::move(mapped);
    for (const auto &[name, entry] : exports_) {
        auto relocated = entry;
        if (!relocated.forwarder) {
            const auto rva = entry.va - base_;
            if (std::uint64_t(base) + rva >= kAddressSpaceSize) {
                return malformed(error, "export address overflow");
            }
            relocated.va = base + rva;
        }
        image.exports_.emplace(name, std::move(relocated));
    }
    return image;
}

std::optional<std::vector<ExecutableImage::ImportedModule>>
ExecutableImage::imports(std::string *error) const {
    std::vector<ImportedModule> modules;
    if (!import_rva_) {
        return modules;
    }
    if (import_size_ && ((size_of_image_ && !contains(size_of_image_, import_rva_, import_size_)) ||
                         !contains(kAddressSpaceSize - base_, import_rva_, import_size_))) {
        return malformed(error, "import directory outside the image");
    }
    const auto end = std::uint64_t(import_rva_) + import_size_;
    for (std::uint64_t at = import_rva_;; at += 20) {
        if (import_size_ && !contains(end, at, 20)) {
            return malformed(error, "import directory has no terminator");
        }
        const auto original = read_rva(at, 4);
        const auto name = read_rva(at + 12, 4);
        const auto first = read_rva(at + 16, 4);
        if (!original || !name || !first) {
            return malformed(error, "import descriptor outside the image");
        }
        if (!*original && !*name && !*first) {
            return modules;
        }
        const auto dll = string_rva(*name);
        if (!dll) {
            return malformed(error, "invalid import DLL name");
        }
        ImportedModule module{*dll, {}};
        const std::uint64_t lookup = *original ? *original : *first;
        for (std::uint64_t offset = 0;; offset += 4) {
            const auto thunk = read_rva(lookup + offset, 4);
            if (!thunk) {
                return malformed(error, "import thunk outside the image");
            }
            if (!*thunk) {
                break;
            }
            if (!read_rva(*first + offset, 4)) {
                return malformed(error, "import IAT slot outside the image");
            }
            ImportedSymbol symbol;
            symbol.iat_va = static_cast<std::uint32_t>(base_ + *first + offset);
            if (*thunk & 0x80000000U) {
                symbol.ordinal = static_cast<std::uint16_t>(*thunk & 0xffffU);
            } else {
                // Preserve the existing four-byte availability check at the hint.
                const auto hint = read_rva(*thunk, 4);
                const auto import_name = string_rva(std::uint64_t(*thunk) + 2);
                if (!hint || !import_name) {
                    return malformed(error, "invalid import name entry");
                }
                symbol.hint = static_cast<std::uint16_t>(*hint & 0xffffU);
                symbol.name = *import_name;
            }
            module.symbols.push_back(std::move(symbol));
        }
        modules.push_back(std::move(module));
    }
}
} // namespace sadx
