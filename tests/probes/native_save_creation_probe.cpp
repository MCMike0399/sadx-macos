#include "game/save_creation.hpp"
#include "game/save_file_list.hpp"
#include <cstring>
extern "C" int sadx_native_save_capture(const std::uint8_t *bytes, const std::uint32_t *words,
                                        const std::uint8_t *story, std::uint8_t *output,
                                        std::uint32_t *result) {
    sadx::save::EncodedData initial;
    std::copy_n(bytes, initial.size(), initial.begin());
    auto data = sadx::file_select::decode_save(initial);
    auto lives = static_cast<std::int8_t>(words[3]);
    const auto character = static_cast<std::uint16_t>(words[2]);
    const auto metal = static_cast<std::uint8_t>(words[4]);
    const auto super = static_cast<std::uint8_t>(words[5]);
    const auto rumble = static_cast<std::uint8_t>(words[7]);
    std::array<std::uint8_t, 512> flags, events;
    std::copy_n(story, 512, flags.begin());
    std::copy_n(story + 512, 512, events.begin());
    std::array<sadx::bootstrap::StorySequence, 8> sequences;
    for (std::size_t i = 0; i < sequences.size(); ++i) {
        const auto *entry = story + 1032 + i * 12;
        auto &sequence = sequences[i];
        sequence.time = static_cast<std::int8_t>(entry[0]);
        sequence.emerald = static_cast<std::int8_t>(entry[1]);
        std::memcpy(&sequence.number, entry + 2, 2);
        std::memcpy(&sequence.section, entry + 4, 2);
        std::memcpy(&sequence.next_section, entry + 6, 2);
        std::memcpy(&sequence.stage, entry + 8, 2);
        std::memcpy(&sequence.destination, entry + 10, 2);
    }
    std::array<std::uint8_t, 344> completed;
    std::array<std::uint8_t, 60> missions;
    std::copy_n(story + 1128, 344, completed.begin());
    std::copy_n(story + 1472, 60, missions.begin());
    sadx::save::CaptureState state{words[0],  words[1],  character, lives,  metal,
                                   super,     words[6],  flags,     events, story + 1024,
                                   sequences, completed, missions,  rumble, words[8]};
    sadx::save::capture_live_data(data, state);
    auto encoded = sadx::save::encode(data);
    std::copy(encoded.begin(), encoded.end(), output);
    result[0] = static_cast<std::uint8_t>(lives);
    return 1;
}
#include "game/save_write.hpp"
#include <vector>
namespace {
struct FixtureFile final : sadx::file_select::File {};
struct WriteFixture final : sadx::save::WriteCalls {
    std::vector<std::uint32_t> calls;
    std::string path;
    sadx::save::EncodedData written{};
    std::uint32_t returned_items;
    explicit WriteFixture(std::uint32_t value) : returned_items(value) {}
    void ensure_directory() override { calls.push_back(1); }
    std::shared_ptr<sadx::file_select::File> open_write(std::string_view value) override {
        calls.push_back(3);
        path = value;
        return std::make_shared<FixtureFile>();
    }
    std::uint32_t write_save(const std::shared_ptr<sadx::file_select::File> &,
                             const sadx::save::EncodedData &bytes) override {
        calls.push_back(4);
        written = bytes;
        return returned_items;
    }
    void reset_input() override { calls.push_back(5); }
    void close_write(const std::shared_ptr<sadx::file_select::File> &) override {
        calls.push_back(6);
    }
};
} // namespace
extern "C" int sadx_native_save_write(const std::uint32_t *input, const char *name,
                                      const char *list_names, const std::uint8_t *bytes,
                                      std::uint32_t *output, char *out_name, char *out_path,
                                      std::uint8_t *written) {
    auto disabled = static_cast<std::int32_t>(input[0]);
    auto retry = static_cast<std::uint8_t>(input[1]), ok = static_cast<std::uint8_t>(input[2]);
    auto count = static_cast<std::uint8_t>(input[3]);
    auto phase = static_cast<std::int32_t>(input[4]);
    auto ready = static_cast<std::uint8_t>(input[5]), exec = static_cast<std::uint8_t>(input[6]);
    auto alert = static_cast<std::uint8_t>(input[7]),
         deleting = static_cast<std::uint8_t>(input[8]);
    auto first = static_cast<std::uint8_t>(input[9]),
         file_count = static_cast<std::uint8_t>(input[10]);
    auto selected = static_cast<std::uint8_t>(input[11]),
         complete = static_cast<std::uint8_t>(input[12]);
    sadx::save::SaveNameState save_name;
    std::unique_ptr<char[]> save_name_backing;
    if (input[13]) {
        save_name_backing = std::make_unique<char[]>(std::strlen(name) + 1);
        std::strcpy(save_name_backing.get(), name);
        save_name.value = save_name_backing.get();
    }
    sadx::save::SaveNameCalls names{[&](const char *value) {
                                        if (value != save_name_backing.get())
                                            throw std::invalid_argument(
                                                "foreign save-name allocation");
                                        save_name_backing.reset();
                                    },
                                    [&](std::size_t size) {
                                        save_name_backing = std::make_unique<char[]>(size);
                                        return save_name_backing.get();
                                    },
                                    [](char *destination, std::string_view value) {
                                        std::memcpy(destination, value.data(), value.size());
                                        destination[value.size()] = 0;
                                    }};
    auto list = std::make_shared<sadx::file_select::ListNode>();
    auto tail = list;
    for (std::uint32_t i = 0; i < input[14]; ++i) {
        tail->next = std::make_shared<sadx::file_select::ListNode>();
        tail = tail->next;
        tail->file = sadx::file_select::FileInfo{list_names + i * 16, 0};
    }
    sadx::save::EncodedData initial;
    std::copy_n(bytes, 1392, initial.begin());
    auto data = sadx::file_select::decode_save(initial);
    sadx::save::WriteState state{disabled, retry,     ok,       count, phase,      ready,
                                 exec,     alert,     deleting, first, file_count, selected,
                                 complete, save_name, list,     data};
    WriteFixture calls(input[15]);
    output[0] = static_cast<std::uint32_t>(
        sadx::save::write_file(state, calls, names, [&] { calls.calls.push_back(2); }));
    const std::uint32_t fields[] = {
        retry,      ok,       count,    static_cast<std::uint32_t>(phase),
        ready,      exec,     alert,    first,
        file_count, selected, complete, static_cast<std::uint32_t>(save_name.value != nullptr)};
    std::copy_n(fields, 12, output + 1);
    output[13] = static_cast<std::uint32_t>(calls.calls.size());
    std::copy(calls.calls.begin(), calls.calls.end(), output + 14);
    if (save_name.value)
        std::strcpy(out_name, save_name.value);
    std::copy(calls.path.begin(), calls.path.end(), out_path);
    std::copy(calls.written.begin(), calls.written.end(), written);
    return 1;
}
#include "game/save_completion.hpp"
namespace {
struct FinishFixture final : sadx::file_select::SaveCompletionCalls {
    sadx::TaskScheduler scheduler;
    std::vector<std::uint32_t> calls;
    std::int8_t selection;
    explicit FinishFixture(std::int8_t value) : selection(value) {
        scheduler.set_default_field30(std::shared_ptr<void>{});
    }
    void request_dialog(std::int32_t index) override {
        calls.push_back(100 + static_cast<std::uint32_t>(index));
    }
    sadx::TaskHandle create_save_warning() override {
        calls.push_back(2);
        return scheduler.append_task(0, nullptr).task;
    }
    std::int8_t dialog_selection() override {
        calls.push_back(3);
        return selection;
    }
    void close_save_warning(sadx::TaskHandle) override { calls.push_back(4); }
    void transition_menu(sadx::title_menu::MenuMode) override { calls.push_back(5); }
    void prepare_new_save() override { calls.push_back(6); }
    void apply_save() override { calls.push_back(7); }
};
} // namespace
extern "C" int sadx_native_save_completion(const std::uint32_t *v, std::uint32_t *out) {
    sadx::title_menu::FileSelectWork work{};
    work.submode = static_cast<std::int32_t>(v[0]);
    work.wait_frames = static_cast<std::int16_t>(v[1]);
    work.presentation.next = static_cast<sadx::title_menu::MenuMode>(v[13]);
    auto alert = static_cast<std::uint8_t>(v[2]), ok = static_cast<std::uint8_t>(v[3]);
    auto fade = static_cast<std::uint8_t>(v[4]);
    auto phase = static_cast<std::int32_t>(v[5]);
    auto ready = static_cast<std::uint8_t>(v[6]), exec = static_cast<std::uint8_t>(v[7]);
    auto flag = static_cast<std::uint8_t>(v[8]), card = static_cast<std::uint8_t>(v[9]);
    auto backup = static_cast<std::uint16_t>(v[10]);
    auto disabled = static_cast<std::int32_t>(v[11]);
    sadx::TaskScheduler scheduler;
    scheduler.set_default_field30(std::shared_ptr<void>{});
    auto task = scheduler.append_task(0, nullptr).task;
    const auto initial = task;
    sadx::file_select::SaveCompletionState state{alert, ok,   fade,   phase,    ready, exec,
                                                 flag,  card, backup, disabled, task};
    FinishFixture calls(static_cast<std::int8_t>(v[12]));
    const auto handled = sadx::file_select::update_save_completion(work, state, calls);
    const std::uint32_t fields[] = {static_cast<std::uint32_t>(work.submode),
                                    static_cast<std::uint16_t>(work.wait_frames),
                                    alert,
                                    ok,
                                    fade,
                                    static_cast<std::uint32_t>(phase),
                                    ready,
                                    exec,
                                    flag,
                                    card,
                                    backup,
                                    static_cast<std::uint32_t>(disabled),
                                    static_cast<std::uint32_t>(task != initial),
                                    static_cast<std::uint32_t>(calls.calls.size())};
    std::copy_n(fields, 14, out);
    std::copy(calls.calls.begin(), calls.calls.end(), out + 14);
    return handled;
}
extern "C" int sadx_native_save_request(const std::uint32_t *v, std::uint32_t *out) {
    auto disabled = static_cast<std::int32_t>(v[0]);
    auto ready = static_cast<std::uint8_t>(v[1]);
    std::uint32_t captures = 0;
    out[0] =
        static_cast<std::uint32_t>(sadx::save::request_write(disabled, ready, [&] { ++captures; }));
    out[1] = ready;
    out[2] = captures;
    return 1;
}
extern "C" int sadx_native_save_prepare(std::uint32_t pattern, std::int32_t initial_rings,
                                        std::uint8_t *save_out, std::uint8_t *story_out,
                                        std::uint32_t *result) {
    std::int32_t card = 0;
    std::uint32_t timer = 0, movie = 0;
    std::uint16_t mode = 0;
    auto rings = static_cast<std::int16_t>(initial_rings);
    sadx::bootstrap::State story({card, timer, rings, movie, mode});
    const auto byte = static_cast<std::uint8_t>(pattern);
    story.sequence_variables.fill(byte);
    story.cutscene_flags.fill(byte);
    story.event_work.fill(byte);
    story.completed_stages.fill(byte);
    std::memset(story.sequences.data(), byte, sizeof(story.sequences));
    sadx::save::EncodedData initial;
    initial.fill(byte);
    auto data = sadx::file_select::decode_save(initial);
    sadx::save::prepare_new(story, data, rings);
    const auto encoded = sadx::save::encode(data);
    std::copy(encoded.begin(), encoded.end(), save_out);
    std::copy(story.sequence_variables.begin(), story.sequence_variables.end(), story_out);
    std::copy(story.cutscene_flags.begin(), story.cutscene_flags.end(), story_out + 512);
    std::copy(story.event_work.begin(), story.event_work.end(), story_out + 1024);
    static_assert(sizeof(story.sequences) == 96);
    std::memcpy(story_out + 1536, story.sequences.data(), 96);
    std::copy(story.completed_stages.begin(), story.completed_stages.end(), story_out + 1632);
    result[0] = static_cast<std::uint16_t>(rings);
    return 1;
}
