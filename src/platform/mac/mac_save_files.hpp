#pragma once
#include "game/save_file_list.hpp"
#include <filesystem>
#include <memory>

namespace sadx::file_select::mac {
// [platform:filesystem] NOTSA: actual native filesystem provider for the source SAVEDATA
// operations. Directory order is the native provider's order; no game sorting is introduced. Only
// the source SonicDX??.snc pattern is qualified, not general NTFS wildcards.
class SaveFiles final : public SaveFileCalls {
  public:
    explicit SaveFiles(std::filesystem::path savedata_directory);
    std::optional<FoundFile> find_first(std::string_view pattern) override;
    std::optional<FileInfo> find_next(const std::shared_ptr<Search> &) override;
    void close_search(const std::shared_ptr<Search> &) override;
    std::shared_ptr<File> open_file(std::string_view path) override;
    std::uint32_t read_save(const std::shared_ptr<File> &, save::EncodedData &) override;
    void close_file(const std::shared_ptr<File> &) override;

  private:
    struct Owner {};
    std::filesystem::path directory_;
    std::shared_ptr<Owner> owner_ = std::make_shared<Owner>();
};
} // namespace sadx::file_select::mac
