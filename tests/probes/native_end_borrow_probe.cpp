#include "engine/message_sprite_queue.hpp"
extern "C" int sadx_native_end_borrow(unsigned size, unsigned *out) {
    using namespace sadx;
    queue::Storage storage;
    storage.bind_buffer(queue::allocate_buffer());
    const auto arena = storage.buffer().arena;
    const auto before = arena->cursor();
    auto bytes = queue::borrow_from_end(storage, size);
    out[0] = static_cast<unsigned>(bytes.bytes - arena->bytes().data());
    out[1] = arena->cursor();
    out[2] = arena->end();
    out[3] = before;
    return 1;
}
