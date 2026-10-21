#include "engine/native_texture_lifetime.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>

extern "C" void sadx_native_texture_lifetime(const std::uint32_t *in, std::uint32_t *out) {
    using namespace sadx::texture_lifetime;
    std::fill_n(out, 512, 0);
    const auto count = in[1];
    std::vector<Memory> pool(count);
    std::array<std::uint32_t, 8> scratch;
    std::copy_n(in + 8, 8, scratch.begin());
    auto cached = static_cast<std::int32_t>(in[2]);
    Memory *active = in[3] < count ? &pool[in[3]] : nullptr;
    List list;
    for (std::size_t index = 0; index < count; ++index)
        list.entries.push_back((in[6] & (1U << index)) ? nullptr : &pool[index]);
    list.first_filename_invalid = in[7] != 0;
    const List *current = in[4] ? &list : nullptr;
    unsigned trace = 200;
    bool observing = true;
    const auto record = [&](unsigned kind, unsigned index) {
        if (!observing)
            return;
        out[trace++] = kind;
        out[trace++] = index;
        out[trace++] = static_cast<std::uint32_t>(cached);
    };
    for (unsigned index = 0; index < count; ++index) {
        auto &memory = pool[index];
        const auto *words = in + 16 + index * 17;
        memory.global_index = static_cast<std::int32_t>(words[0]);
        memory.bank = static_cast<std::int32_t>(words[1]);
        memory.tsp_parameters = words[2];
        memory.texture_parameters = words[3];
        memory.texture_flags = words[4];
        memory.image_data = reinterpret_cast<const void *>(static_cast<std::uintptr_t>(words[5]));
        memory.type = words[6];
        memory.bit_depth = words[7];
        memory.pixel_format = words[8];
        memory.width = words[9];
        memory.height = words[10];
        memory.texture_size = words[11];
        memory.surface_flags = words[12];
        if (words[13]) {
            auto texture = std::shared_ptr<const sadx::graphics::Texture>(
                new sadx::graphics::Texture, [&, index](const auto *pointer) {
                    record(2, index);
                    delete pointer;
                });
            memory.surface = std::make_shared<Surface>(Surface{std::move(texture), {}});
        }
        memory.virtual_data = reinterpret_cast<const void *>(static_cast<std::uintptr_t>(words[14]));
        if (words[15]) {
            memory.physical_data = std::shared_ptr<PhysicalAllocation>(
                new PhysicalAllocation{std::make_unique<std::vector<std::uint8_t>>(4)},
                [&, index](auto *pointer) {
                    record(3, index);
                    delete pointer;
                });
        }
        memory.references = static_cast<std::uint16_t>(words[16]);
        memory.palette_version = static_cast<std::uint16_t>(words[16] >> 16);
    }
    State state{pool, cached, active, current, scratch};
    Services services{[&] {
        record(1, 0);
        if (in[5])
            throw 1;
    }};
    std::int32_t queue = static_cast<std::int32_t>(in[152]);
    std::uint32_t messages = in[153];
    try {
        switch (in[0]) {
        case 0: release_all(state, services); break;
        case 1: clear_temporary_info(state, services); break;
        case 2: release_scene_textures(state, services, queue, messages); break;
        case 3: release_low(in[154] < count ? &pool[in[154]] : nullptr, state, services); break;
        case 4: {
            auto *found = find_memory(in[154], state);
            out[0] = found ? static_cast<std::uint32_t>(found - pool.data()) + 1 : 0;
            break;
        }
        case 5: out[0] = release_list(&list, state, services); break;
        case 6: out[0] = release_texture_list(&list, state, services); break;
        }
    } catch (int) {
        out[6] = 1;
    }
    out[1] = static_cast<std::uint32_t>(cached);
    out[2] = active ? static_cast<std::uint32_t>(active - pool.data()) + 1 : 0;
    out[3] = current != nullptr;
    out[4] = queue;
    out[5] = messages;
    out[7] = (trace - 200) / 3;
    std::copy(scratch.begin(), scratch.end(), out + 8);
    for (unsigned index = 0; index < count; ++index) {
        const auto &m = pool[index];
        auto *words = out + 16 + index * 17;
        const auto *original = in + 16 + index * 17;
        const std::uint32_t values[]{static_cast<std::uint32_t>(m.global_index),
            static_cast<std::uint32_t>(m.bank), *m.tsp_parameters, *m.texture_parameters,
            m.texture_flags, static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(m.image_data)),
            m.type, m.bit_depth, m.pixel_format, m.width, m.height, m.texture_size,
            m.surface_flags, m.surface ? original[13] : 0,
            static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(m.virtual_data)),
            m.physical_data ? original[15] : 0,
            static_cast<std::uint32_t>(m.references) | (static_cast<std::uint32_t>(*m.palette_version) << 16)};
        std::copy(std::begin(values), std::end(values), words);
    }
    // Destruction after the observation is not a source release operation.
    observing = false;
    pool.clear();
}
extern "C" void sadx_native_texture_prepare(const std::uint32_t *in, std::uint32_t *out) {
    using namespace sadx::texture_lifetime;
    std::fill_n(out, 32, 0);
    std::vector<Memory> pool(in[7]);
    std::int32_t cached = -1;
    Memory *active = nullptr;
    const List *current = nullptr;
    std::array<std::uint32_t, 8> scratch{};
    State state{pool, cached, active, current, scratch};
    if (pool.size() > 2 && in[8]) {
        pool[2].global_index = static_cast<std::int32_t>(in[1]);
        pool[2].references = static_cast<std::uint16_t>(in[5]);
        pool[2].width = in[6];
    }
    Memory prepared;
    const auto *image = reinterpret_cast<const void *>(static_cast<std::uintptr_t>(123));
    if (in[0] == 0) {
        sadx::NinjaTextureRecord record;
        record.pixel_format = static_cast<std::uint8_t>(in[2]);
        record.data_format = static_cast<std::uint8_t>(in[3]);
        record.width = static_cast<std::uint16_t>(in[4]);
        record.height = static_cast<std::uint16_t>(in[9]);
        prepared = prepare_pvr(in[1], record, image);
    } else {
        prepared = prepare_argb_image(in[1], image, in[4], in[9]);
    }
    auto *memory = acquire_prepared(std::move(prepared),
                                   std::make_shared<sadx::graphics::Texture>(), state);
    if (memory == nullptr)
        return;
    out[0] = static_cast<std::uint32_t>(memory - pool.data()) + 1;
    const std::uint32_t values[]{static_cast<std::uint32_t>(memory->global_index),
        static_cast<std::uint32_t>(memory->bank), memory->tsp_parameters.has_value(),
        memory->texture_parameters.has_value(), memory->texture_flags, memory->image_data != nullptr,
        memory->type, memory->bit_depth, memory->pixel_format, memory->width, memory->height,
        memory->texture_size, memory->surface_flags, memory->surface != nullptr,
        memory->virtual_data != nullptr, memory->physical_data != nullptr,
        memory->references, memory->palette_version.has_value()};
    std::copy(std::begin(values), std::end(values), out + 1);
}
extern "C" void sadx_native_texture_empty_record(std::uint32_t *out) {
    const sadx::texture_lifetime::Memory memory;
    const std::uint32_t values[]{static_cast<std::uint32_t>(memory.global_index),
        static_cast<std::uint32_t>(memory.bank), *memory.tsp_parameters, *memory.texture_parameters,
        memory.texture_flags, memory.image_data != nullptr, memory.type, memory.bit_depth,
        memory.pixel_format, memory.width, memory.height, memory.texture_size, memory.surface_flags,
        memory.surface != nullptr, memory.virtual_data != nullptr, memory.physical_data != nullptr,
        static_cast<std::uint32_t>(memory.references) |
            (static_cast<std::uint32_t>(*memory.palette_version) << 16)};
    std::copy(std::begin(values), std::end(values), out);
}
