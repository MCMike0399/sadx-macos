#pragma once
#include "guest/override_table.hpp"
#include <array>
#include <optional>
namespace sadx {
class HleProcess;
// Heap owners use the process's existing HLE allocator, preserving addresses and handles.
// The process must outlive the entries. Non-HLE IAT targets stop explicitly.
std::vector<OverrideEntry> heap_overrides(HleProcess&);
// Verified synchronous Win32 heap call, shared by owners with different flags/sites.
std::optional<std::uint32_t> invoke_heap_method(HleProcess&,const char* symbol,
    std::uint32_t iat,std::uint32_t source_call,const std::array<std::uint32_t,3>& arguments);
}
