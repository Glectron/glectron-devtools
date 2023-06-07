using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace DevTools
{
    internal enum InjectorStatus
    {
        NoProcessFound,
        ProcessIncompatible,
        InjectFailed,
        Injected
    }

    internal static class Injector
    {
        [DllImport("kernel32.dll", SetLastError = true, CallingConvention = CallingConvention.Winapi)]
        [return: MarshalAs(UnmanagedType.Bool)]
        static extern bool IsWow64Process([In] IntPtr hProcess, [Out] out bool lpSystemInfo);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern IntPtr OpenProcess(
             uint processAccess,
             bool bInheritHandle,
             uint processId
        );
        [Flags]
        enum ProcessAccessFlags : uint
        {
            All = 0x001F0FFF,
            Terminate = 0x00000001,
            CreateThread = 0x00000002,
            VirtualMemoryOperation = 0x00000008,
            VirtualMemoryRead = 0x00000010,
            VirtualMemoryWrite = 0x00000020,
            DuplicateHandle = 0x00000040,
            CreateProcess = 0x000000080,
            SetQuota = 0x00000100,
            SetInformation = 0x00000200,
            QueryInformation = 0x00000400,
            QueryLimitedInformation = 0x00001000,
            Synchronize = 0x00100000
        }

        [DllImport("kernel32.dll", EntryPoint = "GetModuleHandleW", SetLastError = true, CharSet = CharSet.Unicode)]
        static extern IntPtr GetModuleHandle(string moduleName);

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Ansi)]
        static extern IntPtr GetProcAddress(IntPtr hModule, string procName);

        [DllImport("kernel32.dll", SetLastError = true, ExactSpelling = true)]
        static extern IntPtr VirtualAllocEx(IntPtr hProcess, IntPtr lpAddress,
            IntPtr dwSize, AllocationType flAllocationType, MemoryProtection flProtect);
        [Flags]
        enum AllocationType
        {
            Commit = 0x1000,
            Reserve = 0x2000,
            Decommit = 0x4000,
            Release = 0x8000,
            Reset = 0x80000,
            Physical = 0x400000,
            TopDown = 0x100000,
            WriteWatch = 0x200000,
            LargePages = 0x20000000
        }

        [Flags]
        enum MemoryProtection
        {
            Execute = 0x10,
            ExecuteRead = 0x20,
            ExecuteReadWrite = 0x40,
            ExecuteWriteCopy = 0x80,
            NoAccess = 0x01,
            ReadOnly = 0x02,
            ReadWrite = 0x04,
            WriteCopy = 0x08,
            GuardModifierflag = 0x100,
            NoCacheModifierflag = 0x200,
            WriteCombineModifierflag = 0x400
        }

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool WriteProcessMemory(
          IntPtr hProcess,
          IntPtr lpBaseAddress,
          IntPtr lpBuffer,
          int dwSize,
          out IntPtr lpNumberOfBytesWritten);

        [DllImport("kernel32.dll", SetLastError = true, ExactSpelling = true)]
        static extern bool VirtualFreeEx(IntPtr hProcess, IntPtr lpAddress,
            int dwSize, FreeType dwFreeType);
        [Flags]
        enum FreeType
        {
            Decommit = 0x4000,
            Release = 0x8000,
        }

        [DllImport("kernel32.dll")]
        static extern IntPtr CreateRemoteThread(IntPtr hProcess,
           IntPtr lpThreadAttributes, uint dwStackSize, IntPtr lpStartAddress,
           IntPtr lpParameter, uint dwCreationFlags, out IntPtr lpThreadId);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern int WaitForSingleObject(IntPtr handle, int wait);
        const int WAIT_OBJECT_0 = 0x00;
        const int WAIT_FAILED = -1;

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool CloseHandle(IntPtr hHandle);

        [DllImport("psapi.dll")]
        static extern uint GetModuleFileNameEx(IntPtr hProcess, IntPtr hModule, [Out] StringBuilder lpBaseName, [In] int nSize);

#if X64
        [DllImport("psapi.dll", CallingConvention = CallingConvention.StdCall, SetLastError = true)]
        static extern int EnumProcessModulesEx(IntPtr hProcess, [Out] IntPtr lphModule, uint cb, out uint lpcbNeeded, uint filters);
#else
        [DllImport("psapi.dll", CallingConvention = CallingConvention.StdCall, SetLastError = true)]
        static extern int EnumProcessModules(IntPtr hProcess, [Out] IntPtr lphModule, uint cb, out uint lpcbNeeded);
#endif

        static InjectorStatus _status;

        public static InjectorStatus Status
        {
            get { return _status; }
            private set {
                _status = value;
                OnInjectorStatusChanged?.Invoke(value, new EventArgs());
            }
        }

        public delegate void InjectEventHandler(object sender, EventArgs e);

        public static event InjectEventHandler? OnInjected;
        public static event InjectEventHandler? OnInjectInvalid;
        public static event InjectEventHandler? OnInjectorStatusChanged;

        public static readonly string DllPath = Path.Combine(Path.GetDirectoryName(Environment.ProcessPath) ?? "", "chromium_hook.dll");

        public static bool IsProcessInjected(Process proc)
        {
            var modName = Path.GetFullPath(DllPath);
#if WINDOWS
            var lowerModName = modName.ToLower();
#endif

            var hProc = OpenProcess((uint)(ProcessAccessFlags.QueryInformation | ProcessAccessFlags.VirtualMemoryRead), false, (uint)proc.Id);
            if (hProc == IntPtr.Zero) return false;

            IntPtr[] hMods = new IntPtr[1024];

            GCHandle handle = GCHandle.Alloc(hMods, GCHandleType.Pinned);
            IntPtr pModules = handle.AddrOfPinnedObject();

            uint size = (uint)(Marshal.SizeOf(typeof(IntPtr)) * hMods.Length);
#if X64
            int r = EnumProcessModulesEx(hProc, pModules, size, out uint cbNeeded, 0x03);
#else
            int r = EnumProcessModules(hProc, pModules, size, out uint cbNeeded);
#endif
            if (r == 0) return false;

            int modNum = (int)(cbNeeded / Marshal.SizeOf(typeof(IntPtr)));

            for (int i = 0; i < modNum; i++)
            {
                StringBuilder sb = new(256);

                uint ret = GetModuleFileNameEx(hProc, hMods[i], sb, sb.Capacity);
                if (ret == 0) continue;

                var module = Path.GetFullPath(sb.ToString());
#if WINDOWS
                var lowerModule = module.ToLower();
                if (lowerModName == lowerModule)
#else
                if (modName == module)
#endif
                    return true;
            }

            return false;
        }

        public static bool IsProcessInjectable(Process proc)
        {
            var procHandle = OpenProcess((uint)ProcessAccessFlags.QueryLimitedInformation, false, (uint)proc.Id);
            if (procHandle == IntPtr.Zero) return false;

            IsWow64Process(procHandle, out bool isWow64);
#if X64
            return !isWow64 && Environment.Is64BitOperatingSystem;
#else
            return (Environment.Is64BitOperatingSystem && isWow64) || !Environment.Is64BitOperatingSystem;
#endif
        }

        static bool Inject(IntPtr process)
        {
            var hLib = GetModuleHandle("kernel32.dll");
            if (hLib == IntPtr.Zero) return false;
            var hProc = GetProcAddress(hLib, "LoadLibraryW");
            if (hProc == IntPtr.Zero) return false;

            var len = (DllPath.Length + 1) * 2;

            var remoteAddr = VirtualAllocEx(process, IntPtr.Zero, (IntPtr)len, AllocationType.Commit | AllocationType.Reserve, MemoryProtection.ReadWrite);
            if (remoteAddr == IntPtr.Zero) return false;

            var dllPathPtr = Marshal.StringToHGlobalUni(DllPath);

            bool ret = WriteProcessMemory(process, remoteAddr, dllPathPtr, len, out _);
            if (!ret)
            {
                VirtualFreeEx(process, remoteAddr, len, FreeType.Release);
                return false;
            }

            int waitRet = WAIT_FAILED;
            IntPtr waitHandle = CreateRemoteThread(process, IntPtr.Zero, 0, hProc, remoteAddr, 0, out _);
            if (waitHandle != IntPtr.Zero)
            {
                waitRet = WaitForSingleObject(waitHandle, 10 * 1000);
                CloseHandle(waitHandle);
            }

            VirtualFreeEx(process, remoteAddr, len, FreeType.Release);

            return waitRet == WAIT_OBJECT_0;
        }

        static Injector()
        {
            new Thread(() =>
            {
                while (true)
                {
                    bool injected = false;
                    bool foundInjectable = false;
                    var procs = Process.GetProcessesByName("gmod");
                    foreach (var proc in procs)
                    {
                        if (!IsProcessInjectable(proc)) continue;
                        foundInjectable = true;
                        var procHandle = OpenProcess((uint)(ProcessAccessFlags.VirtualMemoryOperation | ProcessAccessFlags.VirtualMemoryWrite | ProcessAccessFlags.CreateThread | ProcessAccessFlags.QueryInformation), false, (uint)proc.Id);
                        if (procHandle != IntPtr.Zero)
                        {
                            if (IsProcessInjected(proc) || Inject(procHandle))
                            {
                                Status = InjectorStatus.Injected;
                                injected = true;
                                OnInjected?.Invoke(proc, new EventArgs());
                                proc.WaitForExit();
                                OnInjectInvalid?.Invoke(proc, new EventArgs());
                                Status = InjectorStatus.NoProcessFound;
                            } else
                            {
                                Status = InjectorStatus.InjectFailed;
                            }
                        }
                    }
                    if (procs.Length > 0 && !injected && !foundInjectable)
                        Status = InjectorStatus.ProcessIncompatible;
                    Thread.Sleep(300);
                }
            })
            { IsBackground = true }.Start();
        }
    }
}
