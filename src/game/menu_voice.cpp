#include "game/menu_voice.hpp"
#include <stdexcept>

namespace sadx::menu_voice {
void queue_voice(std::int32_t voice, cleanup::TrackState &tracks, const std::int32_t &enabled) {
    if (enabled != 0) tracks.current_voice = voice;
}
std::int32_t request_voice(std::int32_t voice, TaskHandle &canonical, TaskServices &services) {
    if (canonical) return 0;
    const auto task = services.create();
    canonical = task;
    if (task) {
        auto work = services.allocate(task);
        task->auxiliary_native_work = work;
        if (!work)
        throw Error(Boundary::Allocation, "menu voice CAlloc returned null before work stores");
        work->voice = voice;
        work->elapsed = 0;
        return 1;
    }
    return 0;
}
std::int32_t request_character_voice(std::int32_t request, const std::uint8_t &character,
                                     const Catalog &catalog, TaskHandle &canonical, TaskServices &services) {
    for (const auto &entry : catalog)
        if (entry.character == character && entry.request == request)
            return request_voice(entry.voice, canonical, services);
    throw Error(Boundary::Request, "menu voice request is outside the proved imported character map");
}
void update_request(const TaskHandle &task, TaskHandle &canonical, cleanup::TrackState &tracks,
                     const std::int32_t &enabled, const std::int32_t &ready, TaskServices &services) {
    const auto work = std::static_pointer_cast<RequestWork>(task->auxiliary_native_work);
    if (!work)
        throw Error(Boundary::TaskWork, "menu voice request task has no source auxiliary work");
    const auto previous = work->elapsed;
    work->elapsed = static_cast<std::int32_t>(static_cast<std::uint32_t>(previous) + 1U);
    if (previous > 0 && ready != 0) {
        queue_voice(work->voice, tracks, enabled);
        services.destroy(task);
        canonical.reset();
    }
}
} // namespace sadx::menu_voice
