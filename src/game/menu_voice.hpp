#pragma once
#include "engine/task.hpp"
#include "game/game_cleanup.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace sadx::menu_voice {
struct RequestWork {
    std::int32_t voice = 0;
    std::int32_t elapsed = 0;
};
// NOTSA: only imported source indices needed by recovered callers are represented.
struct Entry {
    std::uint8_t character;
    std::int32_t request;
    std::int32_t voice;
};
using Catalog = std::vector<Entry>;
struct TaskServices {
    std::function<TaskHandle()> create;
    std::function<std::shared_ptr<RequestWork>(const TaskHandle &)> allocate;
    std::function<void(const TaskHandle &)> destroy;
};
enum class Boundary { Request, Allocation, TaskWork };
class Error : public std::runtime_error {
  public:
    Error(Boundary boundary, std::string message)
        : std::runtime_error(std::move(message)), boundary_(boundary) {}
    Boundary boundary() const { return boundary_; }
  private:
    Boundary boundary_;
};
// FUNCTION: SONIC 0x425710 Serif_Play
void queue_voice(std::int32_t voice,cleanup::TrackState &,const std::int32_t &enabled);
// FUNCTION: SONIC 0x5059f0 FUN_005059f0
std::int32_t request_voice(std::int32_t voice,TaskHandle &canonical,TaskServices &);
// FUNCTION: SONIC 0x505a40 FUN_00505a40
std::int32_t request_character_voice(std::int32_t request,const std::uint8_t &character,
                                     const Catalog &,TaskHandle &canonical,TaskServices &);
// FUNCTION: SONIC 0x5059b0 FUN_005059b0
void update_request(const TaskHandle &,TaskHandle &canonical,cleanup::TrackState &,
                     const std::int32_t &enabled,const std::int32_t &textures_ready,TaskServices &);
} // namespace sadx::menu_voice
