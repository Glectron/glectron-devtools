using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO.MemoryMappedFiles;
using System.Linq;
using System.Net.NetworkInformation;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace DevTools
{
    internal enum InjectorStatus
    {
        NoProcessFound,
        ProcessIncompatible,
        NoPortAvailable,
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
                try
                {
                    OnInjectorStatusChanged?.Invoke(value, new EventArgs());
                } catch
                {

                }
            }
        }

        public static int DebuggingPort { get; private set; } = 0;

        public delegate void InjectEventHandler(object sender, EventArgs e);

        public static event InjectEventHandler? OnInjected;
        public static event InjectEventHandler? OnInjectInvalid;
        public static event InjectEventHandler? OnInjectorStatusChanged;

        public static readonly string DllPath = Path.Combine(Path.GetDirectoryName(Environment.ProcessPath) ?? "", "chromium_hook.dll");

        public static int FindAvailablePort(int startPort = 1024, int endPort = 65535)
        {
            // Get all active TCP connections and listeners
            var ipGlobalProperties = IPGlobalProperties.GetIPGlobalProperties();
            var activePorts = new HashSet<int>();
            
            foreach (var endpoint in ipGlobalProperties.GetActiveTcpListeners())
            {
                activePorts.Add(endpoint.Port);
            }
            
            foreach (var connection in ipGlobalProperties.GetActiveTcpConnections())
            {
                activePorts.Add(connection.LocalEndPoint.Port);
            }

            foreach (var endpoint in ipGlobalProperties.GetActiveUdpListeners())
            {
                activePorts.Add(endpoint.Port);
            }

            // Start from a random port in the range
            Random random = new();
            int randomStart = random.Next(startPort, endPort + 1);
            
            // Check from random start to end of range
            for (int port = randomStart; port <= endPort; port++)
            {
                if (!activePorts.Contains(port))
                {
                    return port;
                }
            }
            
            // Check backwards from random start to start of range
            for (int port = randomStart - 1; port >= startPort; port--)
            {
                if (!activePorts.Contains(port))
                {
                    return port;
                }
            }
            
            throw new InvalidOperationException($"No available ports found in range {startPort}-{endPort}");
        }

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

        public static int GetInjectedPort(Process proc)
        {
            string shmName = $"GlectrionDevTools_{proc.Id}";
            try
            {
                using var mmf = MemoryMappedFile.OpenExisting(shmName);
                using var accessor = mmf.CreateViewAccessor();
                accessor.Read(0, out int port);
                return port;
            }
            catch
            {
                return -1;
            }
        }

        static bool Inject(Process proc, IntPtr process, int port)
        {
            string shmName = $"GlectrionDevToolsParam_{proc.Id}";
            var mmf = MemoryMappedFile.CreateNew(shmName, sizeof(int));
            using var accessor = mmf.CreateViewAccessor();

            accessor.Write(0, port);

            var hLib = GetModuleHandle("kernel32.dll");
            if (hLib == IntPtr.Zero) return false;
            var hProc = GetProcAddress(hLib, "LoadLibraryW");
            if (hProc == IntPtr.Zero) return false;

            var len = (DllPath.Length + 1) * 2;

            var remoteAddr = VirtualAllocEx(process, IntPtr.Zero, len, AllocationType.Commit | AllocationType.Reserve, MemoryProtection.ReadWrite);
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

            Task.Run(async () =>
            {
                await proc.WaitForExitAsync();
                mmf.Dispose();
            });

            return waitRet == WAIT_OBJECT_0;
        }

        static Injector()
        {
            new Thread(() =>
            {
                bool wasInjected = false;
                while (true)
                {
                    DebuggingPort = 0;
                    bool injected = false;
                    bool foundInjectable = false;
                    int port;
                    try
                    {
                        port = FindAvailablePort(1024, 65535);
                    }
                    catch
                    {
                        Status = InjectorStatus.NoPortAvailable;
                        continue;
                    }
                    var procs = new List<Process>();
                    procs.AddRange(Process.GetProcessesByName("gmod"));
                    foreach (var proc in procs)
                    {
                        if (!IsProcessInjectable(proc)) continue;
                        foundInjectable = true;
                        if (IsProcessInjected(proc))
                        {
                            var existingPort = GetInjectedPort(proc);
                            if (existingPort == -1)
                            {
                                Status = InjectorStatus.NoPortAvailable;
                                continue;
                            }
                            DebuggingPort = existingPort;
                            goto injected;
                        }
                        
                        var procHandle = OpenProcess((uint)(ProcessAccessFlags.VirtualMemoryOperation | ProcessAccessFlags.VirtualMemoryWrite | ProcessAccessFlags.CreateThread | ProcessAccessFlags.QueryInformation), false, (uint)proc.Id);
                        if (procHandle == IntPtr.Zero || !Inject(proc, procHandle, port))
                        {
                            Status = InjectorStatus.InjectFailed;
                            continue;
                        }
                        DebuggingPort = port;

                    injected:
                        Status = InjectorStatus.Injected;
                        wasInjected = true;
                        injected = true;
                        try
                        {
                            OnInjected?.Invoke(proc, new EventArgs());
                        }
                        catch
                        {

                        }
                        proc.WaitForExit();
                        try
                        {
                            OnInjectInvalid?.Invoke(proc, new EventArgs());
                        }
                        catch
                        {

                        }
                        Status = InjectorStatus.NoProcessFound;
                    }
                    if (procs.Count > 0 && !injected && !foundInjectable)
                        Status = InjectorStatus.ProcessIncompatible;
                    else if (procs.Count == 0 && wasInjected)
                        Status = InjectorStatus.NoProcessFound;
                    Thread.Sleep(300);
                }
            })
            { IsBackground = true }.Start();
        }
    }
}
