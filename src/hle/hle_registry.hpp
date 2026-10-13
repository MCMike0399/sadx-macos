#pragma once
#include "guest/x86_guest.hpp"
#include "loader/executable_image.hpp"
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>
namespace sadx {
// High-level emulation of the Win32/D3D8/DirectInput/DirectSound imports the guest calls (ADR-0001).
// Every import of a guest image is bound to its own trap address in a reserved executable page: the
// IAT slot the Windows loader would fill with the DLL's address holds the trap instead. When the
// guest calls a trap, the registered native function runs (and its call is logged); an import with
// no registered function stops the guest with an error naming it. Nothing is silently skipped.
struct HleCallRecord {
    std::string dll,symbol;              // as the import table names them ("#11" for an ordinal)
    std::uint32_t return_address=0;      // the guest instruction after the call
    std::vector<std::uint32_t> arguments;// the logged stack words
    std::uint32_t result=0;              // EAX after the native function
};
class HleCall {
public:
    HleCall(X86Guest& g,std::uint32_t esp):guest(g),esp_(esp){}
    X86Guest& guest;
    // Stack argument i (0 = the first, at [ESP+4] on entry); nullopt when unreadable.
    std::optional<std::uint32_t> argument(unsigned i)const{return guest.u32(esp_+4+4*i);}
private:
    std::uint32_t esp_;
};
struct HleFunction {
    std::uint32_t argument_bytes=0;   // bytes the callee pops (stdcall); 0 for cdecl
    unsigned logged_arguments=0;      // stack words recorded in the call log
    std::function<std::uint32_t(HleCall&)> body; // returns EAX
};
class HleRegistry {
public:
    // DLL names compare case-insensitively (the loader does); symbols are exact, ordinals "#n".
    void add(const std::string& dll,const std::string& symbol,HleFunction);
    const HleFunction* find(const std::string& dll,const std::string& symbol)const;
    // A data export (e.g. msvcrt!_iob): the IAT slot gets the address `provide` returns on first use.
    void add_data(const std::string& dll,const std::string& symbol,std::function<std::optional<std::uint32_t>()> provide);
    std::optional<std::uint32_t> data(const std::string& dll,const std::string& symbol);
    // The same body under another DLL's name (msvcrt.dll and the UCRT API sets share implementations).
    bool alias(const std::string& dll,const std::string& symbol,const std::string& from_dll,const std::string& from_symbol);
    // A system function the modelled Windows version does not have: GetProcAddress returns NULL for it.
    void add_absent(const std::string& dll,const std::string& symbol);
    bool absent(const std::string& dll,const std::string& symbol)const;
    const std::vector<HleCallRecord>& log()const noexcept{return log_;}
    void record(HleCallRecord r){if(logging_)log_.push_back(std::move(r));++calls_;}
    void set_logging(bool on){logging_=on;}bool logging()const noexcept{return logging_;}
    std::uint64_t calls()const noexcept{return calls_;} // every HLE call, logged or not
    void clear_log(){log_.clear();}
    // The trap for an HLE symbol: one per dll!symbol across every image (so GetProcAddress of a system
    // function returns the same address the IATs hold), allocated in the trap region and installed in
    // the guest on first use. nullopt when the region cannot be mapped.
    std::optional<std::uint32_t> trap(X86Guest&,const std::string& dll,const std::string& symbol,std::string* error=nullptr);
private:
    std::map<std::string,HleFunction> functions_;std::set<std::string> absent_;
    std::map<std::string,std::function<std::optional<std::uint32_t>()>> data_providers_;std::map<std::string,std::uint32_t> data_;
    std::vector<HleCallRecord> log_;
    bool logging_=true;std::uint64_t calls_=0;
    std::map<std::string,std::uint32_t> traps_;std::uint32_t next_trap_=0,mapped_trap_end_=0;
};
struct ImportBinding {std::string dll,symbol;std::uint32_t iat_va=0,trap=0;bool bound=false;};
struct ImportBindingResult {
    std::vector<ImportBinding> bindings;std::size_t bound=0,unbound=0;
    std::string error;                // non-empty: the image's imports could not be bound at all
    explicit operator bool()const noexcept{return error.empty();}
};
// Traps live from hle_trap_base, 16 bytes each, in HLT-filled pages (a trap is never executed). For each
// import, `resolve` may name a real guest address (a DLL loaded in the guest); otherwise the IAT slot gets
// the registry's trap. Registry functions are looked up when the trap fires, so later additions still bind.
constexpr std::uint32_t hle_trap_base=0xfe000000U;
using ImportResolver=std::function<std::optional<std::uint32_t>(const std::string& dll,const ExecutableImage::ImportedSymbol&,std::string* error)>;
ImportBindingResult bind_imports(X86Guest&,const ExecutableImage&,HleRegistry&,const ImportResolver& resolve={});
} // namespace sadx
