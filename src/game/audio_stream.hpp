#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>

namespace sadx {
// Audio streaming init 410510. Native handles are owner-issued identities, not
// Win32 handles. The sound object (operator new(4) + 513e60) owns the device
// pointer field (+0, zeroed by the constructor) that 44de80 fills.
struct AudioSoundObject {std::optional<std::uint32_t> device_word_at_0;};
struct AudioStreamState {
    std::optional<std::uint32_t> initialized_at_3b0ef2c;
    std::optional<std::shared_ptr<AudioSoundObject>> sound_object_at_3b0ecc0; // inner null = observed null
    std::array<std::optional<std::uint32_t>,36> words_at_3b0ec30,events_at_3b0ece0,words_at_3b0ee98,words_at_3b0ed70;
    std::optional<std::uint32_t> thread_at_3b0ee00,thread_id_at_3b0ec28;
    bool critical_section_at_3b0ecc4_initialized=false;
};
// The window handle is the value read from 3a8b088 at the call, not an owner.
struct AudioDeviceRequest {std::optional<std::uint32_t> window_from_3a8b088;std::uint32_t cooperative_level=2,channels=2,sample_rate=0x5622,bits=0x10;};
struct AudioStreamOwners {
    // 44de80 on the object: HRESULT (negative = failure). Required.
    std::function<std::optional<std::int32_t>(AudioSoundObject&,const AudioDeviceRequest&)> initialize_device;
    std::function<std::optional<std::uint32_t>()> create_event;                 // CreateEventA(0,0,0,0)
    // CreateThread(0,0,40ff50,0,0,&id): returns {handle,id}; the 40ff50 body is the owner's.
    std::function<std::optional<std::array<std::uint32_t,2>>()> start_stream_thread_40ff50;
    std::function<bool()> initialize_critical_section;                          // 3b0ecc4
};
enum class AudioStreamFailure {none,unknown_state,missing_owner,owner_failed};
struct AudioStreamResult {
    AudioStreamFailure failure=AudioStreamFailure::none;
    std::uint32_t source_boundary=0;
    std::optional<std::uint32_t> returned; // EAX: 1 initialized/already, 0 device failure
    std::optional<std::int32_t> device_hresult;
    explicit operator bool()const noexcept{return failure==AudioStreamFailure::none&&returned.has_value();}
};
AudioStreamResult run_audio_stream_init_410510(AudioStreamState&,const std::optional<std::uint32_t>& window_from_3a8b088,AudioStreamOwners&);
} // namespace sadx
