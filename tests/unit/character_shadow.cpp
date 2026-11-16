#include "player/character_shadow.hpp"

#include <array>
#include <cstdint>
#include <cstring>
#include <optional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using sadx::NinjaVector;

void require(bool value, const std::string &message) {
    if (!value)
        throw std::runtime_error(message);
}

float nan() {
    std::uint32_t bits = 0x7fc00000u;
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

struct Fixture {
    sadx::character_shadow::Body body{10, 20, -20, 16384, 7};
    std::int32_t angle_x = 0x11;
    std::int32_t angle_z = 0x22;
    std::uint32_t top_attributes = 0;
    std::uint32_t bottom_attributes = 0;
    float top_y = 2;
    float bottom_y = -3;
    std::uint32_t prior_top_attributes = 0;
    std::uint32_t prior_bottom_attributes = 0;
    float scale = 1.5f;
    std::vector<sadx::character_shadow::LandCandidate> lands;
    std::vector<sadx::character_shadow::ShadowHit> water_hits;
    std::vector<sadx::character_shadow::ShadowHit> polygon_hits;
    std::vector<float> distances;
    float distance = 0;
    std::size_t water_index = 0;
    std::size_t polygon_index = 0;
    std::vector<std::string> events;
    int land_calls = 0;
    NinjaVector last_query{0, 0, 0};
    float last_radius = 0;
    NinjaVector last_offset{0, 0, 0};
    NinjaVector last_ripple_position{0, 0, 0};
    std::array<std::int32_t, 3> last_ripple_angles{0, 0, 0};
    std::uint8_t last_player = 0xff;
    sadx::fidelity::Context arithmetic{0x007f};

    struct Fake final : sadx::character_shadow::Services {
        Fixture &fixture;
        explicit Fake(Fixture &fixture) : fixture(fixture) {}
        sadx::character_shadow::LandCandidates list_land(const NinjaVector &point,
                                                         float radius) override {
            ++fixture.land_calls;
            fixture.last_query = point;
            fixture.last_radius = radius;
            fixture.events.push_back("land");
            return {fixture.lands.data(), fixture.lands.size()};
        }
        sadx::fidelity::ExactFloat horizontal_distance(const NinjaVector &offset) override {
            fixture.events.push_back("distance");
            fixture.last_offset = offset;
            const auto value = fixture.distance;
            return sadx::fidelity::ExactFloat(fixture.arithmetic, value);
        }
        sadx::character_shadow::ShadowHit get_zx_shadow(
            const NinjaVector &, const sadx::character_shadow::LandCandidate &) override {
            fixture.events.push_back("zx");
            static const sadx::character_shadow::ShadowHit empty{};
            if (fixture.water_index >= fixture.water_hits.size())
                return empty;
            return fixture.water_hits[fixture.water_index++];
        }
        sadx::character_shadow::ShadowHit polygon_test(
            const NinjaVector &, const sadx::character_shadow::LandCandidate &) override {
            fixture.events.push_back("poly");
            static const sadx::character_shadow::ShadowHit empty{};
            if (fixture.polygon_index >= fixture.polygon_hits.size())
                return empty;
            return fixture.polygon_hits[fixture.polygon_index++];
        }
        void splash() override { fixture.events.push_back("splash"); }
        void ripple(std::uint8_t player_number, const NinjaVector &position,
                    const std::array<std::int32_t, 3> &angles) override {
            fixture.events.push_back("ripple");
            fixture.last_player = player_number;
            fixture.last_ripple_position = position;
            fixture.last_ripple_angles = angles;
        }
        void draw_shadow() override { fixture.events.push_back("draw"); }
    };

    void run() {
        Fake services(*this);
        sadx::character_shadow::ShadowWork work{
            angle_x, angle_z, top_attributes, bottom_attributes, top_y, bottom_y,
            prior_top_attributes, prior_bottom_attributes, scale};
        sadx::character_shadow::update(body, work, services, arithmetic);
    }
};

sadx::character_shadow::LandCandidate candidate(std::uint32_t attributes,
                                                bool has_task = false,
                                                std::optional<float> offset = {}) {
    return {attributes, 0x1234, 100, -50, 2, 3, 4, 10, has_task, offset};
}

sadx::character_shadow::ShadowHit hit(bool found, bool lower_found, bool upper_found,
                                      float lower, float upper, std::int32_t angle_x,
                                      std::int32_t angle_z) {
    return {found, lower_found, upper_found, lower, upper, angle_x, angle_z};
}
} // namespace

int main() {
    using namespace sadx;
    { // Empty land: prior attributes rotate, current clear, defaults, draw last.
        Fixture f;
        f.top_attributes = 0x400002;
        f.bottom_attributes = 0x40;
        f.prior_top_attributes = 0x1;
        f.lands = {};
        f.run();
        require(f.events == std::vector<std::string>{"splash", "ripple", "land", "draw"},
                "water splash and ripple on the splash edge");
        require(f.prior_top_attributes == 0x400002, "prior top rotates");
        require(f.prior_bottom_attributes == 0x40, "prior bottom rotates");
        require(f.top_attributes == 0 && f.bottom_attributes == 0, "current attributes clear");
        require(f.top_y == 1000000.0f && f.bottom_y == -1000000.0f, "defaults stored");
        require(f.last_player == 7, "ripple player number");
        require(f.last_ripple_position.x == 10 && f.last_ripple_position.y == 2 &&
                    f.last_ripple_position.z == -20,
                "ripple position uses the previous top height");
        require(f.last_ripple_angles[0] == 0x11 && f.last_ripple_angles[1] == 16384 &&
                    f.last_ripple_angles[2] == 0x22,
                "ripple angles");
        require(f.last_query.x == 10 && f.last_query.y == 20 && f.last_query.z == -20,
                "land query uses the unraised position");
        require(f.last_radius == 200.0f, "land query radius");
        require(f.angle_x == 0x11 && f.angle_z == 0x22, "empty land keeps angles");
    }
    { // Prior already water: ripple without splash.
        Fixture f;
        f.top_attributes = 0x400002;
        f.prior_top_attributes = 0x400000;
        f.run();
        require(f.events == std::vector<std::string>{"ripple", "land", "draw"},
                "no splash when the prior attributes were water");
    }
    { // Dry top: no effect calls.
        Fixture f;
        f.top_attributes = 0x1;
        f.prior_top_attributes = 0x400002;
        f.run();
        require(f.events == std::vector<std::string>{"land", "draw"},
                "no ripple when the current top is dry");
    }
    { // Nearest above updates the top and its attributes.
        Fixture f;
        f.lands = {candidate(0x10)};
        f.polygon_hits = {hit(true, false, true, 0, 205, 0, 0)};
        f.run();
        require(f.events == std::vector<std::string>{"land", "poly", "draw"},
                "non-water without a task skips the cull");
        require(f.top_y == 205, "top updated");
        require(f.top_attributes == 0x10, "top attributes stored");
        require(f.bottom_y == -1000000.0f, "bottom stays default");
    }
    { // A lower hit below the point updates the bottom and its angles.
        Fixture f;
        f.lands = {candidate(0x20)};
        f.polygon_hits = {hit(true, true, false, 5, 0, 0x33, 0x44)};
        f.run();
        require(f.bottom_y == 5, "bottom updated");
        require(f.bottom_attributes == 0x20, "bottom attributes stored");
        require(f.angle_x == 0x33 && f.angle_z == 0x44, "bottom angles stored");
        require(f.top_y == 1000000.0f, "top stays default");
    }
    { // An equal lower hit counts as not above the point.
        Fixture f;
        f.body.y = 4;
        f.lands = {candidate(0x20)};
        f.polygon_hits = {hit(true, true, false, 5, 0, 0x33, 0x44)};
        f.run();
        require(f.bottom_y == 5, "equal lower hit accepted");
    }
    { // Complex-object distance cull skips the polygon test.
        Fixture f;
        f.lands = {candidate(0x10, true, 0.0f)};
        f.distance = 100;
        f.run();
        require(f.events == std::vector<std::string>{"land", "distance", "draw"},
                "culled candidate skips the polygon");
        require(f.top_y == 1000000.0f && f.bottom_y == -1000000.0f, "cull changes nothing");
        require(f.last_offset.x == 90 && f.last_offset.y == 0 && f.last_offset.z == -30,
                "offset is object minus point");
    }
    { // A task without force work still culls but does not shift the hits.
        Fixture f;
        f.lands = {candidate(0x10, true)};
        f.distance = 1;
        f.polygon_hits = {hit(true, false, true, 0, 100, 0, 0)};
        f.run();
        require(f.events == std::vector<std::string>{"land", "distance", "poly", "draw"},
                "task without force work still takes the cull");
        require(f.top_y == 100, "no offset shift without force work");
    }
    { // At the limit the candidate is kept; only an ordered greater culls.
        Fixture f;
        f.lands = {candidate(0x10, true, 0.0f)};
        f.distance = 40;
        f.polygon_hits = {hit(true, false, true, 0, 100, 0, 0)};
        f.run();
        require(f.events == std::vector<std::string>{"land", "distance", "poly", "draw"},
                "equal distance keeps the candidate");
        require(f.top_y == 100, "kept candidate updates the top");
    }
    { // Unordered distance keeps the candidate.
        Fixture f;
        f.lands = {candidate(0x10, true, 0.0f)};
        f.distance = nan();
        f.polygon_hits = {hit(true, false, true, 0, 100, 0, 0)};
        f.run();
        require(f.events == std::vector<std::string>{"land", "distance", "poly", "draw"},
                "unordered distance keeps the candidate");
    }
    { // NaN point height never updates either contact.
        Fixture f;
        f.body.y = nan();
        f.lands = {candidate(0x10)};
        f.polygon_hits = {hit(true, true, true, 5, 100, 1, 2)};
        f.run();
        require(f.top_y == 1000000.0f, "NaN point never enters the top");
        require(f.bottom_y == -1000000.0f, "NaN point never enters the bottom");
    }
    { // NaN hit positions are unordered and never enter a contact.
        Fixture f;
        f.lands = {candidate(0x10)};
        f.polygon_hits = {hit(true, true, true, nan(), nan(), 1, 2)};
        f.run();
        require(f.top_y == 1000000.0f && f.bottom_y == -1000000.0f, "NaN hits ignored");
    }
    { // The task force-work offset shifts both hits before the comparisons.
        Fixture f;
        f.lands = {candidate(0x10, true, 5.0f)};
        f.distance = 1;
        f.polygon_hits = {hit(true, true, true, 6, 100, 1, 2)};
        f.run();
        require(f.top_y == 105, "upper hit shifted");
        require(f.bottom_y == 11, "lower hit shifted");
    }
    { // Water candidates take the water query before the shared polygon test.
        Fixture f;
        f.top_attributes = 0x400002;
        f.lands = {candidate(0x400002)};
        f.water_hits = {hit(true, false, true, 0, 50, 0, 0)};
        f.polygon_hits = {hit(true, false, true, 0, 40, 0, 0)};
        f.run();
        require(f.events ==
                    std::vector<std::string>{"splash", "ripple", "land", "zx", "poly", "draw"},
                "water candidate runs both queries");
        require(f.top_y == 40, "the shared test's upper hit wins after the water test");
        require(f.top_attributes == 0x400002, "water top attributes stored");
    }
    { // Two candidates: the nearest below and the highest not above win.
        Fixture f;
        f.lands = {candidate(0x10), candidate(0x20)};
        f.polygon_hits = {hit(true, true, true, 3, 300, 1, 2),
                          hit(true, true, true, 7, 90, 3, 4)};
        f.run();
        require(f.top_y == 90, "lowest upper hit wins");
        require(f.top_attributes == 0x20, "winning top attributes");
        require(f.bottom_y == 7, "highest lower hit wins");
        require(f.bottom_attributes == 0x20, "winning bottom attributes");
        require(f.angle_x == 3 && f.angle_z == 4, "winning bottom angles");
    }
    std::cout << "character shadow: water edge, land cull, contacts and angles\n";
    return 0;
}
