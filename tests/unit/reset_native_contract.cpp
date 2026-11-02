#include "game/game_cleanup.hpp"
#include "guest/reset_buffer_tokens.hpp"
#include "guest/reset_control_reference.hpp"
#include <cassert>
#include <stdexcept>
#include <vector>

namespace {
struct Stop {};
void replacement(void *context) { ++*static_cast<unsigned *>(context); }
}
int main() {
    using namespace sadx::cleanup;
    for (unsigned failure = 0; failure < 30; ++failure) {
        std::uint32_t frame = 99, seed = 1;
        std::uint16_t editor = 32768;
        int first_token = 0, replacement_token = 0;
        void *first = &first_token, *second = first;
        State state{frame, editor, first, second, seed};
        unsigned calls = 0;
        std::vector<void *> released;
        const auto call = [&] { if (++calls == failure) throw Stop{}; };
        Services operations;
        operations.resume_voice_volume = call;
        operations.set_depth_compare_mode = [&](auto mode) { assert(mode == 1); call(); };
        operations.stop_vibration = [&](auto port) { assert(port == 0); call(); };
        operations.set_blank_queue_mode = call;
        operations.reset_song_state = call;
        operations.reset_current_voice = call;
        operations.stop_all_sounds = call;
        operations.stop_pcm = [&](auto channel) { assert(channel == -1); call(); };
        operations.close_media = [&](auto) { call(); };
        operations.setup_screen = call;
        operations.display_message = [&](auto *message) { assert(message == nullptr); call(); };
        operations.initialize_heap = call;
        operations.initialize_tasks = call;
        unsigned level = 0;
        operations.run_level_destructor = [&](auto mode) { assert(mode == level++); call(); };
        operations.reset_loop_state = call;
        operations.release_buffer = [&](void *buffer) {
            released.push_back(buffer);
            call();
            // A live later slot changes while the first slot is still published.
            if (released.size() == 1) {
                assert(first == &first_token);
                second = &replacement_token;
            }
        };
        operations.enable_fog = call;
        bool stopped = false;
        try { reset(state, operations); } catch (const Stop &) { stopped = true; }
        assert(stopped == (failure > 0 && failure <= 28));
        if (failure > 0 && failure <= 3) {
            assert(frame == 99 && editor == 32768 && seed == 1);
        } else {
            assert(frame == 0 && editor == 0);
        }
        if (stopped) assert(seed == 1);
        else assert(seed == 3735882923U && first == nullptr && second == nullptr);
        if (failure == 26) assert(first == &first_token && second == &first_token);
        if (failure == 27) assert(first == nullptr && second == &replacement_token);
        if (released.size() > 1) assert(released[1] == &replacement_token);
    }
    // Live views retain reached validation after known -> unknown imports.
    sadx::reference::FrameMessageFact frame_fact = 123;
    frame_fact.reset();
    bool frame_stopped = false;
    try { (void)*frame_fact; } catch (const std::logic_error &) { frame_stopped = true; }
    assert(frame_stopped && !frame_fact.fact());
    sadx::reference::EditorFact editor_fact(std::uint16_t{32768});
    auto editor_view = editor_fact.borrow();
    assert(editor_view.signed_status() == -32768);
    editor_fact.reset();
    bool editor_stopped = false;
    try { editor_view.signed_status(); }
    catch (const sadx::control::EditorReadUnavailable &) { editor_stopped = true; }
    assert(editor_stopped);
    editor_fact = std::uint16_t{65535};
    assert(editor_view.signed_status() == -1);
    // Tokens are representation-only; a foreign real pointer must be rejected.
    sadx::reference::ResetBufferTokens tokens;
    assert(tokens.host(0) == nullptr);
    auto *token = tokens.host(42);
    assert(tokens.host(42) == token && tokens.guest(token) == 42);
    int foreign = 0;
    bool rejected = false;
    try { tokens.guest(&foreign); } catch (const std::invalid_argument &) { rejected = true; }
    assert(rejected);

    std::uint32_t count = 1;
    auto *count_pointer = &count;
    PlacementEntry *entries = nullptr;
    std::array<void *, 6> sets{}, cameras{};
    sets[0] = token; sets[1] = token;
    LevelDestructor callback = nullptr;
    unsigned invoked = 0;
    void *context = &invoked;
    std::string debug;
    LevelState level{count_pointer, entries, sets, cameras, callback, context, debug};
    unsigned freed = 0;
    run_level_destructor(0, level, [&](void *value) {
        assert(value == token && count_pointer == nullptr);
        ++freed;
        callback = replacement;
    });
    assert(freed == 2 && invoked == 1 && callback == nullptr && context == nullptr);
    assert(debug == "ReleaseCamFile\n");
}
