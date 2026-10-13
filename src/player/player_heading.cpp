#include "player/player_heading.hpp"
#include <cstring>

namespace sadx {
namespace {
std::int32_t signed_word(std::uint32_t word) noexcept {
    std::int32_t value;std::memcpy(&value,&word,sizeof(value));return value;
}
}

std::int32_t adjust_angle(std::int32_t from,std::int32_t to,std::int32_t step) noexcept {
    const auto current=static_cast<std::uint32_t>(from)&0xffffU;
    const auto target=static_cast<std::uint32_t>(to)&0xffffU;
    const auto difference=(target-current)&0xffffU;
    const auto delta=difference<0x8000U ? static_cast<std::int32_t>(difference)
                                       : static_cast<std::int32_t>(difference)-65536;
    const auto negative_step=signed_word(0U-static_cast<std::uint32_t>(step));
    if (delta<=step && delta>=negative_step) return static_cast<std::int32_t>(target);
    const auto moved=delta<0 ? current-static_cast<std::uint32_t>(step)
                           : current+static_cast<std::uint32_t>(step);
    return static_cast<std::int32_t>(moved&0xffffU);
}

PlayerHeadingResult turn_player_heading(PlayerOrientation& task,PlayerMotion& motion,
                                        std::int32_t target,PlayerCoordinates& coordinates,X87Context& fp) {
    auto temporary=motion.speed;
    auto transformed=coordinates.task_to_world(&task,temporary,fp);
    if(!transformed) return {transformed,0x443e15,temporary};
    task.angles[1]=adjust_angle(task.angles[1],target,0x2000);
    transformed=coordinates.world_to_player(task,temporary,fp);
    if(!transformed) return {transformed,0x443e36,temporary};
    motion.speed=temporary;
    return {};
}

std::uint32_t angle_distance(std::int32_t first,std::int32_t second) noexcept {
    auto difference=static_cast<std::uint32_t>(second)-static_cast<std::uint32_t>(first);
    if((difference & 0x8000U)!=0) difference=0U-difference;
    return difference & 0xffffU;
}

} // namespace sadx
