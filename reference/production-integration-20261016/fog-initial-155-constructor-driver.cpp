#include "guest/native_game_reference.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
int main(int argc, char **argv) {
    for (int i = 1; i < argc; ++i) {
        sadx::NativeGameConfig config{};
        if (std::string(argv[i]) != "-") {
            std::ifstream file(argv[i], std::ios::binary);
            auto parsed = sadx::ExecutableImage::parse({std::istreambuf_iterator<char>(file), {}});
            if (!parsed) throw std::runtime_error("constructor image");
            config.executable = std::make_shared<const sadx::ExecutableImage>(std::move(*parsed));
        }
        const sadx::NativeGame game(config);
        for (const auto &fact : game.fog_facts())
            std::cout << bool(fact) << ':' << fact.value_or(0) << ';';
        std::cout << '\n';
    }
}
