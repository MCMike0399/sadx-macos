#include "apps/native_save_directory.hpp"
#include <cassert>
#include <iostream>
#include <sys/stat.h>

namespace {
std::string read(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), {}};
}
void write(const std::filesystem::path &path, std::string_view value) {
    std::ofstream file(path, std::ios::binary);
    file << value;
    assert(file);
}
} // namespace
int main() {
    @autoreleasepool {
        using namespace sadx::native::app;
        const std::string identity = [NSUUID.UUID.UUIDString UTF8String];
        const auto root = std::filesystem::temp_directory_path() / ("sadx-save-seeding-" + identity);
        const auto assets = root / "imported";
        const auto destination = root / "user" / "savedata";
        std::filesystem::create_directories(assets / "saves");
        const auto source = assets / "saves/SonicDX01.snc";
        write(source, "abc");
        const auto source_time = std::filesystem::last_write_time(source);
        write(assets / "save-files.json", R"({"version":1,"files":[{"name":"SonicDX01.snc",
            "path":"saves/SonicDX01.snc","size":3,
            "sha256":"ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"}]})");
        prepare_save_directory(assets, destination);
        assert(read(destination / "SonicDX01.snc") == "abc");
        assert(std::filesystem::last_write_time(destination / "SonicDX01.snc") == source_time);
        struct stat original{}, copied{};
        assert(stat(source.c_str(), &original) == 0);
        assert(stat((destination / "SonicDX01.snc").c_str(), &copied) == 0);
        assert(original.st_ino != copied.st_ino);
        write(destination / "SonicDX01.snc", "native progress");
        write(assets / "save-files.json", "invalid metadata");
        prepare_save_directory(assets, destination);
        assert(read(destination / "SonicDX01.snc") == "native progress");
        assert(read(source) == "abc" && std::filesystem::last_write_time(source) == source_time);
        const auto empty = root / "empty-profile";
        std::filesystem::create_directory(empty);
        prepare_save_directory(assets, empty);
        assert(std::filesystem::is_empty(empty));
        write(assets / "save-files.json", R"({"version":1,"files":[{"name":"SonicDX01.snc",
            "path":"saves/SonicDX01.snc","size":3,"sha256":"wrong"}]})");
        bool rejected = false;
        try { prepare_save_directory(assets, root / "bad-profile"); }
        catch (const std::runtime_error &) { rejected = true; }
        assert(rejected && !std::filesystem::exists(root / "bad-profile"));
        for (const auto &entry : std::filesystem::directory_iterator(root))
            assert(entry.path().filename().string().find(".sadx-save-seed-") != 0);
        std::filesystem::remove_all(root);
        std::cout << "Native save seeding preserves originals, existing progress and empty profiles; failed publication cleans up.\n";
    }
}
