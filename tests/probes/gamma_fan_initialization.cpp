// Read-only original-runtime witness. All overlays and output belong to the caller.
#include "guest/fpu_shims.hpp"
#include "hle/hle_process.hpp"
#include <unicorn/unicorn.h>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>

namespace {
using namespace sadx;
void require(bool condition, const std::string &message) {
    if (!condition)
        throw std::runtime_error(message);
}
std::uint32_t word(X86Guest &guest, unsigned address) {
    const auto result = guest.u32(address);
    require(result.has_value(), "missing original-runtime word");
    return *result;
}
template <std::size_t N> std::array<std::uint32_t, N> words(X86Guest &guest, unsigned address) {
    std::array<std::uint32_t, N> result;
    require(guest.read(address, result.data(), sizeof(result)), "missing original-runtime words");
    return result;
}
template <class Values> void array(std::ostream &out, const Values &values) {
    out << '[';
    bool comma = false;
    for (const auto &value : values) {
        out << (comma ? "," : "") << value;
        comma = true;
    }
    out << ']';
}
}
int main(int argc, char **argv) {
    using namespace sadx;
    try {
        require(argc == 5, "expected pinned game, Windows system, private overlay, JSON output");
        const std::filesystem::path directory(argv[1]), overlay(argv[3]);
        require(!std::filesystem::exists(overlay), "original witness requires a fresh private overlay");
        std::ifstream input(directory / "sonic.exe", std::ios::binary);
        require(bool(input), "missing pinned reference executable");
        std::string error;
        const auto image = ExecutableImage::parse({std::istreambuf_iterator<char>(input), {}}, &error);
        require(bool(image), error);
        auto process = HleProcess::create(directory.string(), overlay.string(), &error);
        require(bool(process), error);
        process->use_real_cpp_runtime(argv[2]);
        process->hle().set_logging(false);
        require(process->map_main(*image, &error), error);
        FpuShimStats shims;
        require(install_fpu_shims(process->guest(), load_fpu_sites("reference/inventory/fpu-sites.json"),
                                  shims, &error), error);
        std::ofstream output(argv[4]);
        require(bool(output), "cannot create private witness output");
        unsigned frames = 0, producers = 0, gamma_entries = 0;
        bool started = false, returned = false, finished = false, comma = false;
        std::uint32_t destination = 0, getter_index = 0;
        std::array<std::uint32_t, 16> destination_before{}, destination_after{};
        std::uint32_t getter_result = 0;
        std::vector<std::uint32_t> getter_indices;
        output << "{\"snapshots\":[";
        const auto snapshot = [&](const char *phase, X86Guest &guest, bool producer = false) {
            output << (comma ? "," : "") << "{\"phase\":\"" << phase << "\",\"frame\":" << frames
                   << ",\"producer_calls\":" << producers << ",\"gamma_entries\":" << gamma_entries
                   << ",\"count\":" << word(guest, 0x3c58270) << ",\"limit\":" << word(guest, 0x3c5a278)
                   << ",\"control_word\":" << guest.reg(UC_X86_REG_FPCW)
                   << ",\"stack_top\":" << guest.u32(guest.esp()).value_or(0);
            if (producer) {
                output << ",\"argument_words\":";
                array(output, words<5>(guest, guest.esp() + 4));
                const auto action = word(guest, guest.esp() + 8);
                output << ",\"action_words\":";
                array(output, words<2>(guest, action));
                output << ",\"motion_words\":";
                array(output, words<3>(guest, word(guest, action + 4)));
            }
            output << ",\"buffer\":";
            array(output, words<2048>(guest, 0x3c58278));
            output << ",\"gamma_destinations\":[";
            bool separated = false;
            for (unsigned address : {0x3c53ad8, 0x3c53c48, 0x3c53b78, 0x3c53b20, 0x3c53c00}) {
                if (separated) output << ',';
                array(output, words<16>(guest, address));
                separated = true;
            }
            output << "],\"weld_scratch\":[";
            separated = false;
            for (unsigned address : {0x3b42f38, 0x3b42ef8, 0x3b42eb8, 0x3b42e78}) {
                if (separated) output << ',';
                array(output, words<16>(guest, address));
                separated = true;
            }
            output << "],\"stack_depth\":" << word(guest, 0x3d0fa2c)
                   << ",\"current_matrix_pointer\":" << word(guest, 0x3d0f818) << '}';
            comma = true;
            output.flush();
        };
        snapshot("mapped", process->guest());
        require(process->guest().add_instruction_hook(0x4b81f0, [&](X86Guest &guest, std::uint32_t) {
            ++producers;
            snapshot("producer_entry", guest, true);
            return false;
        }, &error), error);
        require(process->guest().add_instruction_hook(0x4b82b8, [&](X86Guest &guest, std::uint32_t) {
            snapshot("producer_return", guest);
            return false;
        }, &error), error);
        require(process->guest().add_instruction_hook(0x483430, [&](X86Guest &guest, std::uint32_t) {
            ++gamma_entries;
            if (gamma_entries == 1) snapshot("first_gamma_entry", guest);
            return false;
        }, &error), error);
        require(process->guest().add_instruction_hook(0x4b82d0, [&](X86Guest &guest, std::uint32_t) {
            const auto caller = word(guest, guest.esp());
            if (gamma_entries == 0 || caller < 0x483430 || caller >= 0x4843e0)
                return false;
            getter_indices.push_back(word(guest, guest.esp() + 4));
            if (started) return false;
            started = true;
            destination = word(guest, guest.esp() + 8);
            getter_index = word(guest, guest.esp() + 4);
            destination_before = words<16>(guest, destination);
            snapshot("first_gamma_get_entry", guest);
            return false;
        }, &error), error);
        for (unsigned address : {0x4b82de, 0x4b82f6}) {
            require(process->guest().add_instruction_hook(address, [&](X86Guest &guest, std::uint32_t) {
                if (!started || returned) return false;
                snapshot("first_gamma_get_return", guest);
                getter_result = guest.reg(UC_X86_REG_EAX);
                destination_after = words<16>(guest, destination);
                returned = true;
                return false;
            }, &error), error);
        }
        std::uint32_t fan_player = 0, fan_extra = 0;
        require(process->guest().add_instruction_hook(0x4c2e90, [&](X86Guest &guest, std::uint32_t) {
            if (!returned || finished || fan_extra) return false;
            snapshot("first_gamma_fan_entry", guest);
            fan_player = word(guest, guest.esp() + 8);
            fan_extra = word(guest, guest.esp() + 12);
            output << ",{\"phase\":\"fan_input\",\"player\":" << fan_player
                   << ",\"extra\":" << fan_extra << ",\"current_player\":" << word(guest, 0x3abdf64)
                   << ",\"primary\":" << word(guest, guest.esp() + 4) << ",\"speed\":";
            array(output, words<3>(guest, fan_player + 56));
            output << ",\"extra_words\":";
            array(output, words<145>(guest, fan_extra));
            output << '}';
            return false;
        }, &error), error);
        for (unsigned address : {0x4c2f2f, 0x4c2f41}) {
            require(process->guest().add_instruction_hook(address, [&](X86Guest &guest, std::uint32_t) {
                if (!fan_extra || finished) return false;
                snapshot("first_gamma_fan_return", guest);
                output << ",{\"phase\":\"fan_output\",\"speed\":";
                array(output, words<3>(guest, fan_player + 56));
                output << ",\"extra_words\":";
                array(output, words<145>(guest, fan_extra));
                output << '}';
                finished = true;
                guest.stop("first Gamma fan return observed");
                return false;
            }, &error), error);
        }
        process->on_present = [&](X86Guest &guest) {
            ++frames;
            for (unsigned press : {500, 750, 950, 1050, 1300, 1500, 1700, 1900}) {
                if (frames == press) process->host_key(13, true);
                if (frames == press + 1) process->host_key(13, false);
            }
            if (frames >= 2100) guest.stop("initialization witness frame limit");
        };
        const auto result = process->guest().call(0x643f96, {}, std::nullopt, std::nullopt, 0x27f, 0);
        output << ']';
        if (returned) {
            output << ",\"getter_result\":" << getter_result << ",\"getter_index\":" << getter_index
                   << ",\"destination_pointer\":" << destination << ",\"destination_before\":";
            array(output, destination_before);
            output << ",\"destination_after\":";
            array(output, destination_after);
            output << ",\"getter_indices\":";
            array(output, getter_indices);
        }
        output << ",\"finished\":" << (finished ? "true" : "false") << ",\"frames\":" << frames << "}\n";
        std::cerr << "original witness: frames=" << frames << " producers=" << producers
                  << " gamma=" << gamma_entries << " stop=" << result.error << '\n';
        return finished ? 0 : 1;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
