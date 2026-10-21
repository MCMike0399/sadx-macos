#include "game/game_cleanup.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>
namespace {
struct Failed { std::uint32_t site; };
}
extern "C" int sadx_native_game_reset(const std::uint32_t *in, std::size_t size,
                                       std::uint32_t *out, std::size_t capacity) {
    if (size != 6 || capacity < 128)
        return 0;
    std::uint32_t status = in[0], seed = in[4];
    std::uint16_t editor = static_cast<std::uint16_t>(in[1]);
    void *first = reinterpret_cast<void *>(static_cast<std::uintptr_t>(in[2]));
    void *second = reinterpret_cast<void *>(static_cast<std::uintptr_t>(in[3]));
    sadx::cleanup::State state{status, editor, first, second, seed};
    std::vector<std::array<std::uint32_t, 3>> trace;
    const auto record = [&](std::uint32_t target, std::uint32_t site, std::uint32_t arg = 0) {
        trace.push_back({target, site, arg});
        if (in[5] != 0 && trace.size() == in[5])
            throw Failed{site};
    };
    sadx::cleanup::Services services;
    services.resume_voice_volume = [&] { record(0x40cd10, 0x40bf42); };
    services.set_depth_compare_mode = [&](std::uint32_t a) { record(0x401410, 0x40bf49, a); };
    services.stop_vibration = [&](std::uint32_t a) { record(0x785330, 0x40bf51, a); };
    services.set_blank_queue_mode = [&] { record(0x403e60, 0x40bf63); };
    services.reset_song_state = [&] { record(0x4257c0, 0x40bf68); };
    services.reset_current_voice = [&] { record(0x4257e0, 0x40bf6d); };
    services.stop_all_sounds = [&] { record(0x424460, 0x40bf72); };
    services.stop_pcm = [&](std::int32_t a) { record(0x410100, 0x40bf79, a); };
    services.close_media = [&](std::uint32_t a) { record(0x40cff0, a == 0 ? 0x40bf7f : 0x40bf86, a); };
    services.setup_screen = [&] { record(0x4208e0, 0x40bf8b); };
    services.display_message = [&](void *a) {
        record(0x40d490, 0x40bf91, static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(a)));
    };
    services.initialize_heap = [&] { record(0x40b1a0, 0x40bf99); };
    services.initialize_tasks = [&] { record(0x40b460, 0x40bf9e); };
    services.run_level_destructor = [&](std::uint32_t a) { record(0x4228f0, 0x40bfa6, a); };
    services.reset_loop_state = [&] { record(0x421fb0, 0x40bfb4); };
    unsigned released = 0;
    services.release_buffer = [&](void *a) {
        const auto site = in[2] != 0 && released++ == 0 ? 0x40bfc3 : 0x40bfdb;
        record(0x780fb0, site, static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(a)));
    };
    services.enable_fog = [&] { record(0x411af0, 0x40c00e); };
    std::uint32_t failure = 0, boundary = 0;
    try {
        sadx::cleanup::reset(state, services);
    } catch (const Failed &failed) {
        failure = 3;
        boundary = failed.site;
    }
    std::fill_n(out, 128, 0);
    const std::uint32_t values[]{failure, boundary, failure == 0, status, editor,
                                 static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(first)),
                                 static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(second)),
                                 seed, static_cast<std::uint32_t>(trace.size())};
    auto cursor = std::copy(std::begin(values), std::end(values), out);
    for (const auto &call : trace)
        cursor = std::copy(call.begin(), call.end(), cursor);
    return 1;
}
#include <cstring>
#include <type_traits>
extern "C" void sadx_native_cleanup_load_save(const std::uint32_t *in, std::uint32_t *out) {
    using namespace sadx;
    save::Data saved;
    static_assert(std::is_trivially_copyable_v<save::Data>);
    std::array<std::uint8_t, sizeof(saved)> bytes;
    auto random = in[0];
    for (auto &byte : bytes) {
        random = random * 1664525U + 1013904223U;
        byte = static_cast<std::uint8_t>(random >> 24);
    }
    std::memcpy(&saved, bytes.data(), bytes.size());
    saved.last_character = static_cast<std::uint8_t>(in[1]);
    std::int32_t card = 0;
    std::uint32_t timer = 0, movie = 0;
    std::int16_t rings = 0;
    std::uint16_t mode = 0;
    bootstrap::State game({card, timer, rings, movie, mode});
    game.character = static_cast<std::uint16_t>(in[3]);
    std::memset(game.sequences.data(), 165, sizeof(game.sequences));
    game.sequence_variables.fill(165);
    game.cutscene_flags.fill(165);
    game.completed_stages.fill(165);
    game.section_work.fill(165);
    std::array<std::uint8_t, 60> missions;
    missions.fill(165);
    const auto metal = static_cast<std::uint8_t>(in[2]);
    std::uint8_t rumble = 165;
    std::uint32_t saved_stage = 0, market_rings = 0;
    cleanup::LoadSaveState state{timer, game.character, game.lives, metal, saved_stage,
                                game.sequence_variables, game.cutscene_flags,
                                game.section_work.data() + 12, game.sequences,
                                game.completed_stages, missions, rumble, market_rings};
    const auto encoded = save::encode(saved);
    cleanup::load_save(saved, state);
    std::fill_n(out, 2048, 0);
    std::memcpy(out, encoded.data(), encoded.size());
    auto cursor = out + 348;
    const std::uint32_t scalar[]{timer, game.character, static_cast<std::uint8_t>(game.lives),
                                 saved_stage, rumble, market_rings};
    cursor = std::copy(std::begin(scalar), std::end(scalar), cursor);
    const auto append = [&](const auto &array) {
        std::memcpy(cursor, array.data(), sizeof(array));
        cursor += sizeof(array) / 4;
    };
    append(game.sequence_variables);
    append(game.cutscene_flags);
    append(game.sequences);
    append(game.completed_stages);
    append(missions);
    append(game.section_work);
}
extern "C" void sadx_native_cleanup_helpers(const std::uint32_t *in, std::uint32_t *out) {
    using namespace sadx::cleanup;
    std::fill_n(out, 512, 0);
    switch (in[0]) {
    case 0: {
        AuxiliaryWork state;
        state.values.fill(in[1]);
        state.limit = in[2];
        out[0] = reset_auxiliary_work(state);
        std::copy(state.values.begin(), state.values.end(), out + 1);
        out[10] = state.limit;
        auto running = static_cast<std::int32_t>(in[3]);
        clear_sequence_running(running);
        out[11] = running;
        break;
    }
    case 1: {
        TrackState state{static_cast<std::int32_t>(in[1]), static_cast<std::int32_t>(in[2]),
                         static_cast<std::int32_t>(in[3]), static_cast<std::int32_t>(in[4]),
                         in[5], in[6]};
        reset_song_state(state);
        reset_current_voice(state);
        out[0] = state.current_song;
        out[1] = state.previous_song;
        out[2] = state.real_previous_song;
        out[3] = state.current_voice;
        out[4] = state.song_queue_state;
        out[5] = state.voice_queue_state;
        break;
    }
    case 2: {
        auto stage = static_cast<std::int16_t>(in[1]);
        auto act = static_cast<std::int16_t>(in[2]);
        auto previous = static_cast<std::int16_t>(in[3]);
        auto previous_act = in[4];
        StageState state{stage, act, previous, previous_act};
        set_stage_number(in[5], in[6], state);
        out[0] = static_cast<std::uint16_t>(stage);
        out[1] = static_cast<std::uint16_t>(act);
        out[2] = static_cast<std::uint16_t>(previous);
        out[3] = previous_act;
        break;
    }
    case 3: {
        Message message{static_cast<std::uint8_t>(in[2])};
        MessageState state;
        state.count = in[1];
        const auto gate = static_cast<std::uint8_t>(in[3]);
        const auto ticks = static_cast<std::int32_t>(in[4]);
        display_message(in[5] ? &message : nullptr, state, gate, ticks);
        out[0] = state.count;
        for (unsigned index = 0; index < 4; ++index)
            out[index + 1] = state.entries[index] == &message;
        break;
    }
    case 4: {
        auto work = in[1];
        auto count = in[2];
        const void *list = in[3] ? in : nullptr;
        LoopState state{work, count, list};
        reset_loop_state(state, [&] {
            ++out[2];
            out[3] = work;
            out[4] = count;
        });
        out[0] = work;
        out[1] = count;
        break;
    }
    case 5: {
        std::int32_t mode = in[1], event = in[2], reset = in[3], enabled = in[4], paused = in[5];
        AuxiliaryWork auxiliary;
        auxiliary.values.fill(in[6]);
        auxiliary.limit = in[7];
        std::int32_t running = in[8], story = in[9];
        std::uint8_t hud = in[10];
        std::uint32_t pause = in[11];
        sadx::save::Data saved{}, backup{};
        saved.total_time = in[12];
        backup.total_time = in[13];
        TitleState state{mode, event, reset, enabled, paused, auxiliary, running,
                         hud, pause, story, saved, backup};
        unsigned cursor = 24;
        const auto record = [&](unsigned value) { out[cursor++] = value; };
        TitleServices services{
            [&] { record(1); }, [&] { record(2); }, [&] {
                record(3);
                out[20] = backup.total_time;
                out[21] = mode | event | reset | enabled | paused | running | hud | pause | story;
                saved.total_time = in[14];
            },
            [&](std::uint32_t value) { record(4); record(value); },
            [&](std::uint8_t stage, std::uint8_t act) { record(5); record(stage); record(act); }
        };
        prepare_title(state, services);
        const std::uint32_t values[]{static_cast<std::uint32_t>(mode), static_cast<std::uint32_t>(event),
            static_cast<std::uint32_t>(reset), static_cast<std::uint32_t>(enabled),
            static_cast<std::uint32_t>(paused), static_cast<std::uint32_t>(running), hud, pause,
            static_cast<std::uint32_t>(story), saved.total_time, backup.total_time, auxiliary.limit};
        std::copy(std::begin(values), std::end(values), out);
        std::copy(auxiliary.values.begin(), auxiliary.values.end(), out + 32);
        break;
    }
    case 6: {
        unsigned cursor = 0;
        ClearServices services{[&] { out[cursor++] = 1; }, [&] { out[cursor++] = 2; },
                               [&] { out[cursor++] = 3; }};
        clear(services);
        break;
    }
    case 7: {
        std::array<sadx::sprite::TextureList, 40> objects;
        auto *common = in[1] ? &objects[0] : nullptr;
        std::array<sadx::sprite::TextureList *, 32> regular{};
        for (unsigned index = 0; index < 32; ++index)
            regular[index] = (in[2] & (1U << index)) ? &objects[index + 1] : nullptr;
        std::array<CharacterTextureLists, 9> descriptions;
        std::array<const CharacterTextureLists *, 9> characters{};
        std::array<std::int8_t, 9> loaded;
        for (unsigned index = 0; index < 9; ++index) {
            descriptions[index].lists = {&objects[index + 1], &objects[index + 10], nullptr};
            characters[index] = (in[3] & (1U << index)) ? &descriptions[index] : nullptr;
            loaded[index] = static_cast<std::int8_t>(in[index + 4]);
        }
        TextureState state{common, regular, characters, loaded};
        unsigned cursor = 64;
        const auto release = [&](sadx::sprite::TextureList &list) {
            out[cursor++] = static_cast<std::uint32_t>(&list - objects.data()) + 1;
        };
        release_regular_object_textures(state, release);
        release_character_textures(state, release);
        out[0] = common ? 1 : 0;
        for (unsigned index = 0; index < 32; ++index)
            out[index + 1] = regular[index] ? index + 2 : 0;
        for (unsigned index = 0; index < 9; ++index)
            out[index + 33] = static_cast<std::uint8_t>(loaded[index]);
        out[63] = cursor - 64;
        break;
    }
    case 8: {
        std::uint32_t count = 1;
        auto *count_pointer = in[5] ? &count : nullptr;
        PlacementEntry *entries = in[5] ? reinterpret_cast<PlacementEntry *>(&count) : nullptr;
        std::array<unsigned, 12> objects{};
        std::array<void *, 6> sets{}, cameras{};
        for (unsigned index = 0; index < 6; ++index) {
            sets[index] = (in[3] & (1U << index)) ? &objects[index] : nullptr;
            cameras[index] = (in[4] & (1U << index)) ? &objects[index + 6] : nullptr;
        }
        unsigned cursor = 32;
        const auto callback = +[](void *context) { ++*static_cast<unsigned *>(context); };
        LevelDestructor destructor = in[2] == 2 ? mission_level_destructor : (in[2] ? callback : nullptr);
        void *context = &out[20];
        std::string message;
        LevelState state{count_pointer, entries, sets, cameras, destructor, context, message};
        unsigned calls = 0;
        try {
            run_level_destructor(in[1], state, [&](void *pointer) {
                out[cursor++] = static_cast<unsigned *>(pointer) - objects.data() + 1;
                if (in[6] != 0 && ++calls == in[6])
                    throw Failed{0};
            });
        } catch (const Failed &) {
            out[21] = 1;
        }
        out[0] = count_pointer != nullptr;
        out[1] = entries != nullptr;
        out[2] = destructor == mission_level_destructor ? 2 : (destructor != nullptr);
        for (unsigned index = 0; index < 6; ++index) {
            out[3 + index] = sets[index] ? index + 1 : 0;
            out[9 + index] = cameras[index] ? index + 7 : 0;
        }
        out[15] = message == "ReleaseSetFile\n" ? 1 : (message == "ReleaseCamFile\n" ? 2 : 0);
        out[16] = cursor - 32;
        break;
    }
    case 9: {
        std::array<sadx::sprite::TextureList, 2> lists;
        std::array<sadx::texture_lifetime::Memory, 2> memories;
        for (unsigned index = 0; index < 2; ++index)
            lists[index].entries.push_back((in[4] & (1U << index)) ? &memories[index] : nullptr);
        CharacterTextureLists entry{{&lists[0], &lists[1], nullptr}};
        std::array<const CharacterTextureLists *, 9> characters{};
        const auto character = in[1] % 9;
        if (in[2])
            characters[character] = &entry;
        std::array<std::int8_t, 9> loaded{};
        loaded[character] = static_cast<std::int8_t>(in[3]);
        sadx::sprite::TextureList *common = nullptr;
        std::array<sadx::sprite::TextureList *, 32> regular{};
        TextureState state{common, regular, characters, loaded};
        unsigned cursor = 2;
        unload_character_textures(character, state, [&](auto &list) {
            out[cursor++] = &list - lists.data() + 1;
        });
        out[0] = static_cast<std::uint8_t>(loaded[character]);
        out[1] = cursor - 2;
        break;
    }
    }
}
