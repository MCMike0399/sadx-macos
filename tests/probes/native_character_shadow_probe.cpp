#include "player/character_shadow.hpp"

#include <array>
#include <cstdint>
#include <cstring>

namespace {
using sadx::NinjaVector;

enum Input : unsigned {
    PosX = 0,
    PosY,
    PosZ,
    Yaw,
    PlayerIndex,
    AngleX,
    AngleZ,
    AttrTop,
    AttrBottom,
    TopY,
    BottomY,
    PriorAttrTop,
    PriorAttrBottom,
    Scale,
    CandidateCount,
    ControlWord,
    CandidateBase,
    Words = CandidateBase + 8 * 21
};
enum Script : unsigned {
    CandAttributes = 0,
    CandObjectX,
    CandObjectZ,
    CandScaleX,
    CandScaleY,
    CandScaleZ,
    CandRadius,
    CandHasTask,
    CandHasForce,
    CandTaskOffset,
    CandDistance,
    CandZxFound,
    CandZxUpperFound,
    CandZxUpperPosition,
    CandPolyFound,
    CandPolyLowerFound,
    CandPolyUpperFound,
    CandPolyLowerPosition,
    CandPolyUpperPosition,
    CandPolyLowerAngleX,
    CandPolyLowerAngleZ,
    CandWords
};
enum EventKind : std::uint32_t {
    EventSplash = 1,
    EventRipple = 2,
    EventWaterQuery = 3,
    EventDistance = 4,
    EventPolygon = 5,
    EventDraw = 6,
    EventLandQuery = 7,
};
enum Output : unsigned {
    OutAngleX = 0,
    OutAngleZ,
    OutAttrTop,
    OutAttrBottom,
    OutTopY,
    OutBottomY,
    OutPriorAttrTop,
    OutPriorAttrBottom,
    OutScale,
    OutEventCount,
    OutEvents,
    OutWords = OutEvents + 24 * 8
};
constexpr std::uintptr_t kObjectBase = 0x22201000;

float float_at(std::uint32_t word) {
    float value;
    std::memcpy(&value, &word, sizeof(value));
    return value;
}
std::uint32_t bits_of(float value) {
    std::uint32_t word;
    std::memcpy(&word, &value, sizeof(word));
    return word;
}

struct Recorder {
    std::array<std::array<std::uint32_t, 8>, 24> events{};
    unsigned count = 0;
    void push(std::uint32_t kind, std::uint32_t a = 0, std::uint32_t b = 0, std::uint32_t c = 0,
              std::uint32_t d = 0, std::uint32_t e = 0, std::uint32_t f = 0,
              std::uint32_t g = 0) {
        if (count >= events.size())
            return;
        events[count] = {kind, a, b, c, d, e, f, g};
        ++count;
    }
};

struct Adapter final : sadx::character_shadow::Services {
    const std::uint32_t *in;
    sadx::fidelity::Context &arithmetic;
    Recorder &recorder;
    std::array<sadx::character_shadow::LandCandidate, 8> lands{};
    std::array<sadx::character_shadow::ShadowHit, 8> water_hits{};
    std::array<sadx::character_shadow::ShadowHit, 8> polygon_hits{};
    std::array<float, 8> distances{};
    std::array<unsigned, 8> distance_order{};
    unsigned candidate_count = 0;
    unsigned distance_entries = 0;
    unsigned distance_index = 0;

    Adapter(const std::uint32_t *input, sadx::fidelity::Context &arithmetic, Recorder &recorder)
        : in(input), arithmetic(arithmetic), recorder(recorder) {
        candidate_count = in[CandidateCount] & 0xff;
        if (candidate_count > lands.size())
            candidate_count = static_cast<unsigned>(lands.size());
        for (unsigned index = 0; index < candidate_count; ++index) {
            const auto *script = in + CandidateBase + index * CandWords;
            lands[index] = {script[CandAttributes],
                            kObjectBase + index * 0x40,
                            float_at(script[CandObjectX]),
                            float_at(script[CandObjectZ]),
                            float_at(script[CandScaleX]),
                            float_at(script[CandScaleY]),
                            float_at(script[CandScaleZ]),
                            float_at(script[CandRadius]),
                            script[CandHasTask] != 0,
                            script[CandHasTask] != 0 && script[CandHasForce] != 0
                                ? std::optional<float>{float_at(script[CandTaskOffset])}
                                : std::optional<float>{}};
            water_hits[index] = {script[CandZxFound] != 0,
                                 false,
                                 script[CandZxUpperFound] == 1,
                                 0,
                                 float_at(script[CandZxUpperPosition]),
                                 0,
                                 0};
            polygon_hits[index] = {script[CandPolyFound] != 0,
                                   script[CandPolyLowerFound] == 1,
                                   script[CandPolyUpperFound] == 1,
                                   float_at(script[CandPolyLowerPosition]),
                                   float_at(script[CandPolyUpperPosition]),
                                   static_cast<std::int32_t>(script[CandPolyLowerAngleX]),
                                   static_cast<std::int32_t>(script[CandPolyLowerAngleZ])};
            distances[index] = float_at(script[CandDistance]);
            if ((lands[index].attributes & 0x400002) == 0 && lands[index].has_task)
                distance_order[distance_entries++] = index;
        }
    }
    sadx::character_shadow::LandCandidates list_land(const NinjaVector &point,
                                                     float radius) override {
        recorder.push(EventLandQuery, bits_of(point.x), bits_of(point.y), bits_of(point.z),
                      bits_of(radius));
        return {lands.data(), candidate_count};
    }
    sadx::fidelity::ExactFloat horizontal_distance(const NinjaVector &offset) override {
        recorder.push(EventDistance, bits_of(offset.x), bits_of(offset.y), bits_of(offset.z));
        const auto index = distance_index < distance_entries ? distance_order[distance_index] : 0;
        if (distance_index < distance_entries)
            ++distance_index;
        return sadx::fidelity::ExactFloat(arithmetic, distances[index]);
    }
    sadx::character_shadow::ShadowHit get_zx_shadow(
        const NinjaVector &point, const sadx::character_shadow::LandCandidate &candidate) override {
        recorder.push(EventWaterQuery, static_cast<std::uint32_t>(candidate.object),
                      bits_of(point.x), bits_of(point.y), bits_of(point.z));
        const auto index = static_cast<unsigned>((candidate.object - kObjectBase) / 0x40);
        return index < candidate_count ? water_hits[index]
                                       : sadx::character_shadow::ShadowHit{};
    }
    sadx::character_shadow::ShadowHit polygon_test(
        const NinjaVector &, const sadx::character_shadow::LandCandidate &candidate) override {
        recorder.push(EventPolygon, static_cast<std::uint32_t>(candidate.object),
                      candidate.attributes);
        const auto index = static_cast<unsigned>((candidate.object - kObjectBase) / 0x40);
        return index < candidate_count ? polygon_hits[index]
                                       : sadx::character_shadow::ShadowHit{};
    }
    void splash() override { recorder.push(EventSplash); }
    void ripple(std::uint8_t player_number, const NinjaVector &position,
                const std::array<std::int32_t, 3> &angles) override {
        recorder.push(EventRipple, player_number, bits_of(position.x), bits_of(position.y),
                      bits_of(position.z), static_cast<std::uint32_t>(angles[0]),
                      static_cast<std::uint32_t>(angles[1]),
                      static_cast<std::uint32_t>(angles[2]));
    }
    void draw_shadow() override { recorder.push(EventDraw); }
};
} // namespace

extern "C" int sadx_native_character_shadow(const std::uint32_t *input,
                                            std::uint32_t *output) {
    try {
        using namespace sadx;
        Recorder recorder;
        fidelity::Context arithmetic(static_cast<std::uint16_t>(input[ControlWord]));
        Adapter services(input, arithmetic, recorder);
        const character_shadow::Body body{float_at(input[PosX]), float_at(input[PosY]),
                                          float_at(input[PosZ]),
                                          static_cast<std::int32_t>(input[Yaw]),
                                          static_cast<std::uint8_t>(input[PlayerIndex])};
        std::int32_t angle_x = static_cast<std::int32_t>(input[AngleX]);
        std::int32_t angle_z = static_cast<std::int32_t>(input[AngleZ]);
        std::uint32_t attr_top = input[AttrTop];
        std::uint32_t attr_bottom = input[AttrBottom];
        float top_y = float_at(input[TopY]);
        float bottom_y = float_at(input[BottomY]);
        std::uint32_t prior_attr_top = input[PriorAttrTop];
        std::uint32_t prior_attr_bottom = input[PriorAttrBottom];
        float scale = float_at(input[Scale]);
        character_shadow::ShadowWork work{angle_x,        angle_z,
                                          attr_top,       attr_bottom,
                                          top_y,          bottom_y,
                                          prior_attr_top, prior_attr_bottom,
                                          scale};
        character_shadow::update(body, work, services, arithmetic);

        std::memset(output, 0, OutWords * sizeof(std::uint32_t));
        output[OutAngleX] = static_cast<std::uint32_t>(angle_x);
        output[OutAngleZ] = static_cast<std::uint32_t>(angle_z);
        output[OutAttrTop] = attr_top;
        output[OutAttrBottom] = attr_bottom;
        output[OutTopY] = bits_of(top_y);
        output[OutBottomY] = bits_of(bottom_y);
        output[OutPriorAttrTop] = prior_attr_top;
        output[OutPriorAttrBottom] = prior_attr_bottom;
        output[OutScale] = bits_of(scale);
        output[OutEventCount] = recorder.count;
        for (unsigned index = 0; index < recorder.count; ++index)
            for (unsigned word = 0; word < 8; ++word)
                output[OutEvents + index * 8 + word] = recorder.events[index][word];
        return 1;
    } catch (...) {
        return 0;
    }
}
