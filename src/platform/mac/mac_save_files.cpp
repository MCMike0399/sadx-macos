#include "platform/mac/mac_save_files.hpp"
#include <cerrno>
#include <cstdio>
#include <dirent.h>
#include <limits>
#include <stdexcept>
#include <sys/stat.h>
#include <system_error>

// [platform:filesystem] POSIX resources replace the source FindFirstFile/fopen APIs.
namespace sadx::file_select::mac {
namespace {
// NOTSA: ownership receipts prevent cross-provider and already-closed resources.
struct NativeSearch final : Search {
    std::shared_ptr<void> owner;
    DIR *directory;
    NativeSearch(std::shared_ptr<void> identity, DIR *value)
        : owner(std::move(identity)), directory(value) {}
    ~NativeSearch() override {
        if (directory)
            closedir(directory);
    }
};
struct NativeFile final : File {
    std::shared_ptr<void> owner;
    std::FILE *file;
    NativeFile(std::shared_ptr<void> identity, std::FILE *value)
        : owner(std::move(identity)), file(value) {}
    ~NativeFile() override {
        if (file)
            std::fclose(file);
    }
};
[[noreturn]] void filesystem_error(const char *operation) {
    throw std::system_error(errno, std::generic_category(), operation);
}
char folded(char value) {
    return value >= 'A' && value <= 'Z' ? static_cast<char>(value + ('a' - 'A')) : value;
}
bool equal_name(std::string_view left, std::string_view right) {
    if (left.size() != right.size())
        return false;
    for (std::size_t i = 0; i < left.size(); ++i)
        if (folded(left[i]) != folded(right[i]))
            return false;
    return true;
}
bool save_name(std::string_view name) {
    return name.size() == 13 && equal_name(name.substr(0, 7), "SonicDX") &&
           equal_name(name.substr(9), ".snc");
}
std::string_view logical_name(std::string_view path) {
    constexpr std::string_view prefix = "./SAVEDATA/";
    if (path.size() <= prefix.size() || !equal_name(path.substr(0, prefix.size()), prefix))
        throw std::invalid_argument("save path is outside the logical SAVEDATA root");
    const auto name = path.substr(prefix.size());
    if (name == "." || name == ".." || name.find_first_of("/\\:") != std::string_view::npos ||
        name.find('\0') != std::string_view::npos)
        throw std::invalid_argument("save path must name one native file");
    return name;
}
// NOTSA: Windows FILETIME counts100ns intervals from1601; native mtime is real,
// not a source fixture timestamp. Sub100ns precision is truncated.
std::uint64_t file_time(const struct stat &metadata) {
#ifdef __APPLE__
    const auto time = metadata.st_mtimespec;
#else
    const auto time = metadata.st_mtim;
#endif
    constexpr std::int64_t epoch_seconds = 11644473600LL;
    constexpr std::uint64_t ticks_per_second = 10000000;
    if (time.tv_sec < -epoch_seconds || time.tv_nsec < 0 || time.tv_nsec >= 1000000000)
        throw std::out_of_range("native save mtime cannot be represented as FILETIME");
    const auto seconds = time.tv_sec < 0 ? static_cast<std::uint64_t>(time.tv_sec + epoch_seconds)
                                         : static_cast<std::uint64_t>(time.tv_sec) +
                                               static_cast<std::uint64_t>(epoch_seconds);
    const auto fraction = static_cast<std::uint64_t>(time.tv_nsec) / 100;
    if (seconds > (std::numeric_limits<std::uint64_t>::max() - fraction) / ticks_per_second)
        throw std::out_of_range("native save mtime exceeds FILETIME");
    return seconds * ticks_per_second + fraction;
}
std::optional<FileInfo> next_match(NativeSearch &search) {
    for (;;) {
        errno = 0;
        const auto *entry = readdir(search.directory);
        if (!entry) {
            if (errno)
                filesystem_error("enumerate native saves");
            return std::nullopt;
        }
        if (!save_name(entry->d_name))
            continue;
        struct stat metadata {};
        if (fstatat(dirfd(search.directory), entry->d_name, &metadata, 0) != 0)
            filesystem_error("read native save metadata");
        return FileInfo{entry->d_name, file_time(metadata)};
    }
}
template <class Native, class Base>
std::shared_ptr<Native> resource(const std::shared_ptr<Base> &base,
                                 const std::shared_ptr<void> &owner) {
    const auto value = std::dynamic_pointer_cast<Native>(base);
    if (!value || value->owner != owner)
        throw std::invalid_argument("save resource belongs to another provider");
    return value;
}
} // namespace
SaveFiles::SaveFiles(std::filesystem::path directory) : directory_(std::move(directory)) {
    struct stat metadata {};
    if (stat(directory_.c_str(), &metadata) != 0)
        filesystem_error("open native saves directory");
    if (!S_ISDIR(metadata.st_mode))
        throw std::invalid_argument("native saves root is not a directory");
}
std::optional<FoundFile> SaveFiles::find_first(std::string_view pattern) {
    if (!equal_name(pattern, "./SAVEDATA/SonicDX??.snc"))
        throw std::invalid_argument("unqualified native save search pattern");
    const auto search = std::make_shared<NativeSearch>(owner_, nullptr);
    search->directory = opendir(directory_.c_str());
    if (!search->directory)
        filesystem_error("open native save search");
    const auto info = next_match(*search);
    if (!info)
        return std::nullopt;
    return FoundFile{search, *info};
}
std::optional<FileInfo> SaveFiles::find_next(const std::shared_ptr<Search> &base) {
    const auto search = resource<NativeSearch>(base, owner_);
    if (!search->directory)
        throw std::invalid_argument("native save search is closed");
    return next_match(*search);
}
void SaveFiles::close_search(const std::shared_ptr<Search> &base) {
    const auto search = resource<NativeSearch>(base, owner_);
    if (!search->directory)
        throw std::invalid_argument("native save search is already closed");
    auto *directory = search->directory;
    search->directory = nullptr;
    if (closedir(directory) != 0)
        filesystem_error("close native save search");
}
std::shared_ptr<File> SaveFiles::open_file(std::string_view path) {
    const auto name = logical_name(path);
    auto *directory = opendir(directory_.c_str());
    if (!directory)
        filesystem_error("resolve native save filename");
    NativeSearch search(owner_, directory);
    std::optional<std::string> actual;
    for (;;) {
        errno = 0;
        const auto *entry = readdir(directory);
        if (!entry) {
            if (errno)
                filesystem_error("resolve native save filename");
            break;
        }
        if (!equal_name(entry->d_name, name))
            continue;
        if (actual)
            throw std::runtime_error("ambiguous case-insensitive native save filename");
        actual = entry->d_name;
    }
    if (!actual)
        return {};
    const auto result = std::make_shared<NativeFile>(owner_, nullptr);
    result->file = std::fopen((directory_ / *actual).c_str(), "rb");
    if (!result->file) {
        if (errno == ENOENT)
            return {};
        filesystem_error("open native save file");
    }
    return result;
}
std::uint32_t SaveFiles::read_save(const std::shared_ptr<File> &base, save::EncodedData &bytes) {
    const auto file = resource<NativeFile>(base, owner_);
    if (!file->file)
        throw std::invalid_argument("native save file is closed");
    const auto items = std::fread(bytes.data(), bytes.size(), 1, file->file);
    if (std::ferror(file->file)) {
        if (!errno)
            errno = EIO;
        filesystem_error("read native save file");
    }
    return static_cast<std::uint32_t>(items);
}
void SaveFiles::close_file(const std::shared_ptr<File> &base) {
    const auto file = resource<NativeFile>(base, owner_);
    if (!file->file)
        throw std::invalid_argument("native save file is already closed");
    auto *stream = file->file;
    file->file = nullptr;
    if (std::fclose(stream) != 0)
        filesystem_error("close native save file");
}
} // namespace sadx::file_select::mac
