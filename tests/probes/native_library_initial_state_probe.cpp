#include "library_initial_state_probe.hpp"
#include <cstddef>
#include <cstring>
namespace sadx::test {
std::array<std::uint32_t, 28> library_initial_words(const LibraryInitialState &values) {
    std::array<std::uint32_t, 28> words{};
    unsigned next = 0;
    words[next++] = static_cast<std::uint32_t>(values.buffer_counts[0]);
    words[next++] = static_cast<std::uint32_t>(values.buffer_counts[1]);
    words[next++] = static_cast<std::uint32_t>(values.buffer_counts[2]);
    words[next++] = static_cast<std::uint32_t>(values.buffer_counts[3]);
    words[next++] = static_cast<std::uint32_t>(values.buffer_counts[4]);
    words[next++] = static_cast<std::uint32_t>(values.buffer_counts[5]);
    words[next++] = static_cast<std::uint32_t>(values.buffer_counts[6]);
    words[next++] = static_cast<std::uint32_t>(values.buffer_counts[7]);
    words[next++] = static_cast<std::uint32_t>(values.buffer_counts[8]);
    words[next++] = static_cast<std::uint32_t>(values.windowed);
    words[next++] = static_cast<std::uint32_t>(values.back_buffer_width);
    words[next++] = static_cast<std::uint32_t>(values.back_buffer_height);
    std::memcpy(&words[next++], &values.scale, sizeof(float));
    std::memcpy(&words[next++], &values.aspect, sizeof(float));
    words[next++] = static_cast<std::uint32_t>(values.configuration.screen_flags);
    words[next++] = static_cast<std::uint32_t>(values.configuration.setup_argument);
    words[next++] = static_cast<std::uint32_t>(values.configuration.buffer_counts[0]);
    words[next++] = static_cast<std::uint32_t>(values.configuration.buffer_counts[1]);
    words[next++] = static_cast<std::uint32_t>(values.configuration.buffer_counts[2]);
    words[next++] = static_cast<std::uint32_t>(values.configuration.buffer_counts[3]);
    words[next++] = static_cast<std::uint32_t>(values.configuration.buffer_counts[4]);
    words[next++] = static_cast<std::uint32_t>(values.configuration.buffer_counts[5]);
    words[next++] = static_cast<std::uint32_t>(values.configuration.buffer_counts[6]);
    words[next++] = static_cast<std::uint32_t>(values.configuration.buffer_counts[7]);
    words[next++] = static_cast<std::uint32_t>(values.configuration.buffer_counts[8]);
    words[next++] = static_cast<std::uint32_t>(values.configuration.high_precision_depth_request);
    words[next++] = static_cast<std::uint32_t>(values.configuration.back_buffer_width);
    words[next++] = static_cast<std::uint32_t>(values.configuration.back_buffer_height);
    return words;
}
LibraryInitialState varied_library_initial_state() {
    auto values = library_initial_state();
    values.buffer_counts[0] = -1234567;
    values.buffer_counts[1] = -2469134;
    values.buffer_counts[2] = -3703701;
    values.buffer_counts[3] = -4938268;
    values.buffer_counts[4] = -6172835;
    values.buffer_counts[5] = -7407402;
    values.buffer_counts[6] = -8641969;
    values.buffer_counts[7] = -9876536;
    values.buffer_counts[8] = -11111103;
    values.windowed = -12345670;
    values.back_buffer_width = -13580237;
    values.back_buffer_height = -14814804;
    {
        const std::uint32_t word = 0x80000000U;
        std::memcpy(&values.scale, &word, sizeof(float));
    }
    {
        const std::uint32_t word = 0x7fa12345U;
        std::memcpy(&values.aspect, &word, sizeof(float));
    }
    values.configuration.screen_flags = 1234567U;
    values.configuration.setup_argument = 2469134U;
    values.configuration.buffer_counts[0] = -3703701;
    values.configuration.buffer_counts[1] = -4938268;
    values.configuration.buffer_counts[2] = -6172835;
    values.configuration.buffer_counts[3] = -7407402;
    values.configuration.buffer_counts[4] = -8641969;
    values.configuration.buffer_counts[5] = -9876536;
    values.configuration.buffer_counts[6] = -11111103;
    values.configuration.buffer_counts[7] = -12345670;
    values.configuration.buffer_counts[8] = -13580237;
    values.configuration.high_precision_depth_request = -14814804;
    values.configuration.back_buffer_width = -16049371;
    values.configuration.back_buffer_height = -17283938;
    return values;
}
} // namespace sadx::test
extern "C" std::size_t sadx_native_library_initial_state(unsigned variant, std::uint32_t *output,
                                                         std::size_t capacity) {
    if (variant > 1 || !output || capacity < 28)
        return 0;
    const auto words = sadx::test::library_initial_words(
        variant == 0 ? sadx::library_initial_state() : sadx::test::varied_library_initial_state());
    for (std::size_t i = 0; i < words.size(); ++i)
        output[i] = words[i];
    return words.size();
}
