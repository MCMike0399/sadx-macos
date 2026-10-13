#pragma once
#include "hle/hle_memory.hpp"
#include "hle/hle_registry.hpp"
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
namespace sadx {
// The guest's module table and the Windows loader behaviour the game and its mods rely on: the main
// executable at its base, DLLs from the game directory mapped (rebased when their preferred base is
// taken), linked to each other through their import tables, and every other import bound to an HLE
// trap. LoadLibrary runs DLL entry points (dependencies first) by redirecting the guest; DllMain
// returning FALSE makes LoadLibrary return NULL, as on Windows.
//
// Paths: the guest sees the install as `windows_dir` (default C:\SADX — the reference install path
// was not captured); a Windows path under it, or relative to it (the process's current directory),
// resolves case-insensitively to a file under the host game directory. A DLL name that resolves to no
// file there is a system DLL: HLE only (its handle is a pseudo-base with no image).
struct GuestModule {
    std::string name;                 // file name as resolved ("CHRMODELS.dll")
    std::string windows_path;         // C:\SADX\system\CHRMODELS.dll
    std::filesystem::path host_path;
    std::uint32_t base=0,size=0,entry=0;
    std::shared_ptr<ExecutableImage> image; // as mapped (rebased); null for system pseudo-modules
    unsigned references=0;bool entry_called=false,system=false;
};
class HleModules {
public:
    HleModules(X86Guest&,HleRegistry&,HleMemory&,std::filesystem::path game_dir,std::string windows_dir="C:\\SADX");
    // Maps the main executable at its base and links its imports (no entry point is run).
    bool map_main(const ExecutableImage&,const std::string& file_name,std::string* error);
    // LoadLibrary without the entry point: maps and links `requested` and its game-directory
    // dependencies. Returns the base; `pending` receives modules whose entry point has not run, in
    // initialization order. nullopt + error when the file is missing or malformed.
    std::optional<std::uint32_t> map_library(const std::string& requested,std::vector<GuestModule*>* pending,std::string* error);
    GuestModule* by_base(std::uint32_t base);
    GuestModule* by_name(const std::string& name); // file name with or without extension, case-insensitive
    std::optional<std::uint32_t> export_address(std::uint32_t base,const std::string& name);
    std::optional<std::filesystem::path> host_path(const std::string& windows_path)const;
    std::string windows_dir()const{return windows_dir_;}
    // System DLLs that run as real guest code instead of HLE (the MSVC C++ runtime): `directory` holds
    // them; a system DLL named in `names` (case-insensitive) is mapped from there as C:\Windows\SysWOW64\<name>.
    void set_real_system_dlls(std::filesystem::path directory,std::vector<std::string> names){system_dir_=std::move(directory);real_system_=std::move(names);}
    const std::vector<std::unique_ptr<GuestModule>>& modules()const noexcept{return modules_;}
    // KERNEL32 module functions: LoadLibraryA/W, LoadLibraryExA/W (flags 0), GetProcAddress,
    // GetModuleHandleA/W, GetModuleFileNameA/W, FreeLibrary (reference count only; images stay mapped),
    // DisableThreadLibraryCalls.
    void register_functions();
    std::function<void(const std::string&,const std::string&,const std::string&)> trace_; // set by HleProcess
    // Runs `pending` entry points (DllMain(base, DLL_PROCESS_ATTACH, 0)) from inside an HLE function via
    // guest redirection, then `done(all_succeeded)`.
    void run_entries(X86Guest&,std::vector<GuestModule*> pending,std::uint32_t pops,std::function<void(X86Guest&,bool)> done);
private:
    X86Guest& guest_;HleRegistry& hle_;HleMemory& memory_;std::filesystem::path game_dir_;std::string windows_dir_;
    std::filesystem::path system_dir_;std::vector<std::string> real_system_;
    std::vector<std::unique_ptr<GuestModule>> modules_;std::uint32_t next_base_=0x20000000U,next_system_=0x7c000000U;
    std::optional<std::uint32_t> link(GuestModule&,std::vector<GuestModule*>* pending,std::string* error);
    std::optional<std::uint32_t> map_file(const std::filesystem::path& host,const std::string& windows_path,std::vector<GuestModule*>* pending,std::string* error);
    std::optional<std::uint32_t> choose_base(std::uint32_t preferred,std::uint32_t size)const;
};
// Guest string helpers (narrow and UTF-16), bounded.
std::optional<std::string> guest_string(const X86Guest&,std::uint32_t address,std::size_t limit=4096);
std::optional<std::string> guest_wide_string(const X86Guest&,std::uint32_t address,std::size_t limit=4096); // ASCII subset; nullopt otherwise
} // namespace sadx
