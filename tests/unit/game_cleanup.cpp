#include "game/game_cleanup.hpp"
#include <cassert>
#include <stdexcept>
#include <vector>

namespace {
void count_destructor(void *context) { ++*static_cast<unsigned *>(context); }
}
int main() {
    using namespace sadx::cleanup;
    std::uint32_t count = 3;
    auto *count_pointer = &count;
    PlacementEntry *entries = nullptr;
    unsigned storage[12]{};
    std::array<void *, 6> sets{}, cameras{};
    for (unsigned index = 0; index < 6; ++index) {
        sets[index] = &storage[index];
        cameras[index] = &storage[index + 6];
    }
    unsigned destroyed = 0;
    LevelDestructor destructor = count_destructor;
    void *context = &destroyed;
    std::string message;
    LevelState level{count_pointer, entries, sets, cameras, destructor, context, message};
    std::vector<void *> freed;
    run_level_destructor(0, level, [&](void *buffer) {
        assert(count_pointer == nullptr && entries == nullptr);
        freed.push_back(buffer);
    });
    assert(freed.size() == 12 && destroyed == 1);
    assert(destructor == nullptr && context == nullptr);
    assert(message == "ReleaseCamFile\n");
    for (unsigned index = 0; index < 12; ++index)
        assert(freed[index] == &storage[index]);
    destructor = mission_level_destructor;
    context = &destroyed;
    run_level_destructor(5, level, {});
    assert(destructor == mission_level_destructor && context == &destroyed);
    bool missing = false;
    try {
        run_level_destructor(1, level, {});
    } catch (const std::runtime_error &) {
        missing = true;
    }
    assert(missing && destructor == mission_level_destructor);

    std::array<sadx::sprite::TextureList, 4> lists;
    auto *common = &lists[0];
    std::array<sadx::sprite::TextureList *, 32> regular{};
    regular[0] = &lists[1];
    regular[2] = &lists[2];
    CharacterTextureLists character{{&lists[3], nullptr}};
    std::array<const CharacterTextureLists *, 9> characters{};
    characters[8] = &character;
    std::array<std::int8_t, 9> loaded{};
    loaded[8] = 1;
    loaded[0] = 5;
    TextureState textures{common, regular, characters, loaded};
    std::vector<sadx::sprite::TextureList *> released;
    release_regular_object_textures(textures, [&](auto &list) { released.push_back(&list); });
    release_character_textures(textures, [&](auto &list) { released.push_back(&list); });
    assert((released == std::vector<sadx::sprite::TextureList *>{&lists[0], &lists[1], &lists[3]}));
    assert(common == nullptr && regular[0] == nullptr && regular[2] == &lists[2]);
    assert(loaded[8] == 0 && loaded[0] == 5);
    common = &lists[0];
    bool failed = false;
    try {
        release_regular_object_textures(textures, [](auto &) { throw std::runtime_error("device"); });
    } catch (const std::runtime_error &) {
        failed = true;
    }
    assert(failed && common == &lists[0]);
    loaded[8] = 1;
    bool missing_texture = false;
    try {
        unload_character_textures(8, textures, [](auto &) {});
    } catch (const std::runtime_error &) {
        missing_texture = true;
    }
    assert(missing_texture && loaded[8] == 1);

    std::array<sadx::SoundRecordInitialValues, 36> records{};
    std::array<sadx::TaskHandle, 36> tasks{};
    records[0].timer = 9;
    records[0].flags = 2;
    unsigned stopped = 0;
    AudioQueueView queue{records, tasks};
    assert(stop_all_sounds(queue, [&](std::int32_t index) {
        assert(index == static_cast<std::int32_t>(stopped));
        assert(records[static_cast<unsigned>(index)].timer == 0);
        assert((records[static_cast<unsigned>(index)].flags & 16) != 0);
        ++stopped;
    }) == 0);
    assert(stopped == 36 && records[0].flags == 18);
    assert(TrackState{}.current_song == -1 && kInitialSequenceRunning == 1);
}
