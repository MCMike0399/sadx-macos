#pragma once
#include <optional>
#include "loader/executable_image.hpp"
#include <array>
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
    bool paused=false;             // pause() was requested; resume() continues
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
    bool unmap(std::uint32_t base,std::uint32_t size,std::string* error);
    bool write(std::uint32_t address,const void* bytes,std::size_t size);
    bool read(std::uint32_t address,void* bytes,std::size_t size)const;
    std::optional<std::uint32_t> u32(std::uint32_t address)const;
    bool set_u32(std::uint32_t address,std::uint32_t value);
    std::uint32_t reg(int unicorn_register)const;
    void set_reg(int unicorn_register,std::uint32_t value);
    // Calls `address` with the stack arguments pushed right-to-left (cdecl/stdcall), ECX/EDX
    // set when given (fastcall/thiscall), a stop return address, and a fresh stack. x87 is
    // reset to the control word given (the game's main thread runs with 0x27f after the CRT).
    // `timeout_us` bounds the host time (Unicorn's instruction-count limit instruments every
    // instruction and is several times slower, so no count is used).
    GuestCallResult call(std::uint32_t address,std::initializer_list<std::uint32_t> stack_arguments,
        std::optional<std::uint32_t> ecx=std::nullopt,std::optional<std::uint32_t> edx=std::nullopt,
        std::uint16_t x87_control_word=0x27f,std::uint64_t timeout_us=600'000'000);
    GuestCallResult call(std::uint32_t address,const std::vector<std::uint32_t>& stack_arguments, // the same, arguments built at run time
        std::optional<std::uint32_t> ecx=std::nullopt,std::optional<std::uint32_t> edx=std::nullopt,
        std::uint16_t x87_control_word=0x27f,std::uint64_t timeout_us=600'000'000,std::uint32_t mxcsr=0x1f80);
    // Runs guest code from `from` until EIP reaches `until` (a span inside a function), on a fresh
    // stack whose top holds the stop address. Registers other than ESP/EBP keep their values.
    GuestCallResult run(std::uint32_t from,std::uint32_t until,std::uint16_t x87_control_word=0x27f,std::uint64_t timeout_us=600'000'000);
    bool add_override(std::uint32_t address,Override,std::string* error);
    // One code hook for a whole region (the HLE trap pages): overrides added inside it need no hook of their own.
    // Unicorn scans every code hook on each hooked instruction, so thousands of per-address hooks are slow.
    bool reserve_hook_region(std::uint32_t begin,std::uint32_t size,std::string* error);
    // From inside an override: instead of returning to the caller now, call the guest function
    // `target` with `arguments` (pushed right-to-left); when it returns (either convention), the
    // stack is put back where the override's caller expects it (its own `pops` bytes removed) and
    // `then` runs. `then` sets the results (EAX…) and may redirect again (a chain of guest calls,
    // e.g. _initterm); when it does not, the guest returns to the override's caller.
    // stack_reserve preserves a source dependency call depth when stack pointers are observable.
    using Continuation=std::function<void(X86Guest&)>;
    void redirect(std::uint32_t target,const std::vector<std::uint32_t>& arguments,std::uint32_t pops,Continuation then,std::uint32_t stack_reserve=0);
    // Protected-mode segments as Windows sets them for a 32-bit thread: a GDT at `gdt` (one page) with
    // flat code/data/stack and an FS segment whose base is the thread's TEB.
    bool set_fs_base(std::uint32_t teb,std::uint32_t gdt,std::string* error);
    // Calls `seen(pc, address, size, value)` for every guest store into [begin, end) (diagnostics, tests). Stores
    // made by HLE functions through write() are not guest instructions and are not reported.
    using WriteWatch=std::function<void(std::uint32_t pc,std::uint32_t address,std::uint32_t size,std::uint64_t value)>;
    bool add_write_watch(std::uint32_t begin,std::uint32_t end,WriteWatch seen,std::string* error);
    // Pauses the current call()/run() after the running override or hook completes normally (the guest's state
    // stays consistent: e.g. an HLE function has returned to its caller); call() returns with `paused` set and
    // resume() continues from there with the same stop address.
    void pause();GuestCallResult resume(std::uint64_t timeout_us=600'000'000);
    // Ends the current call() as a failure with `error` (an override or import trap that cannot
    // proceed, e.g. an unbound import). The guest state is left as it was at the stop.
    void stop(std::string error);
    const std::optional<std::string>& stop_reason()const noexcept{return stopped_;}
    std::uint32_t esp()const;std::uint32_t eip()const;
    void set_eax(std::uint32_t);void set_edx(std::uint32_t);
    // A function returning a float/double leaves it in ST(0): push the ext80 value as FLD would (TOP
    // decremented, the register tagged valid). `significand`/`sign_exponent` are the ext80 fields.
    void return_x87(std::uint64_t significand,std::uint16_t sign_exponent);
    // ST(i) as ext80 fields (significand, sign/exponent) and FSTP: pop ST(0).
    std::pair<std::uint64_t,std::uint16_t> st(unsigned i)const;void pop_x87();
    void set_st(unsigned i,std::uint64_t significand,std::uint16_t sign_exponent); // ST(i) := value (tag valid)
    std::uint16_t fpu_status()const;void set_fpu_status(std::uint16_t);
    // Replaces one instruction: `emulate` runs before the instruction at `address` executes; when it returns
    // true the instruction is skipped (EIP advances by its length), otherwise the guest executes it itself.
    using InstructionHook=std::function<bool(X86Guest&,std::uint32_t address)>;
    bool add_instruction_hook(std::uint32_t address,InstructionHook,std::string* error);

    std::uint64_t xmm0_low()const;void set_xmm0_low(std::uint64_t); // the low 64 bits of XMM0 (double arguments/results)
    std::array<std::uint32_t,4> xmm(unsigned i)const;void set_xmm(unsigned i,const std::array<std::uint32_t,4>&); // XMMi, lane 0 first
    std::uint32_t mxcsr()const;
    std::uint32_t stack_base()const noexcept{return stack_base_;}
    std::uint32_t stop_address()const noexcept{return stop_address_;}
private:
    X86Guest()=default;
    static void code_hook(void*,std::uint64_t address,std::uint32_t size,void* user);
    void* uc_=nullptr;
    std::uint32_t stack_base_=0x7ff00000,stack_size_=0x100000,stop_address_=0xfeed0000;
    std::map<std::uint32_t,Override> overrides_;
    std::vector<std::uint32_t> handles_;
    std::optional<std::string> stopped_;bool pause_requested_=false;
    // Pending redirected calls: continuation trap -> (the stack pointer the caller resumes with, its
    // return address, what to run). Traps live in a page next to the stop address.
    struct Pending {std::uint32_t resume_esp,return_address;Continuation then;};
    std::map<std::uint32_t,Pending> pending_;std::vector<std::uint32_t> free_continuations_;
    bool redirected_=false;std::uint32_t continuation_base_=0xfeee0000;std::uint32_t next_continuation_=0xfeee0000;
    static void continuation_hook(void*,std::uint64_t address,std::uint32_t size,void* user);
    void install_continuations();
    std::vector<std::unique_ptr<WriteWatch>> watches_;
    std::map<std::uint32_t,InstructionHook> instruction_hooks_;
    std::vector<std::pair<std::uint32_t,std::uint32_t>> hook_regions_;

    static void instruction_hook(void*,std::uint64_t address,std::uint32_t size,void* user);
    static void write_hook(void*,int type,std::uint64_t address,int size,std::int64_t value,void* user);
};
} // namespace sadx
