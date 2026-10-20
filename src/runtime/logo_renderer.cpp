#include "runtime/logo_renderer.hpp"
#include "engine/ninja_texture.hpp"
#include "runtime/engine_code_tables.hpp"
#include <algorithm>
#include <fstream>
#include <stdexcept>

namespace sadx::native {
namespace {
// [platform:texture] Preserve packed-format normalization until GPU sampling. Expanding
// through integer RGBA8 first loses precision and differs from the reference device.
graphics::Texture texture_level(const NinjaTextureLevel &level, std::uint8_t format) {
    graphics::Texture texture;
    texture.width = level.width;
    texture.height = level.height;
    texture.format = graphics::TextureFormat::Rgba32Float;
    texture.rgba.reserve(level.packed16.size() * 4);
    for (const auto word : level.packed16) {
        std::array<float, 4> color;
        if (format == 0)
            color = {float((word >> 10) & 31) / 31, float((word >> 5) & 31) / 31,
                     float(word & 31) / 31, float(word >> 15)};
        else if (format == 1)
            color = {float((word >> 11) & 31) / 31, float((word >> 5) & 63) / 63,
                     float(word & 31) / 31, 1};
        else if (format == 2)
            color = {float((word >> 8) & 15) / 15, float((word >> 4) & 15) / 15,
                     float(word & 15) / 15, float(word >> 12) / 15};
        else
            throw std::runtime_error("unsupported native texture normalization");
        texture.rgba.insert(texture.rgba.end(), color.begin(), color.end());
    }
    return texture;
}
} // namespace

LogoRenderer::LogoRenderer(std::filesystem::path directory, graphics::Draw &state,
                           NinjaDepthProjection &projection, X87Context &fp, SseContext &sse,
                           SseReciprocalEstimate reciprocal)
    : system_directory_(std::move(directory)), state_(state), projection_(projection),
      arithmetic_(fp), sse_(sse), reciprocal_(std::move(reciprocal)) {}

void LogoRenderer::begin_frame() {
    frame_.commands.clear();
    // [platform:graphics] The source begins a scene by clearing its color and depth buffers.
    frame_.commands.emplace_back(graphics::Clear{3, background_, 1065353216U, 0});
}
void LogoRenderer::load_logo_archive(std::string_view name) {
    const auto path = system_directory_ / (std::string(name) + ".PVM");
    std::ifstream file(path, std::ios::binary);
    if (!file)
        throw std::runtime_error("missing native logo asset: " + path.string());
    auto parsed = parse_ninja_texture_resource({std::istreambuf_iterator<char>(file), {}});
    if (!parsed)
        throw std::runtime_error("invalid native logo PVM: " + path.string());
    std::vector<TextureSlot> loaded;
    for (std::size_t index = 0; index < parsed.archive.records.size(); ++index) {
        const auto &record = parsed.archive.records[index];
        if (!record.table_global_index)
            throw std::runtime_error("logo PVM has no table global texture index");
        if (std::any_of(loaded.begin(), loaded.end(), [&](const auto &slot) {
                return slot.global_index == *record.table_global_index;
            }))
            throw std::runtime_error("duplicate logo global texture index");
        const auto decoded = decode_ninja_texture(parsed.archive, index);
        if (!decoded || decoded.levels.empty())
            throw std::runtime_error("unsupported native logo texture encoding");
        auto texture = std::make_shared<graphics::Texture>();
        for (std::size_t mip = 0; mip < decoded.levels.size(); ++mip) {
            const auto &level = decoded.levels[mip];
            auto converted = texture_level(level, record.pixel_format);
            if (mip == 0)
                *texture = std::move(converted);
            else
                texture->mipmaps.push_back(std::move(converted));
        }
        loaded.push_back({*record.table_global_index, std::move(texture)});
    }
    if (loaded.size() != 18)
        throw std::runtime_error("logo archive does not match its eighteen-entry texture list");
    textures_ = std::move(loaded);
    selected_ = false;
}
void LogoRenderer::select_logo_archive() {
    if (textures_.empty())
        throw std::runtime_error("logo texture selection before resource load");
    selected_ = true;
}
void LogoRenderer::release_logo_archive() {
    textures_.clear();
    selected_ = false;
}
void LogoRenderer::set_background(std::uint32_t top, std::uint32_t middle, std::uint32_t bottom) {
    if (top != middle || top != bottom)
        throw std::runtime_error("native gradient background is not composed");
    background_ = top | 4278190080U;
}
void LogoRenderer::set_blending(std::uint32_t side, std::uint32_t mode) {
    const auto tables = runtime::engine_code_tables();
    if (side >= tables.blend_targets.size() || mode >= tables.blend_modes.size())
        throw std::runtime_error("invalid Ninja blending mode");
    state_.render_states[static_cast<std::uint32_t>(tables.blend_targets[side])] =
        static_cast<std::uint32_t>(tables.blend_modes[mode]);
}
pvr::TextureInfo LogoRenderer::texture(std::uint32_t index) {
    if (!selected_ || index >= textures_.size())
        throw std::runtime_error("logo texture index is unavailable");
    const auto &slot = textures_[index];
    // The active GVR patch ignores source surface-format flags for alpha selection.
    return {slot.global_index, 0, slot.texture->width, slot.texture->height};
}
pvr::ImmediateDevice LogoRenderer::immediate_device() {
    return {state_, frame_, [this](float reciprocal_depth) {
                return arithmetic_.to_float(ninja_projected_depth(
                    reciprocal_depth, projection_, sse_, arithmetic_, reciprocal_));
            }};
}
void LogoRenderer::draw_texture(const std::array<pvr::Vertex, 4> &vertices,
                                std::uint32_t global_index, std::uint32_t alpha) {
    const auto found = std::find_if(textures_.begin(), textures_.end(), [&](const auto &slot) {
        return slot.global_index == global_index;
    });
    if (found == textures_.end())
        throw std::runtime_error("native logo texture resource has been released");
    auto device = immediate_device();
    const auto fog = state_.render_states.find(28);
    pvr::submit_texture(vertices, found->texture, alpha,
                        fog != state_.render_states.end() && fog->second != 0, device, arithmetic_);
}
void LogoRenderer::draw_polygon(const std::array<pvr::Point, 4> &points, std::uint32_t color,
                                float priority, std::uint32_t flags) {
    auto device = immediate_device();
    pvr::submit_box(points, color, priority, flags, device, arithmetic_);
}
void LogoRenderer::queue_box(float, float, float, float, float, std::uint32_t) {
    throw std::runtime_error("native fog-emulation queue is not composed");
}
} // namespace sadx::native
