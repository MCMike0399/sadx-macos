#include "engine/collision_component.hpp"
#include "math/ninja_length.hpp"
#include <cstring>

namespace cc = sadx::collision_component;
namespace {
template <class T> T read(const unsigned char *p, unsigned at) {
    T value;
    std::memcpy(&value, p + at, sizeof(value));
    return value;
}
template <class T> void write(unsigned char *p, unsigned at, T value) {
    std::memcpy(p + at, &value, sizeof(value));
}
cc::Info decode(const unsigned char *p) {
    cc::Info i;
    i.kind = p[0];
    i.form = static_cast<cc::Form>(p[1]);
    i.push = p[2];
    i.damage = p[3];
    i.attributes = read<std::uint32_t>(p, 4);
    i.center = read<sadx::NinjaVector>(p, 8);
    i.a = read<float>(p, 20);
    i.b = read<float>(p, 24);
    i.c = read<float>(p, 28);
    i.d = read<float>(p, 32);
    i.angles = read<std::array<std::int32_t, 3>>(p, 36);
    return i;
}
void encode(const cc::Work &w, unsigned char *p) {
    write(p, 0, w.id);
    write(p, 2, w.hit_count);
    write(p, 4, w.flags);
    write(p, 6, w.info_count);
    write(p, 8, w.range);
    for (unsigned n = 0; n < 16; ++n) {
        p[16 + n * 8] = w.hits[n].own_number;
        p[17 + n * 8] = w.hits[n].other_number;
        write(p, 18 + n * 8, w.hits[n].flags);
    }
    write(p, 144, w.normal);
    write(p, 160, w.own_number);
    write(p, 162, w.other_number);
}
std::uint32_t pointer(const sadx::TaskWorkHandle &w) {
    return w ? 0x20000000U + w->data_offset() : 0;
}
} // namespace
// Reference-only byte layout and guest identity marshalling. No game algorithm lives here.
extern "C" int sadx_collision_component_numeric(unsigned operation, unsigned control,
                                                const unsigned char *input, unsigned count,
                                                unsigned status, unsigned char *output,
                                                unsigned *result) {
    try {
        cc::Work w;
        w.id = read<std::uint16_t>(input, 0);
        w.hit_count = read<std::int16_t>(input, 2);
        w.flags = read<std::uint16_t>(input, 4);
        w.info_count = read<std::uint16_t>(input, 6);
        w.range = read<float>(input, 8);
        w.normal = read<sadx::NinjaVector>(input, 144);
        w.own_number = read<std::int16_t>(input, 160);
        w.other_number = read<std::int16_t>(input, 162);
        for (unsigned n = 0; n < 16; ++n) {
            w.hits[n].own_number = input[16 + n * 8];
            w.hits[n].other_number = input[17 + n * 8];
            w.hits[n].flags = read<std::uint16_t>(input, 18 + n * 8);
        }
        for (unsigned n = 0; n < count; ++n)
            w.info.push_back(decode(input + 168 + n * 48));
        std::memcpy(output, input, 168 + count * 48);
        std::uint16_t primary_status = status;
        cc::Context context(control);
        if (operation == 0)
            cc::clear_info(w, primary_status);
        else
            cc::calculate_range(&w, context);
        encode(w, output);
        if (operation == 0) {
            for (unsigned n = 0; n < 16; ++n)
                write<std::uint32_t>(output, 20 + n * 8, 0);
            write<std::uint32_t>(output, 164, 0);
        }
        for (unsigned n = 0; n < count; ++n)
            output[168 + n * 48 + 3] = w.info[n].damage;
        result[0] = primary_status;
        result[1] = context.exception_flags();
        return 0;
    } catch (...) {
        return 1;
    }
}

// operation: 0 init, 1 reentry, 2 FreeColliWork, 3 Collision_Free, 4 scheduler cleanup.
// reserve leaves exactly the supplied number of bytes after the primary allocation.
extern "C" int sadx_collision_component_lifecycle(unsigned operation, unsigned control,
                                                  const unsigned char *input, unsigned count,
                                                  unsigned status, unsigned id, unsigned remaining,
                                                  unsigned char *output, unsigned *result) {
    try {
        auto arena = std::make_shared<sadx::TaskWorkArena>(0x20000000);
        sadx::TaskScheduler scheduler;
        if (!scheduler.initialize_source_pool(arena, std::shared_ptr<void>{}))
            return 2;
        const auto task_result = scheduler.create_element(2, 3, nullptr);
        if (!task_result)
            return 3;
        const auto task = task_result.task;
        std::uint16_t primary_status = status;
        cc::Context context(control);
        std::vector<cc::Info> info;
        for (unsigned n = 0; n < count; ++n)
            info.push_back(decode(input + n * 48));
        if (operation != 0) {
            if (cc::initialize(*arena, task, primary_status, info, count, 7, context) !=
                cc::InitResult::Initialized)
                return 4;
        }
        const auto old = arena->reference(task->primary_work, 56).work;
        const auto old_info = old ? arena->reference(old, 12).work : sadx::TaskWorkHandle{};
        if (operation == 2 || operation == 3) {
            const std::uint8_t ownership = id;
            if (!arena->write(old, 5, &ownership, 1))
                return 7;
            if (id == 128 && !arena->set_reference(old, 12, {}))
                return 8;
        }
        if (remaining != 0xffffffffU) {
            const auto free_start = *arena->free_head();
            const auto free_bytes = sadx::TaskWorkArena::usable_bytes - free_start;
            if (free_bytes > remaining + 32) {
                if (!arena->allocate(free_bytes - remaining - 32))
                    return 5;
            }
        }
        cc::InitResult initialized = cc::InitResult::NoPrimary;
        if (operation < 2)
            initialized = cc::initialize(*arena, task, primary_status, info, count, id, context);
        else if (operation == 2)
            cc::free_work(*arena, task->primary_work);
        else if (operation == 3)
            cc::free_collision(*arena, task);
        else if (!scheduler.destroy_task(task))
            return 6;
        const auto current = task->primary_work && !task->primary_work->released()
                                 ? arena->reference(task->primary_work, 56).work
                                 : sadx::TaskWorkHandle{};
        std::memset(output, 0, 168 + count * 48);
        if (current) {
            const auto &w = *current->collision_state;
            encode(w, output);
            write(output, 12, pointer(arena->reference(current, 12).work));
            write<std::uint32_t>(output, 156, 0x11001000);
            for (unsigned n = 0; n < count; ++n) {
                std::memcpy(output + 168 + n * 48, input + n * 48, 48);
                output[168 + n * 48 + 3] = w.info[n].damage;
            }
        }
        result[0] = initialized == cc::InitResult::Initialized ? 1 : 0;
        result[1] = primary_status;
        result[2] = context.exception_flags();
        result[3] = pointer(current);
        result[4] = arena->free_head() ? 0x20000000U + *arena->free_head() : 0;
        result[5] = old ? old->released() : 0;
        result[6] = old_info ? old_info->released() : 0;
        result[7] = static_cast<unsigned>(arena->blocks().size());
        return 0;
    } catch (...) {
        return 1;
    }
}

extern "C" int sadx_collision_component_scalar(unsigned operation, unsigned control,
                                               const unsigned char *input, unsigned *result) {
    try {
        cc::Context context(control);
        const auto value = read<sadx::NinjaVector>(input, 0);
        const auto length = operation == 0
                                ? sadx::vector_length(value, context)
                                : sadx::fidelity::ExactFloat::signed_square_root(context, value.x);
        const float stored = length.rounded();
        std::memcpy(result, &stored, 4);
        result[1] = context.exception_flags();
        return 0;
    } catch (...) {
        return 1;
    }
}
