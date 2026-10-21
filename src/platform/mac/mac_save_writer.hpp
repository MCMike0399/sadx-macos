#pragma once
#include "game/save_write.hpp"
#include <filesystem>
#include <functional>
namespace sadx::save::mac {
// [platform:filesystem] NOTSA: native-owned save publication. A fresh temporary
// inode and atomic rename protect hardlinked/symlinked original/imported saves;
// the source writes the same chosen filename/bytes, but atomic visibility is a
// native resource policy, not a claim about Windows CRT failure behavior.
class SaveWriter final : public WriteCalls {
  public:
    SaveWriter(std::filesystem::path native_root, std::function<void()> reset_input);
    void ensure_directory() override;
    std::shared_ptr<file_select::File> open_write(std::string_view) override;
    std::uint32_t write_save(const std::shared_ptr<file_select::File> &,
                             const EncodedData &) override;
    void close_write(const std::shared_ptr<file_select::File> &) override;
    void reset_input() override;

  private:
    std::filesystem::path root_;
    std::shared_ptr<void> identity_;
    std::function<void()> reset_input_;
};
} // namespace sadx::save::mac
