#pragma once
#include "guest/override_table.hpp"
namespace sadx {
class HleProcess;
// Device-backed overrides. The process/guest must outlive the entries.
// 7912e0 refreshes viewport/projection with original COM stack depths and live callback state.
// Calls only verified HLE COM targets; a guest/proxy vtable is an explicit boundary.
std::vector<OverrideEntry> device_overrides(HleProcess&);
std::vector<OverrideEntry> process_overrides(HleProcess&,const ExecutableImage&);
}
