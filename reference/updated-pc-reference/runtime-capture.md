# Capture the updated PC reference on Windows

The reference PC is **Intel Ultra + NVIDIA, Windows 11 with AtlasOS**, as reported by the user. Those hardware/OS facts have not been observed remotely. This collector records actual CPU/GPU driver/build facts when run locally; the AtlasOS marker remains explicitly **user reported**, with no registry-based detection claim.

Copy this entire `updated-pc-reference` folder to the Windows PC. Open its `game` folder and start **sonic.exe** manually. Once the game reaches its menu, open a **64-bit Windows PowerShell** terminal in the outer `updated-pc-reference` folder and run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\capture-runtime.ps1 -AtlasOSReported
```

The command prints the exact report location under a new `captures` folder. Return that **runtime.json** file. This does not start the game, install software, change AtlasOS, change timer/graphics settings, inject code, write game memory, terminate the game, or suspend threads. Execution-policy bypass applies only to this PowerShell process; it does not change the machine policy. If several processes run this exact copied executable, close the extra copies manually or supply `-TargetProcessId` from the intended copy. Matching by a similar executable name is never used.

For the separate floating-point snapshot, use an existing 64-bit PowerShell terminal in the same outer folder and run:

```powershell
.\capture-runtime.ps1 -AtlasOSReported -CaptureThreadState
```

That explicit option briefly suspends **one selected target thread at a time**, reads its WOW64 context, then releases exactly the collector's own suspension increment in `finally`. It records original suspend count, context flags, thread creation identity, raw x87 control/status/tag words, raw 80-byte register save area, eight raw 10-byte save slots, 512-byte extended-register block, MXCSR interpretation and EIP/ESP. It does not resume a thread to zero in a loop or change register values. These interrupted snapshots are **not frame-cadence or gameplay-tick traces**; interruption can perturb scheduling/locks and the game continues between different thread samples. Don't use this option during a timed attempt.

If cleanup reports a retained handle/suspension failure, keep that same PowerShell terminal open. The collector retains only its own handle and can retry with `[SadxReferenceRuntimeV1.Native]::RetryPendingCleanup()` in that terminal. It does not reopen thread IDs, because those could have been reused. The report remains partial even if an automatic retry recovered cleanup; the original failure is preserved. Process termination, closing the collector externally or a forced host kill cannot be made safe by a normal language `finally`; no unconditional cleanup guarantee is claimed.

## What is verified and what remains unknown

Before process observation, the script checks the **hard-coded exact manifest SHA-256**, the pinned inventory/spec/config interpretation hashes, copied sonic.exe, selected DLLs, both mods' metadata, raw loader/input/Speedrun configs, and absence of executable code files or an unexpected controller mapping database. It verifies only selected critical files on Windows, not every copied asset/save. The immutable manifest is unchanged:

`795918d0e40d30437a0fde905d40917cac861ef46b3e86dff0cffdac901f8cd8`.

It selects the exact full executable path in the requested game directory, then binds an open process handle to PID and **GetProcessTimes creation timestamp**. That handle's liveness/identity is checked again before finishing; PID reuse does not select another process. **IsWow64Process2** must report process machine **0x014c** and native machine **0x8664**. This excludes ARM64 translation but does not detect every possible virtual machine or establish physical bare-metal execution from architecture fields alone.

The default collector reads:

* Actual toolhelp module paths, base addresses, image sizes and current on-disk SHA-256. Required modules must match exact selected paths and hashes; Windows/System DLL paths are retained as observed local provenance. On-disk SHA-256 is **not** a loaded-memory image digest.
* The source-bound required hook addresses from `expected-hooks.json`, 16 bytes each, through **ReadProcessMemory**. It records partial failures, compares expected prefixes, decodes x86 E9 jumps with proper 32-bit address wrap, and checks destination module bounds/expected addresses. These are consistency checks for selected writes, not a complete loaded-code audit or proof every callback behaved correctly.
* Explicit local CPU name/manufacturer/core/architecture facts, installed GPU names/driver versions/dates, Windows caption/version/build/architecture, and **QueryPerformanceFrequency**. Installed GPU facts do not identify which adapter the game actually selected. No usernames, hostnames, serial/license identifiers, process command lines, network state or ambient environment are collected.
* Exact raw config file hashes and pinned typed source interpretation. The script cannot prove when/which values mod Init consumed merely by observing current disk files. Runtime working directory/command-line options are unverified, and source interpretation is labeled separately from observation.

All failures are tagged, partial facts retained, and partial/failed captures produce an error exit rather than a success claim. Missing Win32/CIM access or unavailable services on AtlasOS are reported; no privilege escalation or service/config repair occurs. Module snapshots can race with load/unload and cannot enumerate modules loaded with data-file-only flags. Hook reads and thread samples are not a simultaneous global snapshot.

Actual physical keyboard/controller events, simulation tick/IGT cadence, player/collision/RNG traces, category fixture saves, NVIDIA renderer selection and numerical comparisons against the native Mac port remain later work. A clean module/hook snapshot alone does not satisfy those requirements or establish leaderboard acceptance.

## Review and local validation

The script targets baseline Windows PowerShell **5.1** or PowerShell **7** in a 64-bit host and embeds standalone C# through `Add-Type`. No third-party packages or application/runtime installation are performed. Process rights are synchronize/query-limited/VM-read; optional thread rights are synchronize/query-limited/get-context/suspend-resume. No WriteProcessMemory, SetThreadContext, remote-thread, debugger attachment, launch or termination API is present. Only newly opened handles are closed; failed cleanup handles remain owned in a pending collection.

On Macintosh, the embedded C# compiled with **C# language version 5** and warnings as errors using the existing .NET SDK. Representation checks passed for x64 MODULEENTRY32W **1080 bytes**, THREADENTRY32 **28**, WOW64_FLOATING_SAVE_AREA **112**, WOW64_CONTEXT **716**, FloatSave offset **28**, EIP offset **184**, ExtendedRegisters offset **204**. Known rel32 arithmetic tests covered a negative displacement and wrapping to a high x86 address. Those tests called **no Windows API**.

PowerShell/pwsh and System.Management.Automation were not found on PATH or checked conventional local installation roots, so a PowerShell parser/runtime test was **not available here**. Windows P/Invoke behavior, PowerShell-host integration, permissions, thread suspend/resume and real process capture remain **unverified until an actual native Windows run**. Local structure compilation is not simulated Windows runtime verification.

## Primary API/layout evidence

Microsoft documentation read for this implementation:

* [IsWow64Process2](https://learn.microsoft.com/en-us/windows/win32/api/wow64apiset/nf-wow64apiset-iswow64process2) and [machine constants](https://learn.microsoft.com/en-us/windows/win32/sysinfo/image-file-machine-constants).
* [GetProcessTimes](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getprocesstimes), [GetThreadTimes](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getthreadtimes), [QueryFullProcessImageNameW](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-queryfullprocessimagenamew) and [GetProcessIdOfThread](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getprocessidofthread) for identity/owner facts.
* [CreateToolhelp32Snapshot](https://learn.microsoft.com/en-us/windows/win32/api/tlhelp32/nf-tlhelp32-createtoolhelp32snapshot), [MODULEENTRY32W](https://learn.microsoft.com/en-us/windows/win32/api/tlhelp32/ns-tlhelp32-moduleentry32w), [THREADENTRY32](https://learn.microsoft.com/en-us/windows/win32/api/tlhelp32/ns-tlhelp32-threadentry32).
* [ReadProcessMemory](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-readprocessmemory) and [QueryPerformanceFrequency](https://learn.microsoft.com/en-us/windows/win32/api/profileapi/nf-profileapi-queryperformancefrequency).
* [Wow64GetThreadContext](https://learn.microsoft.com/en-us/windows/win32/api/wow64apiset/nf-wow64apiset-wow64getthreadcontext) requires a suspended thread for a valid context; [Wow64SuspendThread](https://learn.microsoft.com/en-us/windows/win32/api/wow64apiset/nf-wow64apiset-wow64suspendthread) returns the prior suspend count; [ResumeThread](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-resumethread) decrements that count.
* [WOW64_CONTEXT](https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-wow64_context), [WOW64_FLOATING_SAVE_AREA](https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-wow64_floating_save_area), and Microsoft's [published WinNT.h](https://github.com/microsoft/win32metadata/blob/main/generation/WinSDK/RecompiledIdlHeaders/um/winnt.h) establish pack-4 layout, 80/512-byte arrays and flags **0x1002f**. XSAVE_FORMAT places MXCSR at offset **24** within the CPU-specific extension; raw bytes are retained with the FXSAVE interpretation explicitly named, rather than substituting a host float/register representation.

No undocumented TLS slot or invented AtlasOS registry location is read.
