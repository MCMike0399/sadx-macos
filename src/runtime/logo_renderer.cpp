#include "runtime/logo_renderer.hpp"
#include "engine/ninja_queue_native.hpp"
#include "engine/ninja_texture.hpp"
#include "runtime/engine_code_tables.hpp"
#include <algorithm>
#include <fstream>
#include <stdexcept>

namespace sadx::native {
namespace {
// NOTSA: imported Windows asset filenames use a canonical ASCII uppercase spelling.
std::string asset_name(std::string_view name) {
    std::string result(name);
    for (auto &character : result) {
        if (character >= 'a' && character <= 'z')
            character = static_cast<char>(character - 'a' + 'A');
    }
    return result;
}
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
                           graphics::Frame &frame,
                           NinjaDepthProjection &projection, X87Context &fp, SseContext &sse,
                           SseReciprocalEstimate reciprocal, texture_lifetime::State textures,
                           texture_selection::State selection,
                           const texture_lifetime::List *&queued_list, std::uint32_t &queued_index)
    : system_directory_(std::move(directory)), state_(state), projection_(projection),
      arithmetic_(fp), sse_(sse), reciprocal_(std::move(reciprocal)), frame_(frame),
      texture_state_(textures), texture_selection_(selection), queued_texture_list_(queued_list),
      queued_texture_index_(queued_index) {
    // SEGALOGO_E_TEXLIST has eighteen named entries; only attributes/handles start zero.
    constexpr std::array<const char *, 18> names{
        "segalogo0", "segalogo1", "segalogo2", "segalogo3", "segalogo4", "segalogo5",
        "mwlogo0", "mwlogo1", "mwlogo2", "mwlogo3", "mwlogo4", "mwlogo5",
        "stlogo0", "stlogo1", "stlogo2", "stlogo3", "stlogo4", "stlogo5"};
    texture_list_.entries.resize(names.size());
    texture_list_.attributes.resize(names.size());
    for (const auto *name : names)
        texture_list_.filenames.push_back(std::make_shared<const std::string>(name));
}

void LogoRenderer::begin_frame() {
    frame_.commands.clear();
    // [platform:graphics] The source begins a scene by clearing its color and depth buffers.
    frame_.commands.emplace_back(graphics::Clear{3, background_, 1065353216U, 0});
}
std::vector<texture_lifetime::Memory *> LogoRenderer::load_pvm_archive(std::string_view name) {
    const auto path = system_directory_ / (asset_name(name) + ".PVM");
    std::ifstream file(path, std::ios::binary);
    if (!file)
        throw std::runtime_error("missing native texture archive: " + path.string());
    auto parsed = parse_ninja_texture_resource({std::istreambuf_iterator<char>(file), {}});
    if (!parsed)
        throw std::runtime_error("invalid native PVM: " + path.string());
    auto archive = std::make_shared<NinjaTextureArchive>(std::move(parsed.archive));
    std::vector<texture_lifetime::Memory *> loaded;
    for (std::size_t index = 0; index < archive->records.size(); ++index) {
        const auto &record = archive->records[index];
        if (!record.table_global_index)
            throw std::runtime_error("PVM has no table global texture index");
        loaded.push_back(load_record(archive, index, *record.table_global_index));
    }
    return loaded;
}
texture_lifetime::Memory *LogoRenderer::load_record(
    const std::shared_ptr<NinjaTextureArchive> &archive, std::size_t index,
    std::uint32_t global_index) {
    const auto &record = archive->records.at(index);
    const std::shared_ptr<const std::vector<std::uint8_t>> source_bytes(archive, &archive->bytes);
    auto *slot = texture_lifetime::find_memory(global_index, texture_state_);
    if (!slot)
        throw std::runtime_error("native texture pool is full");
    std::shared_ptr<graphics::Texture> texture;
    if (slot->references == 0) {
        const auto decoded = decode_ninja_texture(*archive, index);
        if (!decoded || decoded.levels.empty())
            throw std::runtime_error("unsupported native texture encoding");
        texture = std::make_shared<graphics::Texture>();
        for (std::size_t mip = 0; mip < decoded.levels.size(); ++mip) {
            auto converted = texture_level(decoded.levels[mip], record.pixel_format);
            if (mip == 0)
                *texture = std::move(converted);
            else
                texture->mipmaps.push_back(std::move(converted));
        }
    }
    auto prepared = texture_lifetime::prepare_pvr(
        global_index, record, archive->bytes.data() + record.chunk.offset);
    auto *memory = texture_lifetime::acquire_prepared(
        std::move(prepared), std::move(texture), texture_state_, source_bytes);
    if (!memory)
        throw std::runtime_error("native texture pool is full");
    return memory;
}
std::int32_t LogoRenderer::load_individual_textures(texture_lifetime::List &list) {
    if (list.filenames.size() != list.entries.size() || list.attributes.size() != list.entries.size())
        throw std::runtime_error("individual textures require imported filename and attribute members");
    for (std::size_t index = 0; index < list.entries.size(); ++index) {
        // stSetPaletteBank(NULL) finds no entry: native raw 16-bit loads never register
        // indexed palettes, and source device initialization marks empty identities -1.
        if (!list.filenames[index] || list.attributes[index] != 0)
            throw std::runtime_error("native individual texture type is not recovered");
        const auto path = system_directory_ / (asset_name(*list.filenames[index]) + ".PVR");
        std::ifstream file(path, std::ios::binary);
        if (!file)
            throw std::runtime_error("missing imported individual texture: " + path.string());
        auto parsed = parse_ninja_texture_resource({std::istreambuf_iterator<char>(file), {}});
        if (!parsed || parsed.archive.records.size() != 1)
            throw std::runtime_error("invalid imported individual PVR: " + path.string());
        auto archive = std::make_shared<NinjaTextureArchive>(std::move(parsed.archive));
        const auto global_index = archive->records[0].gbix_global_index.value_or(4294967279U);
        list.entries[index] = load_record(archive, 0, global_index);
    }
    return 1;
}
void LogoRenderer::load_logo_archive(std::string_view name) {
    if (!archive_loader_)
        throw std::runtime_error("native logo requires the effective profile archive loader");
    archive_loader_(name, texture_list_);
    textures_.clear();
    for (auto *memory : texture_list_.entries) {
        if (!memory)
            throw std::runtime_error("native logo texture did not load");
        textures_.push_back({static_cast<std::uint32_t>(memory->global_index), memory});
    }
}
void LogoRenderer::select_logo_archive() {
    if (textures_.empty())
        throw std::runtime_error("logo texture selection before resource load");
    select_texture_list(texture_list_);
}
void LogoRenderer::select_texture_list(const texture_lifetime::List &list) {
    texture_selection::select_queued_list(&list, queued_texture_list_, queued_texture_index_,
                                          texture_selection_, texture_binding());
}
void LogoRenderer::select_direct_texture_list(const texture_lifetime::List &list) {
    texture_selection::select_list(&list, texture_selection_, texture_binding());
}
std::uint32_t LogoRenderer::select_texture_index(std::uint32_t index) {
    return texture_selection::select_index(index, texture_selection_, texture_binding());
}
texture_selection::BindSurface LogoRenderer::texture_binding() {
    return [this](const texture_selection::Surface &surface) {
        // [platform:texture] The device keeps its own immutable image reference.
        if (surface && !surface->texture)
            throw std::runtime_error("native texture surface has no uploaded image");
        bound_surface_ = surface;
        state_.texture = surface ? surface->texture : nullptr;
    };
}
void LogoRenderer::snapshot_bound_texture() {
    // [platform:texture] SetTexture retains an image independently of the manager.
    // A live handle can receive new pixels; dropping its manager reference does not
    // revoke the image already held by the device or by submitted draws.
    if (const auto surface = bound_surface_.lock(); surface && surface->texture)
        state_.texture = surface->texture;
}
void LogoRenderer::unbind_texture() {
    bound_surface_.reset();
    state_.texture.reset();
}
void LogoRenderer::release_logo_archive() {
    texture_lifetime::Services services{[this] {
        unbind_texture();
    }};
    texture_lifetime::release_texture_list(&texture_list_, texture_state_, services);
}
std::size_t LogoRenderer::loaded_textures() const {
    return std::count_if(textures_.begin(), textures_.end(), [](const auto &slot) {
        return slot.memory && slot.memory->references != 0 &&
               static_cast<std::uint32_t>(slot.memory->global_index) == slot.global_index;
    });
}
std::size_t LogoRenderer::texture_slots_in_use() const {
    return std::count_if(texture_state_.pool.begin(), texture_state_.pool.end(),
                         [](const auto &slot) { return slot.references != 0; });
}
void LogoRenderer::set_background(std::uint32_t top, std::uint32_t middle, std::uint32_t bottom) {
    if (top != middle || top != bottom)
        throw std::runtime_error("native gradient background is not composed");
    background_ = top | 4278190080U;
}
void LogoRenderer::set_blending(std::uint32_t side, std::uint32_t mode) {
    const auto tables = runtime::engine_code_tables();
    queue::BlendTables blend{};
    for (std::size_t index = 0; index < blend.targets.size(); ++index)
        blend.targets[index] = static_cast<std::uint32_t>(tables.blend_targets[index]);
    for (std::size_t index = 0; index < blend.modes.size(); ++index)
        blend.modes[index] = static_cast<std::uint32_t>(tables.blend_modes[index]);
    for (std::size_t index = 0; index < blend.following_depth_modes.size(); ++index)
        blend.following_depth_modes[index] =
            static_cast<std::uint32_t>(tables.depth_comparisons[index]);
    queue::set_device_blend(side, mode, blend, state_);
}
pvr::TextureInfo LogoRenderer::texture(std::uint32_t index) {
    const auto *list = texture_state_.current_list;
    if (!list || index >= list->entries.size() || !list->entries[index])
        throw std::runtime_error("selected texture index is unavailable");
    const auto &memory = *list->entries[index];
    // The active GVR patch ignores source surface-format flags for alpha selection.
    return {static_cast<std::uint32_t>(memory.global_index), 0, memory.width, memory.height};
}
std::uint32_t LogoRenderer::texture_global_index(const texture_lifetime::List &list,
                                                std::uint32_t index) const {
    if (index >= list.entries.size() || !list.entries[index])
        throw std::runtime_error("native texture global index is unavailable");
    return static_cast<std::uint32_t>(list.entries[index]->global_index);
}
pvr::ImmediateDevice LogoRenderer::immediate_device() {
    return {state_, frame_, [this](float reciprocal_depth) {
                return arithmetic_.to_float(ninja_projected_depth(
                    reciprocal_depth, projection_, sse_, arithmetic_, reciprocal_));
            }};
}
void LogoRenderer::draw_texture(const std::array<pvr::Vertex, 4> &vertices,
                                std::uint32_t global_index, std::uint32_t alpha) {
    const auto found = std::find_if(texture_state_.pool.begin(), texture_state_.pool.end(),
                                    [&](const auto &memory) {
        return static_cast<std::uint32_t>(memory.global_index) == global_index;
    });
    if (found == texture_state_.pool.end())
        throw std::runtime_error("native texture resource has been released");
    auto *memory = &*found;
    texture_selection::cache(memory, texture_selection_, texture_binding());
    snapshot_bound_texture();
    auto device = immediate_device();
    const auto fog = state_.render_states.find(28);
    pvr::submit_texture(vertices, state_.texture, alpha,
                        fog != state_.render_states.end() && fog->second != 0, device, arithmetic_);
}
void LogoRenderer::draw_polygon(const std::array<pvr::Point, 4> &points, std::uint32_t color,
                                float priority, std::uint32_t flags) {
    draw_colored_polygon(points, {color, color, color, color}, priority, flags);
}
void LogoRenderer::draw_colored_polygon(const std::array<pvr::Point, 4> &points,
                                       const std::array<std::uint32_t, 4> &colors,
                                       float priority, std::uint32_t flags) {
    auto device = immediate_device();
    texture_lifetime::Services services{[this] {
        unbind_texture();
    }};
    pvr::submit_colored_polygon(points, colors, priority, flags, device, arithmetic_, [&] {
        texture_lifetime::unbind_cached_texture(texture_state_, services);
    });
}
void LogoRenderer::queue_box(float, float, float, float, float, std::uint32_t) {
    throw std::runtime_error("native fog-emulation queue is not composed");
}
} // namespace sadx::native
