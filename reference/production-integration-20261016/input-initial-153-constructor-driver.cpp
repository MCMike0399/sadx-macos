#include "guest/native_game_reference.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
std::shared_ptr<const sadx::ExecutableImage> image(const char *path) {
    if (std::string(path) == "-") return {};
    std::ifstream file(path, std::ios::binary);
    auto parsed = sadx::ExecutableImage::parse({std::istreambuf_iterator<char>(file), {}});
    if (!parsed) throw std::runtime_error("constructor image");
    return std::make_shared<const sadx::ExecutableImage>(std::move(*parsed));
}
int main(int argc, char **argv) {
    for (int i = 1; i + 1 < argc; i += 2) {
        sadx::NativeGameConfig config{};
        config.executable = image(argv[i]);
        config.input_mod_dll = image(argv[i + 1]);
        const sadx::NativeGame game(config);
        const auto write = [](const auto &facts) {
            for (const auto &fact : facts)
                std::cout << bool(fact) << ':' << unsigned(fact.value_or(0)) << ';';
        };
        write(game.input_data_facts());
        write(game.input_resource_facts());
        std::cout << '\n';
    }
}
