// D3D8's calling-thread precision contract through actual guest COM dispatch.
#include "hle/hle_process.hpp"
#include <unicorn/unicorn.h>
#include <array>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool ok, const std::string& why) {
    if (!ok) throw std::runtime_error(why);
}
std::uint32_t method(sadx::X86Guest& guest, std::uint32_t object, unsigned slot) {
    const auto table = guest.u32(object);
    require(table.has_value(), "COM table");
    const auto target = guest.u32(*table + 4 * slot);
    require(target.has_value(), "COM target");
    return *target;
}
}
int main() {
    try {
        std::string error;
        auto process = sadx::HleProcess::create(".", "build/guest-d3d8-fpu-overlay", &error);
        require(bool(process), error);
        auto& guest = process->guest();
        const auto trap = process->hle().trap(guest, "d3d8.dll", "Direct3DCreate8", &error);
        require(trap.has_value(), error);
        const auto d3d = guest.call(*trap, {0xdc});
        require(bool(d3d) && d3d.eax, "Direct3DCreate8");
        const auto parameters = process->memory().allocate(52);
        const auto output = process->memory().allocate(4);
        require(parameters && output, "presentation memory");
        const std::uint32_t pp[13] = {1, 1, 22, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0};
        require(guest.write(*parameters, pp, sizeof pp), "presentation bytes");
        const auto create = method(guest, d3d.eax, 15);
        // Tail-jump through a guest wrapper. A hook on an HLE trap itself runs
        // after the process-wide dispatch hook, so it cannot seed the callee.
        constexpr std::uint32_t wrapper = 0x10000000;
        require(guest.map(wrapper, 4096, true, &error), error);
        std::uint32_t seeded_control = 0;
        require(guest.add_instruction_hook(wrapper, [&](sadx::X86Guest& g, std::uint32_t) {
            g.set_reg(UC_X86_REG_FPCW, seeded_control);
            g.set_reg(UC_X86_REG_FPTAG, 0xfffc);
            g.set_st(0, 0x923456789abcdef0ULL, 0x3fff);
            g.set_fpu_status(0x4020);
            g.set_reg(UC_X86_REG_MXCSR, 0x5fa1);
            return false;
        }, &error), error);
        const auto call = [&](std::uint32_t target, std::initializer_list<std::uint32_t> args) {
            const std::uint8_t jump[] = {0xb8, static_cast<std::uint8_t>(target),
                static_cast<std::uint8_t>(target >> 8), static_cast<std::uint8_t>(target >> 16),
                static_cast<std::uint8_t>(target >> 24), 0xff, 0xe0};
            require(guest.write(wrapper, jump, sizeof jump), "wrapper code");
            return guest.call(wrapper, args);
        };
        std::array<std::uint32_t, 2> devices{};
        unsigned cases = 0;
        for (unsigned preserve = 0; preserve < 2; ++preserve) {
            for (const auto& controls : std::array<std::array<std::uint32_t, 2>, 5>{{
                     {0x027f, 0x007f}, {0x023f, 0x007f}, {0x037f, 0x007f},
                     {0x0b7f, 0x007f}, {0x0f55, 0x0055}}}) {
                const auto cw = controls[0];
                const auto expected = preserve ? cw : controls[1];
                const auto check = [&] {
                    require(guest.reg(UC_X86_REG_FPCW) == expected, "x87 precision/rounding/masks");
                    require(guest.fpu_status() == 0x4020, "sticky status unchanged");
                    require(guest.reg(UC_X86_REG_FPTAG) == 0xfffc, "x87 tags unchanged");
                    require(guest.mxcsr() == 0x5fa1, "MXCSR unchanged");
                    require(guest.st(0) == std::pair<std::uint64_t, std::uint16_t>{0x923456789abcdef0ULL, 0x3fff},
                            "x87 stack value unchanged");
                };
                seeded_control = cw;
                const auto result = call(create, {d3d.eax, 0, 1, 0, 0x20U | (preserve ? 2U : 0U),
                                                       *parameters, *output});
                require(bool(result) && result.eax == 0, "CreateDevice succeeds");
                check();
                devices[preserve] = guest.u32(*output).value_or(0);
                require(devices[preserve] != 0, "device identity");
                ++cases;
            }
        }
        // Alternate the two devices: Reset must use each one's creation flag,
        // not the flag of the most recently created device.
        for (unsigned preserve : {0U, 1U, 0U, 1U}) {
            const auto reset = method(guest, devices[preserve], 14);
            seeded_control = 0x0b7f;
            const auto result = call(reset, {devices[preserve], *parameters});
            require(!result && result.error == "IDirect3DDevice8::Reset is not modelled yet",
                    "Reset retains its explicit resource-recreation boundary");
            require(guest.reg(UC_X86_REG_FPCW) == (preserve ? 0x0b7fU : 0x007fU),
                    "Reset honors its own device's preserve flag");
            ++cases;
        }
        unsigned assumptions = 0;
        for (const auto& message : process->assumptions)
            if (message.find("D3DCREATE_FPU_PRESERVE sets") != std::string::npos) ++assumptions;
        require(assumptions == 1, "precision assumption reported once");
        std::cout << "D3D8 FPU: " << cases << " CreateDevice/Reset cases, preserve both ways, "
                     "masks/status/stack/MXCSR retained; Reset resource boundary remains explicit.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
