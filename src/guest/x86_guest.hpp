#pragma once
#include <optional>
#include "loader/executable_image.hpp"
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <map>
#include <memory>
#include <string>
#include <vector>
namespace sadx {
// The in-process x86 guest (ADR-0001): the original sonic.exe (and DLL) code runs on a
// software CPU inside the native process. Everything the guest touches is a guest address
// (std::uint32_t); native owners are reached only through overrides and import traps.
//
// This is the M1 foundation: image mapping, calls with a stop return address, code hooks
// that let a native owner replace a guest function (an "override"), and reads/writes for
// tests. Imports, threads, SEH and the frame loop come in later M1 steps.
struct GuestCallResult {
    bool completed=false;          // emulation reached the stop address
    std::uint32_t eax=0,eip=0,esp=0;
    std::string error;             // Unicorn error text when not completed
    explicit operator bool()const noexcept{return completed;}
};
class X86Guest {
public:
    // An override runs instead of the guest function at `address`. It must leave the
    // guest as the original would (results in registers/memory) and return the number
    // of stack bytes the callee pops (`ret N`); the guest then returns to the caller.
    using Override=std::function<std::uint32_t(X86Guest&)>;
    static std::unique_ptr<X86Guest> create(std::string* error);
    ~X86Guest();
    X86Guest(const X86Guest&)=delete;X86Guest& operator=(const X86Guest&)=delete;
    // Maps every section of a PE image at its preferred base with the file bytes and
    // zeroed virtual tail (.bss). Page 0 stays unmapped so null dereferences fault.
    bool map_image(const ExecutableImage&,std::string* error);
    bool map(std::uint32_t base,std::uint32_t size,bool executable,std::string* error);
    bool write(std::uint32_t address,const void* bytes,std::size_t size);
    bool read(std::uint32_t address,void* bytes,std::size_t size)const;
    std::optional<std::uint32_t> u32(std::uint32_t address)const;
    bool set_u32(std::uint32_t address,std::uint32_t value);
    std::uint32_t reg(int unicorn_register)const;
    void set_reg(int unicorn_register,std::uint32_t value);
    // Calls `address` with the stack arguments pushed right-to-left (cdecl/stdcall), ECX/EDX
    // set when given (fastcall/thiscall), a stop return address, and a fresh stack. x87 is
    // reset to the control word given (the game's main thread runs with 0x27f after the CRT).
    GuestCallResult call(std::uint32_t address,std::initializer_list<std::uint32_t> stack_arguments,
        std::optional<std::uint32_t> ecx=std::nullopt,std::optional<std::uint32_t> edx=std::nullopt,
        std::uint16_t x87_control_word=0x27f,std::uint64_t instruction_limit=50'000'000);
    // Runs guest code from `from` until EIP reaches `until` (a span inside a function), on a fresh
    // stack whose top holds the stop address. Registers other than ESP/EBP keep their values.
    GuestCallResult run(std::uint32_t from,std::uint32_t until,std::uint16_t x87_control_word=0x27f,std::uint64_t instruction_limit=50'000'000);
    bool add_override(std::uint32_t address,Override,std::string* error);
    // Ends the current call() as a failure with `error` (an override or import trap that cannot
    // proceed, e.g. an unbound import). The guest state is left as it was at the stop.
    void stop(std::string error);
    std::uint32_t esp()const;std::uint32_t eip()const;
    void set_eax(std::uint32_t);void set_edx(std::uint32_t);
    std::uint32_t stack_base()const noexcept{return stack_base_;}
    std::uint32_t stop_address()const noexcept{return stop_address_;}
private:
    X86Guest()=default;
    static void code_hook(void*,std::uint64_t address,std::uint32_t size,void* user);
    void* uc_=nullptr;
    std::uint32_t stack_base_=0x7ff00000,stack_size_=0x100000,stop_address_=0xfeed0000;
    std::map<std::uint32_t,Override> overrides_;
    std::vector<std::uint32_t> handles_;
    std::optional<std::string> stopped_;
};
} // namespace sadx
