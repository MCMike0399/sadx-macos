#pragma once
#include "apps/native_assets.hpp"
#include "game/stage_lighting.hpp"
#include <cstring>

namespace sadx::native::app {
// NOTSA: importer-verified finite floats are decoded once into native semantic records.
inline float lighting_number(id value, const char *name) {
    if (![value isKindOfClass:NSNumber.class])
        throw std::runtime_error(std::string("invalid lighting number: ") + name);
    const auto number = [value doubleValue];
    const auto rounded = static_cast<float>(number);
    if (!std::isfinite(number) || !std::isfinite(rounded))
        throw std::runtime_error(std::string("nonfinite lighting value: ") + name);
    // NSJSONSerialization may round the decimal through NSDecimalNumber first.
    // Source fields are binary32; the native importer check compares their resulting bits.
    return rounded;
}
inline std::array<float, 3> lighting_vector(id value, const char *name) {
    const auto values = array(value, 3, name);
    return {lighting_number(values[0], name), lighting_number(values[1], name),
            lighting_number(values[2], name)};
}
inline lighting::Environment lighting_environment(id value) {
    const auto record = dictionary(value, "lighting environment");
    return {
        static_cast<std::uint8_t>(integer(record[@"stage"], 0, 255, "lighting stage")),
        static_cast<std::uint8_t>(integer(record[@"act"], 0, 255, "lighting act")),
        static_cast<std::uint8_t>(integer(record[@"light_index"], 0, 255, "lighting index")),
        static_cast<std::uint8_t>(integer(record[@"flags"], 0, 255, "lighting flags")),
        lighting_vector(record[@"direction"], "lighting direction"),
        lighting_number(record[@"specular"], "lighting specular"),
        lighting_number(record[@"diffuse"], "lighting diffuse"),
        lighting_vector(record[@"color"], "lighting color"),
        lighting_vector(record[@"ambient"], "lighting ambient")};
}
inline lighting::PaletteEnvironment lighting_palette(id value) {
    const auto record = dictionary(value, "lighting palette");
    return {
        static_cast<std::uint8_t>(integer(record[@"stage"], 0, 255, "palette stage")),
        static_cast<std::uint8_t>(integer(record[@"act"], 0, 255, "palette act")),
        static_cast<std::uint8_t>(integer(record[@"character"], 0, 255, "palette character")),
        static_cast<std::uint8_t>(integer(record[@"flags"], 0, 255, "palette flags")),
        lighting_vector(record[@"direction"], "palette direction"),
        lighting_number(record[@"diffuse"], "palette diffuse"),
        lighting_vector(record[@"ambient"], "palette ambient"),
        lighting_number(record[@"upper_color_power"], "upper color power"),
        lighting_vector(record[@"upper_color"], "upper color"),
        lighting_number(record[@"upper_specular_power"], "upper specular power"),
        lighting_vector(record[@"upper_specular"], "upper specular"),
        lighting_number(record[@"lower_color_power"], "lower color power"),
        lighting_vector(record[@"lower_color"], "lower color"),
        lighting_number(record[@"lower_specular_power"], "lower specular power"),
        lighting_vector(record[@"lower_specular"], "lower specular")};
}
inline lighting::Tables load_lighting_assets(const std::filesystem::path &directory) {
    const auto source = read_metadata(directory / "stage-lighting.json");
    lighting::Tables result{};
    result.default_stage = lighting_environment(source[@"default_stage"]);
    result.default_character = lighting_environment(source[@"default_character"]);
    const auto stages = array(source[@"stages"], result.stages.size(), "stage lighting table");
    const auto palettes = array(source[@"palettes"], result.palettes.size(), "palette lighting table");
    for (std::size_t index = 0; index < result.stages.size(); ++index)
        result.stages[index] = lighting_environment(stages[index]);
    for (std::size_t index = 0; index < result.palettes.size(); ++index)
        result.palettes[index] = lighting_palette(palettes[index]);
    return result;
}
} // namespace sadx::native::app
