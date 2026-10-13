#pragma once
#include "hle/hle_process.hpp"
#include <string>
#include <vector>
namespace sadx {
// COM objects the guest holds: a guest block whose first word points at a vtable of HLE traps, one per
// method, registered as "com:<Interface>!<Method>" (stdcall, `this` first). The object's other words are
// free for the implementation (reference count at +4 by convention here). Methods run as HleFunctions.
struct HleComMethod {std::string name;std::uint32_t argument_bytes;std::function<std::uint32_t(HleCall&)> body;}; // argument_bytes includes `this`
class HleComInterface {
public:
    HleComInterface(HleProcess&,std::string interface_name,std::vector<HleComMethod> methods);
    // A new object of `size` bytes (>= 8) with this interface's vtable; nullopt when memory or traps fail.
    std::optional<std::uint32_t> create(std::uint32_t size=8);
    std::uint32_t vtable()const noexcept{return vtable_;}
private:
    HleProcess& p_;std::string name_;std::uint32_t vtable_=0;
};
// IUnknown semantics on the +4 reference count (QueryInterface stops unless the implementation overrides it).
HleComMethod com_add_ref();HleComMethod com_release();HleComMethod com_query_interface_stop(const std::string& interface_name);
std::string guid_text(const X86Guest&,std::uint32_t at);
} // namespace sadx
