#include "game/file_select_display.hpp"
#include <algorithm>
#include <cstring>
#include <vector>
namespace {
using U = std::uint32_t;
float number(U value) {
    float result;
    std::memcpy(&result, &value, 4);
    return result;
}
U bits(float value) {
    U result;
    std::memcpy(&result, &value, 4);
    return result;
}
sadx::file_select_display::Tables tables(const U *input) {
    sadx::file_select_display::Tables result{};
    auto p = input + 64;
    for (auto &row : result.frame_rows) {
        row = {number(p[0]),
               number(p[1]),
               number(p[2]),
               p[3],
               p[4],
               static_cast<std::uint8_t>(p[5]),
               static_cast<std::uint8_t>(p[6]),
               static_cast<std::uint8_t>(p[7]),
               static_cast<std::uint8_t>(p[8])};
        p += 9;
    }
    for (unsigned i = 0; i < 8; ++i) {
        result.sky_tiles.push_back({static_cast<std::int16_t>(p[0]),
                                    static_cast<std::int16_t>(p[1]),
                                    static_cast<std::int16_t>(p[2])});
        p += 3;
    }
    const auto copy = [&](auto &values) {
        for (auto &value : values)
            value = static_cast<std::remove_reference_t<decltype(value)>>(*p++);
    };
    copy(result.detail_labels);
    copy(result.unlocked_icons);
    copy(result.locked_icons);
    copy(result.character_names);
    copy(result.stage_names);
    copy(result.sequence_indices);
    copy(result.clear_flags);
    copy(result.digits);
    return result;
}
struct Calls final : sadx::file_select_display::Calls {
    std::vector<U> trace;
    U list = 0, fail = 0, boundaries = 0;
    void boundary() {
        if (++boundaries == fail)
            throw std::runtime_error("injected native resource boundary");
    }
    void event(U kind, std::initializer_list<U> values) {
        trace.push_back(kind);
        trace.push_back(values.size());
        trace.insert(trace.end(), values);
    }
    void select_menu_texture(sadx::file_select_display::MenuTexture id) override {
        list = static_cast<U>(id);
        event(1, {list});
        boundary();
    }
    void select_standard_texture(sadx::menu_draw::StandardTexture id) override {
        list = id == sadx::menu_draw::StandardTexture::Window ? 100 : 101;
        event(1, {list});
        boundary();
    }
    void set_blend(U target, U mode) override {
        event(2, {target, mode});
        boundary();
    }
    sadx::pvr::TextureInfo texture(U index) override {
        event(3, {list, index});
        boundary();
        return {list * 1000 + index, 0, 32 + (index * 7 + list * 3) % 100,
                17 + (index * 3 + list) % 85};
    }
    void draw_texture(const std::array<sadx::pvr::Vertex, 4> &vertices, U global,
                      U alpha) override {
        event(4, {global, alpha});
        trace[trace.size() - 3] = 26;
        for (const auto &v : vertices)
            for (auto value :
                 {bits(v.x), bits(v.y), bits(v.reciprocal_depth), bits(v.u), bits(v.v), v.color})
                trace.push_back(value);
        boundary();
    }
    void reset_material() override {
        event(5, {});
        boundary();
    }
    void set_depth_compare(U mode) override {
        event(6, {mode});
        boundary();
    }
    void set_depth_write(U enabled) override {
        event(7, {enabled});
        boundary();
    }
    void restore_depth_compare() override {
        event(9, {});
        boundary();
    }
    void draw_colored_polygon(const std::array<sadx::pvr::Point, 4> &points,
                              const std::array<U, 4> &colors, float priority, U flags) override {
        event(8, {flags, bits(priority)});
        trace[trace.size() - 3] = 14;
        for (const auto &point : points) {
            trace.push_back(bits(point.x));
            trace.push_back(bits(point.y));
        }
        trace.insert(trace.end(), colors.begin(), colors.end());
        boundary();
    }
    U texture_global_index(const sadx::texture_lifetime::List &, U index) override {
        event(10, {102, index});
        boundary();
        return 102000 + index;
    }
    sadx::file_select_display::Calendar local_calendar(std::uint64_t time) override {
        event(11, {static_cast<U>(time), static_cast<U>(time >> 32)});
        boundary();
        const auto i = static_cast<unsigned>((time - 132000000000000000ULL) / 10000000);
        return {static_cast<std::uint16_t>(2020 + i),   static_cast<std::uint16_t>(1 + i % 12),
                static_cast<std::uint16_t>(2 + i % 27), static_cast<std::uint16_t>(3 + i % 20),
                static_cast<std::uint16_t>(4 + i % 50), static_cast<std::uint16_t>(5 + i % 50)};
    }
    void display_message(const sadx::TaskHandle &task) override {
        event(12, {task ? 1U : 0U});
        boundary();
    }
    void draw_polygon(const std::array<sadx::pvr::Point, 4> &, U, float, U) override {
        throw std::runtime_error("unexpected uniform polygon");
    }
    void queue_box(float, float, float, float, float, U) override {
        throw std::runtime_error("unexpected queued box");
    }
};
} // namespace
extern "C" int sadx_native_file_select_display(const U *input, const std::uint8_t *saved, U *output,
                                               U *trace, std::size_t capacity) {
    using namespace sadx;
    std::fill_n(output, 64, 0);
    title_menu::FileSelectWork work;
    work.presentation.depth = number(input[0]);
    work.presentation.color = input[1];
    work.offset_x = number(input[2]);
    work.cursor_scale = number(input[3]);
    work.cursor_alpha = static_cast<std::uint8_t>(input[4]);
    auto count = static_cast<std::uint8_t>(input[5]);
    auto scroll = static_cast<std::int16_t>(input[6]);
    auto card = static_cast<std::uint8_t>(input[7]);
    auto selected = static_cast<std::uint8_t>(input[8]);
    auto displayed = static_cast<std::uint8_t>(input[9]);
    auto override = static_cast<std::uint8_t>(input[10]);
    auto mode = static_cast<std::uint8_t>(input[11]);
    auto disabled = static_cast<std::int32_t>(input[12]);
    std::array<std::uint8_t, 512> events{};
    for (unsigned i = 0; i < events.size(); ++i)
        events[i] = static_cast<std::uint8_t>(input[384 + i]);
    save::EncodedData bytes;
    std::copy_n(saved, bytes.size(), bytes.begin());
    auto save = file_select::decode_save(bytes);
    file_select::List list = std::make_shared<file_select::ListNode>();
    auto tail = list;
    for (unsigned i = 1; i <= input[19]; ++i) {
        tail->next = std::make_shared<file_select::ListNode>();
        tail = tail->next;
        char name[32];
        std::snprintf(name, sizeof(name), "SonicDX%02u.snc", i);
        tail->file = file_select::FileInfo{name, 132000000000000000ULL + i * 10000000ULL};
    }
    texture_lifetime::List font_list;
    std::array<std::uint8_t, 128> widths{};
    for (unsigned i = 0; i < 128; ++i)
        widths[i] = static_cast<std::uint8_t>(input[256 + i]);
    font::State font;
    font.width = number(input[28]);
    font.height = number(input[29]);
    font.current = {number(input[24]),
                    number(input[25]),
                    number(input[26]),
                    number(input[27]),
                    &font_list,
                    &widths,
                    {input[30], input[31], input[32], input[33]}};
    auto data = tables(input);
    const float sx = 1, sy = 1;
    const U width = 640, height = 480;
    const auto first = static_cast<std::uint8_t>(input[22]),
               second = static_cast<std::uint8_t>(input[23]);
    TaskScheduler scheduler;
    TaskHandle message = scheduler.append_task(0, nullptr).task;
    file_select_display::View view{sx,       sy,   disabled,  width,  height, save,    events,
                                   override, mode, displayed, scroll, count,  card,    selected,
                                   list,     font, data,      first,  second, message, message};
    pvr::State pvr;
    pvr::initialize(pvr);
    pvr.base_color = input[13];
    for (unsigned i = 0; i < 4; ++i)
        pvr.vertices[i].color = input[14 + i];
    Calls calls;
    calls.fail = input[40];
    X87Context fp(static_cast<std::uint16_t>(input[18]));
    try {
        file_select_display::draw(work, view, pvr, calls, fp);
    } catch (const std::runtime_error &) {
        output[63] = 1;
    }
    for (unsigned i = 0; i < 4; ++i) {
        const auto &v = pvr.vertices[i];
        const U words[]{bits(v.x), bits(v.y), bits(v.reciprocal_depth),
                        bits(v.u), bits(v.v), v.color};
        std::copy_n(words, 6, output + i * 6);
    }
    output[24] = pvr.base_color;
    output[25] = pvr.alpha_enabled;
    output[26] = bits(work.offset_x);
    output[27] = card;
    output[28] = selected;
    output[29] = events[89];
    output[30] = bits(font.current.char_width);
    output[31] = bits(font.current.char_height);
    std::copy(font.current.colors.begin(), font.current.colors.end(), output + 32);
    output[36] = fp.control_word();
    output[37] = fp.exception_flags();
    if (calls.trace.size() > capacity)
        return -1;
    std::copy(calls.trace.begin(), calls.trace.end(), trace);
    return static_cast<int>(calls.trace.size());
}
