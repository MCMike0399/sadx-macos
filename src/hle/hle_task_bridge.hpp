#pragma once
#include "guest/override_table.hpp"
namespace sadx {
class HleProcess;
// Task-work arena owners share persistent native allocation identities per process.
std::vector<OverrideEntry> task_overrides(HleProcess&);
}
