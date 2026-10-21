#pragma once
#include "game/save_file_list.hpp"
#include "game/save_file_name.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <string_view>
namespace sadx::save {
// NOTSA: platform replacement of CreateDirectoryA/fopen/fwrite/fclose, rooted
// ONLY in the native user profile. The game ignores CRT result codes; invalid
// resources and actual native I/O errors must throw at their boundary instead
// of faking success.
struct WriteCalls {
    virtual ~WriteCalls() = default;
    virtual void ensure_directory() = 0;
    virtual std::shared_ptr<file_select::File> open_write(std::string_view path) = 0;
    virtual std::uint32_t write_save(const std::shared_ptr<file_select::File> &,
                                     const EncodedData &) = 0;
    virtual void close_write(const std::shared_ptr<file_select::File> &) = 0;
    virtual void reset_input() = 0;
};
// NOTSA: the same ordinary allocation services used by FileSelect confirmation.
struct SaveNameCalls {
    std::function<void(const char *)> release;
    std::function<char *(std::size_t)> allocate;
    std::function<void(char *, std::string_view)> copy;
};
struct WriteState {
    const std::int32_t &saving_disabled;
    std::uint8_t &save_retry;
    std::uint8_t &save_ok;
    std::uint8_t &save_count;
    std::int32_t &now_saving;
    std::uint8_t &save_ready;
    std::uint8_t &save_exec_flag;
    std::uint8_t &alert_flag;
    const std::uint8_t &file_delete_flag;
    std::uint8_t &first_create_flag;
    std::uint8_t &file_count;
    std::uint8_t &selected_file;
    std::uint8_t &card_complete;
    SaveNameState &save_name;
    const file_select::List &list;
    Data &data;
};
// FUNCTION: SONIC 0x421fd0 WriteSaveFile
std::int32_t write_file(WriteState &, WriteCalls &, SaveNameCalls &,
                        const std::function<void()> &capture_live);
} // namespace sadx::save
