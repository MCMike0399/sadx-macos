#include "player/character_shadow.hpp"

namespace sadx::character_shadow {
namespace {
// Current shadow attributes that mark a water contact.
constexpr std::uint32_t kWaterAttributes = 0x400002;
constexpr float kLandQueryRadius = 200.0f;
// The source defaults before the candidate walk.
constexpr float kTopDefault = 1000000.0f;
constexpr float kBottomDefault = -1000000.0f;

// The source compares with FCOMP and tests C0/C2: a small helper keeps each
// ordered-less branch explicit and leaves greater, equal and unordered out.
bool ordered_less(const fidelity::ExactFloat &left, float right) {
    return left.compare(right) == X87Comparison::less;
}
} // namespace

void update(const Body &body, ShadowWork &work, Services &services,
            fidelity::Context &arithmetic) {
    if ((work.top_attributes & kWaterAttributes) != 0) {
        if ((work.prior_top_attributes & kWaterAttributes) == 0)
            services.splash();
        services.ripple(body.player_number, NinjaVector{body.x, work.top_y, body.z},
                        std::array<std::int32_t, 3>{work.angle_x, body.yaw, work.angle_z});
    }
    work.prior_top_attributes = work.top_attributes;
    work.prior_bottom_attributes = work.bottom_attributes;
    work.top_attributes = 0;
    work.bottom_attributes = 0;

    const auto raised_y = (fidelity::ExactFloat(arithmetic, body.y) + 1.0f).rounded();
    const NinjaVector point{body.x, raised_y, body.z};
    const auto candidates =
        services.list_land(NinjaVector{body.x, body.y, body.z}, kLandQueryRadius);
    work.top_y = kTopDefault;
    work.bottom_y = kBottomDefault;

    const auto count = static_cast<std::int16_t>(candidates.size());
    for (std::uint16_t index = 0; static_cast<std::int32_t>(index) < count; ++index) {
        const auto &candidate = candidates.data[index];
        bool culled = false;
        if ((candidate.attributes & kWaterAttributes) != 0) {
            const auto hit = services.get_zx_shadow(point, candidate);
            if (hit.found && hit.upper_found &&
                ordered_less(fidelity::ExactFloat(arithmetic, point.y), hit.upper_position) &&
                ordered_less(fidelity::ExactFloat(arithmetic, hit.upper_position), work.top_y)) {
                work.top_y = hit.upper_position;
                work.top_attributes = candidate.attributes;
            }
        } else if (candidate.has_task) {
            // The complex-object cull: horizontal distance against the largest
            // model scale times the model radius. Unordered keeps the candidate.
            const auto offset_x =
                (fidelity::ExactFloat(arithmetic, candidate.object_x) - point.x).rounded();
            const auto offset_z =
                (fidelity::ExactFloat(arithmetic, candidate.object_z) - point.z).rounded();
            const auto distance =
                services.horizontal_distance(NinjaVector{offset_x, 0.0f, offset_z});
            auto extent = fidelity::ExactFloat(arithmetic, candidate.scale_x);
            if (ordered_less(extent, candidate.scale_y))
                extent = fidelity::ExactFloat(arithmetic, candidate.scale_y);
            if (ordered_less(extent, candidate.scale_z))
                extent = fidelity::ExactFloat(arithmetic, candidate.scale_z);
            culled = distance.compare(extent * candidate.radius) == X87Comparison::greater;
        }
        if (culled)
            continue;

        auto hit = services.polygon_test(point, candidate);
        if (!hit.found)
            continue;
        if (candidate.vertical_offset) {
            // Both intersections shift by the associated task's force-work y
            // speed; the source stores each result back to binary32.
            hit.upper_position =
                (fidelity::ExactFloat(arithmetic, hit.upper_position) +
                 *candidate.vertical_offset)
                    .rounded();
            hit.lower_position =
                (fidelity::ExactFloat(arithmetic, hit.lower_position) +
                 *candidate.vertical_offset)
                    .rounded();
        }
        if (hit.upper_found &&
            ordered_less(fidelity::ExactFloat(arithmetic, point.y), hit.upper_position) &&
            ordered_less(fidelity::ExactFloat(arithmetic, hit.upper_position), work.top_y)) {
            work.top_y = hit.upper_position;
            work.top_attributes = candidate.attributes;
        }
        const auto relation =
            fidelity::ExactFloat(arithmetic, point.y).compare(hit.lower_position);
        if (hit.lower_found &&
            (relation == X87Comparison::greater || relation == X87Comparison::equal) &&
            ordered_less(fidelity::ExactFloat(arithmetic, work.bottom_y), hit.lower_position)) {
            work.bottom_y = hit.lower_position;
            work.bottom_attributes = candidate.attributes;
            work.angle_x = hit.lower_angle_x;
            work.angle_z = hit.lower_angle_z;
        }
    }
    services.draw_shadow();
}
} // namespace sadx::character_shadow
