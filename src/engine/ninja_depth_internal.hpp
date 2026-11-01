#pragma once

#include <cstdint>

namespace sadx::render_detail {

// NOTSA: shared njSetZCompare decision, with lazy adapter-owned reads.
// The source acquires its device before consuming the comparison table.
template <class AcquireDevice, class ReadComparison, class Issue>
bool apply_depth_comparison(std::uint32_t mode, AcquireDevice acquire_device,
                            ReadComparison read_comparison, Issue issue) {
    constexpr std::uint32_t kDepthComparison = 23;
    auto device = acquire_device();
    const auto comparison = read_comparison(mode & 7U);
    return issue(device, kDepthComparison, comparison);
}

// NOTSA: shared njSetZUpdateMode command; preserve the original full-width value.
template <class Issue> bool apply_depth_write(std::uint32_t enabled, Issue issue) {
    constexpr std::uint32_t kDepthWrite = 14;
    return issue(kDepthWrite, enabled);
}

} // namespace sadx::render_detail
