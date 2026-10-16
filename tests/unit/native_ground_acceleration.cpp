#include "player/ground_acceleration.hpp"
#include "player/player_coordinates.hpp"
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void require(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}
} // namespace
int main() {
    try {
        using namespace sadx;
        using namespace sadx::physics;
        PlayerOrientation orientation{0, {0, 0, 0}};
        MotionWork world{{0, 0, 0}, {1, 0, 0}};
        PlayerMotion player{{11, 22, 33}, {0, 0, 0}};
        float alignment = 1, force = 123, overflow = 456, scale = 1;
        std::uint32_t attributes = 0;
        std::int32_t target = 0;
        GroundMotion motion{orientation, world, player,   alignment, attributes,
                            target,      force, overflow, scale};
        PlayerParameters parameters{};
        parameters.weight = 0.1f;
        parameters.jog_speed = 2;
        parameters.run_speed = 4;
        parameters.max_x_spd = 8;
        parameters.rush_speed = 6;
        parameters.run_accel = 0.2f;
        // Controlled zero-angle fixture: only sine(0) and sine(quarter-turn) are consumed.
        NinjaMath::SineTable sine{};
        sine.back() = 1;
        NinjaMatrix identity{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
        NinjaMatrixStack stack({identity, identity, identity}, 1, 0);
        PlayerCoordinates coordinates(sine, identity, stack);
        unsigned queries = 0;
        GroundEnvironment environment{{0, -1, 0}, {0, 1, 0}, &coordinates, [&](X87Context &) {
                                          ++queries;
                                          return GroundInput{0, 0, false};
                                      }};
        X87Context fp(0x023f);
        calculate_ground_acceleration(motion, parameters, environment, fp);
        require(queries == 1 && force == 0 && player.acceleration.x == 1 &&
                    player.acceleration.y == -0.1f && player.acceleration.z == 0,
                "native inactive path completes using plain state and owned input/coordinates");
        require(fp.control_word() == 0x023f, "native path retains caller precision control");

        orientation.flags = 16384;
        bool stopped = false;
        try {
            calculate_ground_acceleration(motion, parameters, environment, fp);
        } catch (const UnavailableGroundOperation &error) {
            stopped = std::string(error.what()) == "event_acceleration";
        }
        require(stopped && queries == 1, "native event owner stops before querying ordinary input");

        orientation.flags = 0;
        force = 123;
        environment.query_input = [&](X87Context &) {
            ++queries;
            return GroundInput{16384, 1, true};
        };
        stopped = false;
        try {
            calculate_ground_acceleration(motion, parameters, environment, fp);
        } catch (const UnavailableGroundOperation &error) {
            stopped = std::string(error.what()) == "turn_ground_yaw";
        }
        require(stopped && target == 16384 && force == 123,
                "missing native yaw retains its prior target store and stops before force store");
        std::cout << "Native ground: source-independent inactive math, owned input, explicit "
                     "event/yaw stops and prefix stores passed.\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
