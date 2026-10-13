// KERNEL32 for the guest: process/thread identity, virtual time, memory protection and allocation,
// strings, files under the install directory. Each function follows its documented Win32 contract;
// values the reference machine would have supplied are fixed and listed as assumptions.
#include "hle/hle_process.hpp"
#include "hle/hle_text.hpp"
#include "game/win32_profile.hpp"
#include <fstream>
#include <iterator>
#include <algorithm>
#include <cctype>
#include <cstring>
namespace sadx {
namespace {
constexpr std::uint64_t virtual_epoch_filetime=133801920000000000ULL; // 2025-01-01 00:00:00 UTC
constexpr std::uint32_t process_id=0x1000,thread_id=0x1004,current_process=0xffffffffU;
constexpr std::uint32_t ERROR_FILE_NOT_FOUND=2,ERROR_PATH_NOT_FOUND=3,ERROR_INVALID_HANDLE=6,ERROR_NO_MORE_FILES=18,ERROR_INSUFFICIENT_BUFFER=122,ERROR_ALREADY_EXISTS=183;
std::string lower(std::string s){std::transform(s.begin(),s.end(),s.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});return s;}
// A directory listing of the install root has no "." / ".." in our model only if it is a drive root; the
// install directory itself is not a drive root, so every listing gets them.
bool overlay_relative_root(HleProcess*,const std::filesystem::path&){return false;}
// WIN32_FIND_DATAA (320 bytes) / WIN32_FIND_DATAW (592 bytes).
void write_find_data(HleProcess* P,X86Guest& g,std::uint32_t at,const std::filesystem::path& path,bool wide){
    std::vector<std::uint8_t> d(wide?592:320,0);
    const auto name=path.filename().string();std::error_code ec;
    const bool dir=name=="."||name==".."||std::filesystem::is_directory(path,ec);
    const std::uint32_t attributes=dir?0x10U:0x20U; // DIRECTORY / ARCHIVE
    std::memcpy(d.data(),&attributes,4);const std::uint64_t t=virtual_epoch_filetime;for(int k=0;k<3;++k)std::memcpy(d.data()+4+8*k,&t,8);
    const std::uint64_t size=dir?0:std::filesystem::file_size(path,ec);const std::uint32_t high=static_cast<std::uint32_t>(size>>32),low=static_cast<std::uint32_t>(size);
    std::memcpy(d.data()+28,&high,4);std::memcpy(d.data()+32,&low,4);
    for(std::size_t i=0;i<name.size()&&i<259;++i){if(wide){const std::uint16_t w=static_cast<unsigned char>(name[i]);std::memcpy(d.data()+44+2*i,&w,2);}else d[44+i]=static_cast<std::uint8_t>(name[i]);}
    g.write(at,d.data(),d.size());(void)P;
}
}
void register_hle_kernel32(HleProcess& p){
    auto& r=p.hle();auto* P=&p;
    const auto K=[&](const char* name,std::uint32_t pops,unsigned logged,std::function<std::uint32_t(HleCall&)> body){r.add("KERNEL32.dll",name,{pops,logged,std::move(body)});};
    const auto arg=[](HleCall& c,unsigned i){return c.argument(i).value_or(0);};
    // The modelled Windows is 10 22H2 (build 19045): functions introduced later are absent.
    p.assume("the modelled Windows version is 10 22H2 (build 19045): GetProcAddress returns NULL for later functions such as GetTempPath2W; the reference machine's version was not captured");
    for(const char* name:{"GetTempPath2W","GetTempPath2A"})r.add_absent("KERNEL32.dll",name);
    // --- identity and time
    p.assume("process id 0x1000 and main thread id 0x1004 (the reference ids were not captured)");
    K("GetCurrentProcessId",0,0,[](HleCall&)->std::uint32_t{return process_id;});
    K("GetCurrentThreadId",0,0,[](HleCall&)->std::uint32_t{return thread_id;});
    K("GetCurrentProcess",0,0,[](HleCall&)->std::uint32_t{return current_process;});
    K("GetSystemTimeAsFileTime",4,1,[P,arg](HleCall& c)->std::uint32_t{
        P->assume("wall clock: virtual, starting 2025-01-01 00:00:00 UTC at boot and advancing with the virtual counter");
        const std::uint64_t t=virtual_epoch_filetime+P->now_100ns;c.guest.write(arg(c,0),&t,8);return 0;});
    K("QueryPerformanceFrequency",4,1,[arg](HleCall& c)->std::uint32_t{const std::uint64_t f=10000000;c.guest.write(arg(c,0),&f,8);return 1;});
    auto counter_noted=std::make_shared<bool>(false);
    K("QueryPerformanceCounter",4,1,[P,arg,counter_noted](HleCall& c)->std::uint32_t{
        if(!*counter_noted){*counter_noted=true;P->assume("QueryPerformanceCounter: a virtual 10 MHz counter that advances "+std::to_string(P->counter_step_100ns/10)+" us per query and by Sleep");}
        P->now_100ns+=P->counter_step_100ns;const std::uint64_t t=P->now_100ns;c.guest.write(arg(c,0),&t,8);return 1;});
    K("Sleep",4,1,[P,arg](HleCall& c)->std::uint32_t{P->now_100ns+=std::uint64_t(arg(c,0))*10000;return 0;});
    // WINMM timers on the same virtual clock (timeBeginPeriod changes no virtual resolution).
    r.add("WINMM.dll","timeGetTime",{0,0,[P](HleCall&)->std::uint32_t{return static_cast<std::uint32_t>(P->now_100ns/10000);}});
    r.add("WINMM.dll","timeBeginPeriod",{4,1,[](HleCall&)->std::uint32_t{return 0;}}); // TIMERR_NOERROR
    r.add("WINMM.dll","timeEndPeriod",{4,1,[](HleCall&)->std::uint32_t{return 0;}});
    // --- synchronisation objects for one thread: named mutexes/events exist once; waits never block.
    auto named=std::make_shared<std::map<std::string,std::uint32_t>>();
    const auto create_named=[P,arg,named](bool wide,unsigned name_arg){return [P,arg,named,wide,name_arg](HleCall& c)->std::uint32_t{
        const auto at=arg(c,name_arg);std::optional<std::string> name;if(at)name=wide?P->wide_at(at):P->string_at(at);
        if(name){const auto i=named->find(*name);if(i!=named->end()){P->last_error=ERROR_ALREADY_EXISTS;return i->second;}}
        const auto h=P->new_handle();if(name)(*named)[*name]=h;P->last_error=0;return h;};};
    K("CreateMutexA",12,3,create_named(false,2));K("CreateMutexW",12,3,create_named(true,2));
    K("CreateEventA",16,4,create_named(false,3));K("CreateEventW",16,4,create_named(true,3));
    K("ReleaseMutex",4,1,[](HleCall&)->std::uint32_t{return 1;});
    K("SetEvent",4,1,[](HleCall&)->std::uint32_t{return 1;});K("ResetEvent",4,1,[](HleCall&)->std::uint32_t{return 1;});
    K("WaitForSingleObject",8,2,[](HleCall&)->std::uint32_t{return 0;}); // WAIT_OBJECT_0: nothing else can own an object
    // --- private profiles (.\sonicDX.ini) through the native Win32Profile owner (documented Win32 semantics)
    const auto profile=[P](HleCall& c,std::uint32_t file_arg)->std::optional<Win32Profile>{
        const auto file=P->string_at(c.argument(file_arg).value_or(0));if(!file){c.guest.stop("GetPrivateProfile: unreadable file name");return std::nullopt;}
        const auto host=P->host_path(*file);P->trace_file("GetPrivateProfile",*file,host?host->string():"not found");
        std::vector<std::uint8_t> bytes;if(host){std::ifstream f(*host,std::ios::binary);bytes.assign(std::istreambuf_iterator<char>(f),{});}
        Win32Profile w(bytes);if(!w.unsupported.empty()){c.guest.stop("GetPrivateProfile: a value form Win32Profile does not interpret ("+w.unsupported.front()+")");return std::nullopt;}
        return w;};
    K("GetPrivateProfileIntA",16,4,[P,arg,profile](HleCall& c)->std::uint32_t{
        const auto w=profile(c,3);if(!w)return 0;const auto section=P->string_at(arg(c,0)),key=P->string_at(arg(c,1));
        if(!section||!key){c.guest.stop("GetPrivateProfileIntA with a NULL section or key");return 0;}
        return static_cast<std::uint32_t>(w->get_int(*section,*key,static_cast<std::int32_t>(arg(c,2))));});
    K("GetPrivateProfileStringA",24,6,[P,arg,profile](HleCall& c)->std::uint32_t{
        const auto w=profile(c,5);if(!w)return 0;const auto section=P->string_at(arg(c,0)),key=P->string_at(arg(c,1));
        if(!section||!key){c.guest.stop("GetPrivateProfileStringA with a NULL section or key (enumeration) is not implemented");return 0;}
        const auto fallback=arg(c,2)?P->string_at(arg(c,2)).value_or(""):std::string();const auto size=arg(c,4);if(!size)return 0;
        const auto v=w->get_string(*section,*key,fallback,size);P->put_string(arg(c,3),v);return static_cast<std::uint32_t>(v.size());});
    // --- the registry holds none of the game's keys (CTRLFLAG0 and the rest absent, as the native runtime assumes)
    const auto registry_absent=[P](HleCall&)->std::uint32_t{P->assume("the registry holds none of the game's keys (RegOpenKey* returns ERROR_FILE_NOT_FOUND)");return ERROR_FILE_NOT_FOUND;};
    r.add("ADVAPI32.dll","RegOpenKeyExA",{20,5,registry_absent});r.add("ADVAPI32.dll","RegOpenKeyA",{12,3,registry_absent});
    r.add("ADVAPI32.dll","RegOpenKeyExW",{20,5,registry_absent});
    r.add("ADVAPI32.dll","RegCloseKey",{4,1,[](HleCall&)->std::uint32_t{return ERROR_INVALID_HANDLE;}});
    r.add("ADVAPI32.dll","RegQueryValueExA",{24,6,[](HleCall&)->std::uint32_t{return ERROR_INVALID_HANDLE;}});
    // --- drives: one fixed C:\ and no CD-ROM (the native runtime's 401a20 assumption; the digital install needs no disc)
    K("GetLogicalDriveStringsA",8,2,[P,arg](HleCall& c)->std::uint32_t{
        P->assume("the drives are one fixed C:\\ with no CD-ROM holding the disc (the reference machine's drives were not captured)");
        const char text[]={'C',':','\\',0,0};if(arg(c,0)<sizeof text)return sizeof text-1;c.guest.write(arg(c,1),text,sizeof text);return 4;});
    K("GetDriveTypeA",4,1,[P,arg](HleCall& c)->std::uint32_t{const auto root=P->string_at(arg(c,0)).value_or("");
        return root.size()>=2&&(root[0]=='C'||root[0]=='c')&&root[1]==':'?3U:1U;}); // DRIVE_FIXED / DRIVE_NO_ROOT_DIR
    // --- CPU and system facts
    K("IsProcessorFeaturePresent",4,1,[P,arg](HleCall& c)->std::uint32_t{
        P->assume("IsProcessorFeaturePresent: an SSE2 CPU without 3DNow! (the reference CPU's feature set was not captured)");
        switch(arg(c,0)){case 6:case 10:case 23:case 2:case 3:return 1; // XMMI, XMMI64, FASTFAIL, COMPARE_EXCHANGE_DOUBLE, MMX
            default:return 0;}});
    K("IsDebuggerPresent",0,0,[](HleCall&)->std::uint32_t{return 0;});
    K("InitializeSListHead",4,1,[arg](HleCall& c)->std::uint32_t{const std::uint64_t z=0;c.guest.write(arg(c,0),&z,8);return 0;});
    K("SetUnhandledExceptionFilter",4,1,[](HleCall&)->std::uint32_t{return 0;});
    K("UnhandledExceptionFilter",4,1,[](HleCall& c)->std::uint32_t{c.guest.stop("UnhandledExceptionFilter: the guest raised an unhandled exception");return 0;});
    K("TerminateProcess",8,2,[arg](HleCall& c)->std::uint32_t{c.guest.stop("TerminateProcess(exit code "+std::to_string(arg(c,1))+")");return 0;});
    K("ExitProcess",4,1,[arg](HleCall& c)->std::uint32_t{c.guest.stop("ExitProcess("+std::to_string(arg(c,0))+")");return 0;});
    K("GetLastError",0,0,[P](HleCall&)->std::uint32_t{return P->last_error;});
    K("SetLastError",4,1,[P,arg](HleCall& c)->std::uint32_t{P->last_error=arg(c,0);return 0;});
    const auto system_info=[P,arg](HleCall& c)->std::uint32_t{
        P->assume("GetSystemInfo: one x86 processor, 4 KiB pages, 64 KiB allocation granularity");
        std::uint32_t si[9]={0,4096,0x10000,0x7ffeffff,1,1,586,0x10000,0}; // wProcessorArchitecture 0 (x86); NumberOfProcessors 1
        si[8]=(6<<16)|0; // wProcessorLevel 6, revision 0
        c.guest.write(arg(c,0),si,sizeof si);return 0;};
    K("GetSystemInfo",4,1,system_info);K("GetNativeSystemInfo",4,1,system_info);
    K("GetVersionExA",4,1,[P,arg](HleCall& c)->std::uint32_t{
        P->assume("GetVersionExA reports Windows 6.2 build 9200 (an unmanifested process on Windows 8 or later)");
        const auto at=arg(c,0);std::uint32_t size=0;c.guest.read(at,&size,4);
        const std::uint32_t v[5]={size,6,2,9200,2};c.guest.write(at,v,sizeof v);
        const char z[128]={};c.guest.write(at+20,z,128);return 1;});
    // --- memory
    K("VirtualProtect",16,4,[arg](HleCall& c)->std::uint32_t{
        const std::uint32_t old=0x40;if(arg(c,3))c.guest.write(arg(c,3),&old,4); // PAGE_EXECUTE_READWRITE: the guest maps images RWX
        return 1;});
    K("VirtualQuery",12,3,[P,arg](HleCall& c)->std::uint32_t{
        const auto address=arg(c,0)&~0xfffU;
        std::uint32_t info[7]={address,address,0x40,0x1000,0x1000,0x40,0x20000}; // MEM_COMMIT, PAGE_EXECUTE_READWRITE, MEM_PRIVATE
        for(const auto& m:P->modules().modules())if(!m->system&&arg(c,0)>=m->base&&arg(c,0)<m->base+m->size){info[1]=m->base;info[3]=m->base+m->size-address;info[6]=0x1000000;} // MEM_IMAGE
        if(arg(c,2)<sizeof info)return 0;c.guest.write(arg(c,1),info,sizeof info);return sizeof info;});
    K("VirtualAlloc",16,4,[P,arg](HleCall& c)->std::uint32_t{
        if(arg(c,0)){c.guest.stop("VirtualAlloc at a fixed address is not implemented");return 0;}
        return P->memory().allocate(arg(c,1),0x10000).value_or(0);});
    K("VirtualFree",12,3,[P,arg](HleCall& c)->std::uint32_t{return P->memory().free(arg(c,0))?1:0;});
    K("WriteProcessMemory",20,5,[arg](HleCall& c)->std::uint32_t{
        if(arg(c,0)!=current_process){c.guest.stop("WriteProcessMemory into another process");return 0;}
        std::vector<std::uint8_t> b(arg(c,3));if(!c.guest.read(arg(c,2),b.data(),b.size())||!c.guest.write(arg(c,1),b.data(),b.size()))return 0;
        if(arg(c,4)){const auto n=arg(c,3);c.guest.write(arg(c,4),&n,4);}
        return 1;});
    K("LocalFree",4,1,[P,arg](HleCall& c)->std::uint32_t{if(arg(c,0))P->memory().free(arg(c,0));return 0;});
    // --- Win32 heaps: every heap hands out HleMemory blocks; the handle only has to be valid.
    K("GetProcessHeap",0,0,[P](HleCall&)->std::uint32_t{if(!P->process_heap){P->process_heap=P->new_handle();P->heaps.insert(P->process_heap);}return P->process_heap;});
    K("HeapCreate",12,3,[P](HleCall&)->std::uint32_t{const auto h=P->new_handle();P->heaps.insert(h);return h;});
    K("HeapDestroy",4,1,[P,arg](HleCall& c)->std::uint32_t{return P->heaps.erase(arg(c,0))?1:0;});
    K("HeapAlloc",12,3,[P,arg](HleCall& c)->std::uint32_t{
        if(!P->heaps.count(arg(c,0))){c.guest.stop("HeapAlloc on an unknown heap");return 0;}return P->memory().allocate(arg(c,2)).value_or(0);});
    K("HeapFree",12,3,[P,arg](HleCall& c)->std::uint32_t{if(!arg(c,2))return 1;return P->memory().free(arg(c,2))?1:0;});
    K("HeapReAlloc",16,4,[P,arg](HleCall& c)->std::uint32_t{
        if(arg(c,1)&0x10){c.guest.stop("HeapReAlloc with HEAP_REALLOC_IN_PLACE_ONLY is not implemented");return 0;}
        return P->memory().reallocate(arg(c,2),arg(c,3)).value_or(0);});
    K("HeapSize",12,3,[P,arg](HleCall& c)->std::uint32_t{return P->memory().size(arg(c,2)).value_or(0xffffffffU);});
    // --- one thread: critical sections never contend; TLS is one table.
    K("InitializeCriticalSection",4,1,[arg](HleCall& c)->std::uint32_t{const std::uint8_t z[24]={};c.guest.write(arg(c,0),z,24);return 0;});
    K("InitializeCriticalSectionAndSpinCount",8,2,[arg](HleCall& c)->std::uint32_t{const std::uint8_t z[24]={};c.guest.write(arg(c,0),z,24);return 1;});
    K("DeleteCriticalSection",4,1,[](HleCall&)->std::uint32_t{return 0;});
    K("EnterCriticalSection",4,1,[](HleCall&)->std::uint32_t{return 0;});
    K("LeaveCriticalSection",4,1,[](HleCall&)->std::uint32_t{return 0;});
    K("InitializeCriticalSectionEx",12,3,[arg](HleCall& c)->std::uint32_t{const std::uint8_t z[24]={};c.guest.write(arg(c,0),z,24);return 1;});
    K("TryEnterCriticalSection",4,1,[](HleCall&)->std::uint32_t{return 1;});
    // Fiber-local storage is thread-local storage for a thread that never becomes a fiber (FlsAlloc's
    // destructor callback is kept nowhere: threads never exit in the guest).
    K("FlsAlloc",4,1,[P](HleCall&)->std::uint32_t{const auto i=P->next_tls++;P->tls[i]=0;return i;});
    K("FlsFree",4,1,[P,arg](HleCall& c)->std::uint32_t{return P->tls.erase(arg(c,0))?1:0;});
    K("FlsGetValue",4,1,[P,arg](HleCall& c)->std::uint32_t{const auto i=P->tls.find(arg(c,0));return i==P->tls.end()?0:i->second;});
    K("FlsSetValue",8,2,[P,arg](HleCall& c)->std::uint32_t{const auto i=P->tls.find(arg(c,0));if(i==P->tls.end())return 0;i->second=arg(c,1);return 1;});
    // SRW locks and condition variables: one thread never contends; a wait would deadlock and stops.
    for(const char* name:{"InitializeSRWLock","AcquireSRWLockExclusive","ReleaseSRWLockExclusive","AcquireSRWLockShared","ReleaseSRWLockShared","InitializeConditionVariable","WakeAllConditionVariable","WakeConditionVariable"})
        K(name,4,1,[](HleCall&)->std::uint32_t{return 0;});
    K("TryAcquireSRWLockExclusive",4,1,[](HleCall&)->std::uint32_t{return 1;});
    K("SleepConditionVariableSRW",16,4,[](HleCall& c)->std::uint32_t{c.guest.stop("SleepConditionVariableSRW: a wait in a one-thread guest");return 0;});
    K("SleepConditionVariableCS",12,3,[](HleCall& c)->std::uint32_t{c.guest.stop("SleepConditionVariableCS: a wait in a one-thread guest");return 0;});
    // EncodePointer/DecodePointer: the identity keeps Decode(Encode(p)) == p, which is all callers rely on.
    K("EncodePointer",4,1,[arg](HleCall& c)->std::uint32_t{return arg(c,0);});
    K("DecodePointer",4,1,[arg](HleCall& c)->std::uint32_t{return arg(c,0);});
    // InitOnceExecuteOnce(once, fn, parameter, context): runs fn(once, parameter, context) the first time.
    K("InitOnceExecuteOnce",16,4,[P,arg](HleCall& c)->std::uint32_t{
        const auto once=arg(c,0);std::uint32_t state=0;c.guest.read(once,&state,4);
        if(state&2)return 1; // done
        const std::uint32_t pending=1;c.guest.write(once,&pending,4);
        c.guest.redirect(arg(c,1),{once,arg(c,2),arg(c,3)},16,[once](X86Guest& g){
            const auto ok=g.reg(19);const std::uint32_t done=ok?2:0;g.write(once,&done,4);g.set_eax(ok?1:0);});
        (void)P;return 0;});
    // Interlocked arithmetic on a guest LONG (one guest thread: plain read-modify-write).
    K("InterlockedIncrement",4,1,[arg](HleCall& c)->std::uint32_t{const auto v=c.guest.u32(arg(c,0)).value_or(0)+1;c.guest.set_u32(arg(c,0),v);return v;});
    K("InterlockedDecrement",4,1,[arg](HleCall& c)->std::uint32_t{const auto v=c.guest.u32(arg(c,0)).value_or(0)-1;c.guest.set_u32(arg(c,0),v);return v;});
    K("InterlockedExchange",8,2,[arg](HleCall& c)->std::uint32_t{const auto v=c.guest.u32(arg(c,0)).value_or(0);c.guest.set_u32(arg(c,0),arg(c,1));return v;});
    K("InterlockedCompareExchange",12,3,[arg](HleCall& c)->std::uint32_t{const auto v=c.guest.u32(arg(c,0)).value_or(0);if(v==arg(c,2))c.guest.set_u32(arg(c,0),arg(c,1));return v;});
    K("InterlockedExchangeAdd",8,2,[arg](HleCall& c)->std::uint32_t{const auto v=c.guest.u32(arg(c,0)).value_or(0);c.guest.set_u32(arg(c,0),v+arg(c,1));return v;});
    // Interlocked singly-linked lists (SLIST_HEADER: Next, Depth, Sequence) on guest memory.
    K("InterlockedPushEntrySList",8,2,[arg](HleCall& c)->std::uint32_t{
        const auto head=arg(c,0),entry=arg(c,1);const auto old=c.guest.u32(head).value_or(0);std::uint32_t meta=c.guest.u32(head+4).value_or(0);
        c.guest.set_u32(entry,old);c.guest.set_u32(head,entry);meta=((meta&0xffff0000U)+0x10000U)|((meta+1)&0xffffU);c.guest.set_u32(head+4,meta);return old;});
    K("InterlockedFlushSList",4,1,[arg](HleCall& c)->std::uint32_t{
        const auto head=arg(c,0);const auto old=c.guest.u32(head).value_or(0);c.guest.set_u32(head,0);
        const auto meta=c.guest.u32(head+4).value_or(0);c.guest.set_u32(head+4,meta&0xffff0000U);return old;});
    K("GetTickCount64",0,0,[P](HleCall& c)->std::uint32_t{const auto ms=P->now_100ns/10000;c.guest.set_edx(static_cast<std::uint32_t>(ms>>32));return static_cast<std::uint32_t>(ms);});
    K("GetSystemTimePreciseAsFileTime",4,1,[P,arg](HleCall& c)->std::uint32_t{const std::uint64_t t=virtual_epoch_filetime+P->now_100ns;c.guest.write(arg(c,0),&t,8);return 0;});
    K("GetCurrentProcessorNumber",0,0,[](HleCall&)->std::uint32_t{return 0;});
    K("FlushProcessWriteBuffers",0,0,[](HleCall&)->std::uint32_t{return 0;});
    K("SwitchToThread",0,0,[](HleCall&)->std::uint32_t{return 0;});
    K("RtlCaptureStackBackTrace",16,4,[](HleCall&)->std::uint32_t{return 0;});
    K("GetModuleHandleExW",12,3,[P,arg](HleCall& c)->std::uint32_t{
        const auto flags=arg(c,0);std::uint32_t base=0;
        if(flags&4){for(const auto& m:P->modules().modules())if(!m->system&&arg(c,1)>=m->base&&arg(c,1)<m->base+m->size)base=m->base;} // FROM_ADDRESS
        else{const auto name=arg(c,1)?P->wide_at(arg(c,1)):std::optional<std::string>{};if(!arg(c,1))base=P->modules().modules().front()->base;else if(name)if(auto* m=P->modules().by_name(*name))base=m->base;}
        if(arg(c,2))c.guest.set_u32(arg(c,2),base);if(!base){P->last_error=126;return 0;}return 1;});
    K("GetModuleHandleExA",12,3,[P,arg](HleCall& c)->std::uint32_t{
        const auto flags=arg(c,0);std::uint32_t base=0;
        if(flags&4){for(const auto& m:P->modules().modules())if(!m->system&&arg(c,1)>=m->base&&arg(c,1)<m->base+m->size)base=m->base;}
        else{const auto name=arg(c,1)?P->string_at(arg(c,1)):std::optional<std::string>{};if(!arg(c,1))base=P->modules().modules().front()->base;else if(name)if(auto* m=P->modules().by_name(*name))base=m->base;}
        if(arg(c,2))c.guest.set_u32(arg(c,2),base);if(!base){P->last_error=126;return 0;}return 1;});
    // Locale-name functions for the invariant/user locale the CRT asks about.
    K("GetLocaleInfoEx",16,4,[](HleCall& c)->std::uint32_t{c.guest.stop("GetLocaleInfoEx is not implemented");return 0;});
    K("LCMapStringEx",36,9,[](HleCall& c)->std::uint32_t{c.guest.stop("LCMapStringEx is not implemented");return 0;});
    K("CompareStringEx",36,9,[](HleCall& c)->std::uint32_t{c.guest.stop("CompareStringEx is not implemented");return 0;});
    K("TlsAlloc",0,0,[P](HleCall&)->std::uint32_t{const auto i=P->next_tls++;P->tls[i]=0;return i;});
    K("TlsFree",4,1,[P,arg](HleCall& c)->std::uint32_t{return P->tls.erase(arg(c,0))?1:0;});
    K("TlsGetValue",4,1,[P,arg](HleCall& c)->std::uint32_t{P->last_error=0;const auto i=P->tls.find(arg(c,0));return i==P->tls.end()?0:i->second;});
    K("TlsSetValue",8,2,[P,arg](HleCall& c)->std::uint32_t{const auto i=P->tls.find(arg(c,0));if(i==P->tls.end())return 0;i->second=arg(c,1);return 1;});
    // --- process start-up facts for a GUI-subsystem exe
    K("GetTickCount",0,0,[P](HleCall&)->std::uint32_t{return static_cast<std::uint32_t>(P->now_100ns/10000);});
    K("GetVersion",0,0,[P](HleCall&)->std::uint32_t{P->assume("GetVersionExA reports Windows 6.2 build 9200 (an unmanifested process on Windows 8 or later)");return (9200U<<16)|(2U<<8)|6U;});
    K("GetCommandLineA",0,0,[P](HleCall&)->std::uint32_t{
        if(!P->command_line_a)P->command_line_a=P->allocate_string("\""+P->modules().windows_dir()+"\\sonic.exe\"").value_or(0);return P->command_line_a;});
    const auto environment=[P](HleCall&)->std::uint32_t{
        P->assume("the process environment block is empty (the reference environment was not captured)");
        if(!P->command_line_a_env)P->command_line_a_env=P->memory().allocate(8).value_or(0);return P->command_line_a_env;};
    K("GetEnvironmentStrings",0,0,environment);K("GetEnvironmentStringsW",0,0,environment);
    K("FreeEnvironmentStringsA",4,1,[](HleCall&)->std::uint32_t{return 1;});K("FreeEnvironmentStringsW",4,1,[](HleCall&)->std::uint32_t{return 1;});
    // Environment variables: empty at start (see the assumption above), set/get by name, case-insensitive.
    const auto upper=[](std::string s){for(auto& ch:s)ch=static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));return s;};
    const auto get_env=[P,arg,upper](bool wide){return [P,arg,upper,wide](HleCall& c)->std::uint32_t{
        const auto name=wide?P->wide_at(arg(c,0)):P->string_at(arg(c,0));if(!name){c.guest.stop("GetEnvironmentVariable: unreadable name");return 0;}
        const auto i=P->environment_variables.find(upper(*name));if(i==P->environment_variables.end()){P->last_error=203;return 0;} // ERROR_ENVVAR_NOT_FOUND
        const auto& value=i->second;if(value.size()+1>arg(c,2))return static_cast<std::uint32_t>(value.size()+1);
        if(wide)P->put_wide(arg(c,1),value);else P->put_string(arg(c,1),value);return static_cast<std::uint32_t>(value.size());};};
    K("GetEnvironmentVariableA",12,3,get_env(false));K("GetEnvironmentVariableW",12,3,get_env(true));
    const auto set_env=[P,arg,upper](bool wide){return [P,arg,upper,wide](HleCall& c)->std::uint32_t{
        const auto name=wide?P->wide_at(arg(c,0)):P->string_at(arg(c,0));if(!name){c.guest.stop("SetEnvironmentVariable: unreadable name");return 0;}
        if(!arg(c,1)){P->environment_variables.erase(upper(*name));return 1;}
        const auto value=wide?P->wide_at(arg(c,1)):P->string_at(arg(c,1));if(!value){c.guest.stop("SetEnvironmentVariable: unreadable value");return 0;}
        P->environment_variables[upper(*name)]=*value;return 1;};};
    K("SetEnvironmentVariableA",8,2,set_env(false));K("SetEnvironmentVariableW",8,2,set_env(true));
    K("GetStartupInfoA",4,1,[arg](HleCall& c)->std::uint32_t{std::uint8_t si[68]={};si[0]=68;c.guest.write(arg(c,0),si,68);return 0;});
    K("GetStdHandle",4,1,[](HleCall&)->std::uint32_t{return 0;}); // a GUI-subsystem process has no standard handles
    K("GetFileType",4,1,[P,arg](HleCall& c)->std::uint32_t{return P->file_handles.count(arg(c,0))?1U:0U;}); // FILE_TYPE_DISK / UNKNOWN
    K("SetHandleCount",4,1,[arg](HleCall& c)->std::uint32_t{return arg(c,0);});
    K("GetACP",0,0,[P](HleCall&)->std::uint32_t{P->assume("ANSI code page 1252 and OEM code page 437 (a Western-European Windows; the reference locale was not captured)");return 1252;});
    K("GetOEMCP",0,0,[P](HleCall&)->std::uint32_t{P->assume("ANSI code page 1252 and OEM code page 437 (a Western-European Windows; the reference locale was not captured)");return 437;});
    K("GetCPInfo",8,2,[arg](HleCall& c)->std::uint32_t{
        const auto cp=arg(c,0);if(cp!=1252&&cp!=437&&cp!=0&&cp!=1){c.guest.stop("GetCPInfo for code page "+std::to_string(cp));return 0;}
        std::uint8_t info[20]={};info[0]=1;info[4]='?';c.guest.write(arg(c,1),info,20);return 1;}); // MaxCharSize 1, DefaultChar '?', no lead bytes
    // GetStringTypeA/W (CT_CTYPE1) and LCMapStringA/W (LCMAP_LOWERCASE / LCMAP_UPPERCASE), the A forms through 1252.
    const auto read_units=[](HleCall& c,std::uint32_t at,std::int32_t n,bool wide){
        std::vector<std::uint16_t> s;const auto unit=wide?2U:1U;
        for(std::uint32_t i=0;n<0||i<static_cast<std::uint32_t>(n);++i){std::uint16_t ch=0;if(!c.guest.read(at+unit*i,&ch,unit))break;s.push_back(wide?ch:cp1252_to_utf16(static_cast<std::uint8_t>(ch)));if(n<0&&!ch)break;}
        return s;};
    K("GetStringTypeA",20,5,[P,arg,read_units](HleCall& c)->std::uint32_t{
        if(arg(c,1)!=1){c.guest.stop("GetStringTypeA other than CT_CTYPE1");return 0;}
        P->assume("CT_CTYPE1 above U+007F from Unicode categories (Windows' NLS tables were not captured)");
        const auto s=read_units(c,arg(c,2),static_cast<std::int32_t>(arg(c,3)),false);
        for(std::size_t i=0;i<s.size();++i){const auto t=ctype1(s[i]);c.guest.write(arg(c,4)+2*static_cast<std::uint32_t>(i),&t,2);}return 1;});
    K("GetStringTypeW",16,4,[P,arg,read_units](HleCall& c)->std::uint32_t{
        if(arg(c,0)!=1){c.guest.stop("GetStringTypeW other than CT_CTYPE1");return 0;}
        P->assume("CT_CTYPE1 above U+007F from Unicode categories (Windows' NLS tables were not captured)");
        const auto s=read_units(c,arg(c,1),static_cast<std::int32_t>(arg(c,2)),true);
        for(std::size_t i=0;i<s.size();++i){const auto t=ctype1(s[i]);c.guest.write(arg(c,3)+2*static_cast<std::uint32_t>(i),&t,2);}return 1;});
    const auto map_string=[arg,read_units](bool wide){return [arg,read_units,wide](HleCall& c)->std::uint32_t{
        const auto flags=arg(c,1);if(flags!=0x100&&flags!=0x200){c.guest.stop("LCMapString with flags "+std::to_string(flags)+" is not implemented");return 0;}
        const auto s=read_units(c,arg(c,2),static_cast<std::int32_t>(arg(c,3)),wide);
        if(!arg(c,5))return static_cast<std::uint32_t>(s.size());if(s.size()>arg(c,5))return 0;
        for(std::size_t i=0;i<s.size();++i){const auto u=flags==0x100?to_lower16(s[i]):to_upper16(s[i]);
            if(wide)c.guest.write(arg(c,4)+2*static_cast<std::uint32_t>(i),&u,2);
            else{const auto b=utf16_to_cp1252(u).value_or(static_cast<std::uint8_t>('?'));c.guest.write(arg(c,4)+static_cast<std::uint32_t>(i),&b,1);}}
        return static_cast<std::uint32_t>(s.size());};};
    K("LCMapStringA",24,6,map_string(false));K("LCMapStringW",24,6,map_string(true));
    K("RtlUnwind",16,4,[](HleCall& c)->std::uint32_t{c.guest.stop("RtlUnwind: SEH unwinding is not modelled");return 0;});
    // --- strings
    K("lstrlenA",4,1,[P,arg](HleCall& c)->std::uint32_t{if(!arg(c,0))return 0;const auto s=P->string_at(arg(c,0));return s?static_cast<std::uint32_t>(s->size()):0;});
    K("lstrcmpA",8,2,[P,arg](HleCall& c)->std::uint32_t{const auto a=P->string_at(arg(c,0)).value_or(""),b=P->string_at(arg(c,1)).value_or("");return static_cast<std::uint32_t>(a<b?-1:a>b?1:0);});
    K("lstrcmpiA",8,2,[P,arg](HleCall& c)->std::uint32_t{const auto a=lower(P->string_at(arg(c,0)).value_or("")),b=lower(P->string_at(arg(c,1)).value_or(""));return static_cast<std::uint32_t>(a<b?-1:a>b?1:0);});
    K("AreFileApisANSI",0,0,[](HleCall&)->std::uint32_t{return 1;});
    // --- code pages: 0/1252 (ANSI) and 65001 (UTF-8); others stop explicitly.
    const auto code_page=[P](HleCall& c,std::uint32_t cp)->std::optional<std::uint32_t>{
        if(cp==0||cp==3)cp=1252;
        if(cp!=1252&&cp!=65001){c.guest.stop("code page "+std::to_string(cp)+" is not implemented");return std::nullopt;}
        if(cp==1252)P->assume("ANSI code page 1252 and OEM code page 437 (a Western-European Windows; the reference locale was not captured)");
        return cp;};
    K("MultiByteToWideChar",24,6,[P,arg,code_page](HleCall& c)->std::uint32_t{
        const auto cp=code_page(c,arg(c,0));if(!cp)return 0;
        const auto src=arg(c,2);const auto n=static_cast<std::int32_t>(arg(c,3));const auto dst=arg(c,4),capacity=arg(c,5);
        std::vector<std::uint8_t> bytes;
        if(n<0){for(std::uint32_t i=0;;++i){std::uint8_t b;if(!c.guest.read(src+i,&b,1)){c.guest.stop("MultiByteToWideChar: unreadable source");return 0;}bytes.push_back(b);if(!b)break;}}
        else{bytes.resize(static_cast<std::size_t>(n));if(n&&!c.guest.read(src,bytes.data(),bytes.size())){c.guest.stop("MultiByteToWideChar: unreadable source");return 0;}}
        std::vector<std::uint16_t> w;
        if(*cp==1252)for(const auto b:bytes)w.push_back(cp1252_to_utf16(b));
        else{const auto d=utf8_to_utf16(bytes);if(!d){P->last_error=1113;return 0;}w=*d;} // ERROR_NO_UNICODE_TRANSLATION
        if(!capacity)return static_cast<std::uint32_t>(w.size());
        if(w.size()>capacity){P->last_error=ERROR_INSUFFICIENT_BUFFER;return 0;}
        c.guest.write(dst,w.data(),w.size()*2);return static_cast<std::uint32_t>(w.size());});
    K("WideCharToMultiByte",32,8,[P,arg,code_page](HleCall& c)->std::uint32_t{
        const auto cp=code_page(c,arg(c,0));if(!cp)return 0;
        const auto src=arg(c,2);const auto n=static_cast<std::int32_t>(arg(c,3));const auto dst=arg(c,4),capacity=arg(c,5);
        std::vector<std::uint16_t> w;
        if(n<0){for(std::uint32_t i=0;;++i){std::uint16_t ch=0;if(!c.guest.read(src+2*i,&ch,2)){c.guest.stop("WideCharToMultiByte: unreadable source");return 0;}w.push_back(ch);if(!ch)break;}}
        else{w.resize(static_cast<std::size_t>(n));if(n&&!c.guest.read(src,w.data(),w.size()*2)){c.guest.stop("WideCharToMultiByte: unreadable source");return 0;}}
        std::vector<std::uint8_t> bytes;
        if(*cp==1252)for(const auto u:w){const auto b=utf16_to_cp1252(u);if(!b){c.guest.stop("WideCharToMultiByte: a character without an exact 1252 mapping (best-fit is not implemented)");return 0;}bytes.push_back(*b);}
        else bytes=utf16_to_utf8(w);
        if(!capacity)return static_cast<std::uint32_t>(bytes.size());
        if(bytes.size()>capacity){P->last_error=ERROR_INSUFFICIENT_BUFFER;return 0;}
        c.guest.write(dst,bytes.data(),bytes.size());if(arg(c,7)){const std::uint32_t z=0;c.guest.write(arg(c,7),&z,4);}
        return static_cast<std::uint32_t>(bytes.size());});
    K("GetCommandLineW",0,0,[P](HleCall&)->std::uint32_t{
        if(!P->command_line_w){P->command_line_w=P->memory().allocate(128).value_or(0);P->put_wide(P->command_line_w,"\""+P->modules().windows_dir()+"\\sonic.exe\"");}
        return P->command_line_w;});
    // --- files under the install
    K("GetFileAttributesA",4,1,[P,arg](HleCall& c)->std::uint32_t{
        const auto name=P->string_at(arg(c,0));const auto host=name?P->host_path(*name):std::nullopt;
        P->trace_file("GetFileAttributesA",name.value_or("?"),host?host->string():"not found");
        if(!host){P->last_error=ERROR_FILE_NOT_FOUND;return 0xffffffffU;}
        return std::filesystem::is_directory(*host)?0x10U:0x80U;}); // FILE_ATTRIBUTE_DIRECTORY / NORMAL
    K("GetFileAttributesW",4,1,[P,arg](HleCall& c)->std::uint32_t{
        const auto name=P->wide_at(arg(c,0));const auto host=name?P->host_path(*name):std::nullopt;
        P->trace_file("GetFileAttributesW",name.value_or("?"),host?host->string():"not found");
        if(!host){P->last_error=ERROR_FILE_NOT_FOUND;return 0xffffffffU;}
        return std::filesystem::is_directory(*host)?0x10U:0x80U;});
    K("CloseHandle",4,1,[P,arg](HleCall& c)->std::uint32_t{
        const auto i=P->file_handles.find(arg(c,0));if(i==P->file_handles.end()){P->last_error=ERROR_INVALID_HANDLE;return 0;}
        if(i->second.file)std::fclose(i->second.file);P->file_handles.erase(i);return 1;});
    const auto create_file=[P,arg](bool wide){return [P,arg,wide](HleCall& c)->std::uint32_t{
        const auto name=wide?P->wide_at(arg(c,0)):P->string_at(arg(c,0));
        if(!name){c.guest.stop("CreateFile: unreadable or non-ASCII file name");return 0;}
        const auto access=arg(c,1),disposition=arg(c,4);const bool write=(access&0x40000000U)!=0;
        if(name->rfind("\\\\.\\",0)==0||(name->find(':')==std::string::npos&&name->rfind("CON",0)==0)){c.guest.stop("CreateFile on a device ("+*name+") is not implemented");return 0;}
        auto host=P->host_path(*name);
        P->trace_file(wide?"CreateFileW":"CreateFileA",*name,(host?host->string():"not found")+(write?" (write)":""));
        if(disposition==1&&host){P->last_error=80;return 0xffffffffU;} // CREATE_NEW on an existing file: ERROR_FILE_EXISTS
        if((disposition==3||disposition==5)&&!host){P->last_error=ERROR_FILE_NOT_FOUND;return 0xffffffffU;}
        if(host&&std::filesystem::is_directory(*host)){P->last_error=5;return 0xffffffffU;} // ERROR_ACCESS_DENIED
        if(write||disposition==1||disposition==2||disposition==4||disposition==5){
            const auto out=P->write_path(*name);if(!out){P->last_error=ERROR_PATH_NOT_FOUND;return 0xffffffffU;}
            const bool truncate=disposition==2||disposition==5||(disposition==1);
            std::FILE* f=std::fopen(out->string().c_str(),truncate||!std::filesystem::exists(*out)?"w+b":"r+b");
            if(!f){P->last_error=5;return 0xffffffffU;}
            const auto h=P->new_handle();P->file_handles[h]={*out,f,true};
            P->last_error=(disposition==2||disposition==4)&&host?ERROR_ALREADY_EXISTS:0;return h;}
        std::FILE* f=std::fopen(host->string().c_str(),"rb");if(!f){P->last_error=5;return 0xffffffffU;}
        const auto h=P->new_handle();P->file_handles[h]={*host,f,false};P->last_error=0;return h;};};
    K("CreateFileA",28,7,create_file(false));K("CreateFileW",28,7,create_file(true));
    K("ReadFile",20,5,[P,arg](HleCall& c)->std::uint32_t{
        const auto i=P->file_handles.find(arg(c,0));if(i==P->file_handles.end()){P->last_error=ERROR_INVALID_HANDLE;return 0;}
        if(arg(c,4)){c.guest.stop("ReadFile with OVERLAPPED is not implemented");return 0;}
        std::vector<std::uint8_t> b(arg(c,2));const auto n=std::fread(b.data(),1,b.size(),i->second.file);
        c.guest.write(arg(c,1),b.data(),n);if(arg(c,3)){const auto v=static_cast<std::uint32_t>(n);c.guest.write(arg(c,3),&v,4);}return 1;});
    // ReadFileEx: the read completes at once from the host file (OVERLAPPED Offset/OffsetHigh); the completion
    // routine is queued as an APC and runs at the thread's next alertable wait, as Windows delivers it.
    K("ReadFileEx",20,5,[P,arg](HleCall& c)->std::uint32_t{
        P->assume("ReadFileEx completes immediately from the host file; its completion routine runs at the next alertable wait (the reference disk timing was not captured)");
        const auto i=P->file_handles.find(arg(c,0));if(i==P->file_handles.end()){P->last_error=ERROR_INVALID_HANDLE;return 0;}
        const auto overlapped=arg(c,3);std::uint32_t o[5]={};c.guest.read(overlapped,o,20);
        const std::int64_t offset=(std::int64_t(o[3])<<32)|o[2];std::fseek(i->second.file,static_cast<long>(offset),SEEK_SET);
        std::vector<std::uint8_t> b(arg(c,2));const auto n=static_cast<std::uint32_t>(std::fread(b.data(),1,b.size(),i->second.file));c.guest.write(arg(c,1),b.data(),n);
        o[0]=0;o[1]=n;c.guest.write(overlapped,o,8); // Internal = STATUS_SUCCESS, InternalHigh = bytes
        P->apcs.push_back({arg(c,4),n||!arg(c,2)?0U:38U,n,overlapped});return 1;}); // ERROR_HANDLE_EOF when nothing was read
    K("CancelIo",4,1,[](HleCall&)->std::uint32_t{return 1;});
    // Alertable waits deliver the queued APCs (stdcall routine(error, bytes, overlapped)), then return WAIT_IO_COMPLETION.
    const auto deliver=[P](HleCall& c,std::uint32_t pops,std::uint32_t idle_result,std::uint32_t idle_ms)->std::uint32_t{
        if(P->apcs.empty()){P->now_100ns+=std::uint64_t(idle_ms==0xffffffffU?0:idle_ms)*10000;return idle_result;}
        auto queue=std::make_shared<std::vector<HleProcess::Apc>>(std::move(P->apcs));P->apcs.clear();
        auto next=std::make_shared<std::size_t>(0);auto step=std::make_shared<std::function<void(X86Guest&,std::uint32_t)>>();
        *step=[queue,next,step](X86Guest& g,std::uint32_t pop){
            if(*next>=queue->size()){g.set_eax(0xc0);return;} // WAIT_IO_COMPLETION
            const auto a=(*queue)[(*next)++];g.redirect(a.routine,{a.error,a.bytes,a.overlapped},pop,[step](X86Guest& g2){(*step)(g2,0);});};
        (*step)(c.guest,pops);return 0xc0;};
    K("SleepEx",8,2,[P,arg,deliver](HleCall& c)->std::uint32_t{
        if(!arg(c,1)){P->now_100ns+=std::uint64_t(arg(c,0))*10000;return 0;}return deliver(c,8,0,arg(c,0));});
    K("WaitForSingleObjectEx",12,3,[arg,deliver](HleCall& c)->std::uint32_t{if(!arg(c,2))return 0;return deliver(c,12,0,0);});
    r.add("USER32.dll","MsgWaitForMultipleObjectsEx",{20,5,[arg,deliver](HleCall& c)->std::uint32_t{
        if(!(arg(c,4)&2))return arg(c,0); // not MWMO_ALERTABLE: WAIT_TIMEOUT-free model: "input available" = count (nothing to wait for)
        return deliver(c,20,arg(c,0),arg(c,2));}});
    K("WriteFile",20,5,[P,arg](HleCall& c)->std::uint32_t{
        const auto i=P->file_handles.find(arg(c,0));if(i==P->file_handles.end()||!i->second.writable){P->last_error=ERROR_INVALID_HANDLE;return 0;}
        std::vector<std::uint8_t> b(arg(c,2));c.guest.read(arg(c,1),b.data(),b.size());const auto n=std::fwrite(b.data(),1,b.size(),i->second.file);
        if(arg(c,3)){const auto v=static_cast<std::uint32_t>(n);c.guest.write(arg(c,3),&v,4);}return 1;});
    K("SetFilePointer",16,4,[P,arg](HleCall& c)->std::uint32_t{
        const auto i=P->file_handles.find(arg(c,0));if(i==P->file_handles.end()){P->last_error=ERROR_INVALID_HANDLE;return 0xffffffffU;}
        std::int64_t distance=static_cast<std::int32_t>(arg(c,1));if(arg(c,2)){std::int32_t high=0;c.guest.read(arg(c,2),&high,4);distance=(std::int64_t(high)<<32)|arg(c,1);}
        const int whence=arg(c,3)==0?SEEK_SET:arg(c,3)==1?SEEK_CUR:SEEK_END;
        if(std::fseek(i->second.file,static_cast<long>(distance),whence)){P->last_error=131;return 0xffffffffU;} // ERROR_NEGATIVE_SEEK
        const auto at=static_cast<std::uint64_t>(std::ftell(i->second.file));if(arg(c,2)){const auto high=static_cast<std::uint32_t>(at>>32);c.guest.write(arg(c,2),&high,4);}
        return static_cast<std::uint32_t>(at);});
    K("GetFileSize",8,2,[P,arg](HleCall& c)->std::uint32_t{
        const auto i=P->file_handles.find(arg(c,0));if(i==P->file_handles.end()){P->last_error=ERROR_INVALID_HANDLE;return 0xffffffffU;}
        std::fflush(i->second.file);std::error_code ec;const auto size=std::filesystem::file_size(i->second.host,ec);
        if(arg(c,1)){const auto high=static_cast<std::uint32_t>(size>>32);c.guest.write(arg(c,1),&high,4);}return static_cast<std::uint32_t>(size);});
    const auto create_directory=[P,arg](bool wide){return [P,arg,wide](HleCall& c)->std::uint32_t{
        const auto name=wide?P->wide_at(arg(c,0)):P->string_at(arg(c,0));if(!name){c.guest.stop("CreateDirectory: unreadable name");return 0;}
        if(P->host_path(*name)){P->last_error=ERROR_ALREADY_EXISTS;return 0;}
        const auto out=P->write_path(*name+"\\.keep");if(!out){P->last_error=ERROR_PATH_NOT_FOUND;return 0;}
        std::error_code ec;std::filesystem::create_directories(out->parent_path(),ec);return 1;};};
    K("CreateDirectoryA",8,2,create_directory(false));K("CreateDirectoryW",8,2,create_directory(true));
    // FindFirstFile/FindNextFile: entries in case-insensitive name order (NTFS's), "." and ".." first for a
    // non-root directory, '*' and '?' wildcards on the last path component, fixed virtual file times.
    const auto find_first=[P,arg](bool wide,unsigned data_arg){return [P,arg,wide,data_arg](HleCall& c)->std::uint32_t{
        P->assume("FindFirstFile: NTFS name order (case-insensitive), virtual file times");
        const auto pattern=wide?P->wide_at(arg(c,0)):P->string_at(arg(c,0));if(!pattern){c.guest.stop("FindFirstFile: unreadable pattern");return 0;}
        auto p=*pattern;std::replace(p.begin(),p.end(),'/','\\');const auto slash=p.rfind('\\');
        const auto dir=slash==std::string::npos?std::string("."):p.substr(0,slash);const auto mask=lower(slash==std::string::npos?p:p.substr(slash+1));
        const auto host=P->host_path(dir.empty()?std::string("\\"):dir);
        P->trace_file("FindFirstFile",*pattern,host?host->string():"no directory");
        if(!host||!std::filesystem::is_directory(*host)){P->last_error=ERROR_PATH_NOT_FOUND;return 0xffffffffU;}
        std::function<bool(const char*,const char*)> match=[&](const char* m,const char* s)->bool{
            if(!*m)return !*s;if(*m=='*')return match(m+1,s)||(*s&&match(m,s+1));if(*s&&(*m=='?'||*m==*s))return match(m+1,s+1);return false;};
        std::vector<std::filesystem::path> entries;std::error_code ec;
        std::map<std::string,std::filesystem::path> sorted;
        for(const auto& e:std::filesystem::directory_iterator(*host,ec))sorted[lower(e.path().filename().string())]=e.path();
        if(!overlay_relative_root(P,*host))for(const char* special:{".",".."})if(match(mask=="*.*"?"*":mask.c_str(),special))entries.push_back(*host/special);
        for(const auto& [name,path]:sorted)if(match(mask=="*.*"?"*":mask.c_str(),name.c_str()))entries.push_back(path);
        if(entries.empty()){P->last_error=ERROR_FILE_NOT_FOUND;return 0xffffffffU;}
        const auto h=P->new_handle();P->finds[h]=std::vector<std::filesystem::path>(entries.begin()+1,entries.end());
        write_find_data(P,c.guest,arg(c,data_arg),entries.front(),wide);return h;};};
    K("FindFirstFileA",8,2,find_first(false,1));K("FindFirstFileW",8,2,find_first(true,1));
    K("FindFirstFileExW",24,6,[find_first](HleCall& c)->std::uint32_t{return find_first(true,2)(c);});
    const auto find_next=[P,arg](bool wide){return [P,arg,wide](HleCall& c)->std::uint32_t{
        const auto i=P->finds.find(arg(c,0));if(i==P->finds.end()){P->last_error=ERROR_INVALID_HANDLE;return 0;}
        if(i->second.empty()){P->last_error=ERROR_NO_MORE_FILES;return 0;}
        const auto next=i->second.front();i->second.erase(i->second.begin());write_find_data(P,c.guest,arg(c,1),next,wide);return 1;};};
    K("FindNextFileA",8,2,find_next(false));K("FindNextFileW",8,2,find_next(true));
    K("FindClose",4,1,[P,arg](HleCall& c)->std::uint32_t{return P->finds.erase(arg(c,0))?1:0;});
    K("GetFileAttributesExW",12,3,[P,arg](HleCall& c)->std::uint32_t{
        const auto name=P->wide_at(arg(c,0));const auto host=name?P->host_path(*name):std::nullopt;
        if(!host){P->last_error=ERROR_FILE_NOT_FOUND;return 0;}
        std::error_code ec;const bool dir=std::filesystem::is_directory(*host);const std::uint64_t size=dir?0:std::filesystem::file_size(*host,ec);
        std::uint32_t data[9]={dir?0x10U:0x80U,0,0,0,0,0,0,static_cast<std::uint32_t>(size>>32),static_cast<std::uint32_t>(size)};
        const std::uint64_t t=virtual_epoch_filetime;for(int k=0;k<3;++k)std::memcpy(&data[1+2*k],&t,8);
        c.guest.write(arg(c,2),data,sizeof data);return 1;});
    (void)ERROR_PATH_NOT_FOUND;(void)ERROR_NO_MORE_FILES;(void)ERROR_ALREADY_EXISTS;
}
} // namespace sadx
namespace sadx {
// dbghelp's PE header helpers over guest memory (the image as mapped).
void register_hle_dbghelp(HleProcess& p){
    auto& r=p.hle();
    r.add("dbghelp.dll","ImageNtHeader",{4,1,[](HleCall& c)->std::uint32_t{
        const auto base=c.argument(0).value_or(0);const auto lfanew=c.guest.u32(base+0x3c);return lfanew?base+*lfanew:0;}});
    r.add("dbghelp.dll","ImageDirectoryEntryToData",{16,4,[](HleCall& c)->std::uint32_t{
        const auto base=c.argument(0).value_or(0),entry=c.argument(2).value_or(0),size_out=c.argument(3).value_or(0);
        if(!c.argument(1).value_or(0)){c.guest.stop("ImageDirectoryEntryToData on a file-layout image is not implemented");return 0;}
        const auto lfanew=c.guest.u32(base+0x3c);if(!lfanew||entry>=16)return 0;
        const auto rva=c.guest.u32(base+*lfanew+24+96+8*entry),size=c.guest.u32(base+*lfanew+24+100+8*entry);
        if(!rva||!size||!*rva)return 0;if(size_out)c.guest.set_u32(size_out,*size);return base+*rva;}});
}
} // namespace sadx
