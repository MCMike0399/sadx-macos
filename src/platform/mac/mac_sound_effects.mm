#include "platform/mac/mac_sound_effects.hpp"
#include "cpu/x87_math.hpp"
#include <Foundation/Foundation.h>
#include <cmath>
#include <map>
#include <mutex>
#include <tuple>

namespace sadx::sound_effects::mac {
// [platform:audio] AudioQueue buffers replace DirectSound resources. Game
// scheduling and parameter conversions remain in sound_effects.
namespace {
std::string text(id value) {
    if (![value isKindOfClass:NSString.class] || ![value UTF8String])
        throw std::runtime_error("invalid imported sound sample text");
    return [value UTF8String];
}
std::int64_t number(id value, std::int64_t low, std::int64_t high) {
    if (![value isKindOfClass:NSNumber.class] ||
        CFGetTypeID((__bridge CFTypeRef)value) == CFBooleanGetTypeID())
        throw std::runtime_error("invalid imported sound sample integer");
    const auto real = [value doubleValue];
    if (!std::isfinite(real) || real < double(low) || real > double(high) || std::trunc(real) != real)
        throw std::runtime_error("imported sound sample integer outside range");
    return [value longLongValue];
}
std::string lower(std::string value) {
    for (auto &character : value)
        if (character >= 'A' && character <= 'Z')
            character = static_cast<char>(character + ('a' - 'A'));
    return value;
}
struct Metadata {
    std::uint32_t source_bytes, source_rate;
    std::int32_t loops;
};
class DecodedSample final : public Sample {
  public:
    DecodedSample(std::shared_ptr<const audio::mac::AudioClip> value, Metadata metadata)
        : clip(std::move(value)), metadata(metadata) {}
    std::shared_ptr<const audio::mac::AudioClip> clip;
    Metadata metadata;
};
float gain(std::int32_t attenuation) {
    if (attenuation < -10000 || attenuation > 0)
        throw std::runtime_error("native PCM attenuation outside DirectSound range");
    return attenuation == -10000 ? 0.0f :
        static_cast<float>(std::pow(10.0, static_cast<double>(attenuation) / 2000.0));
}
// NOTSA: a loaded buffer has a native lease but no mixer voice until PlayPCM.
class LoadedSound final : public audio::PcmSound {
  public:
    LoadedSound(audio::mac::AudioOutput &output, std::shared_ptr<const DecodedSample> sample,
                bool looping) : output_(output), sample_(std::move(sample)), looping_(looping) {}
    ~LoadedSound() override {
        if (token_)
            output_.stop(token_);
    }
    void volume(std::int32_t value) {
        volume_ = gain(value);
        update_gains();
    }
    void pan(std::int32_t value) {
        if (value < -10000 || value > 10000)
            throw std::runtime_error("native PCM pan outside DirectSound range");
        // DirectSound SetPan attenuates one side in hundredths of a dB, cumulatively
        // with volume; the native title integration evidence links its API contract.
        left_ = gain(value > 0 ? -value : 0);
        right_ = gain(value < 0 ? value : 0);
        update_gains();
    }
    void frequency(std::uint32_t value) {
        if (value != sample_->metadata.source_rate)
            throw std::runtime_error("native PCM variable-frequency playback is unrecovered");
    }
    void play() {
        if (token_)
            throw std::runtime_error("native PCM replay of an existing buffer is unrecovered");
        token_ = output_.play(sample_->clip, looping_, volume_ * left_, volume_ * right_);
        if (!token_)
            throw std::runtime_error("native PCM mixer rejected a loaded voice");
    }
  private:
    void update_gains() {
        if (token_)
            output_.set_gain(token_, volume_ * left_, volume_ * right_);
    }
    audio::mac::AudioOutput &output_;
    std::shared_ptr<const DecodedSample> sample_;
    bool looping_;
    std::uint64_t token_ = 0;
    float volume_ = 1, left_ = 1, right_ = 1;
};
} // namespace
struct Output::Impl {
    Queue queue;
    sound_bank::State &state;
    sound_bank::mac::Banks &banks;
    audio::PcmState &pcm;
    audio::mac::AudioOutput &output;
    X87Context &fp;
    using Key = std::tuple<std::string, std::string, std::size_t>;
    std::map<Key, Metadata> metadata;
    std::uint32_t loads = 0, plays = 0;
    LoadedSound &voice(std::int32_t channel) {
        if (channel < 0 || channel >= static_cast<std::int32_t>(audio::kPcmVoiceCount))
            throw std::runtime_error("native PCM channel outside range");
        auto *voice = dynamic_cast<LoadedSound *>(pcm.active(static_cast<std::size_t>(channel)));
        if (!voice)
            throw std::runtime_error("native PCM channel has no loaded buffer");
        return *voice;
    }
};
Output::Output(const std::filesystem::path &directory, Queue queue, sound_bank::State &state,
               sound_bank::mac::Banks &banks, audio::PcmState &pcm,
               audio::mac::AudioOutput &output, X87Context &fp)
    : impl_(std::make_unique<Impl>(Impl{queue, state, banks, pcm, output, fp, {}, 0, 0})) {
    const auto path = directory / "sound-effects.json";
    const auto data = [NSData dataWithContentsOfFile:[NSString stringWithUTF8String:path.c_str()]];
    NSError *error = nil;
    id root = data ? [NSJSONSerialization JSONObjectWithData:data options:0 error:&error] : nil;
    if (error || ![root isKindOfClass:NSDictionary.class] ||
        number(root[@"version"], 1, 1) != 1 ||
        ![root[@"sample_metadata"] isKindOfClass:NSArray.class])
        throw std::runtime_error("missing imported sound-effect sample metadata");
    for (id item in root[@"sample_metadata"]) {
        if (![item isKindOfClass:NSDictionary.class])
            throw std::runtime_error("invalid imported sound-effect sample record");
        const auto origin = text(item[@"origin"]);
        const auto bank = lower(text(item[@"bank"]));
        const auto ordinal = static_cast<std::size_t>(number(item[@"ordinal"], 0, UINT32_MAX));
        Metadata metadata{
            static_cast<std::uint32_t>(number(item[@"source_bytes"], 1, UINT32_MAX)),
            static_cast<std::uint32_t>(number(item[@"source_rate"], 1, UINT32_MAX)),
            static_cast<std::int32_t>(number(item[@"loop_count"], INT32_MIN, 1))};
        const std::filesystem::path relative(text(item[@"path"]));
        if (relative.is_absolute() || relative.lexically_normal() != relative ||
            relative.empty() || *relative.begin() != "banks" ||
            std::filesystem::file_size(directory / relative) != metadata.source_bytes)
            throw std::runtime_error("imported sound-effect payload size or path mismatch");
        if (!impl_->metadata.emplace(Impl::Key{origin, bank, ordinal}, metadata).second)
            throw std::runtime_error("duplicate imported sound-effect sample identity");
    }
}
Output::~Output() = default;
BankView Output::bank(std::uint32_t slot) {
    const auto &owner = impl_->state.slots.at(slot);
    BankView view{bool(owner), {}};
    if (owner)
        for (std::size_t index = 0; index < impl_->banks.sample_count(owner); ++index)
            view.names.push_back(impl_->banks.sample_name(owner, index));
    return view;
}
SelectedSample Output::sample(std::uint32_t slot, std::size_t ordinal) {
    const auto &owner = impl_->state.slots.at(slot);
    if (!owner)
        return {};
    const auto path = lower(owner->loaded_path());
    const auto separator = path.find_last_of('\\');
    const auto filename = path.substr(separator == std::string::npos ? 0 : separator + 1);
    if (filename.size() < 4 || filename.substr(filename.size() - 4) != ".dat")
        throw std::runtime_error("native sound sample has no imported DAT identity");
    const Impl::Key key{path.rfind("system\\", 0) == 0 ? "system" : "cd",
                        filename.substr(0, filename.size() - 4),
                        impl_->banks.source_ordinal(owner, ordinal)};
    const auto found = impl_->metadata.find(key);
    if (found == impl_->metadata.end())
        throw std::runtime_error("native sound sample lacks imported reader metadata");
    auto resource = std::make_shared<DecodedSample>(impl_->banks.sample(owner, ordinal), found->second);
    return {std::move(resource), found->second.source_bytes};
}
void Output::load_pcm(std::int32_t channel, const SelectedSample &selected, std::int32_t loop) {
    if (impl_->pcm.initialized == 0)
        return;
    if (!impl_->output.ready())
        throw std::runtime_error("native PCM output is not ready");
    const auto sample = std::dynamic_pointer_cast<const DecodedSample>(selected.resource);
    if (!sample || sample->metadata.source_bytes != selected.source_size)
        throw std::runtime_error("native PCM load requires its selected imported sample");
    if (loop == 2) {
        if (sample->metadata.loops != 0)
            throw std::runtime_error("native PCM sampler loop regions are unrecovered");
        loop = 0;
    }
    if (loop != 0 && loop != 1)
        throw std::runtime_error("native PCM loop argument outside supported range");
    std::unique_ptr<audio::PcmSound> sound =
        std::make_unique<LoadedSound>(impl_->output, sample, loop != 0);
    if (!audio::stop_pcm(impl_->pcm, channel) ||
        !impl_->pcm.adopt_active(static_cast<std::size_t>(channel), sound))
        throw std::runtime_error("native PCM load could not replace its channel owner");
    ++impl_->loads;
}
void Output::volume(std::int32_t value, std::int32_t channel) {
    sound_effects::set_volume(value, channel, impl_->queue, *this);
}
void Output::pan(std::int32_t value, std::int32_t channel) {
    sound_effects::set_pan(value, channel, *this);
}
void Output::pitch(std::int32_t value, std::int32_t channel) {
    sound_effects::set_pitch(value, channel, *this, impl_->fp);
}
void Output::set_volume(std::int32_t channel, std::int32_t value) {
    std::lock_guard lock(impl_->pcm.active_mutex());
    impl_->voice(channel).volume(value);
}
void Output::set_pan(std::int32_t channel, std::int32_t value) {
    std::lock_guard lock(impl_->pcm.active_mutex());
    impl_->voice(channel).pan(value);
}
void Output::set_frequency(std::int32_t channel, std::uint32_t value) {
    std::lock_guard lock(impl_->pcm.active_mutex());
    impl_->voice(channel).frequency(value);
}
void Output::play_pcm(std::int32_t channel) {
    std::lock_guard lock(impl_->pcm.active_mutex());
    impl_->voice(channel).play();
    ++impl_->plays;
}
void Output::stop_pcm(std::int32_t channel) {
    if (!audio::stop_pcm(impl_->pcm, channel))
        throw std::runtime_error("native sound server PCM stop failed");
}
void Output::load_3d_pcm(std::int32_t, const SelectedSample &) {
    throw std::runtime_error("native 3D PCM buffers are unrecovered");
}
void Output::set_3d_min_max(std::int32_t, float, float) {
    throw std::runtime_error("native 3D PCM attenuation range is unrecovered");
}
std::int32_t Output::distance_volume(std::int32_t) {
    throw std::runtime_error("native sound distance-volume calculation is unrecovered");
}
void Output::set_position(std::int32_t, float, float, float) {
    throw std::runtime_error("native 3D PCM position is unrecovered");
}
void Output::play_voice(std::int32_t) {
    throw std::runtime_error("native queued sound-effect voice requests are unrecovered");
}
void Output::update_positions() {
    throw std::runtime_error("native sound listener update is unrecovered");
}
std::uint32_t Output::load_events() const { return impl_->loads; }
std::uint32_t Output::play_events() const { return impl_->plays; }
} // namespace sadx::sound_effects::mac
