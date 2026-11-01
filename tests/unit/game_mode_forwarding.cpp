#include "game/game_mode_internal.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace {
using namespace sadx::flow;
void require(bool value, const char *message) {
    if (!value) { std::cerr << message << '\n'; std::exit(1); }
}
using Words = std::array<std::uint32_t, 16>;
Words words(const GameFlowState &s) {
    return {{static_cast<std::uint32_t>(s.mode), s.pause_state,
        static_cast<std::uint32_t>(s.save_phase), s.save_ready, s.message_resource.value,
        s.action_restart_requested, s.title_mode, s.title_load_phase,
        s.playing_super_sonic, s.tutorial_variant, s.alert_flag, s.alert_blocked,
        s.movie_number, s.menu_message_enabled, s.sequence != nullptr,
        s.sequence ? static_cast<std::uint32_t>(s.sequence->level) : 0U}};
}
struct Event {
    std::string name;
    std::vector<std::int64_t> arguments;
    Words state;
    bool operator==(const Event &other) const {
        return name == other.name && arguments == other.arguments && state == other.state;
    }
};
struct CallbackStop {};
struct Services final : NativeServices {
    GameFlowState &state;
    std::vector<Event> events;
    std::function<void(const std::string &)> mutate;
    std::size_t fail_at = 0;
    std::uint32_t status = 0, logos = 0, level = 1;
    std::int32_t readiness = -1, movie = 1, title = 0;
    explicit Services(GameFlowState &value) : state(value) {}
    void call(const char *name, std::vector<std::int64_t> args = {}) {
        events.push_back({name, std::move(args), words(state)});
        if (events.size() == fail_at) throw CallbackStop{};
        if (mutate) mutate(name);
    }
    void initialize_game() override { call("initialize_game"); }
    void initialize_demo_state() override { call("initialize_demo_state"); }
    std::uint32_t startup_status() override { call("startup_status"); return status; }
    std::int32_t startup_readiness() override { call("startup_readiness"); return readiness; }
    void initialize_sega_logo() override { call("initialize_sega_logo"); }
    std::uint32_t run_logos() override { call("run_logos"); return logos; }
    void load_sega_logo() override { call("load_sega_logo"); }
    void set_frame_rate_mode(std::int32_t a, std::int32_t b) override {
        call("set_frame_rate_mode", {a, b});
    }
    void set_wait_vsync_count(std::int32_t value) override { call("set_wait_vsync_count", {value}); }
    void draw_screen_fade() override { call("draw_screen_fade"); }
    void increment_frame_counter() override { call("increment_frame_counter"); }
    std::uint32_t initialize_level_game() override { call("initialize_level_game"); return level; }
    void setup_character(std::uint32_t value) override { call("setup_character", {value}); }
    void reset_player_work() override { call("reset_player_work"); }
    std::int32_t run_movie(std::uint32_t value) override { call("run_movie", {value}); return movie; }
    void clear_game() override { call("clear_game"); }
    void reset_game() override { call("reset_game"); }
    void prepare_title_initialization() override { call("prepare_title_initialization"); }
    void run_level_destructor(std::uint32_t value) override { call("run_level_destructor", {value}); }
    void free_regular_object_texlists() override { call("free_regular_object_texlists"); }
    void free_character_texlists() override { call("free_character_texlists"); }
    void release_all_textures() override { call("release_all_textures"); }
    void prepare_title_assets() override { call("prepare_title_assets"); }
    void initialize_title_menu() override { call("initialize_title_menu"); }
    void load_level_sounds() override { call("load_level_sounds"); }
    std::int32_t run_title_menu() override { call("run_title_menu"); return title; }
    void close_title_menu(std::int32_t value) override { call("close_title_menu", {value}); }
};
struct Forward {
    const char *name;
    std::vector<std::int64_t> arguments;
    std::uint32_t expected;
    std::function<std::uint32_t(FlowContext &)> invoke;
};
void forwarding() {
    // Deliberately unequal signed/unsigned operands observe narrowing and argument order.
    const std::uint32_t high = 3456789012U;
    const std::int32_t negative = -123456789;
    const std::vector<Forward> calls{
        {"initialize_game", {}, 0, [](auto &f) { initialize_game(f); return 0; }},
        {"initialize_demo_state", {}, 0, [](auto &f) { initialize_demo_state(f); return 0; }},
        {"startup_status", {}, high, [](auto &f) { return startup_status(f); }},
        {"startup_readiness", {}, static_cast<std::uint32_t>(negative), [](auto &f) { return startup_readiness(f); }},
        {"initialize_sega_logo", {}, 0, [](auto &f) { initialize_sega_logo(f); return 0; }},
        {"run_logos", {}, high, [](auto &f) { return run_logos(f); }},
        {"load_sega_logo", {}, 0, [](auto &f) { load_sega_logo(f); return 0; }},
        {"set_frame_rate_mode", {-7, std::numeric_limits<std::int32_t>::min()}, 0,
            [](auto &f) { set_frame_rate_mode(f, -7, std::numeric_limits<std::int32_t>::min()); return 0; }},
        {"set_wait_vsync_count", {-13}, 0, [](auto &f) { set_wait_vsync_count(f, -13); return 0; }},
        {"draw_screen_fade", {}, 0, [](auto &f) { draw_screen_fade(f); return 0; }},
        {"increment_frame_counter", {}, 0, [](auto &f) { increment_frame_counter(f); return 0; }},
        {"initialize_level_game", {}, high, [](auto &f) { return initialize_level_game(f); }},
        {"setup_character", {4294967295U}, 0, [](auto &f) { setup_character(f, 4294967295U); return 0; }},
        {"reset_player_work", {}, 0, [](auto &f) { reset_player_work(f); return 0; }},
        {"run_movie", {4294967294U}, static_cast<std::uint32_t>(negative),
            [](auto &f) { return run_movie(f, 4294967294U); }},
        {"clear_game", {}, 0, [](auto &f) { clear_game(f); return 0; }},
        {"reset_game", {}, 0, [](auto &f) { reset_game(f); return 0; }},
        {"prepare_title_initialization", {}, 0, [](auto &f) { prepare_title_initialization(f); return 0; }},
        {"run_level_destructor", {4294967293U}, 0, [](auto &f) { run_level_destructor(f, 4294967293U); return 0; }},
        {"free_regular_object_texlists", {}, 0, [](auto &f) { free_regular_object_texlists(f); return 0; }},
        {"free_character_texlists", {}, 0, [](auto &f) { free_character_texlists(f); return 0; }},
        {"release_all_textures", {}, 0, [](auto &f) { release_all_textures(f); return 0; }},
        {"prepare_title_assets", {}, 0, [](auto &f) { prepare_title_assets(f); return 0; }},
        {"initialize_title_menu", {}, 0, [](auto &f) { initialize_title_menu(f); return 0; }},
        {"load_level_sounds", {}, 0, [](auto &f) { load_level_sounds(f); return 0; }},
        {"run_title_menu", {}, static_cast<std::uint32_t>(negative), [](auto &f) { return run_title_menu(f); }},
        {"close_title_menu", {-47}, 0, [](auto &f) { close_title_menu(f, -47); return 0; }}
    };
    require(calls.size() == 27, "all finite native operations must have observations");
    for (const auto &item : calls) {
        GameFlowState state{};
        Services service(state);
        service.status = service.logos = service.level = high;
        service.readiness = service.movie = service.title = negative;
        FlowContext context{state, FlowPhase::Dispatch, &service};
        const auto initial = words(state);
        service.mutate = [&](const auto &) { state.save_phase = -37; };
        require(item.invoke(context) == item.expected, "native return bits changed");
        require(service.events == std::vector<Event>{{item.name, item.arguments, initial}},
                "native invocation identity, arguments or callback snapshot changed");
        require(state.save_phase == -37, "callback mutation must remain live");
        state = {};
        service.events.clear(); service.mutate = {}; service.fail_at = 1;
        bool stopped = false;
        try { item.invoke(context); } catch (const CallbackStop &) { stopped = true; }
        require(stopped && service.events.size() == 1 && words(state) == initial,
                "native callback exception must propagate without later stores or retry");
        context.native = nullptr;
        stopped = false;
        try { item.invoke(context); }
        catch (const UnrecoveredGameFlowCall &error) { stopped = error.what() == std::string(item.name); }
        require(stopped && words(state) == initial, "missing native service must keep its exact named stop");
    }
    std::cout << "native-forwarders 27\n";
}
struct Scenario {
    const char *name;
    std::function<void(GameFlowState &, Services &)> configure;
    std::vector<std::string> calls;
    std::function<void(const GameFlowState &)> check;
};
void scenarios() {
    const std::vector<Scenario> cases{
        {"start-not-ready", [](auto &s, auto &) { s.mode = GameMode::Start; },
            {"initialize_game", "initialize_demo_state", "startup_status", "startup_readiness", "draw_screen_fade", "increment_frame_counter"},
            [](const auto &s) { require(s.mode == GameMode::NoSaveWindowInit, "negative readiness must stay not ready"); }},
        {"start-short-circuit", [](auto &s, auto &c) { s.mode = GameMode::Start; c.status = 1; },
            {"initialize_game", "initialize_demo_state", "startup_status", "draw_screen_fade", "increment_frame_counter"},
            [](const auto &s) { require(s.mode == GameMode::LogoInit, "nonzero startup status must skip readiness"); }},
        {"logo-title", [](auto &s, auto &c) { s.mode = GameMode::LogoInit; s.title_mode = 9; c.logos = 1; },
            {"initialize_sega_logo", "run_logos", "set_frame_rate_mode", "prepare_title_initialization", "draw_screen_fade", "increment_frame_counter"},
            [](const auto &s) { require(s.mode == GameMode::TitleInit && s.title_mode == 0, "logo title transition"); }},
        {"logo-movie", [](auto &s, auto &c) { s.mode = GameMode::Logo; s.movie_number = 19; c.logos = 3; },
            {"run_logos", "set_frame_rate_mode", "draw_screen_fade", "increment_frame_counter"},
            [](const auto &s) { require(s.mode == GameMode::Movie && s.movie_number == 0, "logo movie stores"); }},
        {"movie-title", [](auto &s, auto &) { s.mode = GameMode::Movie; s.movie_number = 4294967294U; },
            {"run_movie", "clear_game", "reset_game", "prepare_title_initialization", "draw_screen_fade", "increment_frame_counter"},
            [](const auto &s) { require(s.mode == GameMode::TitleInit, "movie completion cleanup"); }},
        {"action-restart", [](auto &s, auto &) { s.mode = GameMode::Action; s.action_restart_requested = 255; },
            {"clear_game", "reset_game", "draw_screen_fade", "increment_frame_counter"},
            [](const auto &s) { require(s.mode == GameMode::StaffRollInit && s.action_restart_requested == 0, "action restart stores"); }},
        {"title-cancel", [](auto &s, auto &c) { s.mode = GameMode::TitleMenu; s.title_mode = 9; c.title = -2; },
            {"run_title_menu", "clear_game", "reset_game", "prepare_title_initialization", "draw_screen_fade", "increment_frame_counter"},
            [](const auto &s) { require(s.mode == GameMode::TitleInit && s.title_mode == 0, "title cancellation stores"); }},
        {"title-summary", [](auto &s, auto &c) { s.mode = GameMode::TitleMenu; c.title = 101; },
            {"run_title_menu", "close_title_menu", "draw_screen_fade", "increment_frame_counter"},
            [](const auto &s) { require(s.mode == GameMode::SummaryInit, "title summary transition"); }},
        {"title-live-resource-and-mode", [](auto &s, auto &c) {
            s.mode = GameMode::TitleInit; s.playing_super_sonic = 255;
            c.mutate = [&](const std::string &name) {
                if (name == "prepare_title_assets") s.title_load_phase = 2;
                if (name == "set_wait_vsync_count") s.mode = static_cast<GameMode>(-1);
            };
        }, {"run_level_destructor", "run_level_destructor", "run_level_destructor", "run_level_destructor", "run_level_destructor", "run_level_destructor", "run_level_destructor", "free_regular_object_texlists", "free_character_texlists", "release_all_textures", "prepare_title_assets", "initialize_title_menu", "set_wait_vsync_count", "load_level_sounds", "draw_screen_fade", "increment_frame_counter"},
            [](const auto &s) { require(s.mode == GameMode::Start && s.title_load_phase == 0 && s.playing_super_sonic == 0, "title callback reads, wrapping increment and store order"); }}
    };
    std::size_t prefixes = 0;
    for (const auto &item : cases) {
        GameFlowState state{};
        Services service(state);
        item.configure(state, service);
        advance_game_mode(state, service);
        item.check(state);
        require(service.events.size() == item.calls.size(), "unexpected callback count");
        for (std::size_t i = 0; i < item.calls.size(); ++i)
            require(service.events[i].name == item.calls[i], "callback order changed");
        if (std::string(item.name) == "title-live-resource-and-mode") {
            const std::array<std::int64_t, 7> order{{4, 0, 5, 7, 6, 8, 9}};
            for (std::size_t i = 0; i < order.size(); ++i)
                require(service.events[i].arguments == std::vector<std::int64_t>{order[i]}, "title destructor order changed");
            require(service.events[12].state[8] == 255 && service.events[13].state[8] == 0,
                    "SuperSonic store must follow vsync callback and precede sound load");
        }
        for (std::size_t n = 1; n <= service.events.size(); ++n) {
            GameFlowState partial{};
            Services failing(partial);
            item.configure(partial, failing); failing.fail_at = n;
            bool stopped = false;
            try { advance_game_mode(partial, failing); } catch (const CallbackStop &) { stopped = true; }
            require(stopped && failing.events.size() == n, "callback failure must preserve exact call prefix");
            require(std::equal(failing.events.begin(), failing.events.end(), service.events.begin()),
                    "callback failure changed an earlier argument or state snapshot");
            require(words(partial) == service.events[n - 1].state, "callback failure ran a later store");
            ++prefixes;
        }
    }
    std::cout << "native-scenarios " << cases.size() << " failure-prefixes " << prefixes << '\n';
}
void sequence_and_reentry() {
    GameFlowState state{};
    Services service(state);
    FlowContext context{state, FlowPhase::Dispatch, &service};
    const auto before_module = words(state);
    for (const auto module : {FlowModule::AlertWindow, FlowModule::Advertise, FlowModule::Summary})
        request_file_module(context, module);
    require(service.events.empty() && words(state) == before_module,
            "disabled diagnostic-only module request must not load or mutate game state");
    SequenceSelection sequence{-32768}; state.sequence = &sequence;
    require(current_sequence_level(context) == -32768, "signed borrowed sequence level");
    sequence.level = 32767;
    require(current_sequence_level(context) == 32767, "sequence level must be read live");
    state.sequence = nullptr;
    bool stopped = false;
    try { current_sequence_level(context); }
    catch (const UnrecoveredGameFlowCall &e) { stopped = std::string(e.what()) == "current_sequence_level"; }
    require(stopped && service.events.empty(), "missing sequence must not become a fabricated level");
    GameFlowState nested{}; nested.mode = GameMode::Continue;
    Services other(nested);
    service.mutate = [&](const std::string &name) {
        if (name == "initialize_game") advance_game_mode(nested, other);
    };
    advance_game_mode(state, service);
    require(other.events.size() == 2 && other.events[0].name == "draw_screen_fade" &&
            other.events[1].name == "increment_frame_counter", "reentrant world callback routing");
    require(state.mode == GameMode::NoSaveWindowInit && nested.mode == GameMode::Continue,
            "reentrant worlds must retain distinct state and services");
    std::cout << "native-sequence-and-reentry 4\n";
}
} // namespace
int main() { forwarding(); scenarios(); sequence_and_reentry(); }
