#include "reset_final_parent.hpp"
#include "runtime/native_session.hpp"
#include "runtime/native_display.hpp"
#include "runtime/native_title_scene.hpp"
#include <charconv>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace sadx::native {
// NOTSA: exposes the actual native owner only to this separately built reserved binding.
struct ResetSessionFixture {
    static control::FrameMessageState &frame_message(Session &session) {
        return session.frame_message_;
    }
};
} // namespace sadx::native

namespace {
std::uint32_t input_word(const char *text) {
    std::uint32_t value = 0;
    const std::string input(text);
    const auto parsed = std::from_chars(input.data(), input.data() + input.size(), value);
    if (parsed.ec != std::errc{} || parsed.ptr != input.data() + input.size() || value == 0)
        throw std::invalid_argument("expected a nonzero uint32 frame word");
    return value;
}

void observe_session(const std::filesystem::path &assets, std::uint32_t input) {
    using namespace sadx;
    frame::ModeState mode;
    native::Display display(assets / "system", mode);
    const auto characters = std::make_shared<character_pack::CharacterPack>(
        character_pack::CharacterPack::load(assets / "characters/sonic-selection.actions"));
    native::CharacterTextureAssets textures{};
    unsigned vibration_calls = 0;
    native::Session session(display.output(), characters, display.frame_state(),
                            display.arithmetic(), mode, textures, 0, [&](std::uint32_t port) {
        if (port != 0)
            throw std::runtime_error("unexpected reset vibration port");
        ++vibration_calls;
    });
    const native::TitleAssets title_assets{};
    native::TitleScene title(session, display, title_assets);
    session.attach_title(title);
    auto configuration = audio::normalize_audio_configuration({1, 30, 1, 70});
    session.attach_cleanup(display, configuration);
    auto &frame = native::ResetSessionFixture::frame_message(session);
    auto *identity = &frame;
    frame.work = input;
    std::string boundary;
    try {
        session.complete_menu_messages();
    } catch (const flow::UnrecoveredGameFlowCall &failure) {
        boundary = failure.what();
    }
    if (boundary != "native_pending_text_window" || frame.work != input)
        throw std::runtime_error("nonzero native message boundary was not preserved");
    // This executes the real Session reset and its attached native owners. No callback
    // substitutes the reset algorithm or either frame owner. Empty asset declarations
    // are unused by this prefix; the actual imported CharacterPack is required.
    session.reset_game();
    const bool same_owner = identity == &native::ResetSessionFixture::frame_message(session);
    if (!same_owner || frame.work != 0 || vibration_calls != 1)
        throw std::runtime_error("real reset did not clear the live native message owner");
    std::cout << "{\"case\":\"reserved-native-frame-alias\",\"input\":[" << input
              << "],\"boundary\":\"" << boundary << "\",\"same_owner\":" << same_owner
              << ",\"frame\":" << frame.work << ",\"vibration_calls\":"
              << vibration_calls << "}\n";
}
} // namespace

int main(int argc, char **argv) {
    // Root's separately reviewed runner supplies the frozen input only after admission.
    // Building this executable does not execute or consume a reserved case.
    if (argc != 5)
        return 64;
    try {
        sadx::test::retain_reset_parent(input_word(argv[3]),
                                         input_word(argv[4]));
        observe_session(argv[1], input_word(argv[2]));
    } catch (const std::exception &failure) {
        std::cerr << failure.what() << '\n';
        return 1;
    }
}
