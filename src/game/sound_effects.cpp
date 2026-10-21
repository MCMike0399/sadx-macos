#include "game/sound_effects.hpp"
#include "cpu/fidelity_float.hpp"
#include <stdexcept>
#include <cstdio>
#include <algorithm>

namespace sadx::sound_effects {
namespace {
// NOTSA: source lstrcmpiA names are ASCII in the imported bank catalog.
bool equal_sample_suffix(std::string_view first, std::string_view second) {
    if (first.size() < 3 || second.size() < 3)
        throw Error(Boundary::SampleName, "sound-bank name has no source bank prefix");
    first.remove_prefix(3);
    second.remove_prefix(3);
    const auto lower = [](unsigned char byte) {
        if (byte > 127) throw Error(Boundary::SampleName, "non-ASCII sound-bank comparison");
        return byte >= 'A' && byte <= 'Z' ? byte + ('a' - 'A') : byte;
    };
    if (first.size() != second.size()) return false;
    for (std::size_t i = 0; i < first.size(); ++i)
        if (lower(first[i]) != lower(second[i])) return false;
    return true;
}
// NOTSA: names the source's wrapping integer pan scaling.
std::int32_t pan_value(std::int32_t angle) {
    const auto scaled = static_cast<std::int32_t>(static_cast<std::uint32_t>(angle) * 79U);
    return std::clamp(scaled, -10000, 10000);
}
} // namespace
void set_volume(std::int32_t value, std::int32_t channel, const Queue &queue,
                 PcmParameterCalls &calls) {
    std::int32_t attenuation;
    if (value <= -100) {
        if (channel < 0 || channel >= 36)
            throw Error(Boundary::ChannelCursor, "volume channel is outside the source queue");
        attenuation = -10000;
        if ((queue.sound.records[static_cast<std::size_t>(channel)].flags & 16384U) != 0)
            calls.set_position(channel, 0, 5000.0f, 0);
    } else {
        attenuation = static_cast<std::int32_t>((static_cast<std::uint32_t>(value) * 5U - 640U) * 2U);
    }
    calls.set_volume(channel, attenuation);
}
void set_pan(std::int32_t value, std::int32_t channel, PcmParameterCalls &calls) {
    calls.set_pan(channel, pan_value(value));
}
void set_pitch(std::int32_t value, std::int32_t channel, PcmParameterCalls &calls, X87Context &fp) {
    const auto scaled = fidelity::ExactFloat::integer(fp, value) * -1.5f;
    const auto difference = static_cast<std::int32_t>(22050U - static_cast<std::uint32_t>(scaled.truncated_integer()));
    calls.set_frequency(channel, static_cast<std::uint32_t>(std::clamp(difference, 100, 100000)));
}
bool opponent_uses_bank(std::int32_t tone, const OpponentState &opponent) {
    if (opponent.character < 0 || !opponent.task) return false;
    switch (opponent.character) {
    case 0: return (tone > 761 && tone < 779) || (tone > 1221 && tone < 1254);
    case 3: return (tone > 779 && tone < 795) || (tone > 1254 && tone < 1278);
    case 6: return (tone > 821 && tone < 840) || (tone > 1296 && tone < 1309);
    default: return false;
    }
}
bool allow_overlap(std::uint32_t tone, Queue &queue) {
    for (std::size_t i = 0; i <= 36; ++i) {
        auto &sound = i == 36 ? queue.work.overlap_tail[0] : queue.sound.overlap[i * 2];
        if (sound == 0) {
            sound = static_cast<std::uint8_t>(tone);
            return true;
        }
        if (sound != tone) continue;
        auto &count = i == 36 ? queue.work.overlap_tail[1] : queue.sound.overlap[i * 2 + 1];
        count = static_cast<std::uint8_t>(count + 1);
        return count < 3;
    }
    return true;
}
std::int32_t find_sample_name(std::string_view name, const BankView &bank) {
    if (!bank.present) return -1;
    for (std::size_t i = 0; i < bank.names.size(); ++i)
        if (equal_sample_suffix(bank.names[i], name)) return static_cast<std::int32_t>(i);
    return -1;
}
void select_bank_sample(std::uint32_t bank, std::int32_t cue, State &state, SampleCalls &calls) {
    const auto view = calls.bank(bank);
    if (!view.present) {
        state.selected_sample = {};
        return;
    }
    std::array<char, 256> name{};
    std::snprintf(name.data(), name.size(), "B%02d_00_%02d.wav", static_cast<std::int32_t>(bank), cue);
    const auto index = find_sample_name(name.data(), view);
    if (index < 0)
        throw Error(Boundary::SampleIndex, "missing source cue indexes before the bank entry array");
    const auto selected = calls.sample(bank, static_cast<std::size_t>(index));
    // Source publishes the pointer first; null data clears its size on the failure tail.
    state.selected_sample.resource = selected.resource;
    state.selected_sample.source_size = selected.resource ? selected.source_size : 0;
}
namespace {
// NOTSA: names the active-record portion of dsSoundServer.
void start_record(std::int32_t channel, ServerView &view, ServerCalls &calls) {
    auto &queue = view.queue;
    const auto index = static_cast<std::size_t>(channel);
    auto &record = queue.sound.records[index];
    const auto tone = record.tone;
    if (tone > 1384) {
        calls.play_voice(static_cast<std::int32_t>(static_cast<std::uint32_t>(tone) + 345U));
        record.flags &= ~16U;
        record.timer = 0;
        record.flags &= ~1U;
        return;
    }
    std::int32_t threshold = 0;
    std::uint32_t bank = 0;
    bool terminated = false;
    for (const auto &entry : view.bank_table) {
        if (entry.tone < 0) {
            terminated = true;
            break;
        }
        if (tone > entry.tone) {
            threshold = entry.tone;
            bank = entry.bank;
            terminated = true;
            break;
        }
    }
    if (!terminated) throw Error(Boundary::BankTable, "sound-bank thresholds have no terminator");
    if (opponent_uses_bank(tone, view.opponent)) bank = (bank == 6 ? 1U : 0U) + (bank == 3 ? 4U : 0U);
    if (allow_overlap(static_cast<std::uint32_t>(tone), queue)) {
        const auto cue = threshold == 0 ? tone : static_cast<std::int32_t>(
            static_cast<std::uint32_t>(tone) - static_cast<std::uint32_t>(threshold) - 1U);
        if (threshold == 0) bank = 0;
        select_bank_sample(bank, cue, queue.work, calls);
        queue.bindings.bank_slots[index] = bank;
        if ((record.flags & 16384U) == 0) {
            calls.load_pcm(channel, queue.work.selected_sample, 2);
        } else {
            calls.load_3d_pcm(channel, queue.work.selected_sample);
            calls.set_3d_min_max(channel, 10.0f, 500.0f);
        }
        if ((record.flags & 16384U) == 0) {
            if ((record.flags & 256U) != 0) record.current_volume = calls.distance_volume(channel);
            else record.current_volume = (record.flags & 512U) != 0 ? record.volume : 0;
            calls.volume(record.current_volume, channel);
        }
        if ((record.flags & 2048U) != 0) calls.pan(record.pan_angle, channel);
        if ((record.flags & 8192U) != 0) calls.pitch(record.pitch, channel);
        calls.play_pcm(channel);
    }
    record.flags &= ~1U;
}
// NOTSA: names the continuing/expired-record portion of dsSoundServer.
void continue_record(std::int32_t channel, ServerView &view, ServerCalls &calls) {
    auto &queue = view.queue;
    const auto index = static_cast<std::size_t>(channel);
    auto &record = queue.sound.records[index];
    if (record.timer > 0) {
        if ((record.flags & 16384U) == 0) {
            if ((record.flags & 34U) != 0) {
                if ((record.flags & 256U) != 0) {
                    const auto volume = calls.distance_volume(channel);
                    if (record.current_volume != volume) {
                        calls.volume(volume, channel);
                        record.current_volume = volume;
                    }
                }
                record.flags &= ~2U;
            }
            if ((record.flags & 512U) != 0 && record.current_volume != record.volume) {
                const auto volume = record.volume;
                calls.volume(volume, channel);
                record.current_volume = volume;
            }
        }
        if ((record.flags & 2048U) != 0) {
            calls.set_pan(channel, pan_value(record.pan_angle));
            record.flags &= ~2048U;
        }
        if ((record.flags & 8192U) != 0) {
            calls.pitch(record.pitch, channel);
            record.flags &= ~8192U;
        }
        record.timer = static_cast<std::int32_t>(static_cast<std::uint32_t>(record.timer) - 1U);
    }
    if (record.timer == 0) {
        if ((record.flags & 16U) != 0) calls.stop_pcm(channel);
        record.timer = 0;
        queue.bindings.identities[index] = nullptr;
        record.flags = 0;
        record.tone = -1;
        view.positional_tasks[index] = {};
    }
}
} // namespace
void update_sound_server(ServerView &view, ServerCalls &calls) {
    auto &sound = view.queue.sound;
    if (sound.paused != 0) return;
    const auto previous = sound.frame_counter;
    sound.frame_counter = static_cast<std::uint8_t>(previous + 1);
    if (previous > 3) {
        sound.overlap.fill(0);
        sound.frame_counter = 0;
    }
    for (std::int32_t channel = 0; channel < 36; ++channel) {
        if ((sound.records[channel].flags & 1U) != 0) start_record(channel, view, calls);
        else continue_record(channel, view, calls);
    }
    if (sound.positional_paused == 0 && view.listener) calls.update_positions();
}
std::int32_t find_free_channel(Queue &queue, std::int32_t priority) {
    auto candidate = queue.sound.queue_count;
    std::int32_t selected = -1;
    for (unsigned remaining = 35; remaining != 0; --remaining) {
        if (candidate < 0 || candidate >= 36)
            throw Error(Boundary::ChannelCursor, "sound channel cursor is outside the source queue");
        const auto index = static_cast<std::size_t>(candidate);
        if (queue.sound.records[index].timer == 0 || queue.work.records[index].priority <= priority)
            selected = candidate;
        // Original quirk: search wraps at35, while the stored next cursor wraps at36.
        ++candidate;
        if (candidate >= 35) candidate = 0;
        if (selected >= 0) break;
    }
    queue.sound.queue_count = selected + 1;
    if (queue.sound.queue_count >= 36) queue.sound.queue_count = 0;
    return selected;
}
std::int32_t play_one_shot(std::int32_t tone, const void *identity,
                           std::int32_t priority, std::int32_t volume_offset, Queue &queue) {
    const auto channel = find_free_channel(queue, priority);
    if (channel < 0) return -1;
    const auto index = static_cast<std::size_t>(channel);
    auto &record = queue.sound.records[index];
    auto &work = queue.work.records[index];
    queue.bindings.identities[index] = identity;
    record.timer = 120;
    // Original quirk: requested priority selects a channel but every one-shot stores−1.
    work.priority = -1;
    record.flags = 17;
    record.tone = tone;
    record.pan_angle = 0;
    record.volume = volume_offset;
    work.volume_limit = 0;
    record.pitch = 0;
    work.position = {};
    if (tone == 7) {
        record.pan_angle = queue.work.alternating_pan;
        record.flags = 2049;
        queue.work.alternating_pan = static_cast<std::int32_t>(
            0U - static_cast<std::uint32_t>(queue.work.alternating_pan));
    }
    return 1;
}
void play_title_confirmation(Queue &queue) {
    play_one_shot(2, nullptr, 0, 0, queue);
}
} // namespace sadx::sound_effects
