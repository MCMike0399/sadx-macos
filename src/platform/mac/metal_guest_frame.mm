#include "platform/mac/metal_guest_frame.hpp"
#include <cstring>

namespace sadx {
namespace {
// HLE resource decoding stays here; the Metal device never reads guest memory.
std::optional<graphics::Texture> decode_level(const X86Guest &memory, std::uint32_t object,
                                              unsigned level, std::string &error) {
    const auto table = memory.u32(object + 40);
    const auto surface = table ? memory.u32(*table + 4 * level) : std::nullopt;
    if (!surface) {
        error = "texture level missing";
        return {};
    }
    const auto width = memory.u32(*surface + 8), height = memory.u32(*surface + 12);
    const auto format = memory.u32(*surface + 16), pitch = memory.u32(*surface + 20),
               bits = memory.u32(*surface + 24);
    if (!width || !height || !format || !pitch || !bits || !*width || !*height) {
        error = "texture metadata";
        return {};
    }
    const bool packed16 = *format == 23 || *format == 24 || *format == 25 || *format == 26;
    if (!packed16 && *format != 21 && *format != 22) {
        error = "texture format " + std::to_string(*format);
        return {};
    }
    if (std::uint64_t(*width) * (packed16 ? 2 : 4) > *pitch) {
        error = "texture pitch";
        return {};
    }
    std::vector<std::uint8_t> raw(std::size_t(*pitch) * *height);
    if (!memory.read(*bits, raw.data(), raw.size())) {
        error = "unmapped texture bytes";
        return {};
    }
    graphics::Texture image;
    image.width = *width;
    image.height = *height;
    if (packed16) {
        image.format = graphics::TextureFormat::Rgba32Float;
        image.rgba.resize(std::size_t(*width) * *height * 4);
    } else
        image.bgra.resize(std::size_t(*width) * *height * 4);
    for (unsigned y = 0; y < *height; ++y)
        for (unsigned x = 0; x < *width; ++x) {
            const auto offset = (std::size_t(y) * *width + x) * 4;
            const auto *source = raw.data() + std::size_t(y) * *pitch;
            if (!packed16) {
                std::memcpy(image.bgra.data() + offset, source + 4 * x, 4);
                if (*format == 22)
                    image.bgra[offset + 3] = 255;
                continue;
            }
            std::uint16_t word = 0;
            std::memcpy(&word, source + 2 * x, 2);
            auto *rgba = image.rgba.data() + offset;
            if (*format == 23) {
                rgba[0] = float((word >> 11) & 31) / 31;
                rgba[1] = float((word >> 5) & 63) / 63;
                rgba[2] = float(word & 31) / 31;
                rgba[3] = 1;
            } else if (*format == 24 || *format == 25) {
                rgba[0] = float((word >> 10) & 31) / 31;
                rgba[1] = float((word >> 5) & 31) / 31;
                rgba[2] = float(word & 31) / 31;
                rgba[3] = *format == 24 ? 1.0F : float(word >> 15);
            } else {
                rgba[0] = float((word >> 8) & 15) / 15;
                rgba[1] = float((word >> 4) & 15) / 15;
                rgba[2] = float(word & 15) / 15;
                rgba[3] = float(word >> 12) / 15;
            }
        }
    return image;
}
std::shared_ptr<const graphics::Texture> texture_image(const X86Guest &memory, std::uint32_t object,
                                                       std::string &error) {
    auto base = decode_level(memory, object, 0, error);
    if (!base)
        return {};
    auto image = std::make_shared<graphics::Texture>(std::move(*base));
    const auto levels = memory.u32(object + 36);
    if (!levels || !*levels) {
        error = "texture level count missing";
        return {};
    }
    unsigned maximum = 1;
    for (auto size = std::max(image->width, image->height); size > 1; size /= 2)
        ++maximum;
    if (*levels > maximum) {
        error = "texture mip count exceeds dimensions";
        return {};
    }
    for (unsigned level = 1; level < *levels; ++level) {
        auto mip = decode_level(memory, object, level, error);
        if (!mip)
            return {};
        if (mip->width != std::max(1U, image->width >> level) ||
            mip->height != std::max(1U, image->height >> level) || mip->format != image->format) {
            error = "texture mip dimensions/format";
            return {};
        }
        image->mipmaps.push_back(std::move(*mip));
    }
    return image;
}

} // namespace
struct MetalGuestFrameRenderer::Impl {
    std::unique_ptr<MetalDeviceFrameRenderer> renderer;
};
MetalGuestFrameRenderer::MetalGuestFrameRenderer(std::unique_ptr<Impl> value)
    : impl(std::move(value)) {}
MetalGuestFrameRenderer::~MetalGuestFrameRenderer() = default;
std::unique_ptr<MetalGuestFrameRenderer> MetalGuestFrameRenderer::create(id<MTLDevice> device,
                                                                         std::string *error) {
    auto value = std::make_unique<Impl>();
    value->renderer = MetalDeviceFrameRenderer::create(device, error);
    if (!value->renderer)
        return {};
    return std::unique_ptr<MetalGuestFrameRenderer>(new MetalGuestFrameRenderer(std::move(value)));
}
MetalGuestFrameStats MetalGuestFrameRenderer::encode(const HleProcess::D3D8Frame &frame,
                                                     const X86Guest &memory,
                                                     id<MTLCommandBuffer> commands,
                                                     MTLRenderPassDescriptor *pass) {
    graphics::Frame decoded;
    std::map<std::uint32_t, std::shared_ptr<const graphics::Texture>> textures;
    const auto draw = [&](std::size_t index) {
        graphics::Draw target;
        if (index >= frame.draws.size()) {
            target.unsupported = "draw event index";
            decoded.commands.push_back(target);
            return;
        }
        const auto &source = frame.draws[index];
        target.primitive_type = source.primitive_type;
        target.primitive_count = source.primitive_count;
        target.render_states = source.render_states;
        target.stage0_states = source.stage0_states;
        target.indices = source.indices;
        target.unsupported = source.unsupported;
        if (target.unsupported.empty())
            target.unsupported =
                graphics::decode_vertices(target, source.fvf, source.vertices, source.stride);
        for (const auto &[stage, states] : source.texture_stages) {
            const auto color_op = states.find(1);
            if (stage > 0 && color_op != states.end() && color_op->second != 1)
                target.unsupported = "active texture stage " + std::to_string(stage);
        }
        const auto matrix = [&](unsigned key, graphics::Matrix &value) {
            if (auto found = source.transforms.find(key); found != source.transforms.end())
                value = found->second;
        };
        matrix(256, target.world);
        matrix(2, target.view);
        matrix(3, target.projection);
        matrix(16, target.texture_transform);
        if (!source.material.empty()) {
            if (source.material.size() != 68)
                target.unsupported = "material layout";
            else {
                std::memcpy(target.material.diffuse.data(), source.material.data(), 16);
                std::memcpy(target.material.ambient.data(), source.material.data() + 16, 16);
                std::memcpy(target.material.specular.data(), source.material.data() + 32, 16);
                std::memcpy(target.material.emissive.data(), source.material.data() + 48, 16);
                std::memcpy(&target.material.power, source.material.data() + 64, 4);
            }
        }
        for (const auto &[index, enabled] : source.light_enabled) {
            if (!enabled)
                continue;
            const auto found = source.lights.find(index);
            if (found == source.lights.end() || found->second.size() != 104) {
                target.unsupported = "light layout";
                continue;
            }
            const auto &bytes = found->second;
            std::uint32_t type = 0;
            std::memcpy(&type, bytes.data(), 4);
            if (type < 1 || type > 3) {
                target.unsupported = "light type " + std::to_string(type);
                continue;
            }
            graphics::Light light;
            light.type = static_cast<graphics::LightType>(type);
            std::memcpy(light.diffuse.data(), bytes.data() + 4, 16);
            std::memcpy(light.specular.data(), bytes.data() + 20, 16);
            std::memcpy(light.ambient.data(), bytes.data() + 36, 16);
            std::memcpy(light.position.data(), bytes.data() + 52, 12);
            std::memcpy(light.direction.data(), bytes.data() + 64, 12);
            std::memcpy(&light.range, bytes.data() + 76, 4);
            std::memcpy(&light.falloff, bytes.data() + 80, 4);
            std::memcpy(light.attenuation.data(), bytes.data() + 84, 12);
            std::memcpy(&light.theta, bytes.data() + 96, 4);
            std::memcpy(&light.phi, bytes.data() + 100, 4);
            target.lights.push_back(light);
        }

        target.viewport.width = static_cast<float>(pass.colorAttachments[0].texture.width);
        target.viewport.height = static_cast<float>(pass.colorAttachments[0].texture.height);
        const auto viewport = source.viewport ? source.viewport : frame.viewport;
        if (viewport) {
            target.viewport.x = static_cast<float>((*viewport)[0]);
            target.viewport.y = static_cast<float>((*viewport)[1]);
            target.viewport.width = static_cast<float>((*viewport)[2]);
            target.viewport.height = static_cast<float>((*viewport)[3]);
            std::memcpy(&target.viewport.near_depth, &(*viewport)[4], 4);
            std::memcpy(&target.viewport.far_depth, &(*viewport)[5], 4);
        }
        if (source.texture && (target.texture_coordinates || (source.stage0_states.count(11) &&
                                                              source.stage0_states.at(11) >> 16))) {
            if (!textures.count(source.texture))
                textures[source.texture] =
                    texture_image(memory, source.texture, target.unsupported);
            target.texture = textures[source.texture];
            if (!target.texture && target.unsupported.empty())
                target.unsupported = "texture decoding";
        }
        decoded.commands.push_back(std::move(target));
    };
    const auto clear = [&](std::size_t index) {
        if (index >= frame.clears.size())
            return;
        const auto &source = frame.clears[index];
        decoded.commands.push_back(
            graphics::Clear{source.flags, source.color, source.z_bits, source.stencil});
    };
    if (frame.events.empty()) {
        // Legacy inspection fixtures have separate lists; actual HLE frames record event order.
        for (std::size_t index = 0; index < frame.clears.size(); ++index)
            clear(index);
        for (std::size_t index = 0; index < frame.draws.size(); ++index)
            draw(index);
    } else
        for (const auto &event : frame.events) {
            if (event.draw)
                draw(event.index);
            else
                clear(event.index);
        }
    return impl->renderer->encode(decoded, commands, pass);
}
} // namespace sadx
