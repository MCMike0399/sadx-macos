#include "platform/mac/mac_sound_bank.hpp"
#include <CommonCrypto/CommonDigest.h>
#include <Foundation/Foundation.h>
#include <fstream>
#include <unordered_map>

// [platform:audio] Imported bank payloads become native decoded clips and AudioQueue
// leases; file priority and sample-name behavior remain in the compiled profile code.
namespace sadx::sound_bank::mac {
namespace {
std::string text(id value) {
    if (![value isKindOfClass:[NSString class]])
        throw Error(Boundary::Open, "invalid imported sound-bank text");
    return [value UTF8String];
}
std::uint32_t number(id value) {
    if (![value isKindOfClass:[NSNumber class]] ||
        CFGetTypeID((__bridge CFTypeRef)value) == CFBooleanGetTypeID() ||
        [value doubleValue] != [value unsignedLongLongValue] ||
        [value unsignedLongLongValue] > UINT32_MAX)
        throw Error(Boundary::Open, "invalid imported sound-bank number");
    return [value unsignedIntValue];
}
std::int32_t signed_number(id value) {
    if (![value isKindOfClass:[NSNumber class]] ||
        CFGetTypeID((__bridge CFTypeRef)value) == CFBooleanGetTypeID() ||
        [value doubleValue] != [value longLongValue] || [value longLongValue] < INT32_MIN ||
        [value longLongValue] > INT32_MAX)
        throw Error(Boundary::Priority, "invalid imported signed replacement priority");
    return [value intValue];
}
NSArray *array(id value) {
    if (![value isKindOfClass:[NSArray class]])
        throw Error(Boundary::Open, "invalid imported sound-bank array");
    return value;
}
NSDictionary *dictionary(id value) {
    if (![value isKindOfClass:[NSDictionary class]])
        throw Error(Boundary::Open, "invalid imported sound-bank record");
    return value;
}
std::string lower(std::string value) {
    for (auto &character : value) {
        const auto byte = static_cast<unsigned char>(character);
        if (byte > 127)
            throw Error(Boundary::Path, "sound-bank logical paths require ASCII");
        if (character >= 'A' && character <= 'Z')
            character = static_cast<char>(character + ('a' - 'A'));
    }
    return value;
}
std::string digest(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary);
    if (!file)
        throw Error(Boundary::Open, "imported bank sample missing: " + path.string());
    std::vector<unsigned char> bytes{std::istreambuf_iterator<char>(file), {}};
    if (!file.eof() && file.fail())
        throw Error(Boundary::Open, "imported bank sample read failed: " + path.string());
    unsigned char output[CC_SHA256_DIGEST_LENGTH];
    CC_SHA256(bytes.data(), static_cast<CC_LONG>(bytes.size()), output);
    constexpr char hex[] = "0123456789abcdef";
    std::string result;
    for (const auto byte : output) {
        result += hex[byte >> 4];
        result += hex[byte & 15];
    }
    return result;
}
struct SampleSpec {
    std::filesystem::path path;
    std::string sha256, original_name;
};
struct BankSpec {
    std::uint8_t archive_tag = 0;
    std::vector<SampleSpec> samples;
};
class DecodedBank final : public Bank {
  public:
    explicit DecodedBank(std::string path) : Bank(std::move(path)) {}
    std::vector<std::shared_ptr<const audio::mac::AudioClip>> samples;
    std::vector<std::string> names;
    std::vector<std::size_t> source_ordinals;
    bool closed = false;
};
DecodedBank &decoded(const Handle &bank) {
    auto *result = dynamic_cast<DecodedBank *>(bank.get());
    if (!result)
        throw Error(Boundary::Decode, "sound-bank resource has no native decoded owner");
    return *result;
}
std::string key(std::string_view origin, std::string_view name) {
    return std::string(origin) + ':' + lower(std::string(name));
}
} // namespace
struct Banks::Impl {
    Catalog catalog;
    std::filesystem::path directory;
    std::unordered_map<std::string, BankSpec> specs;
    std::unordered_map<std::string, bool> missing, missing_packs;
    std::unordered_map<std::string, std::int32_t> mod_indices;
    std::vector<OpenAttempt> attempts;
    bool cd_imported = false;
};
Banks::Banks(const std::filesystem::path &directory) : impl_(std::make_unique<Impl>()) {
    impl_->directory = std::filesystem::canonical(directory);
    const auto path = directory / "sound-banks.json";
    NSData *data = [NSData dataWithContentsOfFile:[NSString stringWithUTF8String:path.c_str()]];
    NSError *error = nil;
    NSDictionary *root = dictionary(
        data ? [NSJSONSerialization JSONObjectWithData:data options:0 error:&error] : nil);
    if (error || number(root[@"version"]) != 2)
        throw Error(Boundary::Open, "unsupported imported sound-bank metadata");
    NSArray *lists = array(root[@"lists"]);
    if (lists.count != kListCount)
        throw Error(Boundary::ListIndex, "imported catalog must retain all 123 sound lists");
    for (std::size_t index = 0; index != kListCount; ++index) {
        for (id raw in array(lists[index])) {
            NSDictionary *entry = dictionary(raw);
            const auto slot = number(entry[@"slot"]);
            if (slot >= kSlotCount)
                throw Error(Boundary::Slot, "imported sound-bank slot is out of range");
            impl_->catalog[index].push_back(
                {static_cast<std::uint8_t>(slot), text(entry[@"name"])});
        }
    }
    id imported = root[@"cd_source_imported"];
    if (!imported || CFGetTypeID((__bridge CFTypeRef)imported) != CFBooleanGetTypeID())
        throw Error(Boundary::Open, "imported CD source availability must be explicit");
    impl_->cd_imported = [imported boolValue];
    NSDictionary *priorities = dictionary(root[@"mod_indices"]);
    for (NSString *name in priorities) {
        impl_->mod_indices.emplace(lower(text(name)), signed_number(priorities[name]));
    }
    for (id raw in array(root[@"missing_packs"])) {
        NSDictionary *record = dictionary(raw);
        impl_->missing_packs.emplace(key(text(record[@"origin"]), text(record[@"name"])), true);
    }
    for (id raw in array(root[@"missing"])) {
        NSDictionary *record = dictionary(raw);
        impl_->missing.emplace(key(text(record[@"origin"]), text(record[@"name"])), true);
    }
    for (id raw in array(root[@"banks"])) {
        NSDictionary *record = dictionary(raw);
        BankSpec bank;
        const auto archive_tag = number(record[@"archive_tag"]);
        if (archive_tag > 255)
            throw Error(Boundary::Decode, "imported archive tag is outside source byte range");
        bank.archive_tag = static_cast<std::uint8_t>(archive_tag);
        for (id sample_raw in array(record[@"samples"])) {
            NSDictionary *sample = dictionary(sample_raw);
            if (number(sample[@"ordinal"]) != bank.samples.size())
                throw Error(Boundary::Decode, "imported sample ordinals are not contiguous");
            const std::filesystem::path relative(text(sample[@"path"]));
            if (relative.empty() || relative.is_absolute())
                throw Error(Boundary::Path, "invalid imported sample path");
            for (const auto &part : relative)
                if (part == "..")
                    throw Error(Boundary::Path, "imported sample path escapes asset directory");
            const auto file = std::filesystem::weakly_canonical(impl_->directory / relative);
            const auto inside = file.lexically_relative(impl_->directory);
            if (inside.empty() || *inside.begin() == "..")
                throw Error(Boundary::Path, "imported sample resolves outside asset directory");
            bank.samples.push_back({file, text(sample[@"sha256"]), text(sample[@"name"])});
        }
        if (!impl_->specs
                 .emplace(key(text(record[@"origin"]), text(record[@"name"])), std::move(bank))
                 .second)
            throw Error(Boundary::Open, "duplicate imported sound-bank resource");
    }
}
Banks::~Banks() = default;
const Catalog &Banks::catalog() const {
    return impl_->catalog;
}
std::int32_t Banks::mod_index(std::string_view path) const {
    const auto found = impl_->mod_indices.find(lower(std::string(path)));
    return found == impl_->mod_indices.end() ? -1 : found->second;
}
Handle Banks::open_pack(std::string_view path, std::uint32_t) {
    const auto folded = lower(std::string(path));
    if (folded.empty() || folded.back() != '\\')
        throw Error(Boundary::Path, "loose sound-pack path must retain its trailing separator");
    const auto directory = folded.substr(0, folded.size() - 1);
    const auto separator = directory.find_last_of('\\');
    const auto name = directory.substr(separator == std::string::npos ? 0 : separator + 1);
    if (!impl_->missing_packs.count(key("system", name)))
        throw Error(Boundary::Pack, "requested loose sound pack is not imported");
    impl_->attempts.push_back({std::string(path), false, "source index.txt absent"});
    return nullptr;
}
Handle Banks::open_file(std::string_view logical_path, std::uint32_t slot) {
    const auto folded = lower(std::string(logical_path));
    const auto separator = folded.find_last_of('\\');
    const auto filename = separator == std::string::npos ? folded : folded.substr(separator + 1);
    if (filename.size() < 4 || filename.substr(filename.size() - 4) != ".dat")
        throw Error(Boundary::Path, "sound-bank logical path lacks DAT suffix");
    const auto name = filename.substr(0, filename.size() - 4);
    const auto origin = folded.rfind("system\\", 0) == 0 ? "system" : "cd";
    const auto lookup = key(origin, name);
    const auto found = impl_->specs.find(lookup);
    if (found == impl_->specs.end()) {
        if (impl_->missing.count(lookup) ||
            (std::string_view(origin) == "cd" && !impl_->cd_imported)) {
            impl_->attempts.push_back(
                {std::string(logical_path), false, "source bank file absent"});
            return nullptr;
        }
        throw Error(Boundary::Open,
                    "requested bank has not been imported: " + std::string(logical_path));
    }
    auto result = std::make_shared<DecodedBank>(std::string(logical_path));
    std::int32_t counter = 0;
    for (std::size_t ordinal = 0; ordinal < found->second.samples.size(); ++ordinal) {
        const auto &sample = found->second.samples[ordinal];
        const auto name =
            normalize_name(sample.original_name, slot, counter, found->second.archive_tag == 'Z');
        if (!name)
            continue;
        if (digest(sample.path) != sample.sha256)
            throw Error(Boundary::Decode,
                        "imported bank sample hash mismatch: " + sample.path.string());
        auto clip = std::make_shared<audio::mac::AudioClip>();
        std::string error;
        if (!audio::mac::load_clip(sample.path.string(), *clip, error))
            throw Error(Boundary::Decode, error);
        result->samples.push_back(std::move(clip));
        result->source_ordinals.push_back(ordinal);
        result->names.push_back(*name);
    }
    impl_->attempts.push_back({std::string(logical_path), true,
                               "decoded " + std::to_string(result->samples.size()) + " samples"});
    return result;
}
void Banks::close(const Handle &bank) {
    if (!bank)
        return;
    auto &resource = decoded(bank);
    resource.samples.clear();
    resource.names.clear();
    resource.closed = true;
}
Services Banks::services(Channels channels, audio::PcmState &pcm) {
    return {[channels, &pcm](std::uint32_t slot) mutable {
                release_samples(slot, channels, [&pcm](std::int32_t index) {
                    if (!audio::stop_pcm(pcm, index))
                        throw Error(Boundary::ReleaseSamples, "native bank StopPCM failed");
                });
            },
            [this](const Handle &bank) {
                close(bank);
            },
            [this](std::string_view path) {
                return mod_index(path);
            },
            [this](std::string_view path, std::uint32_t slot) {
                return open_pack(path, slot);
            },
            [this](std::string_view path, std::uint32_t slot) {
                return open_file(path, slot);
            }};
}
std::size_t Banks::sample_count(const Handle &bank) const {
    const auto &resource = decoded(bank);
    if (resource.closed)
        throw Error(Boundary::Decode, "sound-bank resource is already closed");
    return resource.samples.size();
}
std::shared_ptr<const audio::mac::AudioClip> Banks::sample(const Handle &bank,
                                                           std::size_t ordinal) const {
    const auto &resource = decoded(bank);
    if (resource.closed || ordinal >= resource.samples.size())
        throw Error(Boundary::Decode, "sound-bank sample is unavailable");
    return resource.samples[ordinal];
}
std::string_view Banks::sample_name(const Handle &bank, std::size_t ordinal) const {
    const auto &resource = decoded(bank);
    if (resource.closed || ordinal >= resource.names.size())
        throw Error(Boundary::Decode, "sound-bank sample name is unavailable");
    return resource.names[ordinal];
}
std::size_t Banks::source_ordinal(const Handle &bank, std::size_t ordinal) const {
    const auto &resource = decoded(bank);
    if (resource.closed || ordinal >= resource.source_ordinals.size())
        throw Error(Boundary::Decode, "sound-bank source sample identity is unavailable");
    return resource.source_ordinals[ordinal];
}
const std::vector<OpenAttempt> &Banks::attempts() const {
    return impl_->attempts;
}
} // namespace sadx::sound_bank::mac
