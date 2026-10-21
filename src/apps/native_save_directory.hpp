#pragma once
#include "apps/native_assets.hpp"
#include <CommonCrypto/CommonDigest.h>
#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <fstream>
#include <set>
#include <sys/stdio.h>

namespace sadx::native::app {
// NOTSA: new native profiles receive copies of imported saves. Existing profiles
// are authoritative and are never reseeded, including an intentionally empty one.
inline void prepare_save_directory(const std::filesystem::path &assets,
                                   const std::filesystem::path &destination) {
    if (std::filesystem::exists(destination)) {
        if (!std::filesystem::is_directory(destination))
            throw std::runtime_error("native save path is not a directory");
        return;
    }
    const auto metadata = read_metadata(assets / "save-files.json");
    id records = metadata[@"files"];
    if (![records isKindOfClass:NSArray.class])
        throw std::runtime_error("invalid imported save snapshot");
    std::filesystem::create_directories(destination.parent_path());
    const std::string identity = [NSUUID.UUID.UUIDString UTF8String];
    const auto temporary = destination.parent_path() / (".sadx-save-seed-" + identity);
    if (!std::filesystem::create_directory(temporary))
        throw std::runtime_error("cannot create native save staging directory");
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() {
            std::error_code ignored;
            std::filesystem::remove_all(path, ignored);
        }
    } cleanup{temporary};
    std::set<std::string> names;
    for (id record in records) {
        const auto item = dictionary(record, "save snapshot entry");
        const auto name = string(item[@"name"], "save filename");
        const std::filesystem::path relative(string(item[@"path"], "save snapshot path"));
        auto folded = name;
        for (auto &character : folded)
            if (character >= 'A' && character <= 'Z')
                character = static_cast<char>(character + ('a' - 'A'));
        if (name.empty() || name.find_first_of("/\\") != std::string::npos ||
            name.find('\0') != std::string::npos || folded.size() < 5 ||
            folded.substr(folded.size() - 4) != ".snc" || !names.insert(folded).second ||
            relative != std::filesystem::path("saves") / name)
            throw std::runtime_error("invalid or ambiguous imported save filename");
        const auto source = assets / relative;
        const auto target = temporary / name;
        const auto size = static_cast<std::uintmax_t>(
            integer(item[@"size"], 0, INT64_MAX, "save snapshot size"));
        if (std::filesystem::file_size(source) != size)
            throw std::runtime_error("imported save snapshot size differs from metadata");
        std::filesystem::copy_file(source, target); // Fresh inode; never aliases an original save.
        std::filesystem::last_write_time(target, std::filesystem::last_write_time(source));
        std::ifstream file(target, std::ios::binary);
        std::vector<unsigned char> bytes{std::istreambuf_iterator<char>(file), {}};
        if (!file || bytes.size() != size)
            throw std::runtime_error("native save seed read failed");
        unsigned char digest[CC_SHA256_DIGEST_LENGTH];
        if (size > UINT32_MAX)
            throw std::runtime_error("native save seed exceeds supported digest input size");
        CC_SHA256(bytes.data(), static_cast<CC_LONG>(bytes.size()), digest);
        constexpr char digits[] = "0123456789abcdef";
        std::string actual;
        for (const auto byte : digest) {
            actual += digits[byte >> 4];
            actual += digits[byte & 15];
        }
        if (actual != string(item[@"sha256"], "save snapshot digest"))
            throw std::runtime_error("imported save snapshot digest mismatch");
    }
    // Exclusive publication also preserves a profile created by another app instance.
    if (renameatx_np(AT_FDCWD, temporary.c_str(), AT_FDCWD, destination.c_str(), RENAME_EXCL) != 0) {
        const auto error = errno;
        if (error == EEXIST && std::filesystem::is_directory(destination))
            return;
        throw std::filesystem::filesystem_error("publish native save directory", temporary,
            destination, std::error_code(error, std::generic_category()));
    }
}
} // namespace sadx::native::app
