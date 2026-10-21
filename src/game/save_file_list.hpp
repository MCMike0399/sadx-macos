#pragma once
#include "game/save_data.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
namespace sadx::file_select {
struct FileInfo {
    std::string name;
    std::uint64_t last_write_time = 0;
};
struct ListNode {
    // The first source MAlloc node leaves name/time indeterminate; only next is initialized.
    std::optional<FileInfo> file;
    std::shared_ptr<ListNode> next;
};
using List = std::shared_ptr<ListNode>;
class Search {
  public:
    virtual ~Search() = default;
};
class File {
  public:
    virtual ~File() = default;
};
struct FoundFile {
    std::shared_ptr<Search> search;
    FileInfo info;
};
enum class Boundary { IndeterminateSentinel, MissingFile, ShortFileName, ListIndex, Path };
class Error : public std::runtime_error {
  public:
    Error(Boundary boundary, std::string message)
        : std::runtime_error(std::move(message)), boundary_(boundary) {}
    Boundary boundary() const { return boundary_; }
  private:
    Boundary boundary_;
};
struct SaveFileCalls {
    virtual ~SaveFileCalls() = default;
    virtual std::optional<FoundFile> find_first(std::string_view pattern) = 0;
    virtual std::optional<FileInfo> find_next(const std::shared_ptr<Search> &) = 0;
    virtual void close_search(const std::shared_ptr<Search> &) = 0;
    virtual std::shared_ptr<File> open_file(std::string_view path) = 0;
    // Writes even a partial prefix; returns the number of complete1392-byte items.
    virtual std::uint32_t read_save(const std::shared_ptr<File> &, save::EncodedData &) = 0;
    virtual void close_file(const std::shared_ptr<File> &) = 0;
};
// NOTSA: inverse of the established1392-byte native save codec, at the file boundary.
save::Data decode_save(const save::EncodedData &);
// FUNCTION: SONIC 0x504e50 AddLineList
void append_save_list(List &, const std::optional<std::string> &name, const FileInfo &);
// FUNCTION: SONIC 0x504ed0 FUN_00504ed0
void free_save_list(List &);
struct LoadState {
    const std::int32_t &saving_disabled;
    const std::uint8_t &selected_file;
    const char *fallback_name;
    List &list;
    save::Data &live;
    std::int16_t &rings;
};
// FUNCTION: SONIC 0x421de0 dsVMSLoadGame_do
std::int32_t load_selected_save(LoadState &, SaveFileCalls &);
// FUNCTION: SONIC 0x505050 CountSaveNum
std::uint8_t count_save_files(List &, save::Data &, SaveFileCalls &);
} // namespace sadx::file_select
