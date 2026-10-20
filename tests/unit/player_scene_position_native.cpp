#include "math/ninja_sine_table.hpp"
#include "player/player_scene_position.hpp"
#include <iostream>

namespace {
void require(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}
void check(std::uint16_t control, bool alternate) {
    using namespace sadx;
    const NinjaMatrix unit{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    NinjaMatrixStack stack(std::vector<NinjaMatrix>(4, unit), 1, 0);
    PlayerCoordinates coordinates(ninja_sine_table(), unit, stack);
    X87Context arithmetic(control);
    scene::Actor actor{{1, 2, 3}, {3, {0, 0, 0}}, 0};
    std::uint8_t character = 0, action = 0;
    scene::PlayerWork player{};
    player.status = 7;
    scene::WorldMotion world{};
    PlayerParameters parameters{};
    parameters.height = 10;
    scene::Contact contact{};
    contact.radius = 9;
    contact.center = {11, 12, 13};
    scene::Globals globals{};
    globals.special_surface = 6;
    scene::Landscape landscape{};
    std::array<scene::RegisteredPlayer, 8> registered{};
    std::unordered_map<std::size_t, scene::GeometryTask> geometry;
    std::array<TaskHandle, 8> tasks{};
    std::unordered_map<const Task *, scene::PlatformWork *> platforms;
    std::vector<NinjaVector> responses;
    scene::Runtime runtime{actor, character, action, player, world, parameters, contact,
                           globals, landscape, registered, geometry, tasks, platforms,
                           responses, coordinates, arithmetic};
    bool stopped = false;
    try {
        if (alternate)
            (void)scene::position_player_in_scene_alternate(runtime);
        else
            (void)scene::position_player_in_scene(runtime);
    } catch (const scene::Error &error) {
        stopped = error.failure() == scene::Failure::UncomposedCallee &&
                  std::string(error.what()) == "initialize_player_contact_offset";
    }
    require(stopped, "native dispatcher reports its first uncomposed callee");
    require(player.status == 3 && actor.orientation.flags == 2,
            "native dispatcher borrows and updates the caller's live flags before the seam");
    require(contact.direction.x == 0 && contact.direction.y == 1 && contact.direction.z == 0,
            "native direction transform is retained before the seam");
    require(contact.radius == 9 && contact.center.x == 11 && contact.center.y == 12 &&
                contact.center.z == 13 && actor.position.x == 1 && actor.position.y == 2 &&
                actor.position.z == 3,
            "stopped dispatcher preserves the stores that belong after the seam");
    require(globals.special_surface == (alternate ? 6U : 0U),
            "ordinary and alternate dispatcher keep their distinct initial stores");
}
} // namespace
int main() {
    for (const auto precision : {0U, 512U, 768U})
        for (const auto rounding : {0U, 1024U, 2048U, 3072U})
            for (const bool alternate : {false, true})
                check(static_cast<std::uint16_t>(precision | rounding | 127U), alternate);
    std::cout << "Native scene dispatcher: both paths preserve live owners and partial stores "
                 "at the uncomposed contact initializer in all 12 x87 profiles.\n";
}
