#include "game/game_cleanup.hpp"
#include "game/save_creation.hpp"
#include "platform/mac/mac_save_writer.hpp"
#include <cassert>
#include <cstring>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
namespace fs = std::filesystem;
namespace {
std::string read(const fs::path &path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), {}};
}
void put(const fs::path &path, const std::string &bytes) {
    std::ofstream file(path, std::ios::binary);
    file << bytes;
    assert(file.good());
}
template <class Fn> void rejects(Fn function) {
    bool failed = false;
    try {
        function();
    } catch (const std::exception &) {
        failed = true;
    }
    assert(failed);
}
} // namespace
int main() {
    char pattern[] = "/private/tmp/native-save-write-XXXXXX";
    const auto created = ::mkdtemp(pattern);
    assert(created);
    const fs::path directory = created;
    struct Cleanup {
        fs::path path;
        ~Cleanup() { fs::remove_all(path); }
    } cleanup{directory};
    const auto root = directory / "SAVEDATA";
    int resets = 0;
    sadx::save::mac::SaveWriter writer(root, [&] { ++resets; });
    writer.ensure_directory();
    sadx::save::EncodedData bytes;
    for (std::size_t i = 0; i < bytes.size(); ++i)
        bytes[i] = static_cast<std::uint8_t>(i);
    auto file = writer.open_write("./SAVEDATA/SonicDX01.snc");
    assert(!fs::exists(root / "SonicDX01.snc"));
    assert(writer.write_save(file, bytes) == 1);
    writer.close_write(file);
    assert(read(root / "SonicDX01.snc") == std::string(bytes.begin(), bytes.end()));
    rejects([&] { writer.close_write(file); });
    rejects([&] { writer.write_save(file, bytes); });
    fs::rename(root / "SonicDX01.snc", root / "sonicdx01.SNC");
    bytes.fill(73);
    file = writer.open_write("./SAVEDATA/SonicDX01.snc");
    writer.write_save(file, bytes);
    writer.close_write(file);
    assert(fs::directory_iterator(root)->path().filename() == "sonicdx01.SNC");
    assert(read(root / "sonicdx01.SNC") == std::string(1392, 'I'));
    const auto original = directory / "owned-install-save.snc";
    put(original, "original-owned-by-user");
    struct stat before{}, after{};
    assert(::stat(original.c_str(), &before) == 0);
    fs::create_hard_link(original, root / "SonicDX02.snc");
    file = writer.open_write("./SAVEDATA/SonicDX02.snc");
    writer.write_save(file, bytes);
    writer.close_write(file);
    assert(read(original) == "original-owned-by-user");
    assert(::stat(original.c_str(), &after) == 0);
    assert(before.st_ino == after.st_ino && before.st_mtime == after.st_mtime);
    assert(read(root / "SonicDX02.snc") == std::string(1392, 'I'));
    fs::create_symlink(original, root / "SonicDX03.snc");
    rejects([&] { writer.open_write("./SAVEDATA/SonicDX03.snc"); });
    rejects([&] { writer.open_write("./SAVEDATA/../escape"); });
    rejects([&] { writer.open_write("./SAVEDATA/SonicDX00.snc/../../escape"); });
    file = writer.open_write("./SAVEDATA/SonicDX04.snc");
    rejects([&] { writer.close_write(file); });
    file.reset();
    assert(!fs::exists(root / "SonicDX04.snc"));
    file = writer.open_write("./SAVEDATA/SonicDX05.snc");
    writer.write_save(file, bytes);
    fs::create_directory(root / "SonicDX05.snc");
    rejects([&] { writer.close_write(file); });
    file.reset();
    fs::remove(root / "SonicDX05.snc");
    for (const auto &entry : fs::directory_iterator(root))
        assert(entry.path().filename().string().find(".native-save-") != 0);
    sadx::save::mac::SaveWriter other(root, [] {});
    file = writer.open_write("./SAVEDATA/SonicDX06.snc");
    rejects([&] { other.write_save(file, bytes); });
    file.reset();
    fs::create_symlink(root, directory / "alias");
    sadx::save::mac::SaveWriter linked(directory / "alias", [] {});
    rejects([&] { linked.ensure_directory(); });
    // Actual new-save game writer chooses first free logical slot from canonical
    // provider list.
    auto list = std::make_shared<sadx::file_select::ListNode>();
    list->next = std::make_shared<sadx::file_select::ListNode>();
    list->next->file = sadx::file_select::FileInfo{"SonicDX01.snc", 0};
    std::int32_t disabled = 0, phase = 1;
    std::uint8_t retry = 9, ok = 0, count = 1, ready = 1, exec = 1, alert = 1;
    std::uint8_t deleting = 0, first = 1, file_count = 1, selected = 0, complete = 1;
    sadx::save::SaveNameState name;
    std::unique_ptr<char[]> name_backing = std::make_unique<char[]>(14);
    std::strcpy(name_backing.get(), "SonicDX42.snc");
    name.value = name_backing.get();
    std::vector<int> name_calls;
    bool fail_name_allocation = false;
    sadx::save::SaveNameCalls names{
        [&](const char *pointer) {
            assert(pointer == name_backing.get() && name.value == pointer && first == 0 &&
                   file_count == 1 && selected == 0);
            name_calls.push_back(1);
            name_backing.reset();
        },
        [&](std::size_t size) {
            assert(size == 14 && name.value == nullptr && first == 0 && file_count == 1 &&
                   selected == 0);
            name_calls.push_back(2);
            if (fail_name_allocation)
                return static_cast<char *>(nullptr);
            name_backing = std::make_unique<char[]>(size);
            return name_backing.get();
        },
        [&](char *destination, std::string_view value) {
            assert(name.value == destination && file_count == 2 && selected == 2);
            name_calls.push_back(3);
            std::memcpy(destination, value.data(), value.size());
            destination[value.size()] = 0;
        }};
    sadx::save::Data data;
    std::int16_t rings = 20;
    sadx::save::initialize(data, rings);
    sadx::save::WriteState state{disabled, retry, ok,       count, phase,      ready,
                                 exec,     alert, deleting, first, file_count, selected,
                                 complete, name,  list,     data};
    assert(sadx::save::write_file(state, writer, names, [&] {
               data.checksum = sadx::save::checksum(sadx::save::encode(data));
           }) == 0);
    assert(name.value && std::string_view(name.value) == "SonicDX02.snc" && file_count == 2 &&
           selected == 2 && ok == 1 && first == 0 && ready == 0 && exec == 0 && phase == 0 &&
           resets == 1);
    assert(name_calls == std::vector<int>({1, 2, 3}));
    auto saved = read(root / "SonicDX02.snc");
    const auto encoded = sadx::save::encode(data);
    assert(saved == std::string(encoded.begin(), encoded.end()));
    assert(read(original) == "original-owned-by-user");
    // A failed commit retains the source prefix but never publishes completion.
    first = 1;
    ok = 0;
    file_count = 1;
    selected = 0;
    name.value = nullptr;
    name_backing.reset();
    fs::remove(root / "SonicDX02.snc");
    fs::create_directory(root / "SonicDX02.snc");
    rejects([&] { sadx::save::write_file(state, writer, names, [] {}); });
    assert(ok == 0 && first == 0 && file_count == 2 && selected == 2 && name.value &&
           std::string_view(name.value) == "SonicDX02.snc");
    // Actual malloc-null prefix: old resource released, pointer published null,
    // then slot/count stored before the source formatter's invalid destination.
    first = 1;
    ok = 0;
    file_count = 1;
    selected = 0;
    name_calls.clear();
    fail_name_allocation = true;
    rejects([&] { sadx::save::write_file(state, writer, names, [] {}); });
    assert(name.value == nullptr && file_count == 2 && selected == 2 && ok == 0);
    assert(name_calls == std::vector<int>({1, 2}));
    return 0;
}
