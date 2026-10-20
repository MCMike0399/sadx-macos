#include "game/library_initial_state.hpp"
#include "guest/game_library_init_reference.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace {
void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
} // namespace
int main() {
    try {
        auto values = sadx::library_initial_state();
        require(values.buffer_counts.back() == 12000 &&
                    values.configuration.buffer_counts.back() == 32000,
                "cold count precedes library configuration copy");
        sadx::GameLibraryInitState state{};
        for (unsigned i = 0; i < 9; ++i)
            state.words_at_389d864[i] = static_cast<std::uint32_t>(values.buffer_counts[i]);
        state.word_at_38a5dc4 = static_cast<std::uint32_t>(values.windowed);
        state.word_at_38a5dc8 = static_cast<std::uint32_t>(values.back_buffer_width);
        state.word_at_38a5dcc = static_cast<std::uint32_t>(values.back_buffer_height);
        state.scale_at_3d0f800 = values.scale;
        state.aspect_at_3d0f9c0 = values.aspect;
        sadx::X87Context fp(127);
        std::vector<sadx::GameLibraryCall> calls;
        sadx::GameLibraryInitRuntime runtime{
            state,
            0x892940,
            [&](std::uint32_t address) -> std::optional<std::uint32_t> {
                // Explicit fixture text/path pointers and independently owned live settings.
                if (address >= 0x892940 && address < 0x892950)
                    return 100 + (address - 0x892940);
                if (address == 0x892950)
                    return 0;
                if (address == 0x89295c)
                    return 2;
                if (address == 0x892954)
                    return static_cast<std::uint32_t>(values.configuration.screen_flags);
                if (address == 0x892958)
                    return static_cast<std::uint32_t>(values.configuration.setup_argument);
                if (address == 0x892968)
                    return static_cast<std::uint32_t>(values.configuration.buffer_counts[0]);
                if (address == 0x89296c)
                    return static_cast<std::uint32_t>(values.configuration.buffer_counts[1]);
                if (address == 0x892970)
                    return static_cast<std::uint32_t>(values.configuration.buffer_counts[2]);
                if (address == 0x892974)
                    return static_cast<std::uint32_t>(values.configuration.buffer_counts[3]);
                if (address == 0x892978)
                    return static_cast<std::uint32_t>(values.configuration.buffer_counts[4]);
                if (address == 0x89297c)
                    return static_cast<std::uint32_t>(values.configuration.buffer_counts[5]);
                if (address == 0x892980)
                    return static_cast<std::uint32_t>(values.configuration.buffer_counts[6]);
                if (address == 0x892984)
                    return static_cast<std::uint32_t>(values.configuration.buffer_counts[7]);
                if (address == 0x892988)
                    return static_cast<std::uint32_t>(values.configuration.buffer_counts[8]);
                if (address == 0x89298c)
                    return static_cast<std::uint32_t>(
                        values.configuration.high_precision_depth_request);
                if (address == 0x892990)
                    return static_cast<std::uint32_t>(values.configuration.back_buffer_width);
                if (address == 0x892994)
                    return static_cast<std::uint32_t>(values.configuration.back_buffer_height);
                return std::nullopt;
            },
            [](std::uint32_t address) -> std::optional<std::uint32_t> {
                return address == 0x3d12788 ? 640 : 480;
            },
            fp,
            [&](const sadx::GameLibraryCall &call) -> sadx::GameLibraryOwnerResult {
                calls.push_back(call);
                return {{sadx::GameLogicOwnerState::completed, 0, {}}, 0};
            }};
        for (const int request : {0, 2, 1}) {
            calls.clear();
            values.configuration.high_precision_depth_request = request;
            const auto result = sadx::run_library_init_781070(runtime);
            require(bool(result) && result.tries_left == 8,
                    "native defaults feed existing library consumer");
            require(state.words_at_389d864.back() == 32000 && state.word_at_38a5dc4 == 0 &&
                        state.word_at_38a5dc8 == 640 && state.word_at_38a5dcc == 480 &&
                        state.scale_at_3d0f800 == 1.0F && state.aspect_at_3d0f9c0 == 0.75F,
                    "configuration and viewport overwrite cold values");
            const auto high_precision =
                std::count_if(calls.begin(), calls.end(),
                              [](const auto &call) { return call.callee == 0x794050; });
            require(high_precision == (request == 1 ? 1 : 0),
                    "high precision request compares exactly one");
            const auto screen = std::find_if(calls.begin(), calls.end(), [](const auto &call) {
                return call.callee == 0x780c30;
            });
            require(screen != calls.end() && screen->arguments[0] == 49 &&
                        screen->arguments[1] == 5 && screen->arguments[2] == 2,
                    "native numeric screen settings preserve separate live timing multiplier");
            require(calls.back().callee == 0x789c50, "library consumer reaches final focus owner");
        }
        std::cout << "Library defaults: cold/configuration counts, display values and existing "
                     "consumer passed.\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
