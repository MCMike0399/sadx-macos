// The test and agent harness of sadx-app --guest: drive the game like a player and observe it.
//
// Reference tier (ADR-0007): it reads guest memory at the original addresses, named from the
// symbol database. Commands come from a script file (--script FILE) and/or from stdin
// (--control -). One command per line; blank lines and `#` comments are ignored.
//
//   [@FRAME] VERB ARGS...
//
// `@FRAME` schedules the command at guest Present FRAME (the same frame numbering as --keys and
// --exit-after). Without it, a script command is an error, and a control command runs at the
// next frame boundary.
//
//   press KEY [FRAMES]          key down now, up after FRAMES frames (default 5)
//   down KEY | up KEY           hold or release a key
//   pad BUTTONS [FRAMES]        hold pad buttons (a b x y start back lb rb up down left right
//                               ls rs, joined with +, or a numeric XInput mask); 0 or `none`
//                               releases them. With FRAMES they are released afterwards.
//   stick X Y [FRAMES]          left stick, -1..1 each (Y up); `stick 0 0` centres it
//   rstick X Y [FRAMES]         right stick
//   triggers L R [FRAMES]       0..1 each
//   release                     release every key, button, stick and trigger
//   screenshot PATH             write this frame as .png (or .ppm)
//   state [PATH]                write the game state as JSON (to PATH, and to stdout)
//   pause | run | step N        stop advancing the guest / run freely / advance N frames, then pause
//                               (with --control the game starts paused on its first frame)
//   quit                        exit after this frame
//
// KEY is a Windows virtual key name (enter esc space tab backspace up down left right shift ctrl
// alt a..z 0..9) or a number. Every executed command answers on stdout with one line
// `harness: <frame> <verb> ok|error ...`, so a driver can wait for it
// (tools/journey/sadx_driver.py).
#pragma once
#include <fcntl.h>
#include <unistd.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <vector>
#include "hle/hle_process.hpp"
#include "hle/hle_xinput.hpp"

namespace sadx::harness {

struct Command {
    std::optional<std::uint64_t> frame;
    std::string verb;
    std::vector<std::string> args;
    std::string text;
};

inline std::optional<Command> parse_command(const std::string& line, std::string* error) {
    std::istringstream in(line.substr(0, line.find('#')));
    Command command;
    std::string word;
    while (in >> word) {
        if (command.verb.empty() && !command.frame && word[0] == '@') {
            command.frame = std::strtoull(word.c_str() + 1, nullptr, 10);
        } else if (command.verb.empty()) {
            command.verb = word;
        } else {
            command.args.push_back(word);
        }
    }
    if (command.verb.empty()) {
        if (command.frame)
            *error = "a frame without a command";
        return std::nullopt;
    }
    command.text = line;
    return command;
}

inline std::optional<std::uint32_t> virtual_key(const std::string& name) {
    static const std::map<std::string, std::uint32_t> names = {
        {"enter", 0x0d}, {"return", 0x0d}, {"esc", 0x1b}, {"escape", 0x1b}, {"space", 0x20},
        {"tab", 0x09}, {"backspace", 0x08}, {"left", 0x25}, {"up", 0x26}, {"right", 0x27},
        {"down", 0x28}, {"shift", 0xa0}, {"rshift", 0xa1}, {"ctrl", 0xa2}, {"rctrl", 0xa3},
        {"alt", 0xa4}, {"ralt", 0xa5}};
    if (const auto it = names.find(name); it != names.end())
        return it->second;
    if (name.size() == 1 && std::isalnum(static_cast<unsigned char>(name[0])))
        return static_cast<std::uint32_t>(std::toupper(static_cast<unsigned char>(name[0])));
    char* end = nullptr;
    const auto value = std::strtoul(name.c_str(), &end, 0);
    if (end && *end == 0 && value > 0 && value < 256)
        return static_cast<std::uint32_t>(value);
    return std::nullopt;
}

// XInput button bits (XINPUT_GAMEPAD_*).
inline std::optional<std::uint16_t> pad_buttons(const std::string& text) {
    static const std::map<std::string, std::uint16_t> names = {
        {"up", 0x0001}, {"down", 0x0002}, {"left", 0x0004}, {"right", 0x0008},
        {"start", 0x0010}, {"back", 0x0020}, {"ls", 0x0040}, {"rs", 0x0080},
        {"lb", 0x0100}, {"rb", 0x0200}, {"a", 0x1000}, {"b", 0x2000}, {"x", 0x4000},
        {"y", 0x8000}};
    if (text == "none")
        return 0;
    char* end = nullptr;
    const auto number = std::strtoul(text.c_str(), &end, 0);
    if (end && *end == 0)
        return number <= 0xffff ? std::optional<std::uint16_t>(static_cast<std::uint16_t>(number))
                                : std::nullopt;
    std::uint16_t mask = 0;
    std::size_t at = 0;
    while (at <= text.size()) {
        const auto plus = std::min(text.find('+', at), text.size());
        const auto it = names.find(text.substr(at, plus - at));
        if (it == names.end())
            return std::nullopt;
        mask |= it->second;
        at = plus + 1;
    }
    return mask;
}

inline std::int16_t axis(double value) {
    return static_cast<std::int16_t>(std::lround(std::clamp(value, -1.0, 1.0) * 32767.0));
}

inline std::uint8_t trigger(double value) {
    return static_cast<std::uint8_t>(std::lround(std::clamp(value, 0.0, 1.0) * 255.0));
}

// A JSON snapshot of the game state that tests and agents need, read from guest memory.
// Names are the symbol database's (mod loader *New.h names first, classic names in comments).
inline std::string game_state_json(HleProcess& process, std::uint64_t frame) {
    const auto& g = process.guest();
    const auto u32 = [&](std::uint32_t address) -> long long {
        const auto v = g.u32(address);
        return v ? static_cast<long long>(*v) : -1;
    };
    const auto s16 = [&](std::uint32_t address) -> long long {
        std::int16_t v = 0;
        return g.read(address, &v, sizeof v) ? v : -1;
    };
    const auto f32 = [&](std::uint32_t address) -> std::string {
        float v = 0;
        if (!g.read(address, &v, sizeof v))
            return "null";
        if (!std::isfinite(v))
            return "\"" + std::to_string(v) + "\"";
        char text[32];
        std::snprintf(text, sizeof text, "%.9g", v);
        return text;
    };
    const auto vector = [&](std::uint32_t address) {
        return "[" + f32(address) + "," + f32(address + 4) + "," + f32(address + 8) + "]";
    };
    std::ostringstream out;
    out << "{\"frame\":" << frame << ",\"presents\":" << process.d3d8.presents
        << ",\"game_mode\":" << u32(0x3abdc7c)       // ulGlobalMode (GameMode)
        << ",\"game_state\":" << s16(0x3b22de4)      // ssGameMode (GameState)
        << ",\"stage\":" << s16(0x3b22dcc)           // ssStageNumber (CurrentLevel)
        << ",\"act\":" << s16(0x3b22dec)             // ssActNumber (CurrentAct)
        << ",\"rings\":" << s16(0x3b0f0e4)           // Rings
        << ",\"level_frames\":" << u32(0x3b0f108)    // LevelFrameCount
        << ",\"global_timer\":" << u32(0x3b0f13c);   // ulGlobalTimer (FrameCounter)
    // playertwp[0] (EntityData1Ptrs[0]) and CharObj2Ptrs[0]: the first player, when it exists.
    const auto task = g.u32(0x3b42e10).value_or(0);
    if (task != 0) {
        std::uint8_t action = 0, character = 0;
        std::uint16_t status = 0;
        std::int32_t rotation[3] = {};
        g.read(task + 0x00, &action, 1);
        g.read(task + 0x04, &status, 2);
        g.read(task + 0x09, &character, 1);
        g.read(task + 0x14, rotation, sizeof rotation);
        out << ",\"player\":{\"action\":" << int(action) << ",\"status\":" << status
            << ",\"character\":" << int(character) << ",\"rotation\":[" << rotation[0] << ","
            << rotation[1] << "," << rotation[2] << "],\"position\":" << vector(task + 0x20);
        if (const auto physics = g.u32(0x3b3cdf0).value_or(0))
            out << ",\"speed\":" << vector(physics + 0x38);
        out << "}";
    } else {
        out << ",\"player\":null";
    }
    out << "}";
    return out.str();
}

class Harness {
public:
    bool load_script(const std::string& path, std::string* error) {
        std::ifstream in(path);
        if (!in) {
            *error = "cannot read script " + path;
            return false;
        }
        std::string line;
        int number = 0;
        while (std::getline(in, line)) {
            ++number;
            std::string why;
            auto command = parse_command(line, &why);
            if (!command && !why.empty()) {
                *error = path + ":" + std::to_string(number) + ": " + why;
                return false;
            }
            if (!command)
                continue;
            if (!command->frame) {
                *error = path + ":" + std::to_string(number) + ": script commands need @FRAME";
                return false;
            }
            scheduled_.push_back(*command);
        }
        std::stable_sort(scheduled_.begin(), scheduled_.end(),
                         [](const Command& a, const Command& b) { return *a.frame < *b.frame; });
        return true;
    }

    // Interactive control starts paused on the first frame: the driver decides when time runs.
    void enable_stdin_control() {
        control_ = true;
        paused_ = true;
        fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);
    }

    bool active() const { return control_ || !scheduled_.empty(); }
    bool quit_requested() const { return quit_; }
    bool control_closed() const { return control_ && stdin_closed_; }

    // Whether the guest may run its next frame. A paused harness keeps polling its control input.
    bool may_advance() {
        if (paused_ && step_budget_ == 0)
            poll_control(current_frame_);
        if (!paused_)
            return true;
        if (step_budget_ > 0) {
            --step_budget_;
            return true;
        }
        return false;
    }

    // Called when the guest reaches Present FRAME (inputs for this frame are applied here).
    void on_present(HleProcess& process, std::uint64_t frame) {
        process_ = &process;
        current_frame_ = frame;
        for (auto it = releases_.begin(); it != releases_.end();) {
            if (it->first <= frame) {
                apply(process, it->second, frame, true);
                it = releases_.erase(it);
            } else {
                ++it;
            }
        }
        while (next_ < scheduled_.size() && *scheduled_[next_].frame <= frame)
            execute(process, scheduled_[next_++], frame);
        poll_control(frame);
        if (pad_dirty_) {
            process.host_xinput(pad_);
            pad_dirty_ = false;
        }
    }

    bool drives_pad() const { return pad_used_; }

    // Screenshots requested for the frame about to be drawn (the Present FRAME image).
    std::vector<std::string> take_screenshots(std::uint64_t frame) {
        std::vector<std::string> out;
        for (auto it = screenshots_.begin(); it != screenshots_.end();) {
            if (it->first == frame) {
                out.push_back(it->second);
                it = screenshots_.erase(it);
            } else {
                ++it;
            }
        }
        return out;
    }

    static void reply(std::uint64_t frame, const std::string& verb, const std::string& result) {
        std::printf("harness: %llu %s %s\n", static_cast<unsigned long long>(frame), verb.c_str(),
                    result.c_str());
        std::fflush(stdout);
    }

private:
    void poll_control(std::uint64_t frame) {
        if (!control_ || stdin_closed_)
            return;
        char buffer[4096];
        for (;;) {
            const auto n = read(STDIN_FILENO, buffer, sizeof buffer);
            if (n > 0) {
                pending_.append(buffer, static_cast<std::size_t>(n));
                continue;
            }
            if (n == 0)
                stdin_closed_ = true;
            break;
        }
        std::size_t newline;
        while ((newline = pending_.find('\n')) != std::string::npos) {
            const auto line = pending_.substr(0, newline);
            pending_.erase(0, newline + 1);
            std::string why;
            auto command = parse_command(line, &why);
            if (!command) {
                if (!why.empty())
                    reply(frame, "parse", "error " + why);
                continue;
            }
            if (command->frame && *command->frame > frame) {
                scheduled_.insert(std::upper_bound(scheduled_.begin() + static_cast<long>(next_),
                                                   scheduled_.end(), *command,
                                                   [](const Command& a, const Command& b) {
                                                       return *a.frame < *b.frame;
                                                   }),
                                  *command);
            } else if (process_) {
                execute(*process_, *command, frame);
            }
        }
        if (stdin_closed_ && pending_.empty())
            quit_ = true; // the driver closed the control channel
    }

    void apply(HleProcess& process, const Command& command, std::uint64_t frame, bool release) {
        (void)frame;
        const auto& verb = command.verb;
        if (verb == "press" || verb == "down" || verb == "up") {
            if (const auto key = virtual_key(command.args.at(0)))
                process.host_key(*key, verb != "up" && !release);
        } else if (verb == "pad") {
            pad_.buttons = release ? 0 : *pad_buttons(command.args.at(0));
        } else if (verb == "stick") {
            pad_.left_x = release ? 0 : axis(std::atof(command.args.at(0).c_str()));
            pad_.left_y = release ? 0 : axis(std::atof(command.args.at(1).c_str()));
        } else if (verb == "rstick") {
            pad_.right_x = release ? 0 : axis(std::atof(command.args.at(0).c_str()));
            pad_.right_y = release ? 0 : axis(std::atof(command.args.at(1).c_str()));
        } else if (verb == "triggers") {
            pad_.left_trigger = release ? 0 : trigger(std::atof(command.args.at(0).c_str()));
            pad_.right_trigger = release ? 0 : trigger(std::atof(command.args.at(1).c_str()));
        }
        if (verb == "pad" || verb == "stick" || verb == "rstick" || verb == "triggers")
            pad_used_ = pad_dirty_ = true;
    }

    // Validates and runs one command at FRAME.
    void execute(HleProcess& process, const Command& command, std::uint64_t frame) {
        const auto& verb = command.verb;
        const auto& args = command.args;
        const auto hold_index = [&](std::size_t index) -> std::optional<std::uint64_t> {
            if (args.size() <= index)
                return std::nullopt;
            return std::strtoull(args[index].c_str(), nullptr, 10);
        };
        const auto schedule_release = [&](std::optional<std::uint64_t> frames) {
            if (frames)
                releases_.emplace_back(frame + std::max<std::uint64_t>(*frames, 1), command);
        };
        if ((verb == "press" || verb == "down" || verb == "up")) {
            if (args.empty() || !virtual_key(args[0]))
                return reply(frame, verb, "error unknown key");
            apply(process, command, frame, false);
            if (verb == "press")
                schedule_release(hold_index(1).value_or(5));
        } else if (verb == "pad") {
            if (args.empty() || !pad_buttons(args[0]))
                return reply(frame, verb, "error unknown buttons");
            apply(process, command, frame, false);
            schedule_release(hold_index(1));
        } else if (verb == "stick" || verb == "rstick" || verb == "triggers") {
            if (args.size() < 2)
                return reply(frame, verb, "error needs two values");
            apply(process, command, frame, false);
            schedule_release(hold_index(2));
        } else if (verb == "release") {
            for (std::uint32_t key = 1; key < 256; ++key)
                process.host_key(key, false);
            pad_ = HleXInputGamepad{};
            pad_used_ = pad_dirty_ = true;
            releases_.clear();
        } else if (verb == "screenshot") {
            if (args.empty())
                return reply(frame, verb, "error needs a path");
            screenshots_.emplace_back(frame, args[0]);
            return; // answered once the image is written
        } else if (verb == "state") {
            const auto json = game_state_json(process, frame);
            if (!args.empty())
                std::ofstream(args[0]) << json << "\n";
            return reply(frame, verb, "ok " + json);
        } else if (verb == "pause") {
            paused_ = true;
            step_budget_ = 0;
        } else if (verb == "run") {
            paused_ = false;
        } else if (verb == "step") {
            paused_ = true;
            step_budget_ = hold_index(0).value_or(1);
        } else if (verb == "quit") {
            quit_ = true;
        } else {
            return reply(frame, verb, "error unknown command");
        }
        reply(frame, verb, "ok");
    }

    std::vector<Command> scheduled_;
    std::size_t next_ = 0;
    std::vector<std::pair<std::uint64_t, Command>> releases_;
    std::vector<std::pair<std::uint64_t, std::string>> screenshots_;
    HleXInputGamepad pad_{};
    bool pad_used_ = false, pad_dirty_ = false;
    bool control_ = false, stdin_closed_ = false, paused_ = false, quit_ = false;
    std::uint64_t step_budget_ = 0, current_frame_ = 0;
    std::string pending_;
    HleProcess* process_ = nullptr;
};

} // namespace sadx::harness
