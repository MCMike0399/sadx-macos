#include "guest/polybuff_owners.hpp"
#include "engine/ninja_device_init.hpp"
#include "engine/ninja_system_init.hpp"

namespace sadx::polybuff_reference {
Word& frame_vertices(FrameFacts& facts) { return facts.vertices; }
std::array<Record,9>& frame_records(FrameFacts& facts) { return facts.records; }
}
namespace sadx {
NinjaFrameCallState::NinjaFrameCallState()
    : polygon_facts(std::make_unique<polybuff_reference::FrameFacts>(polygon_buffers)) {}
NinjaFrameCallState::~NinjaFrameCallState() = default;
NinjaDeviceState::NinjaDeviceState()
    : polygon_scratch_facts(std::make_unique<SourceBytes<0x3d0fae0, 0x200>>(polygon_scratch)) {}
NinjaDeviceState::~NinjaDeviceState() = default;
NinjaSystemInitState::NinjaSystemInitState()
    : polygon_base_fact(std::make_unique<polybuff_reference::Word>(polygon_base.value)) {}
NinjaSystemInitState::~NinjaSystemInitState() = default;
const std::array<std::optional<std::uint32_t>,27>& NinjaSystemInitState::polygon_descriptions() const {
    return tables_at_389d888;
}
const std::array<std::optional<std::uint32_t>,9>& NinjaSystemInitBorrowed::polygon_counts() const {
    return sizes_at_389d864;
}
PolygonScratchBorrowed& NinjaSystemInitBorrowed::polygon_scratch() const {
    return block_at_3d0fae0;
}
} // namespace sadx
