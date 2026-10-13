#pragma once
#include "hle/hle_modules.hpp"
#include "hle/hle_user32.hpp"
#include "runtime/game_runtime.hpp"
#include <cstdio>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
namespace sadx {
// One guest process: the CPU, the HLE registry, guest-address memory, the module table and the
// process-wide state the HLE functions share (last error, virtual clock, handles, CRT data). Every fact
// the HLE has to choose because the reference machine's value was not captured is listed once in
// `assumptions` (the guest-boot report shows them), never silently defaulted.
struct HleFile {std::FILE* host=nullptr;std::string windows_path;bool text=false,readable=false,writable=false,eof=false,error=false;std::vector<int> pushback;};
class HleProcess {
public:
    // `overlay_dir` receives every file the guest writes (and is read before the install), so the
    // install under `game_dir` is never modified.
    static std::unique_ptr<HleProcess> create(std::filesystem::path game_dir,std::filesystem::path overlay_dir,std::string* error);
    // The MSVC C++ runtime (msvcp140.dll, vcruntime140.dll) runs as real guest code from `directory`
    // (reference/windows-system/, see its PROVENANCE.md).
    void use_real_cpp_runtime(std::filesystem::path directory);
    X86Guest& guest(){return *guest_;}
    HleRegistry& hle(){return hle_;}
    HleMemory& memory(){return *memory_;}
    HleModules& modules(){return *modules_;}
    // Maps sonic.exe (the main module) and links its imports.
    bool map_main(const ExecutableImage&,std::string* error);
    std::vector<std::string> assumptions;
    // File-system requests in call order (function, the guest's path, the outcome), for traces and tests.
    std::vector<std::string> file_trace;
    std::vector<std::string> unmodelled_lookups; // GetProcAddress results that are traps without a body
    void trace_file(const std::string& function,const std::string& path,const std::string& outcome){file_trace.push_back(function+"("+path+") -> "+outcome);}
    void assume(const std::string& a){if(assumed_.insert(a).second)assumptions.push_back(a);}
    // Virtual time: 100 ns units since the virtual boot. QueryPerformanceCounter is 10 MHz on the same
    // count; each query advances it by 1 µs and Sleep(ms) by ms, so runs are deterministic.
    std::uint64_t now_100ns=0;
    std::uint32_t last_error=0;
    // Kernel handles (files, mappings, find handles) are small tokens from 0x100, step 4.
    std::uint32_t new_handle(){const auto h=next_handle_;next_handle_+=4;return h;}
    std::map<std::uint32_t,HleFile> files;          // CRT FILE* (guest address) -> host file
    std::map<std::uint32_t,std::vector<std::filesystem::path>> finds; // FindFirstFile handles -> remaining entries
    struct Win32File {std::filesystem::path host;std::FILE* file=nullptr;bool writable=false;};
    std::map<std::uint32_t,Win32File> file_handles;                   // CreateFile handles
    // CRT state in guest memory.
    std::uint32_t teb=0,peb=0;
    HleWindows windows;
    // Direct3D 8: the recorded device state (the native runtime's owner type), the D3DPRESENT_PARAMETERS words
    // of CreateDevice, and a hook run on every IDirect3DDevice8::Present (tests stop the guest there).
    D3D8StateOwner d3d8;std::vector<std::uint32_t> d3d8_presentation;std::function<void(X86Guest&)> on_present;
    std::map<std::string,std::string> environment_variables; // names compared case-insensitively (stored upper-case)
    std::map<std::uint32_t,std::uint32_t> tls;std::uint32_t next_tls=0;std::set<std::uint32_t> heaps;std::uint32_t process_heap=0;std::uint32_t command_line_a_env=0;
    std::uint32_t errno_address=0,iob_address=0,narrow_argv=0,environment=0,command_line_w=0,command_line_a=0;
    std::map<std::uint32_t,std::vector<std::uint32_t>> onexit_tables; // table address -> registered functions
    // Guest string helpers.
    std::optional<std::string> string_at(std::uint32_t a,std::size_t limit=4096)const{return guest_string(*guest_,a,limit);}
    std::optional<std::string> wide_at(std::uint32_t a)const{return guest_wide_string(*guest_,a);}
    bool put_string(std::uint32_t at,const std::string& s);         // with NUL
    bool put_wide(std::uint32_t at,const std::string& ascii);       // UTF-16 with NUL
    std::optional<std::uint32_t> allocate_string(const std::string& s);
    std::optional<std::filesystem::path> host_path(const std::string& windows_path)const; // overlay first, then the install
    std::optional<std::filesystem::path> write_path(const std::string& windows_path);    // always in the overlay (directories created)
    std::filesystem::path overlay_dir;
private:
    HleProcess()=default;
    std::unique_ptr<X86Guest> guest_;HleRegistry hle_;std::unique_ptr<HleMemory> memory_;std::unique_ptr<HleModules> modules_;
    std::set<std::string> assumed_;std::uint32_t next_handle_=0x100;
};
void register_hle_kernel32(HleProcess&);
void register_hle_crt(HleProcess&);
void register_hle_dbghelp(HleProcess&);
void register_hle_user32(HleProcess&);
void register_hle_dinput(HleProcess&);
void register_hle_d3d8(HleProcess&);
} // namespace sadx
