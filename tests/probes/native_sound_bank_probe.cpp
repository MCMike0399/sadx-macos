#include "game/sound_bank.hpp"
#include <cstring>
#include <sstream>
namespace {
struct ProbeBank : sadx::sound_bank::Bank {
    unsigned id;
    ProbeBank(std::string path, unsigned id) : Bank(std::move(path)), id(id) {}
};
unsigned identity(const sadx::sound_bank::Handle &handle) {
    return handle ? static_cast<const ProbeBank &>(*handle).id : 0;
}
} // namespace
extern "C" int sound_bank_select_probe(int index, unsigned initialized, unsigned voice,
                                       const unsigned *slots, const char *const *names,
                                       unsigned count, const char *cd, const char *const *initial,
                                       unsigned open_mask, unsigned fail_event, int pack_priority,
                                       int file_priority, unsigned *out, char *trace,
                                       std::size_t trace_capacity) {
    using namespace sadx::sound_bank;
    Catalog catalog;
    if (index >= 0 && index < static_cast<int>(kListCount))
        for (unsigned n = 0; n != count; ++n)
            catalog[static_cast<std::size_t>(index)].push_back(
                {static_cast<std::uint8_t>(slots[n]), names[n]});
    State state;
    for (unsigned n = 0; n != kSlotCount; ++n)
        if (initial[n])
            state.slots[n] = std::make_shared<ProbeBank>(initial[n], n + 1);
    std::ostringstream text;
    unsigned events = 0, opened = 0, boundary = 0;
    auto check = [&] {
        if (++events == fail_event)
            throw Error(Boundary::Open, "injected unavailable external owner");
    };
    const auto load = [&](std::string_view path, unsigned slot, bool pack) -> Handle {
        boundary = pack ? 0x10097b4a : path.rfind("SYSTEM\\", 0) == 0 ? 0x10097b68 : 0x10097ba7;
        text << (pack ? "P|" : "F|") << path << '|' << slot << '\n';
        check();
        const auto number = opened++;
        return number < 32 && (open_mask & (1u << number))
                   ? std::make_shared<ProbeBank>(std::string(path), 101 + number)
                   : nullptr;
    };
    Services services{[&](unsigned slot) {
                          boundary = 0x10097acb;
                          text << "R|" << slot << '\n';
                          check();
                      },
                      [&](const Handle &bank) {
                          boundary = 0x10097ade;
                          text << "C|" << identity(bank) << '\n';
                          check();
                      },
                      [&](std::string_view path) {
                          const bool pack = path.back() == '\\';
                          boundary = pack ? 0x10097b2c : 0x10097b38;
                          const auto priority = pack ? pack_priority : file_priority;
                          text << "M|" << path << '|' << priority << '\n';
                          check();
                          return priority;
                      },
                      [&](std::string_view path, unsigned slot) {
                          return load(path, slot, true);
                      },
                      [&](std::string_view path, unsigned slot) {
                          return load(path, slot, false);
                      }};
    out[0] = 0;
    out[1] = 0;
    try {
        select(index, catalog, state, initialized, voice != 0, cd, services);
    } catch (const Error &error) {
        out[1] = error.boundary() == Boundary::ListIndex ? 0x100979fa : boundary;
    }
    for (unsigned n = 0; n != kSlotCount; ++n)
        out[2 + n] = identity(state.slots[n]);
    const auto result = text.str();
    if (result.size() + 1 > trace_capacity)
        return 0;
    std::memcpy(trace, result.c_str(), result.size() + 1);
    return 1;
}
extern "C" int sound_bank_release_probe(unsigned slot, const unsigned *input, unsigned fail_channel,
                                        unsigned *out, unsigned *calls) {
    using namespace sadx;
    std::array<SoundRecordInitialValues, 36> records{};
    std::array<TaskHandle, 36> tasks{};
    sound_bank::ChannelBindings bindings;
    TaskScheduler scheduler;
    std::array<std::unique_ptr<unsigned>, 36> identities;
    std::array<TaskHandle, 36> original_tasks;
    for (unsigned n = 0; n != 36; ++n) {
        const auto *row = input + n * 6;
        records[n].timer = static_cast<int>(row[0]);
        records[n].flags = row[2];
        records[n].tone = static_cast<int>(row[3]);
        if (row[1]) {
            identities[n] = std::make_unique<unsigned>(row[1]);
            bindings.identities[n] = identities[n].get();
        }
        bindings.bank_slots[n] = row[4];
        if (row[5]) {
            tasks[n] = scheduler.append_task(0, TaskScheduler::source_no_op).task;
            original_tasks[n] = tasks[n];
        }
    }
    sound_bank::Channels channels{records, tasks, bindings};
    unsigned count = 0;
    bool failed = false;
    try {
        sound_bank::release_samples(slot, channels, [&](int channel) {
            calls[++count] = static_cast<unsigned>(channel);
            if (static_cast<unsigned>(channel) == fail_channel)
                throw sound_bank::Error(sound_bank::Boundary::ReleaseSamples,
                                        "unavailable StopPCM");
        });
    } catch (const sound_bank::Error &) {
        failed = true;
    }
    calls[0] = count;
    for (unsigned n = 0; n != 36; ++n) {
        auto *row = out + n * 6;
        row[0] = static_cast<unsigned>(records[n].timer);
        row[1] =
            bindings.identities[n] ? *static_cast<const unsigned *>(bindings.identities[n]) : 0;
        row[2] = records[n].flags;
        row[3] = static_cast<unsigned>(records[n].tone);
        row[4] = bindings.bank_slots[n];
        row[5] = tasks[n] == original_tasks[n] ? input[n * 6 + 5] : 0;
    }
    return failed ? 0 : 1;
}
extern "C" int sound_bank_name_probe(const char *name, unsigned slot, int counter, unsigned adx,
                                     char *out, unsigned *next) {
    try {
        const auto result = sadx::sound_bank::normalize_name(name, slot, counter, adx != 0);
        *next = static_cast<unsigned>(counter);
        if (!result)
            return 0;
        std::memcpy(out, result->c_str(), result->size() + 1);
        return 1;
    } catch (const std::out_of_range &) {
        *next = static_cast<unsigned>(counter);
        return -1;
    }
}
