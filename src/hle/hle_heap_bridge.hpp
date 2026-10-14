#pragma once
#include "guest/override_table.hpp"
namespace sadx {
class HleProcess;
// Heap owners use the process's existing HLE allocator, preserving addresses and handles.
// The process must outlive the entries. Non-HLE IAT targets stop explicitly.
std::vector<OverrideEntry> heap_overrides(HleProcess&);
}
