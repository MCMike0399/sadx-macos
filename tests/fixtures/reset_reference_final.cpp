#include "reset_final_parent.hpp"
#if __has_include("guest/game_reset_reference.hpp")
#include "guest/game_reset_reference.hpp"
#include "guest/game_reset_leaves_reference.hpp"
using FrameFact = sadx::reference::FrameMessageFact;
#define RESET_CURRENT_BINDING 1
#else
#include "game/game_reset.hpp"
#include "game/game_reset_leaves.hpp"
using FrameFact = std::optional<std::uint32_t>;
#endif
#include "game/audio_playback.hpp"
#include <charconv>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
std::uint32_t word(const char *text) {
    std::uint32_t value = 0;
    const std::string input(text);
    const auto parsed = std::from_chars(input.data(), input.data() + input.size(), value);
    if (parsed.ec != std::errc{} || parsed.ptr != input.data() + input.size())
        throw std::invalid_argument("expected uint32 input");
    return value;
}
template<class Optional> void fact(std::ostream &out, const Optional &value) {
    out << '[' << bool(value) << ',' << (value ? std::uint32_t(*value) : 0) << ']';
}
std::string reset_state(const sadx::GameResetState &state) {
    std::ostringstream out;
    out << '[';
#ifdef RESET_CURRENT_BINDING
    fact(out, state.frame_message);
    out << ',';
    fact(out, state.editor);
#else
    fact(out, state.word_at_3abdf80);
    out << ',';
    fact(out, state.word_at_3b29d08);
#endif
    out << ',';
    fact(out, state.heap_at_3b290d0);
    out << ',';
    fact(out, state.heap_at_3b0e330);
    out << ',';
    fact(out, state.rng.seed_at_2bcbdf8);
    return out.str() + ']';
}
void call(std::ostream &out, const sadx::GameResetCall &value) {
    out << '[' << value.source_call << ',' << value.callee << ',';
    fact(out, value.argument);
    out << ']';
}
void reset_case(const std::string &id, const std::vector<std::uint32_t> &input) {
    const unsigned failure = id == "reserved-media-failure" ? 9 :
                             id == "reserved-task-failure" ? 14 : 0;
    FrameFact frame = input[0];
    sadx::CrtRandState random{input[4]};
    sadx::GameResetState state{frame, static_cast<std::uint16_t>(input[1]),
                               input[2], input[3], random};
    std::vector<sadx::GameResetCall> calls;
    std::vector<std::string> before_calls;
    sadx::GameResetRuntime runtime{state, [&](const auto &operation) {
        calls.push_back(operation);
        before_calls.push_back(reset_state(state));
        if (operation.callee == 0x780fb0 && operation.argument == input[2]) {
            if (id == "reserved-buffer-replacement")
                state.heap_at_3b0e330 = input[2];
            if (id == "reserved-later-buffer-unknown")
                state.heap_at_3b0e330.reset();
        }
        return sadx::GameLogicOwnerResult{
            failure && calls.size() == failure ? sadx::GameLogicOwnerState::failed
                                               : sadx::GameLogicOwnerState::completed,
            failure && calls.size() == failure ? 0x123U : 0U, {}};
    }};
    const auto result = sadx::run_game_reset_40bf40(runtime);
    std::cout << "\"result\":[" << unsigned(result.failure) << ',' << result.source_boundary
              << ',' << result.nested_source_boundary << ',' << result.completed
              << "],\"pending\":";
    if (result.pending_call)
        call(std::cout, *result.pending_call);
    else
        std::cout << "null";
    std::cout << ",\"state\":" << reset_state(state) << ",\"calls\":[";
    for (std::size_t index = 0; index < calls.size(); ++index) {
        if (index)
            std::cout << ',';
        std::cout << "[";
        call(std::cout, calls[index]);
        std::cout << ',' << before_calls[index] << ']';
    }
    std::cout << ']';
}
void level_case(const std::vector<std::uint32_t> &input) {
    std::optional<std::uint32_t> depth = 1, queue = 2, ready = 0;
    std::optional<std::int32_t> timing = 7;
    std::array<std::optional<std::uint32_t>, 36> records{};
    sadx::ResetLeafState state{{}, {}, depth, queue, {}, {}, timing, input[5], records,
                               {}, {}, ready, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
                               {}, {}, {}, {}};
    state.set_files_at_3abdf40.fill(0);
    state.cam_files_at_3abdf28.fill(0);
    state.set_files_at_3abdf40[0] = input[2];
    state.set_files_at_3abdf40[1] = input[2];
    state.cam_files_at_3abdf28[0] = input[3];
    std::vector<sadx::ResetLeafCall> calls;
    const auto result = sadx::run_reset_leaf(0x4228f0, input[6], state, [&](const auto &operation) {
        calls.push_back(operation);
        if (operation.callee == 0x780fb0)
            state.callback_at_3b291b4 = input[5] + 1;
        const bool failed_callback = operation.callee == input[5] + 1;
        return sadx::GameLogicOwnerResult{
            failed_callback ? sadx::GameLogicOwnerState::failed :
                              sadx::GameLogicOwnerState::completed,
            failed_callback ? 0x123U : 0U, {}};
    });
    std::cout << "\"result\":[" << unsigned(result.outcome) << ',' << result.source_boundary
              << ',' << unsigned(result.nested.state) << ','
              << result.nested.source_boundary << ',';
    fact(std::cout, result.eax);
    std::cout << "],\"state\":[";
    fact(std::cout, state.word_at_3c4e458);
    std::cout << ',';
    fact(std::cout, state.word_at_3c4e45c);
    std::cout << ',';
    fact(std::cout, state.callback_at_3b291b4);
    for (const auto &value : state.set_files_at_3abdf40) {
        std::cout << ',';
        fact(std::cout, value);
    }
    for (const auto &value : state.cam_files_at_3abdf28) {
        std::cout << ',';
        fact(std::cout, value);
    }
    std::cout << "],\"debug_bytes\":[";
    for (std::size_t index = 0; index < state.debug_text_at_38f6d08.size(); ++index) {
        if (index)
            std::cout << ',';
        std::cout << unsigned(static_cast<unsigned char>(state.debug_text_at_38f6d08[index]));
    }
    std::cout << "],\"calls\":[";
    for (std::size_t index = 0; index < calls.size(); ++index) {
        if (index)
            std::cout << ',';
        const auto &operation = calls[index];
        std::cout << '[' << operation.source_call << ',' << operation.callee << ',';
        fact(std::cout, operation.this_object);
        std::cout << ',';
        fact(std::cout, operation.argument);
        std::cout << ']';
    }
    std::cout << ']';
}

struct Player final : sadx::audio::StreamPlayer {
    std::vector<unsigned> &trace;
    unsigned identity;
    std::function<void()> on_stop;
    Player(std::vector<unsigned> &events, unsigned id) : trace(events), identity(id) {}
    void stop() override {
        trace.push_back(identity * 10 + 1);
        if (on_stop)
            on_stop();
    }
    void reset() override { trace.push_back(identity * 10 + 2); }
};
void media_case(const std::vector<std::uint32_t> &input) {
    std::cout << "\"subobservations\":[";
    for (unsigned clear_replacement = 0; clear_replacement < 2; ++clear_replacement) {
        std::vector<unsigned> trace;
        sadx::audio::MediaState media;
        auto first = std::make_unique<Player>(trace, input[4]);
        auto second = std::make_unique<Player>(trace, input[5]);
        std::unique_ptr<sadx::audio::StreamPlayer> retained_stopped;
        first->on_stop = [&] {
            // Keep the stopped object alive through its own method, as in the existing unit.
            retained_stopped = std::move(media.voice_player);
            if (!clear_replacement)
                media.voice_player = std::move(second);
        };
        media.voice_player = std::move(first);
        media.music_open = input[0];
        media.music_status = input[1];
        media.shared_status = input[2];
        media.music_looping = input[3];
        const auto result = sadx::audio::close_media(media, input[6]);
        if (clear_replacement)
            std::cout << ',';
        std::cout << '[' << clear_replacement << ',' << unsigned(result.failure) << ','
                  << bool(media.voice_player) << ',' << media.music_open << ','
                  << media.music_status
                  << ',' << media.shared_status << ',' << media.music_looping << ",[";
        for (std::size_t index = 0; index < trace.size(); ++index) {
            if (index)
                std::cout << ',';
            std::cout << trace[index];
        }
        std::cout << "]]";
    }
    std::cout << ']';
}
} // namespace

int main(int argc, char **argv) {
    if (argc != 11)
        return 64;
    try {
        sadx::test::retain_reset_parent(word(argv[9]),
                                         word(argv[10]));
        const std::string id = argv[1];
        if (id != "reserved-media-failure" && id != "reserved-task-failure" &&
            id != "reserved-buffer-replacement" && id != "reserved-later-buffer-unknown" &&
            id != "reserved-level-substitution" && id != "reserved-media-reacquire")
            throw std::invalid_argument("unknown reserved reference case");
        std::vector<std::uint32_t> input;
        for (int index = 2; index < 9; ++index)
            input.push_back(word(argv[index]));
        if (id != "reserved-media-reacquire" && input[1] > 65535)
            throw std::invalid_argument("editor exceeds uint16");
        std::cout << "{\"case\":" << std::quoted(id) << ",\"input\":[";
        for (std::size_t index = 0; index < input.size(); ++index) {
            if (index)
                std::cout << ',';
            std::cout << input[index];
        }
        std::cout << "],";
        if (id == "reserved-level-substitution")
            level_case(input);
        else if (id == "reserved-media-reacquire")
            media_case(input);
        else
            reset_case(id, input);
        std::cout << "}\n";
    } catch (const std::exception &failure) {
        std::cerr << failure.what() << '\n';
        return 1;
    }
}
