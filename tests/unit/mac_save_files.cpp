#include "platform/mac/mac_save_files.hpp"
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace {
struct Directory {
    std::filesystem::path path;
    Directory() {
        auto name = (std::filesystem::temp_directory_path() / "sadx-native-saves-XXXXXX").string();
        std::vector<char> bytes(name.begin(), name.end());
        bytes.push_back(0);
        const auto *created = mkdtemp(bytes.data());
        assert(created);
        path = created;
    }
    ~Directory() { std::filesystem::remove_all(path); }
};
void write(const std::filesystem::path &path, const std::uint8_t *bytes, std::size_t size) {
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char *>(bytes), static_cast<std::streamsize>(size));
    assert(file.good());
}
template <class F> void fails(F operation) {
    bool failed = false;
    try {
        operation();
    } catch (const std::exception &) {
        failed = true;
    }
    assert(failed);
}
int available_descriptor() {
    const auto fd = open("/dev/null", O_RDONLY);
    assert(fd >= 0);
    close(fd);
    return fd;
}
} // namespace
int main(int argc, char **argv) {
    using namespace sadx;
    Directory temporary;
    file_select::mac::SaveFiles files(temporary.path);
    fails([&] { file_select::mac::SaveFiles absent(temporary.path / "absent"); });
    fails([&] { files.find_first("./SAVEDATA/*"); });
    const auto descriptor = available_descriptor();
    assert(!files.find_first("./SAVEDATA/SonicDX??.snc"));
    assert(available_descriptor() == descriptor);
    auto data = save::encode(save::Data{});
    const auto crc = save::checksum(data);
    data[0] = static_cast<std::uint8_t>(crc);
    data[1] = static_cast<std::uint8_t>(crc >> 8);
    write(temporary.path / "sONICdx01.SNC", data.data(), data.size());
    write(temporary.path / "SonicDX02.snc", data.data(), 79);
    auto bad = data;
    bad[20] ^= 1;
    write(temporary.path / "SonicDX03.snc", bad.data(), bad.size());
    auto long_data = std::vector<std::uint8_t>(data.begin(), data.end());
    long_data.insert(long_data.end(), 100, 19);
    write(temporary.path / "SonicDXab.snc", long_data.data(), long_data.size());
    write(temporary.path / "SonicDX001.snc", data.data(), data.size());
    write(temporary.path / "irrelevant.snc", data.data(), data.size());
    const timespec times[2] = {{1700000000, 123456700}, {1700000000, 123456700}};
    assert(utimensat(AT_FDCWD, (temporary.path / "sONICdx01.SNC").c_str(), times, 0) == 0);
    const auto first = files.find_first("./SAVEDATA/SonicDX??.snc");
    assert(first);
    std::vector<file_select::FileInfo> found{first->info};
    while (const auto next = files.find_next(first->search))
        found.push_back(*next);
    assert(found.size() == 4);
    const auto actual = std::find_if(found.begin(), found.end(),
                                     [](const auto &info) { return info.name == "sONICdx01.SNC"; });
    assert(actual != found.end());
    assert(actual->last_write_time == 133444736001234567ULL);
    // Compare against the actual native provider's order, without inventing a sort.
    auto *directory = opendir(temporary.path.c_str());
    assert(directory);
    std::vector<std::string> native;
    while (const auto *entry = readdir(directory)) {
        const std::string name = entry->d_name;
        if (name == "sONICdx01.SNC" || name == "SonicDX02.snc" || name == "SonicDX03.snc" ||
            name == "SonicDXab.snc")
            native.push_back(name);
    }
    assert(closedir(directory) == 0);
    for (std::size_t i = 0; i < native.size(); ++i)
        assert(found[i].name == native[i]);
    files.close_search(first->search);
    fails([&] { files.find_next(first->search); });
    auto file = files.open_file("./savedata/SonicDX01.snc");
    assert(file);
    save::EncodedData read{};
    assert(files.read_save(file, read) == 1 && read == data);
    assert(save::checksum(read) == static_cast<std::uint16_t>(read[0] | read[1] << 8));
    files.close_file(file);
    fails([&] { files.read_save(file, read); });
    file = files.open_file("./SAVEDATA/sonicdx02.snc");
    read.fill(165);
    assert(files.read_save(file, read) == 0);
    assert(std::equal(read.begin(), read.begin() + 79, data.begin()));
    assert(std::all_of(read.begin() + 79, read.end(), [](auto value) { return value == 165; }));
    files.close_file(file);
    file = files.open_file("./SAVEDATA/SonicDX03.snc");
    assert(files.read_save(file, read) == 1 && read == bad);
    assert(save::checksum(read) != static_cast<std::uint16_t>(read[0] | read[1] << 8));
    files.close_file(file);
    file = files.open_file("./SAVEDATA/SonicDXab.snc");
    assert(files.read_save(file, read) == 1 && read == data);
    // Destruction closes real resources even when the game unwinds at a boundary.
    file.reset();
    {
        const auto abandoned = files.find_first("./SAVEDATA/SonicDX??.snc");
        assert(abandoned);
    }
    assert(available_descriptor() == descriptor);
    assert(!files.open_file("./SAVEDATA/missing.snc"));
    fails([&] { files.open_file("./SAVEDATA/../outside"); });
    fails([&] { files.open_file("/tmp/SAVEDATA/SonicDX01.snc"); });
    file_select::mac::SaveFiles other(temporary.path);
    file = files.open_file("./SAVEDATA/SonicDX01.snc");
    fails([&] { other.read_save(file, read); });
    files.close_file(file);

    // A real filesystem I/O error is explicit and never overwrites with fake zeros.
    std::filesystem::create_directory(temporary.path / "SonicDX04.snc");
    file = files.open_file("./SAVEDATA/SonicDX04.snc");
    if (file) {
        read.fill(165);
        fails([&] { files.read_save(file, read); });
        assert(std::all_of(read.begin(), read.end(), [](auto value) { return value == 165; }));
        files.close_file(file);
    }
    assert(available_descriptor() == descriptor);
    if (argc == 2) {
        Directory copies;
        for (const auto *name : {"SonicDX01.snc", "SonicDX02.snc"}) {
            const auto source = std::filesystem::path(argv[1]) / name;
            const auto destination = copies.path / name;
            std::filesystem::copy_file(source, destination);
            std::filesystem::last_write_time(destination, std::filesystem::last_write_time(source));
            std::ifstream original(source, std::ios::binary);
            save::EncodedData expected{};
            original.read(reinterpret_cast<char *>(expected.data()), expected.size());
            assert(original.gcount() == static_cast<std::streamsize>(expected.size()));
            file_select::mac::SaveFiles copied(copies.path);
            auto input = copied.open_file(std::string("./SAVEDATA/") + name);
            assert(copied.read_save(input, read) == 1 && read == expected);
            copied.close_file(input);
        }
    }
    // If the native volume permits two case aliases, reject ambiguous PC lookup.
    write(temporary.path / "SONICDX01.snc", data.data(), data.size());
    std::size_t aliases = 0;
    for (const auto &entry : std::filesystem::directory_iterator(temporary.path))
        if (entry.path().filename() == "SONICDX01.snc" ||
            entry.path().filename() == "sONICdx01.SNC")
            ++aliases;
    if (aliases == 2)
        fails([&] { files.open_file("./SAVEDATA/SonicDX01.snc"); });
    assert(available_descriptor() == descriptor);
}
