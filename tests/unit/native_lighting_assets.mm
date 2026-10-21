#include "apps/native_lighting_assets.hpp"
#include <fstream>
#include <stdexcept>
#include <CommonCrypto/CommonDigest.h>
#include <iomanip>
#include <sstream>

static std::string digest(const void *data, std::size_t size) {
    unsigned char bytes[CC_SHA256_DIGEST_LENGTH];
    CC_SHA256(data, static_cast<CC_LONG>(size), bytes);
    std::ostringstream result;
    for (auto byte : bytes)
        result << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(byte);
    return result.str();
}

int main(int argc, char **argv) {
    if (argc != 3)
        throw std::runtime_error("native-lighting-assets-test IMPORTED_DIRECTORY OUTPUT");
    @autoreleasepool {
        const auto tables = sadx::native::app::load_lighting_assets(argv[1]);
        static_assert(sizeof(sadx::lighting::Environment) == 48);
        static_assert(sizeof(sadx::lighting::PaletteEnvironment) == 96);
        const auto evidence = sadx::native::app::read_metadata(
            std::filesystem::path(argv[1]) / "stage-lighting-evidence.json");
        const auto records = sadx::native::app::array(evidence[@"tables"], 4, "lighting proof");
        const std::array<std::pair<const void *, std::size_t>, 4> source{{
            {&tables.default_stage, sizeof(tables.default_stage)},
            {&tables.default_character, sizeof(tables.default_character)},
            {tables.stages.data(), sizeof(tables.stages)},
            {tables.palettes.data(), sizeof(tables.palettes)}}};
        for (std::size_t index = 0; index < source.size(); ++index) {
            const auto proof = sadx::native::app::dictionary(records[index], "lighting proof");
            if (digest(source[index].first, source[index].second) !=
                sadx::native::app::string(proof[@"sha256"], "source table digest"))
                throw std::runtime_error("native parsed lighting differs from original table bits");
        }
        std::ofstream output(argv[2], std::ios::binary);
        for (const auto *record : {&tables.default_stage, &tables.default_character})
            output.write(reinterpret_cast<const char *>(record), sizeof(*record));
        output.write(reinterpret_cast<const char *>(tables.stages.data()), sizeof(tables.stages));
        output.write(reinterpret_cast<const char *>(tables.palettes.data()), sizeof(tables.palettes));
        if (!output)
            throw std::runtime_error("could not write native lighting verification data");
    }
}
