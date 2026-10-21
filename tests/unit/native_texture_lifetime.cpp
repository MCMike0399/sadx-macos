#include "engine/native_texture_lifetime.hpp"
#include <cassert>
#include <stdexcept>

int main() {
    using namespace sadx::texture_lifetime;
    std::vector<Memory> pool(4);
    std::int32_t cached = -1;
    Memory *active = nullptr;
    const List *current = nullptr;
    std::array<std::uint32_t, 8> scratch{};
    State state{pool, cached, active, current, scratch};
    unsigned unbinds = 0;
    Services services{[&] { ++unbinds; }};
    auto image = std::make_shared<sadx::graphics::Texture>();
    std::weak_ptr<const sadx::graphics::Texture> weak = image;
    Memory prepared;
    prepared.global_index = 7;
    prepared.tsp_parameters.reset();
    prepared.texture_parameters.reset();
    prepared.palette_version.reset();
    auto bytes = std::make_shared<std::vector<std::uint8_t>>(16);
    std::weak_ptr<const std::vector<std::uint8_t>> weak_bytes = bytes;
    prepared.image_data = bytes->data();
    auto *first = acquire_prepared(prepared, image, state, bytes);
    bytes.reset();
    assert(!weak_bytes.expired());
    assert(first == &pool[0] && first->references == 1);
    assert(!first->tsp_parameters && !first->palette_version);
    auto unused_bytes = std::make_shared<std::vector<std::uint8_t>>(8);
    std::weak_ptr<const std::vector<std::uint8_t>> weak_unused_bytes = unused_bytes;
    assert(acquire_prepared(prepared, {}, state, unused_bytes) == first && first->references == 2);
    unused_bytes.reset();
    assert(weak_unused_bytes.expired() && !weak_bytes.expired());
    first->references = 65535;
    assert(acquire_prepared(prepared, {}, state) == first && first->references == 0);
    first->references = 2;
    List list{{first}, false};
    current = &list;
    cached = 7;
    active = first;
    release_all(state, services);
    assert(first->references == 1 && unbinds == 0);
    assert(current == &list && active == first);
    sadx::graphics::Draw submitted;
    submitted.texture = first->surface->texture;
    image.reset();
    assert(release_texture_list(&list, state, services) == 1);
    assert(unbinds == 1 && current == nullptr && active == nullptr && cached == -1);
    assert(first->references == 0 && !first->surface && first->global_index == -1);
    assert(first->tsp_parameters == 0 && first->palette_version == 65535);
    assert(!weak.expired());
    assert(weak_bytes.expired());
    submitted.texture.reset();
    assert(weak.expired());

    // ID lookup wins over an earlier free slot, including a matching zero-count entry.
    pool[2].global_index = 99;
    assert(find_memory(99, state) == &pool[2]);
    pool[2].references = 5;
    assert(find_memory(99, state) == &pool[2]);
    assert(find_memory(123, state) == &pool[0]);
    for (auto &memory : pool)
        memory.references = 1;
    assert(find_memory(123, state) == nullptr);

    cached = 1;
    pool[0].global_index = 1;
    active = &pool[0];
    scratch.fill(123);
    std::int32_t queue = 5;
    std::uint32_t messages = 8;
    Services failure{[] { throw std::runtime_error("device unavailable"); }};
    bool failed = false;
    try {
        clear_temporary_info(state, failure);
    } catch (const std::runtime_error &) {
        failed = true;
    }
    assert(failed && pool[0].references == 0 && pool[0].global_index == 1);
    assert(cached == 1 && active == &pool[0] && scratch[0] == 123);
    pool[0].references = 1;
    clear_temporary_info(state, services);
    assert(scratch[0] == 0);
    release_scene_textures(state, services, queue, messages);
    assert(queue == 2 && messages == 0);
}
