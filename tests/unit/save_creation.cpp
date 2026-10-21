#include "game/save_creation.hpp"
#include "game/save_completion.hpp"
#include <cassert>
namespace {
struct Calls final : sadx::file_select::SaveCompletionCalls {
    int transitions = 0;
    void request_dialog(std::int32_t) override { assert(false); }
    sadx::TaskHandle create_save_warning() override {
        assert(false);
        return {};
    }
    std::int8_t dialog_selection() override {
        assert(false);
        return -1;
    }
    void close_save_warning(sadx::TaskHandle) override { assert(false); }
    void transition_menu(sadx::title_menu::MenuMode next) override {
        assert(next == sadx::title_menu::MenuMode::CharacterSelect);
        ++transitions;
    }
    void prepare_new_save() override { assert(false); }
    void apply_save() override { assert(false); }
};
} // namespace
int main() {
    std::int32_t card = 0;
    std::uint32_t timer = 42, movie = 1;
    std::int16_t rings = 99;
    std::uint16_t mode = 0;
    sadx::bootstrap::State story({card, timer, rings, movie, mode});
    story.sequence_variables.fill(255);
    story.sequences[0].emerald = 99;
    story.completed_stages.fill(255);
    story.cutscene_flags.fill(255);
    sadx::save::Data data;
    sadx::save::prepare_new(story, data, rings);
    assert(rings == 0 && data.vibration == 1 && data.last_stage == 100);
    for (std::size_t i = 0; i < story.sequence_variables.size(); ++i)
        assert(story.sequence_variables[i] == (i == 65 ? 1 : 0));
    for (auto flag : story.cutscene_flags)
        assert(flag == 0);
    assert(story.sequences[0].emerald == 0);
    assert(data.checksum == sadx::save::checksum(sadx::save::encode(data)));
    std::int32_t disabled = 0;
    std::uint8_t ready = 0;
    int captures = 0;
    assert(sadx::save::request_write(disabled, ready, [&] { ++captures; }) == 0 && ready == 1 &&
           captures == 0);
    disabled = 1;
    ready = 77;
    sadx::save::request_write(disabled, ready, [&] { ++captures; });
    assert(ready == 77 && captures == 1);
    // Actual repeated wait frames consume only a successful writer-owned SaveOK.
    sadx::title_menu::FileSelectWork work{};
    work.submode = 12;
    work.presentation.next = sadx::title_menu::MenuMode::CharacterSelect;
    std::uint8_t alert = 0, ok = 1, fade = 1, exec = 0, flag = 1, selected = 3;
    std::uint16_t backup = 0;
    std::int32_t phase = 0;
    disabled = 0;
    sadx::TaskHandle warning;
    sadx::file_select::SaveCompletionState state{alert, ok,       fade,   phase,    ready,  exec,
                                                 flag,  selected, backup, disabled, warning};
    Calls calls;
    for (int frame = 0; frame < 20 && work.submode != 10; ++frame)
        assert(sadx::file_select::update_save_completion(work, state, calls));
    assert(work.submode == 10 && calls.transitions == 1 && backup == 3 && ok == 0 && fade == 0 &&
           alert == 0);
    assert(!sadx::file_select::update_save_completion(work, state, calls));
}
