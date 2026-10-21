#include "platform/mac/mac_save_writer.hpp"
#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <stdexcept>
#include <sys/stat.h>
#include <system_error>
#include <unistd.h>
#include <vector>
namespace sadx::save::mac {
namespace {
struct Output final : file_select::File {
    std::shared_ptr<void> identity;
    std::FILE *stream = nullptr;
    std::filesystem::path temporary, target;
    bool complete = false;
    ~Output() override {
        if (stream)
            std::fclose(stream);
        if (!temporary.empty())
            ::unlink(temporary.c_str());
    }
};
[[noreturn]] void fail(const char *operation) {
    throw std::system_error(errno, std::generic_category(), operation);
}
char fold(char value) {
    return value >= 'A' && value <= 'Z' ? static_cast<char>(value + 32) : value;
}
bool equal(std::string_view left, std::string_view right) {
    if (left.size() != right.size())
        return false;
    for (std::size_t i = 0; i < left.size(); ++i)
        if (fold(left[i]) != fold(right[i]))
            return false;
    return true;
}
std::string file_name(std::string_view path) {
    constexpr std::string_view prefix = "./SAVEDATA/";
    if (path.size() != prefix.size() + 13 || !equal(path.substr(0, prefix.size()), prefix))
        throw std::invalid_argument("write path is outside native SAVEDATA file contract");
    auto name = path.substr(prefix.size());
    if (!equal(name.substr(0, 7), "SonicDX") || name[7] < '0' || name[7] > '9' || name[8] < '0' ||
        name[8] > '9' || !equal(name.substr(9), ".snc"))
        throw std::invalid_argument("write path is not a canonical source save filename");
    return std::string(name);
}
std::shared_ptr<Output> resource(const std::shared_ptr<file_select::File> &file,
                                 const std::shared_ptr<void> &identity) {
    auto output = std::dynamic_pointer_cast<Output>(file);
    if (!output || output->identity != identity || !output->stream)
        throw std::invalid_argument("native write resource is foreign or closed");
    return output;
}
} // namespace
SaveWriter::SaveWriter(std::filesystem::path root, std::function<void()> reset)
    : root_(std::move(root)), identity_(std::make_shared<int>(0)), reset_input_(std::move(reset)) {
    if (!reset_input_)
        throw std::invalid_argument("save writer requires real input reset service");
}
void SaveWriter::ensure_directory() {
    struct stat metadata{};
    if (::lstat(root_.c_str(), &metadata) != 0) {
        if (errno != ENOENT)
            fail("stat native save directory");
        if (::mkdir(root_.c_str(), 0700) != 0)
            fail("create native save directory");
        if (::lstat(root_.c_str(), &metadata) != 0)
            fail("stat created native save directory");
    }
    if (!S_ISDIR(metadata.st_mode))
        throw std::invalid_argument("native save root must be a real directory");
}
std::shared_ptr<file_select::File> SaveWriter::open_write(std::string_view path) {
    auto name = file_name(path);
    std::optional<std::filesystem::path> existing;
    for (const auto &entry : std::filesystem::directory_iterator(root_)) {
        if (equal(entry.path().filename().string(), name)) {
            if (existing)
                throw std::invalid_argument("ambiguous case-insensitive native save filename");
            existing = entry.path();
        }
    }
    auto result = std::make_shared<Output>();
    result->identity = identity_;
    result->target = existing ? *existing : root_ / name;
    struct stat target{};
    if (::lstat(result->target.c_str(), &target) == 0) {
        if (!S_ISREG(target.st_mode))
            throw std::invalid_argument("native save target is not a regular file");
    } else if (errno != ENOENT)
        fail("stat native save target");
    auto pattern = (root_ / ".native-save-XXXXXX").string();
    std::vector<char> temporary(pattern.begin(), pattern.end());
    temporary.push_back(0);
    auto descriptor = ::mkstemp(temporary.data());
    if (descriptor < 0)
        fail("create fresh native save inode");
    result->temporary = temporary.data();
    result->stream = ::fdopen(descriptor, "wb");
    if (!result->stream) {
        const auto saved = errno;
        ::close(descriptor);
        errno = saved;
        fail("open native save stream");
    }
    return result;
}
std::uint32_t SaveWriter::write_save(const std::shared_ptr<file_select::File> &file,
                                     const EncodedData &data) {
    auto output = resource(file, identity_);
    const auto count = std::fwrite(data.data(), data.size(), 1, output->stream);
    if (count != 1)
        fail("write complete native save record");
    output->complete = true;
    return static_cast<std::uint32_t>(count);
}
void SaveWriter::close_write(const std::shared_ptr<file_select::File> &file) {
    auto output = resource(file, identity_);
    if (!output->complete)
        throw std::invalid_argument("native save record was never completely written");
    auto stream = output->stream;
    output->stream = nullptr;
    if (std::fclose(stream) != 0)
        fail("close native save stream");
    if (::rename(output->temporary.c_str(), output->target.c_str()) != 0)
        fail("publish native save record");
    output->temporary.clear();
}
void SaveWriter::reset_input() { reset_input_(); }
} // namespace sadx::save::mac
