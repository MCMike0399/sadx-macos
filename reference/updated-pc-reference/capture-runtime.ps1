# Windows 11 x64, Windows PowerShell 5.1 or PowerShell 7. No downloads/install.
# Default: read-only process observation. Thread suspension is opt-in only.
[CmdletBinding()]
param(
    [string]$GameDirectory = '',
    [ValidateRange(0, 4294967295)][uint32]$TargetProcessId = 0,
    [string]$OutputDirectory = '',
    [switch]$AtlasOSReported,
    [switch]$CaptureThreadState
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$expectedManifestSha = '795918d0e40d30437a0fde905d40917cac861ef46b3e86dff0cffdac901f8cd8'
$expectedHookFileSha = '8b5300ee71ef42083759ab23169e1f7eb9922ab5b166f68da44e086b11925ad8'
$expectedEffectiveConfigSha = '94d26126a135e667f664cfd370e57425663e1be3b108c427df7705014b7a58e3'
$result = [ordered]@{
    format_version = 1; state = 'failed'; evidence_type = 'native Windows process snapshot'
    started_utc = [DateTime]::UtcNow.ToString('o'); ended_utc = $null
    atlas_os = @{ user_reported = [bool]$AtlasOSReported; independently_detected = $false }
    thread_state_requested = [bool]$CaptureThreadState
    observer = @{ target_launch = $false; target_termination = $false; target_memory_write = $false
                  injection = $false; default_thread_suspension = $false
                  cadence_trace = $false; snapshot_not_tick_trace = $true }
    profile = $null; hardware = @{}; process = $null; modules = @(); required_modules = @()
    hooks = @(); threads = @(); errors = [Collections.Generic.List[object]]::new()
    cleanup = @(); limitations = @(
        'Disk file hashes do not prove loaded-image byte identity.',
        'Config files verified at capture time do not prove which values Init previously consumed.',
        'Hook/module snapshots race with initialization, unloading and execution; no simultaneous snapshot.',
        'Native x64/WOW64 architecture checks exclude ARM64 translation, not every possible virtual machine.',
        'Thread state sampling briefly suspends one thread at a time and perturbs target execution.',
        'No physical input, gameplay, per-tick trace, frame cadence or speedrun acceptance is verified.'
    )
}
$outputPath = $null
$processHandle = $null

function Record-Failure([string]$tag, [string]$operation, [string]$message, $nativeCode = $null) {
    $result.errors.Add([ordered]@{ tag = $tag; operation = $operation; message = $message; win32_error = $nativeCode })
}
function File-Sha([string]$path) { return (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() }
function Same-Path([string]$left, [string]$right) {
    return [StringComparer]::OrdinalIgnoreCase.Equals([IO.Path]::GetFullPath($left), [IO.Path]::GetFullPath($right))
}
function Distinct-ModuleMappings([object[]]$records) {
    $distinct = [Collections.Generic.List[object]]::new()
    foreach ($record in $records) {
        $duplicate = $false
        foreach ($prior in $distinct) {
            if ($record.base_address_u64 -eq $prior.base_address_u64 -and
                $record.image_size -eq $prior.image_size -and (Same-Path $record.path $prior.path)) {
                if ($record.disk_sha256 -ne $prior.disk_sha256) { throw 'Duplicate module observations disagree on disk identity.' }
                $duplicate = $true
                break
            }
        }
        if (-not $duplicate) { $distinct.Add($record) }
    }
    return $distinct.ToArray()
}
function Game-Path([string]$relative) {
    if (-not $relative.StartsWith('game/', [StringComparison]::Ordinal)) { throw 'Invalid manifest game-relative path.' }
    $suffix = $relative.Substring(5).Replace('/', [IO.Path]::DirectorySeparatorChar)
    $path = [IO.Path]::GetFullPath((Join-Path $GameDirectory $suffix))
    $prefix = $GameDirectory.TrimEnd([IO.Path]::DirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
    if (-not $path.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) { throw 'Manifest path escaped selected game directory.' }
    return $path
}

$nativeSource = @'
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;

namespace SadxReferenceRuntimeV1 {
 public struct FileTime { public uint Low, High; public ulong Raw { get { return ((ulong)High << 32) | Low; } } }
 [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
 public struct ModuleEntry {
  public uint Size, ModuleId, ProcessId, GlobalUsage, ProcessUsage;
  public IntPtr Base; public uint ImageSize; public IntPtr Module;
  [MarshalAs(UnmanagedType.ByValTStr, SizeConst=256)] public string Name;
  [MarshalAs(UnmanagedType.ByValTStr, SizeConst=260)] public string Path;
 }
 [StructLayout(LayoutKind.Sequential)]
 public struct ThreadEntry { public uint Size, Usage, Id, ProcessId; public int BasePriority, DeltaPriority; public uint Flags; }
 [StructLayout(LayoutKind.Sequential, Pack=4)]
 public struct FloatSave {
  public uint ControlWord, StatusWord, TagWord, ErrorOffset, ErrorSelector, DataOffset, DataSelector;
  [MarshalAs(UnmanagedType.ByValArray, SizeConst=80)] public byte[] RegisterArea;
  public uint Cr0NpxState;
 }
 [StructLayout(LayoutKind.Sequential, Pack=4)]
 public struct WowContext {
  public uint Flags, Dr0, Dr1, Dr2, Dr3, Dr6, Dr7; public FloatSave FloatSave;
  public uint SegGs, SegFs, SegEs, SegDs, Edi, Esi, Ebx, Edx, Ecx, Eax;
  public uint Ebp, Eip, SegCs, EFlags, Esp, SegSs;
  [MarshalAs(UnmanagedType.ByValArray, SizeConst=512)] public byte[] ExtendedRegisters;
 }
 public class ModuleFact { public string Path, Name; public ulong Base; public uint ImageSize; }
 public class MappedPathFact { public string DevicePath, DiskPath; public int Win32Error; }
 public class ReadFact { public bool Complete, ClockComplete; public byte[] Bytes; public ulong BytesRead; public int Win32Error; public long QpcBefore, QpcAfter; }
 public class ThreadFact {
  public uint ThreadId, OwnerProcessId; public string CreationFileTime100ns;
  public string State = "failed", FailedOperation, ErrorMessage; public int? Win32Error;
  public bool SuspendedByCollector, ResumeSucceeded, CloseSucceeded, ContextSucceeded, FlagsComplete, ClockComplete;
  public uint? SuspendCountBefore, ResumeCountBefore, ReturnedFlags, Eip, Esp;
  public uint? X87ControlWordRaw, X87StatusWordRaw, X87TagWordRaw, MxcsrFromFxsaveLayout;
  public string RegisterArea80BytesHex, ExtendedRegisters512BytesHex;
  public string[] RawTenByteSaveSlots; public long QpcBefore, QpcAfter;
 }
 public class CleanupFact { public string Operation, Owner, State; public int Win32Error; public bool NeedsResume; public uint? ExpectedResumeCount, ResumeCountObserved; internal IntPtr Handle; }
 public sealed class ProcessReference : IDisposable {
  internal IntPtr Handle; public uint Id; public string ImagePath, CreationFileTime100ns, CreationUtc;
  public ushort ProcessMachine, NativeMachine;
  public ProcessReference(uint id) {
   Handle = Native.OpenProcess(0x00101410, false, id); // synchronize | query information/limited | VM read
   if (Handle == IntPtr.Zero) throw Native.Error("OpenProcess");
   try {
    Id = id; EnsureAlive(); ImagePath = Native.ImagePath(Handle);
    FileTime c,e,k,u; if (!Native.GetProcessTimes(Handle,out c,out e,out k,out u)) throw Native.Error("GetProcessTimes");
    CreationFileTime100ns = c.Raw.ToString(); CreationUtc = DateTime.FromFileTimeUtc((long)c.Raw).ToString("o");
    ushort process,native; if (!Native.IsWow64Process2(Handle,out process,out native)) throw Native.Error("IsWow64Process2");
    ProcessMachine=process; NativeMachine=native;
   } catch { Dispose(); throw; }
  }
  public void EnsureAlive() {
   uint wait = Native.WaitForSingleObject(Handle,0);
   if (wait == 0) throw new InvalidOperationException("Bound process has exited; no PID reselection is permitted.");
   if (wait != 258) throw Native.Error("WaitForSingleObject(process)");
  }
  public bool IdentityStillMatches() {
   EnsureAlive(); FileTime c,e,k,u;
   if (!Native.GetProcessTimes(Handle,out c,out e,out k,out u)) throw Native.Error("GetProcessTimes(recheck)");
   return c.Raw.ToString()==CreationFileTime100ns && String.Equals(Native.ImagePath(Handle),ImagePath,StringComparison.OrdinalIgnoreCase);
  }
  public ReadFact Read(ulong address, int length) {
   EnsureAlive(); ReadFact fact = new ReadFact(); byte[] bytes = new byte[length]; UIntPtr count;
   bool clockBefore=Native.QueryPerformanceCounter(out fact.QpcBefore);
   bool ok = Native.ReadProcessMemory(Handle,new IntPtr((long)address),bytes,new UIntPtr((uint)length),out count);
   fact.Win32Error=ok ? 0 : Marshal.GetLastWin32Error(); fact.BytesRead=count.ToUInt64();
   bool clockAfter=Native.QueryPerformanceCounter(out fact.QpcAfter); fact.ClockComplete=clockBefore && clockAfter;
   int retained=(int)Math.Min(fact.BytesRead,(ulong)length); fact.Bytes=new byte[retained]; Array.Copy(bytes,fact.Bytes,retained);
   fact.Complete=ok && fact.BytesRead==(ulong)length; return fact;
  }
  public void Dispose() {
   if (Handle!=IntPtr.Zero) { Native.CloseOwned(Handle,"process "+Id,false); Handle=IntPtr.Zero; }
  }
 }
 public static class Native {
  public const string BuildSourceHash = "__SOURCE_HASH__";
  internal static readonly List<CleanupFact> Pending = new List<CleanupFact>();
  public static readonly List<CleanupFact> CleanupEvents = new List<CleanupFact>();
  static readonly IntPtr InvalidHandle = new IntPtr(-1);
  [DllImport("kernel32.dll",SetLastError=true)] internal static extern IntPtr OpenProcess(uint rights,bool inherit,uint id);
  [DllImport("kernel32.dll",SetLastError=true)] internal static extern bool GetProcessTimes(IntPtr h,out FileTime c,out FileTime e,out FileTime k,out FileTime u);
  [DllImport("kernel32.dll",CharSet=CharSet.Unicode,SetLastError=true)] static extern bool QueryFullProcessImageNameW(IntPtr h,uint flags,StringBuilder path,ref uint size);
  [DllImport("kernel32.dll",SetLastError=true)] internal static extern bool IsWow64Process2(IntPtr h,out ushort process,out ushort native);
  [DllImport("kernel32.dll",SetLastError=true)] internal static extern uint WaitForSingleObject(IntPtr h,uint milliseconds);
  [DllImport("kernel32.dll",SetLastError=true)] static extern bool CloseHandle(IntPtr h);
  [DllImport("kernel32.dll",SetLastError=true)] internal static extern bool ReadProcessMemory(IntPtr h,IntPtr address,[Out]byte[] bytes,UIntPtr size,out UIntPtr read);
  [DllImport("kernel32.dll",SetLastError=true)] static extern IntPtr CreateToolhelp32Snapshot(uint flags,uint id);
  [DllImport("kernel32.dll",CharSet=CharSet.Unicode,SetLastError=true)] static extern bool Module32FirstW(IntPtr h,ref ModuleEntry entry);
  [DllImport("kernel32.dll",CharSet=CharSet.Unicode,SetLastError=true)] static extern bool Module32NextW(IntPtr h,ref ModuleEntry entry);
  [DllImport("psapi.dll",EntryPoint="GetMappedFileNameW",CharSet=CharSet.Unicode,ExactSpelling=true,SetLastError=true)] static extern uint GetMappedFileName(IntPtr h,IntPtr address,StringBuilder name,uint capacity);
  [DllImport("kernel32.dll",EntryPoint="QueryDosDeviceW",CharSet=CharSet.Unicode,ExactSpelling=true,SetLastError=true)] static extern uint QueryDosDevice(string device,StringBuilder target,uint capacity);
  [DllImport("kernel32.dll",SetLastError=true)] static extern bool Thread32First(IntPtr h,ref ThreadEntry entry);
  [DllImport("kernel32.dll",SetLastError=true)] static extern bool Thread32Next(IntPtr h,ref ThreadEntry entry);
  [DllImport("kernel32.dll",SetLastError=true)] static extern IntPtr OpenThread(uint rights,bool inherit,uint id);
  [DllImport("kernel32.dll",SetLastError=true)] static extern uint GetProcessIdOfThread(IntPtr h);
  [DllImport("kernel32.dll",SetLastError=true)] static extern bool GetThreadTimes(IntPtr h,out FileTime c,out FileTime e,out FileTime k,out FileTime u);
  [DllImport("kernel32.dll",SetLastError=true)] static extern uint Wow64SuspendThread(IntPtr h);
  [DllImport("kernel32.dll",SetLastError=true)] static extern bool Wow64GetThreadContext(IntPtr h,ref WowContext context);
  [DllImport("kernel32.dll",SetLastError=true)] static extern uint ResumeThread(IntPtr h);
  [DllImport("kernel32.dll",SetLastError=true)] public static extern bool QueryPerformanceFrequency(out long value);
  [DllImport("kernel32.dll",SetLastError=true)] internal static extern bool QueryPerformanceCounter(out long value);
  internal static Win32Exception Error(string operation) { int code=Marshal.GetLastWin32Error(); return new Win32Exception(code,operation+" failed (Win32 "+code+")."); }
  public static void CheckLayout() {
   if (IntPtr.Size!=8 || Marshal.SizeOf(typeof(ModuleEntry))!=1080 || Marshal.SizeOf(typeof(ThreadEntry))!=28 ||
       Marshal.SizeOf(typeof(FloatSave))!=112 || Marshal.SizeOf(typeof(WowContext))!=716 ||
       Marshal.OffsetOf(typeof(WowContext),"FloatSave").ToInt32()!=28 ||
       Marshal.OffsetOf(typeof(WowContext),"Eip").ToInt32()!=184 ||
       Marshal.OffsetOf(typeof(WowContext),"ExtendedRegisters").ToInt32()!=204)
    throw new InvalidOperationException("Unexpected x64 collector/WOW64 structure layout.");
  }
  internal static string ImagePath(IntPtr h) {
   StringBuilder path=new StringBuilder(32768); uint length=(uint)path.Capacity;
   if (!QueryFullProcessImageNameW(h,0,path,ref length)) throw Error("QueryFullProcessImageNameW");
   return path.ToString();
  }
  internal static bool CloseOwned(IntPtr h,string owner,bool needsResume) {
   bool ok=CloseHandle(h); int error=ok ? 0 : Marshal.GetLastWin32Error();
   if (!ok) {
    CleanupFact fact=new CleanupFact { Operation="CloseHandle",Owner=owner,State="failed-handle-retained",Win32Error=error,Handle=h,NeedsResume=needsResume };
    Pending.Add(fact); CleanupEvents.Add(fact);
   }
   return ok;
  }
  public static CleanupFact[] PendingCleanup() { return Pending.ToArray(); }
  public static void BeginCleanupAudit() {
   if(Pending.Count!=0) throw new InvalidOperationException("Previous capture retains owned cleanup; retry it in this same shell before another capture.");
   CleanupEvents.Clear();
  }
  // Retry only handles/increments already owned by this collector. Never reopen a PID/TID.
  public static void RetryPendingCleanup() {
   for (int i=Pending.Count-1;i>=0;i--) {
    CleanupFact fact=Pending[i];
    if (fact.NeedsResume) {
     uint previous=ResumeThread(fact.Handle);
     if (previous==UInt32.MaxValue) {
      fact.Win32Error=Marshal.GetLastWin32Error();
      if (WaitForSingleObject(fact.Handle,0)!=0) continue;
      fact.State="target-thread-terminated-before-resume";
     } else {
      fact.NeedsResume=false; fact.ResumeCountObserved=previous;
      fact.State=fact.ExpectedResumeCount.HasValue && previous==fact.ExpectedResumeCount.Value
          ? "own-suspend-increment-released" : "suspend-count-changed-concurrently; ownership-attribution-uncertain";
     }
    }
    if (!CloseHandle(fact.Handle)) { fact.Win32Error=Marshal.GetLastWin32Error(); continue; }
    fact.State += "; handle-closed"; Pending.RemoveAt(i);
   }
  }
  public static uint[] ExactPathProcesses(string expected) {
   List<uint> found=new List<uint>();
   foreach (Process process in Process.GetProcesses()) {
    using(process) {
     IntPtr h=OpenProcess(0x1000,false,(uint)process.Id);
     if (h==IntPtr.Zero) continue;
     try { if (String.Equals(Path.GetFullPath(ImagePath(h)),Path.GetFullPath(expected),StringComparison.OrdinalIgnoreCase)) found.Add((uint)process.Id); }
     catch (Win32Exception) { /* inaccessible unrelated process: not persisted */ }
     finally { CloseOwned(h,"candidate process handle",false); }
    }
   }
   return found.ToArray();
  }
  static IntPtr Snapshot(uint flags,uint id) {
   for (int retry=0;retry<8;retry++) {
    IntPtr h=CreateToolhelp32Snapshot(flags,id);
    if (h!=InvalidHandle) return h;
    if (Marshal.GetLastWin32Error()!=24) throw Error("CreateToolhelp32Snapshot");
   }
   throw Error("CreateToolhelp32Snapshot(ERROR_BAD_LENGTH after eight attempts)");
  }
  public static ModuleFact[] Modules(ProcessReference process) {
   process.EnsureAlive(); List<ModuleFact> modules=new List<ModuleFact>(); IntPtr h=Snapshot(0x18,process.Id);
   try {
    ModuleEntry entry=new ModuleEntry(); entry.Size=(uint)Marshal.SizeOf(typeof(ModuleEntry));
    if (!Module32FirstW(h,ref entry)) { if (Marshal.GetLastWin32Error()==18) return modules.ToArray(); throw Error("Module32FirstW"); }
    do {
     if (entry.ProcessId!=process.Id) throw new InvalidOperationException("Module snapshot owner changed.");
     modules.Add(new ModuleFact { Path=entry.Path,Name=entry.Name,Base=(ulong)entry.Base.ToInt64(),ImageSize=entry.ImageSize });
     entry.Size=(uint)Marshal.SizeOf(typeof(ModuleEntry));
    } while (Module32NextW(h,ref entry));
    if (Marshal.GetLastWin32Error()!=18) throw Error("Module32NextW");
    process.EnsureAlive(); return modules.ToArray();
   } finally { CloseOwned(h,"module snapshot",false); }
  }
  public static string DevicePathToDrivePath(string path,string drive,string device) {
   if (String.IsNullOrEmpty(path) || String.IsNullOrEmpty(device) || String.IsNullOrEmpty(drive)) return null;
   if (!path.StartsWith(device,StringComparison.OrdinalIgnoreCase) || path.Length<=device.Length || path[device.Length]!='\\') return null;
   return drive + path.Substring(device.Length);
  }
  public static MappedPathFact MappedFilePath(ProcessReference process,ulong address) {
   process.EnsureAlive(); MappedPathFact fact=new MappedPathFact();
   StringBuilder path=new StringBuilder(32768);
   uint count=GetMappedFileName(process.Handle,new IntPtr(unchecked((long)address)),path,(uint)path.Capacity);
   if (count==0) { fact.Win32Error=Marshal.GetLastWin32Error(); return fact; }
   if (count>=path.Capacity) { fact.Win32Error=122; return fact; }
   fact.DevicePath=path.ToString();
   foreach(string root in Directory.GetLogicalDrives()) {
    if(root.Length<2) continue;
    string drive=root.Substring(0,2);StringBuilder device=new StringBuilder(32768);
    if(QueryDosDevice(drive,device,(uint)device.Capacity)==0) continue;
    string resolved=DevicePathToDrivePath(fact.DevicePath,drive,device.ToString());
    if(resolved!=null) { fact.DiskPath=resolved;break; }
   }
   process.EnsureAlive();return fact;
  }
  public static uint[] Threads(ProcessReference process) {
   process.EnsureAlive(); List<uint> threads=new List<uint>(); IntPtr h=Snapshot(4,0);
   try {
    ThreadEntry entry=new ThreadEntry(); entry.Size=(uint)Marshal.SizeOf(typeof(ThreadEntry));
    if (!Thread32First(h,ref entry)) { if (Marshal.GetLastWin32Error()==18) return threads.ToArray(); throw Error("Thread32First"); }
    do { if (entry.ProcessId==process.Id) threads.Add(entry.Id); entry.Size=(uint)Marshal.SizeOf(typeof(ThreadEntry)); } while(Thread32Next(h,ref entry));
    if (Marshal.GetLastWin32Error()!=18) throw Error("Thread32Next");
    process.EnsureAlive(); return threads.ToArray();
   } finally { CloseOwned(h,"thread snapshot",false); }
  }
  public static string Hex(byte[] bytes) { return BitConverter.ToString(bytes).Replace("-","").ToLowerInvariant(); }
  public static uint DecodeX86RelativeJump(uint source,byte[] bytes) {
   if(bytes.Length<5 || bytes[0]!=0xe9) throw new ArgumentException("Requires a complete x86 E9 rel32 instruction.");
   return unchecked((uint)((long)source+5+BitConverter.ToInt32(bytes,1))); // x86 address wrap
  }
  public static ThreadFact CaptureThread(ProcessReference process,uint threadId) {
   process.EnsureAlive(); ThreadFact fact=new ThreadFact { ThreadId=threadId }; IntPtr h=IntPtr.Zero; bool ownIncrement=false, clockBefore=false; string operation="OpenThread";
   try {
    h=OpenThread(0x0010080a,false,threadId); // synchronize | query limited | get context | suspend/resume
    if(h==IntPtr.Zero) throw Error(operation);
    operation="GetProcessIdOfThread"; fact.OwnerProcessId=GetProcessIdOfThread(h);
    if(fact.OwnerProcessId==0) throw Error(operation);
    if(fact.OwnerProcessId!=process.Id) throw new InvalidOperationException("Thread no longer belongs to bound process.");
    operation="GetThreadTimes"; FileTime c,e,k,u; if(!GetThreadTimes(h,out c,out e,out k,out u)) throw Error(operation);
    fact.CreationFileTime100ns=c.Raw.ToString(); process.EnsureAlive();
    operation="Wow64SuspendThread"; uint previous=Wow64SuspendThread(h); if(previous==UInt32.MaxValue) throw Error(operation);
    ownIncrement=true; fact.SuspendedByCollector=true; fact.SuspendCountBefore=previous;
    clockBefore=QueryPerformanceCounter(out fact.QpcBefore);
    WowContext context=new WowContext { Flags=0x0001002f,FloatSave=new FloatSave { RegisterArea=new byte[80] },ExtendedRegisters=new byte[512] };
    operation="Wow64GetThreadContext"; if(!Wow64GetThreadContext(h,ref context)) throw Error(operation);
    fact.ContextSucceeded=true; fact.ReturnedFlags=context.Flags; fact.FlagsComplete=(context.Flags & 0x1002f)==0x1002f;
    if((context.Flags & 0x10001)==0x10001) { fact.Eip=context.Eip; fact.Esp=context.Esp; }
    if((context.Flags & 0x10008)==0x10008) {
     fact.X87ControlWordRaw=context.FloatSave.ControlWord; fact.X87StatusWordRaw=context.FloatSave.StatusWord; fact.X87TagWordRaw=context.FloatSave.TagWord;
     fact.RegisterArea80BytesHex=Hex(context.FloatSave.RegisterArea); fact.RawTenByteSaveSlots=new string[8];
     for(int i=0;i<8;i++) { byte[] slot=new byte[10]; Array.Copy(context.FloatSave.RegisterArea,i*10,slot,0,10); fact.RawTenByteSaveSlots[i]=Hex(slot); }
    }
    if((context.Flags & 0x10020)==0x10020) { fact.ExtendedRegisters512BytesHex=Hex(context.ExtendedRegisters); fact.MxcsrFromFxsaveLayout=BitConverter.ToUInt32(context.ExtendedRegisters,24); }
    fact.State=fact.FlagsComplete ? "context-captured" : "partial-context-flags";
   } catch(Exception error) {
    fact.FailedOperation=operation; fact.ErrorMessage=error.Message;
    Win32Exception native=error as Win32Exception; if(native!=null) fact.Win32Error=native.NativeErrorCode;
   } finally {
    // Only our own successful increment is released; never resume to zero in a loop.
    if(ownIncrement) {
     uint previous=ResumeThread(h); fact.ResumeCountBefore=previous;
     if(previous==UInt32.MaxValue) {
      CleanupFact pending=new CleanupFact { Operation="ResumeThread",Owner="thread "+threadId+" created "+fact.CreationFileTime100ns,State="failed-own-increment-retained",Win32Error=Marshal.GetLastWin32Error(),NeedsResume=true,ExpectedResumeCount=fact.SuspendCountBefore.Value+1,Handle=h };
      Pending.Add(pending); CleanupEvents.Add(pending); fact.State="cleanup-failed"; h=IntPtr.Zero; // retained by Pending, not closed
     } else {
      fact.ResumeSucceeded=true;
      if(fact.SuspendCountBefore.HasValue && previous!=fact.SuspendCountBefore.Value+1) fact.State="suspend-count-changed-concurrently";
     }
    }
    bool clockAfter=QueryPerformanceCounter(out fact.QpcAfter); fact.ClockComplete=clockBefore && clockAfter;
    if(h!=IntPtr.Zero) fact.CloseSucceeded=CloseOwned(h,"thread "+threadId,false);
   }
   return fact;
  }
 }
}
'@

try {
    if ([Environment]::OSVersion.Platform -ne [PlatformID]::Win32NT -or -not [Environment]::Is64BitProcess) {
        throw 'Use 64-bit Windows PowerShell on the selected Windows 11 PC.'
    }
    if ([string]::IsNullOrWhiteSpace($GameDirectory)) {
        $GameDirectory = Join-Path $PSScriptRoot 'game'
    }
    $GameDirectory = (Resolve-Path -LiteralPath $GameDirectory).ProviderPath
    if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
        $OutputDirectory = Join-Path $PSScriptRoot ('captures/' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss') + '-' + [Guid]::NewGuid().ToString('N').Substring(0,8))
    }
    $OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
    if ($OutputDirectory.StartsWith($GameDirectory.TrimEnd('\') + '\',[StringComparison]::OrdinalIgnoreCase) -or (Same-Path $OutputDirectory $GameDirectory)) { throw 'Capture output must be outside the game directory.' }
    if (Test-Path -LiteralPath $OutputDirectory) { throw 'Capture output directory already exists; overwrite refused.' }
    [void][IO.Directory]::CreateDirectory($OutputDirectory)
    $outputPath = Join-Path $OutputDirectory 'runtime.json'
    $manifestPath = Join-Path $PSScriptRoot 'manifest.json'
    if ((File-Sha $manifestPath) -ne $expectedManifestSha) { throw 'Pinned manifest SHA-256 mismatch; process capture refused.' }
    $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    $inventoryPath = Join-Path $PSScriptRoot 'game-inventory.json'
    if ((File-Sha $inventoryPath) -ne $manifest.game_inventory_sha256) { throw 'Pinned game inventory SHA-256 mismatch.' }
    # Windows PowerShell 5.1 sends a JSON array as one pipeline object.
    # Wrapping that pipeline in @() creates a nested array, whose .path/.sha256
    # properties enumerate every file instead of selecting one inventory row.
    [object[]]$inventory = (Get-Content -LiteralPath $inventoryPath -Raw | ConvertFrom-Json)
    $hooksPath = Join-Path $PSScriptRoot 'expected-hooks.json'
    if ((File-Sha $hooksPath) -ne $expectedHookFileSha) { throw 'Pinned hook specification SHA-256 mismatch.' }
    $hookSpecification = Get-Content -LiteralPath $hooksPath -Raw | ConvertFrom-Json
    $effectiveConfigPath = Join-Path $PSScriptRoot 'effective-config.json'
    if ((File-Sha $effectiveConfigPath) -ne $expectedEffectiveConfigSha) { throw 'Pinned typed config interpretation SHA-256 mismatch.' }
    $effectiveConfig = Get-Content -LiteralPath $effectiveConfigPath -Raw | ConvertFrom-Json
    $selectedFiles = @('game/sonic.exe','game/system/CHRMODELS.dll','game/system/CHRMODELS_orig.dll',
        'game/mods/sadx-input-mod/sadx-input-mod.dll','game/mods/sadx-input-mod/SDL2.dll',
        'game/mods/sadx-speedrun-mod/sadx-speedrun-mod.dll','game/mods/sadx-input-mod/mod.ini',
        'game/mods/sadx-speedrun-mod/mod.ini') + @($manifest.raw_configs | ForEach-Object { $_.path })
    $verifiedFiles = @()
    foreach ($relative in ($selectedFiles | Select-Object -Unique)) {
        $entry = @($inventory | Where-Object { $_.path -ceq $relative })
        if ($entry.Count -ne 1) { throw "Selected file absent/ambiguous in pinned inventory: $relative" }
        $path = Game-Path $relative
        $digest = File-Sha $path
        if ($digest -ne $entry[0].sha256 -or (Get-Item -LiteralPath $path).Length -ne $entry[0].size) { throw "Profile file mismatch: $relative" }
        $verifiedFiles += [ordered]@{ path = $path; sha256 = $digest; size = $entry[0].size }
    }
    foreach ($relative in $manifest.absent_executable_code_files) {
        if (Test-Path -LiteralPath (Join-Path $GameDirectory $relative)) { throw "Unexpected executable code file: $relative" }
    }
    if (Test-Path -LiteralPath (Join-Path $GameDirectory 'mods/sadx-input-mod/gamecontrollerdb.txt')) { throw 'Unexpected controller mapping database; this would change the pinned input profile.' }
    $profiles = Get-Content -LiteralPath (Game-Path 'game/mods/.modloader/profiles/Profiles.json') -Raw | ConvertFrom-Json
    $profileName = $profiles.ProfilesList[$profiles.ProfileIndex].Filename
    $loadedProfile = Get-Content -LiteralPath (Game-Path ('game/mods/.modloader/profiles/' + $profileName)) -Raw | ConvertFrom-Json
    $result.profile = [ordered]@{ manifest_sha256 = $expectedManifestSha; game_directory = $GameDirectory
        raw_file_verification = $verifiedFiles; loader_profile_filename = $profileName; loader_profile = $loadedProfile
        input_source_interpretation = $effectiveConfig.input; speedrun_source_interpretation = $effectiveConfig.speedrun
        frame_limit_source_interpretation = $effectiveConfig.frame_limit_integer
        verification_scope = 'selected executable, DLLs, metadata and raw configs; other game assets/saves not rehashed by this collector'
        runtime_values_consumed_by_init = 'unknown'; module_activation = 'not yet inspected' }
    $sourceBytes = [Text.Encoding]::UTF8.GetBytes($nativeSource)
    $sourceSha = [BitConverter]::ToString([Security.Cryptography.SHA256]::Create().ComputeHash($sourceBytes)).Replace('-','').ToLowerInvariant()
    $existingType = 'SadxReferenceRuntimeV1.Native' -as [type]
    if ($null -ne $existingType -and [SadxReferenceRuntimeV1.Native]::BuildSourceHash -ne $sourceSha) { throw 'A different collector was compiled in this shell; open a fresh 64-bit PowerShell shell.' }
    if ($null -eq $existingType) { Add-Type -TypeDefinition ($nativeSource.Replace('__SOURCE_HASH__',$sourceSha)) -Language CSharp }
    [SadxReferenceRuntimeV1.Native]::CheckLayout()
    [SadxReferenceRuntimeV1.Native]::BeginCleanupAudit()
    $result.observer['script_sha256'] = File-Sha $PSCommandPath
    $result.observer['embedded_csharp_source_sha256'] = $sourceSha
    $result.observer['powershell_version'] = $PSVersionTable.PSVersion.ToString()
    $executable = Game-Path 'game/sonic.exe'
    if ($TargetProcessId -eq 0) {
        $exactProcessIds = @([SadxReferenceRuntimeV1.Native]::ExactPathProcesses($executable))
        if ($exactProcessIds.Count -ne 1) { throw "Found $($exactProcessIds.Count) processes with the exact selected executable path. Start this copy manually or supply -TargetProcessId for one of those exact-path processes." }
        $TargetProcessId = [uint32]$exactProcessIds[0]
    }
    $processHandle = [SadxReferenceRuntimeV1.ProcessReference]::new([uint32]$TargetProcessId)
    if (-not (Same-Path $processHandle.ImagePath $executable)) { throw 'Selected PID does not run the exact selected sonic.exe path.' }
    if ($processHandle.ProcessMachine -ne 0x14c -or $processHandle.NativeMachine -ne 0x8664) { throw 'Require i386 game under WOW64 on native x64 Windows; ARM64/other architectures refused.' }
    $result.process = [ordered]@{ process_id = $TargetProcessId; image_path = $processHandle.ImagePath
        creation_filetime_100ns = $processHandle.CreationFileTime100ns; creation_utc = $processHandle.CreationUtc
        process_machine = '0x014c'; native_machine = '0x8664'; identity_rechecked_at_end = $false
        runtime_command_line = 'not collected'; runtime_working_directory = 'not verified' }
    foreach ($query in @(
        @{ Key='cpu'; Class='Win32_Processor'; Fields=@('Name','Manufacturer','Architecture','NumberOfCores','NumberOfLogicalProcessors','MaxClockSpeed') },
        @{ Key='gpu'; Class='Win32_VideoController'; Fields=@('Name','AdapterCompatibility','DriverVersion','DriverDate') },
        @{ Key='windows'; Class='Win32_OperatingSystem'; Fields=@('Caption','Version','BuildNumber','OSArchitecture') }
    )) {
        try {
            $facts = @(Get-CimInstance -ClassName $query.Class | Select-Object -Property $query.Fields)
            if ($facts.Count -eq 0) { throw 'No hardware/OS facts returned by the selected local CIM query.' }
            $result.hardware[$query.Key] = $facts
        }
        catch { Record-Failure 'hardware_query_failed' $query.Class $_.Exception.Message }
    }
    [long]$qpcFrequency = 0
    if (-not [SadxReferenceRuntimeV1.Native]::QueryPerformanceFrequency([ref]$qpcFrequency)) { throw 'QueryPerformanceFrequency failed.' }
    $result.hardware['qpc_frequency_hz'] = $qpcFrequency
    $result.hardware['game_selected_gpu'] = 'not verified; reported GPU list is installed adapters'
    $nativeModules = @([SadxReferenceRuntimeV1.Native]::Modules($processHandle))
    foreach ($module in $nativeModules) {
        $moduleFact = [ordered]@{ path=$module.Path; name=$module.Name; base_address=('0x{0:x}' -f $module.Base)
            base_address_u64=$module.Base; image_size=$module.ImageSize; disk_sha256=$null; hash_kind='current on-disk file, not loaded memory image'
            disk_hash_path=$module.Path; mapped_device_path=$null; path_lookup_error=$null }
        try { $moduleFact.disk_sha256 = File-Sha $module.Path }
        catch {
            $reportedPathFailure = $_.Exception.Message
            try {
                $mappedPath = [SadxReferenceRuntimeV1.Native]::MappedFilePath($processHandle,$module.Base)
                $moduleFact.mapped_device_path = $mappedPath.DevicePath
                if ([string]::IsNullOrWhiteSpace($mappedPath.DiskPath)) {
                    throw ('Mapped file path could not be resolved; Win32 ' + $mappedPath.Win32Error)
                }
                $moduleFact.disk_hash_path = $mappedPath.DiskPath
                $moduleFact.disk_sha256 = File-Sha $mappedPath.DiskPath
            } catch {
                $moduleFact.path_lookup_error = [ordered]@{ reported_path_failure=$reportedPathFailure; mapped_lookup_failure=$_.Exception.Message }
                Record-Failure 'module_file_hash_failed' $module.Path $_.Exception.Message
            }
        }
        $result.modules += $moduleFact
    }
    # Retain every raw record; exact duplicate mappings do not create a second
    # executable. Different bases/sizes/paths or conflicting hashes stay distinct.
    $moduleMappings = @(Distinct-ModuleMappings $result.modules)
    foreach ($relative in @('game/sonic.exe','game/system/CHRMODELS.dll','game/system/CHRMODELS_orig.dll',
        'game/mods/sadx-input-mod/sadx-input-mod.dll','game/mods/sadx-input-mod/SDL2.dll','game/mods/sadx-speedrun-mod/sadx-speedrun-mod.dll')) {
        $wantedPath = Game-Path $relative
        $wantedEntry = @($inventory | Where-Object { $_.path -ceq $relative })[0]
        $rawLoaded = @($result.modules | Where-Object { Same-Path $_.path $wantedPath })
        $loaded = @($moduleMappings | Where-Object { Same-Path $_.path $wantedPath })
        $matched = $loaded.Count -eq 1 -and $loaded[0].disk_sha256 -eq $wantedEntry.sha256
        $result.required_modules += [ordered]@{ expected_path=$wantedPath; expected_sha256=$wantedEntry.sha256; exact_path_and_disk_hash_match=$matched; observed_count=$rawLoaded.Count; distinct_mapping_count=$loaded.Count }
        if (-not $matched) { Record-Failure 'required_module_not_verified' $wantedPath 'Required exact module path/hash not present in observed snapshot.' }
    }
    $exeModule = @($moduleMappings | Where-Object { Same-Path $_.path $executable })
    if ($exeModule.Count -ne 1 -or $exeModule[0].base_address_u64 -ne 0x400000) { throw 'Executable base is not the pinned absolute-address layout; hook reads refused.' }
    foreach ($hook in $hookSpecification.entries) {
        $address = [Convert]::ToUInt64($hook.va.Substring(2),16)
        $read = $processHandle.Read($address,16)
        $observed = [SadxReferenceRuntimeV1.Native]::Hex($read.Bytes)
        $prefixMatch = $read.Complete -and $observed.StartsWith($hook.expected_live_prefix,[StringComparison]::Ordinal)
        $entry = [ordered]@{ va=$hook.va; operation=$hook.operation; expected_owner=$hook.owner
            read_complete=$read.Complete; bytes_read=$read.BytesRead; observed_hex=$observed
            expected_prefix=$hook.expected_live_prefix; prefix_matches=$prefixMatch; win32_error=$read.Win32Error
            qpc_before=$read.QpcBefore; qpc_after=$read.QpcAfter; qpc_complete=$read.ClockComplete
            jump_target=$null; destination_verified=$null }
        if ($null -ne $hook.required_target_module) {
            $destinationOk = $false
            if ($read.Complete -and $read.Bytes[0] -eq 0xe9) {
                [long]$destination = [SadxReferenceRuntimeV1.Native]::DecodeX86RelativeJump([uint32]$address,$read.Bytes)
                $entry.jump_target = '0x{0:x}' -f $destination
                if ($hook.required_target_module -eq 'sonic.exe:0x40fdb3') { $destinationOk = $destination -eq 0x40fdb3 }
                else {
                    $relative = switch ($hook.required_target_module) {
                        'sadx-speedrun-mod.dll' { 'game/mods/sadx-speedrun-mod/sadx-speedrun-mod.dll' }
                        'sadx-input-mod.dll' { 'game/mods/sadx-input-mod/sadx-input-mod.dll' }
                        'CHRMODELS.dll' { 'game/system/CHRMODELS.dll' }
                        default { throw 'Unknown destination owner in pinned specification.' }
                    }
                    $targetPath = Game-Path $relative
                    $targetModule = @($moduleMappings | Where-Object { Same-Path $_.path $targetPath })
                    if ($targetModule.Count -eq 1) { $destinationOk = $destination -ge $targetModule[0].base_address_u64 -and $destination -lt ($targetModule[0].base_address_u64 + $targetModule[0].image_size) }
                }
            }
            $entry.destination_verified = $destinationOk
            if (-not $destinationOk) { Record-Failure 'hook_destination_not_verified' $hook.va 'Jump destination did not match the selected expected module/address.' }
        }
        $result.hooks += $entry
        if (-not $read.ClockComplete) { Record-Failure 'qpc_timestamp_failed' $hook.va 'QPC timestamp sampling was incomplete; byte read facts are retained separately.' }
        if (-not $prefixMatch) { Record-Failure 'hook_readback_not_verified' $hook.va 'Pinned live prefix not read completely/matched.' $read.Win32Error }
    }
    if ($CaptureThreadState) {
        $threadIds = @([SadxReferenceRuntimeV1.Native]::Threads($processHandle))
        if ($threadIds.Count -eq 0) { Record-Failure 'no_target_threads' 'Thread32 snapshot' 'No target threads observed.' }
        foreach ($threadId in $threadIds) {
            $thread = [SadxReferenceRuntimeV1.Native]::CaptureThread($processHandle,$threadId)
            $result.threads += $thread
            if ($thread.State -ne 'context-captured' -or -not $thread.ResumeSucceeded -or -not $thread.CloseSucceeded -or -not $thread.ClockComplete) {
                Record-Failure 'thread_snapshot_partial' ('thread ' + $threadId) $thread.State $thread.Win32Error
            }
        }
    }
    if (-not $processHandle.IdentityStillMatches()) { throw 'Bound process identity changed during capture.' }
    $result.process.identity_rechecked_at_end = $true
    foreach ($entry in $verifiedFiles) { if ((File-Sha $entry.path) -ne $entry.sha256) { throw 'A selected on-disk profile file changed during capture.' } }
    $result.profile.module_activation = 'required module paths/hashes and selected hooks inspected; evaluate individual results'
    $result.state = if ($result.errors.Count -eq 0) { 'required-readbacks-complete' } else { 'partial' }
} catch {
    $code = $null
    $errorObject = $_.Exception
    while ($null -ne $errorObject.InnerException) { $errorObject = $errorObject.InnerException }
    if ($errorObject -is [ComponentModel.Win32Exception]) { $code = $errorObject.NativeErrorCode }
    Record-Failure 'capture_failed' 'collector' $errorObject.Message $code
    $result.state = if ($null -ne $result.process) { 'partial' } else { 'failed' }
} finally {
    if ($null -ne $processHandle) { $processHandle.Dispose() }
    if ($null -ne ('SadxReferenceRuntimeV1.Native' -as [type])) {
        [SadxReferenceRuntimeV1.Native]::RetryPendingCleanup()
        $result.cleanup = @([SadxReferenceRuntimeV1.Native]::CleanupEvents)
        $pending = @([SadxReferenceRuntimeV1.Native]::PendingCleanup())
        if ($result.cleanup.Count -gt 0 -or $pending.Count -gt 0) {
            $result.state = 'partial'
            Record-Failure 'cleanup_not_clean' 'owned handles/suspend increments' 'Cleanup failure occurred; inspect retained facts even if a retry recovered it.'
        }
        if ($pending.Count -gt 0) {
            Write-Warning 'Owned cleanup failed. Keep this PowerShell session open; retained handles remain available through [SadxReferenceRuntimeV1.Native]::RetryPendingCleanup(). Do not reopen thread IDs or resume unrelated threads.'
        }
    }
    $result.ended_utc = [DateTime]::UtcNow.ToString('o')
    if ($null -ne $outputPath) {
        [IO.File]::WriteAllText($outputPath,($result | ConvertTo-Json -Depth 30),[Text.UTF8Encoding]::new($false))
        Write-Host ('Capture state: ' + $result.state)
        Write-Host ('Report: ' + $outputPath)
    } else {
        $result | ConvertTo-Json -Depth 30
    }
}
if ($result.state -ne 'required-readbacks-complete') {
    foreach ($captureFailure in $result.errors) {
        Write-Host ('[' + $captureFailure.tag + '] ' + $captureFailure.operation + ': ' + $captureFailure.message)
    }
    $global:LASTEXITCODE = 1
    throw ('Capture is ' + $result.state + '; inspect the recorded errors. No successful complete capture is claimed.')
}
$global:LASTEXITCODE = 0
