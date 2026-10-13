#pragma once
#include "guest/x86_guest.hpp"
#include "loader/executable_image.hpp"
#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>
namespace sadx {
// The override table (ADR-0001, M1 4d): native owners bound to guest addresses. Each entry's bridge loads the
// owner's state from guest memory (the `*_at_<hex>` globals and the stack arguments), runs the native port with
// the guest's x87 control word, writes the owner's globals back and returns the callee's pop count. A port
// that cannot proceed (a failure result) stops the guest with the port's boundary instead of returning.
// `writes` are the guest ranges the original may store to: override-equivalence compares exactly those between
// the original and the override (these functions return nothing: EAX is leftover, e.g. an FNSTSW result).
struct OverrideEntry {
    std::uint32_t address=0;std::string name;
    std::vector<std::pair<std::uint32_t,std::uint32_t>> writes; // (address, bytes)
    X86Guest::Override bridge;
};
// The frame-loop owners ported so far whose state is guest globals: 7899a0 (frame multiplier), 7815e0
// (projection range clamp), 7815c0 (sprite projection scale), 781570 (camera block). `image` supplies the
// .rdata/.data constants the ports take as inputs (the 88af78 sine table, the 7dcb0c/7dce50 floats).
std::vector<OverrideEntry> frame_loop_overrides(const ExecutableImage& image);
bool install_overrides(X86Guest&,const std::vector<OverrideEntry>&,std::string* error);
} // namespace sadx
