#include "player/player_motion.hpp"

#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

std::uint32_t bits(float value) {
    std::uint32_t result;
    std::memcpy(&result, &value, sizeof(result));
    return result;
}

bool same_vector(const sadx::NinjaVector& a, const sadx::NinjaVector& b) {
    return bits(a.x) == bits(b.x) && bits(a.y) == bits(b.y) && bits(a.z) == bits(b.z);
}

void test_clear_speed() {
    const float nan = std::numeric_limits<float>::quiet_NaN();
    sadx::MotionWork motion{{-0.0F, nan, std::numeric_limits<float>::infinity()}, {1, 2, 3}};
    sadx::PlayerMotion player{{4, 5, 6}, {7, 8, 9}};
    const auto motion_acceleration = motion.acceleration;
    const auto player_acceleration = player.acceleration;
    sadx::clear_speed(&motion, &player);
    require(same_vector(motion.speed, {0, 0, 0}), "motion positive zero speed stores");
    require(same_vector(player.speed, {0, 0, 0}), "player positive zero speed stores");
    require(same_vector(motion.acceleration, motion_acceleration), "motion acceleration must survive clear");
    require(same_vector(player.acceleration, player_acceleration), "player acceleration must survive clear");
    player.speed = {3, 4, 5};
    sadx::clear_speed(&motion, nullptr);
    require(same_vector(player.speed, {3, 4, 5}), "absent player owner must not be touched");
    motion.speed = {6, 7, 8};
    sadx::clear_speed(nullptr, &player);
    require(same_vector(motion.speed, {6, 7, 8}), "absent motion owner must not be touched");
    sadx::clear_speed(nullptr, nullptr);
}

void test_gravity_boundary() {
    sadx::X87Context context(0x027f);
    const sadx::PlayerOrientation orientation{0x2001U, {123, -456, 789}};
    const sadx::MotionWork motion{{4, 5, 6}, {1, 2, 3}};
    sadx::PlayerMotion player{{9, 8, 7}, {10, 11, 12}};
    int calls = 0;
    const sadx::WorldToPlayerTransform supplied_transform =
        [&](const sadx::PlayerOrientation& received, sadx::NinjaVector& vector) {
            ++calls;
            require(received.flags == orientation.flags && received.angles == orientation.angles,
                    "transform must receive original orientation without normalization");
            require(same_vector(vector, {3, -2, 9}), "gravity component order/weight before transform");
            // Test boundary behavior, not a claimed implementation of 0x43ec00.
            vector = {vector.z, vector.x, vector.y};
            return sadx::PlayerMotionResult{};
        };
    const auto result = sadx::get_gravity(orientation, motion, player, {1, -2, 3}, 2,
                                         supplied_transform, context);
    require(static_cast<bool>(result) && calls == 1, "gravity transform must execute exactly once");
    require(same_vector(player.acceleration, {9, 3, -2}), "transformed acceleration must be stored");
    require(same_vector(player.speed, {10, 11, 12}), "player speed must not change");
    require(same_vector(motion.acceleration, {1, 2, 3}) && same_vector(motion.speed, {4, 5, 6}),
            "source motion state must not change");

    const auto before = player.acceleration;
    auto disabled = orientation;
    disabled.flags |= 0x4000U;
    const auto skipped = sadx::get_gravity(disabled, motion, player, {0, 0, 0}, 0, {}, context);
    require(static_cast<bool>(skipped) && same_vector(player.acceleration, before),
            "flag4000 must skip without requiring an unavailable transform");
    const auto missing = sadx::get_gravity(orientation, motion, player, {0, 0, 0}, 0, {}, context);
    require(missing.failure == sadx::PlayerMotionFailure::missing_world_to_player_transform &&
            same_vector(player.acceleration, before), "missing transform must preserve output and fail");
    const sadx::WorldToPlayerTransform unavailable =
        [](const sadx::PlayerOrientation&, sadx::NinjaVector& vector) {
            vector = {100, 200, 300};
            return sadx::PlayerMotionResult{sadx::PlayerMotionFailure::world_to_player_transform_unavailable};
        };
    const auto rejected = sadx::get_gravity(orientation, motion, player, {0, 0, 0}, 0, unavailable, context);
    require(rejected.failure == sadx::PlayerMotionFailure::world_to_player_transform_unavailable &&
            same_vector(player.acceleration, before), "unavailable transform must propagate without partial stores");
    bool threw = false;
    try {
        (void)sadx::get_gravity(orientation, motion, player, {0, 0, 0}, 0,
            [](const sadx::PlayerOrientation&, sadx::NinjaVector& vector) -> sadx::PlayerMotionResult {
                vector = {300, 200, 100};
                throw std::runtime_error("native transform failure");
            }, context);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    require(threw && same_vector(player.acceleration, before), "native transform exception must preserve output");
}

void test_gravity_float_stores() {
    sadx::X87Context context(0x027f);
    const sadx::PlayerOrientation orientation{0, {0, 0, 0}};
    sadx::MotionWork motion{{1, 2, 3}, {-1.0F, -1.0F, -1.0F}};
    sadx::PlayerMotion player{{0, 0, 0}, {4, 5, 6}};
    const float adjacent = 0x1.000006p0F; // three binary32 steps above one
    const sadx::WorldToPlayerTransform explicit_identity =
        [](const sadx::PlayerOrientation&, sadx::NinjaVector&) { return sadx::PlayerMotionResult{}; };
    require(static_cast<bool>(sadx::get_gravity(orientation, motion, player,
            {adjacent, adjacent, adjacent}, adjacent, explicit_identity, context)), "supplied identity test transform");
    const float expected = static_cast<float>(static_cast<double>(adjacent) * adjacent - 1.0);
    const float premature_round = adjacent * adjacent - 1.0F;
    require(bits(expected) != bits(premature_round), "fixture must distinguish premature float multiplication");
    require(same_vector(player.acceleration, {expected, expected, expected}),
            "product must not round to float before addition");
    const float nan = std::numeric_limits<float>::quiet_NaN();
    require(static_cast<bool>(sadx::get_gravity(orientation, motion, player, {nan, 0, 1}, nan,
                                               explicit_identity, context)), "NaN source data must not be rejected");
    require(std::isnan(player.acceleration.x) && std::isnan(player.acceleration.y) &&
            std::isnan(player.acceleration.z), "NaN gravity/weight must propagate through supplied boundary");
}

void test_speed_limits() {
    sadx::X87Context context(0x027f);
    sadx::MotionWork motion{{10, -12, -11}, {1, 2, 3}};
    sadx::limit_motion_speed(motion, {5, 7}, context);
    require(same_vector(motion.speed, {5, -7, -5}), "axis-specific symmetric limits");
    require(same_vector(motion.acceleration, {1, 2, 3}), "limits must not modify acceleration");
    motion.speed = {5, 7, -5};
    sadx::limit_motion_speed(motion, {5, 7}, context);
    require(same_vector(motion.speed, {5, 7, -5}), "exact bounds remain unchanged");
    motion.speed = {-0.0F, 0.0F, -0.0F};
    sadx::limit_motion_speed(motion, {0, -0.0F}, context);
    require(same_vector(motion.speed, {-0.0F, 0.0F, -0.0F}), "signed zeros at equal bounds preserved");
    motion.speed = {0, 0, 0};
    sadx::limit_motion_speed(motion, {-5, -7}, context);
    require(same_vector(motion.speed, {5, 7, 5}), "negative limits use original sequential comparisons");
    const float nan = std::numeric_limits<float>::quiet_NaN();
    motion.speed = {nan, -100, 100};
    sadx::limit_motion_speed(motion, {nan, 7}, context);
    require(bits(motion.speed.x) == bits(nan) && motion.speed.y == -7 && motion.speed.z == 100,
            "unordered components and limits must not be normalized");
    const float infinity = std::numeric_limits<float>::infinity();
    motion.speed = {infinity, -infinity, -infinity};
    sadx::limit_motion_speed(motion, {5, 7}, context);
    require(same_vector(motion.speed, {5, -7, -5}), "infinite speeds clamp against finite bounds");
    motion.speed = {infinity, -infinity, -infinity};
    sadx::limit_motion_speed(motion, {infinity, infinity}, context);
    require(same_vector(motion.speed, {infinity, -infinity, -infinity}), "infinite equal bounds survive");
}

} // namespace

int main() {
    try {
        test_clear_speed();
        test_gravity_boundary();
        test_gravity_float_stores();
        test_speed_limits();
        std::cout << "Player motion: nullable speed clear, gravity arithmetic/required transform, "
                     "failure preservation and ordered axis limits passed.\n"
                     "PGetSpeed, original rotation and x87/runtime equivalence remain unverified.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
