#include "game/main_menu.hpp"
#include "game/save_file_list.hpp"
#include "cpu/x87_math.hpp"
#include <algorithm>
#include <cstring>
#include <vector>

namespace m = sadx::main_menu;
namespace {
float number(std::uint32_t v) { float x; std::memcpy(&x, &v, 4); return x; }
std::uint32_t bits(float v) { std::uint32_t x; std::memcpy(&x, &v, 4); return x; }
struct Calls final : m::UpdateCalls, m::DrawCalls, m::ControlCalls {
    const std::uint32_t *in;
    std::vector<std::uint32_t> trace;
    m::Work &work;
    sadx::save::Data &save;
    unsigned text_reads = 0, voice_reads = 0;
    Calls(const std::uint32_t *i, m::Work &w, sadx::save::Data &s) : in(i), work(w), save(s) {}
    void record(std::uint32_t kind, std::uint32_t value = 0) {
        trace.push_back(kind); trace.push_back(value);
    }
    std::int8_t text_language() override {
        const auto v = static_cast<std::int8_t>(in[24 + text_reads++ % 4]); record(1, static_cast<std::uint32_t>(v)); return v;
    }
    std::int8_t voice_language() override {
        const auto v = static_cast<std::int8_t>(in[28 + voice_reads++ % 4]); record(2, static_cast<std::uint32_t>(v)); return v;
    }
    void load_mode_textures(m::Mode mode) override { record(3, static_cast<std::uint32_t>(mode)); }
    void request_menu_music(std::int32_t cue) override { record(4, static_cast<std::uint32_t>(cue)); }
    std::uint32_t fade_out_color(float x) override { record(5, bits(x)); return bits(x) ^ 0x7f123456U; }
    std::uint32_t fade_in_color(float x) override { record(6, bits(x)); return bits(x) ^ 0x40123456U; }
    void update_controls(m::Work &) override { record(7); work.submode = static_cast<std::int32_t>(in[32]); }
    void release_mode_textures(m::Mode mode) override { record(8, static_cast<std::uint32_t>(mode)); }
    void default_blending() override { record(9); }
    void set_base_color(std::uint32_t x) override { record(10, x); }
    void set_material(std::uint32_t x) override { record(11, x); }
    void select_texture(m::TextureList x) override { record(12, static_cast<std::uint32_t>(x)); }
    void draw_scaled(std::uint32_t i, float x, float y, float z, float sx, float sy) override {
        trace.insert(trace.end(), {13, i, bits(x), bits(y), bits(z), bits(sx), bits(sy)});
    }
    bool query(unsigned kind, unsigned index) { record(kind, in[index]); return in[index] != 0; }
    bool trial_enabled() override { return query(21, 40); }
    bool mission_enabled() override { return query(22, 41); }
    bool optional_entry_enabled() override { return query(23, 42); }
    void open_dialog(m::Dialog d, std::int8_t cursor, const std::array<std::int8_t, 3> &disabled,
                     bool quick) override {
        std::uint32_t packed = 0;
        for (unsigned n = 0; n < disabled.size(); ++n) packed |= static_cast<std::uint32_t>(static_cast<std::uint8_t>(disabled[n])) << (8 * n);
        trace.insert(trace.end(), {24, static_cast<std::uint32_t>(d), static_cast<std::uint8_t>(cursor), packed, quick ? 1U : 0U});
    }
    std::int8_t dialog_result() override { record(25, in[43]); return static_cast<std::int8_t>(in[43]); }
    bool trial_stages_enabled() override { return query(26, 44); }
    bool minigames_enabled() override { return query(27, 45); }
    void prepare_character_select(std::int32_t x) override { record(28, static_cast<std::uint32_t>(x)); }
    void transition_menu(m::Mode x) override { record(29, static_cast<std::uint32_t>(x)); }
    std::int32_t music_state() override { record(30, in[46]); return static_cast<std::int32_t>(in[46]); }
    std::int16_t dialog_idle_frames() override { record(31, static_cast<std::uint16_t>(in[47])); return static_cast<std::int16_t>(in[47]); }
    void reset_dialog_idle_frames() override { record(32); }
    bool close_dialog() override { return query(33, 48); }
    void apply_save() override { record(34, save.last_character); if (in[49] != 0xffffffffU) save.checksum = in[49]; }
};
}
extern "C" void sadx_native_main_menu_probe(const std::uint32_t *in, std::uint32_t *out,
                                            const std::uint8_t *mapping) {
    std::fill_n(out, 1024, 0);
    m::Work work{};
    work.presentation.phase = static_cast<m::Phase>(in[1]);
    work.presentation.previous = static_cast<m::Mode>(in[2]);
    work.presentation.next = static_cast<m::Mode>(in[3]);
    work.next_sub = static_cast<m::Mode>(in[4]);
    work.presentation.depth = number(in[5]); work.presentation.fade_progress = number(in[6]);
    work.presentation.color = in[7]; work.submode = static_cast<std::int32_t>(in[8]);
    work.selected_menu = static_cast<std::int32_t>(in[9]);
    work.frames = static_cast<std::int16_t>(in[10]); work.wake = static_cast<std::uint8_t>(in[10] >> 16);
    m::Sequence sequence{};
    sequence.mode = static_cast<m::Mode>(in[11]); sequence.old_mode = static_cast<m::Mode>(in[12]);
    sequence.fixed_mode = static_cast<m::Mode>(in[13]); sequence.return_value = static_cast<std::int32_t>(in[23]);
    sequence.waiting = static_cast<std::uint8_t>(in[14]); sequence.fade_end = static_cast<std::uint8_t>(in[14] >> 8);
    sequence.fade_out = static_cast<std::uint8_t>(in[14] >> 16); sequence.fade_out_secondary = static_cast<std::uint8_t>(in[14] >> 24);
    auto text = static_cast<std::int32_t>(in[16]), voice = static_cast<std::int32_t>(in[17]);
    auto variant = static_cast<std::uint8_t>(in[18]), count = static_cast<std::uint8_t>(in[19]);
    sadx::title_menu::TitleWork title{}; title.next = static_cast<m::Mode>(in[20]);
    m::Work canonical{}; canonical.presentation.previous = static_cast<m::Mode>(in[21]);
    m::InitState state{text, voice, variant, count, title, in[22] ? work : canonical};
    sadx::save::EncodedData save_bytes{}, backup_bytes{};
    for (unsigned n = 0; n < save_bytes.size(); ++n) { save_bytes[n] = static_cast<std::uint8_t>(n * 17 + in[50]); backup_bytes[n] = static_cast<std::uint8_t>(n * 29 + in[51]); }
    auto save = sadx::file_select::decode_save(save_bytes), backup = sadx::file_select::decode_save(backup_bytes);
    Calls calls(in, work, save); sadx::X87Context fp{static_cast<std::uint16_t>(in[33])};
    auto sx = number(in[34]), sy = number(in[35]); auto window = in[36];
    sadx::ui_scale::State scaling(sx,sy);
    m::DrawState view{sx, sy, window, in[52], in[53]};
    if (in[0] == 0) m::initialize(work, state, calls);
    else if (in[0] == 1) m::update(work, sequence, state, in[15], calls, fp);
    else if (in[0] == 2) m::draw_background(work.presentation.depth, view, calls, fp);
    else if (in[0] == 3) m::draw(work, view, calls, fp);
    else if (in[0] == 5) {
        try { m::draw_profiled(work, view, calls, fp, scaling); sadx::ui_scale::require_message_forwarding(scaling); }
        catch (const std::out_of_range &) { out[30] = 2; }
    }
    else if (in[0] == 4) {
        auto ready = static_cast<std::int32_t>(in[37]); auto trial = static_cast<std::uint8_t>(in[38]); auto exit = in[39];
        std::array<std::uint8_t, 7> variants{}; std::copy_n(mapping, 7, variants.begin());
        m::ControlState control{ready, variant, trial, exit, save, backup, sequence, variants};
        try { m::update_controls(work, control, calls); } catch (const std::out_of_range &) { out[30] = 1; }
        out[26] = trial; out[27] = exit;
    }
    const std::uint32_t words[]{static_cast<std::uint32_t>(work.presentation.phase), static_cast<std::uint32_t>(work.presentation.previous), static_cast<std::uint32_t>(work.presentation.next), static_cast<std::uint32_t>(work.next_sub), bits(work.presentation.depth), bits(work.presentation.fade_progress), work.presentation.color, static_cast<std::uint32_t>(work.submode), static_cast<std::uint32_t>(work.selected_menu), static_cast<std::uint16_t>(work.frames) | (static_cast<std::uint32_t>(work.wake) << 16)};
    std::copy_n(words, 10, out);
    out[10] = static_cast<std::uint32_t>(text); out[11] = static_cast<std::uint32_t>(voice); out[12] = variant; out[13] = count;
    out[14] = static_cast<std::uint32_t>(title.next); out[15] = static_cast<std::uint32_t>((in[22] ? work : canonical).presentation.previous);
    out[16] = static_cast<std::uint32_t>(sequence.fixed_mode); out[17] = static_cast<std::uint32_t>(sequence.return_value);
    out[18] = sequence.waiting | (static_cast<std::uint32_t>(sequence.fade_end) << 8) | (static_cast<std::uint32_t>(sequence.fade_out) << 16) | (static_cast<std::uint32_t>(sequence.fade_out_secondary) << 24);
    out[19] = window; out[31] = fp.exception_flags(); out[32] = static_cast<std::uint32_t>(calls.trace.size());
    std::copy(calls.trace.begin(), calls.trace.end(), out + 40);
    const auto current_bytes = sadx::save::encode(save), copied_bytes = sadx::save::encode(backup);
    std::memcpy(out + 128, current_bytes.data(), current_bytes.size());
    std::memcpy(out + 476, copied_bytes.data(), copied_bytes.size());
}
