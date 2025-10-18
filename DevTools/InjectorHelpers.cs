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
    public partial class Injector
    {
        public static string? GetWindowTitle(IntPtr hWnd)
        {
            var length = GetWindowTextLength(hWnd) + 1;
            var title = new char[length];
            GetWindowText(hWnd, title, length);
            return title.ToString();
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
                return 0;
            }
        }

        public static bool IsInjectable(Process proc)
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

        private static string[] GetModules(Process proc)
        {
            var hProc = OpenProcess((uint)(ProcessAccessFlags.QueryInformation | ProcessAccessFlags.VirtualMemoryRead), false, (uint)proc.Id);
            if (hProc == IntPtr.Zero) return [];

            IntPtr[] hMods = new IntPtr[1024];

            GCHandle handle = GCHandle.Alloc(hMods, GCHandleType.Pinned);
            IntPtr pModules = handle.AddrOfPinnedObject();

            uint size = (uint)(Marshal.SizeOf(typeof(IntPtr)) * hMods.Length);
#if X64
            int r = EnumProcessModulesEx(hProc, pModules, size, out uint cbNeeded, 0x03);
#else
            int r = EnumProcessModules(hProc, pModules, size, out uint cbNeeded);
#endif
            if (r == 0) return [];

            int modNum = (int)(cbNeeded / Marshal.SizeOf(typeof(IntPtr)));

            List<string> modules = [];

            for (int i = 0; i < modNum; i++)
            {
                var buf = new char[256];

                uint ret = GetModuleFileNameEx(hProc, hMods[i], buf, buf.Length);
                if (ret == 0) continue;

                var moduleName = buf.ToString();
                if (moduleName == null) continue;

                var module = Path.GetFullPath(moduleName);
                var lowerModule = module.ToLower();
                modules.Add(lowerModule);
            }

            return [.. modules];
        }

        public static bool IsInjected(Process proc)
        {
            return GetModules(proc).Any((v) => v == Path.GetFullPath(DllPath));
        }

        public static bool HasChromiumLoaded(Process proc)
        {
            return GetModules(proc).Any((v) =>
            {
                var name = Path.GetFileName(v);
                return name == "html_chromium.dll" || name == "libcef.dll";
            });
        }

        public static bool IsSubprocess(Process proc)
        {
            var handle = OpenProcess((uint)ProcessAccessFlags.QueryLimitedInformation, false, (uint)proc.Id);
            if (handle == IntPtr.Zero) return false;
            ProcessBasicInformation pbi = new();
            int status = NtQueryInformationProcess(handle, 0, ref pbi, (uint)Marshal.SizeOf(pbi), out _);
            CloseHandle(handle);
            if (status != 0) return false;
            var parentProcessId = pbi.InheritedFromUniqueProcessId.ToInt32();
            try
            {
                var process = Process.GetProcessById(parentProcessId);
                if (proc.MainModule == null || process.MainModule == null) return false;
                if (Path.GetFullPath(proc.MainModule.FileName).Equals(Path.GetFullPath(process.MainModule.FileName), StringComparison.CurrentCultureIgnoreCase))
                {
                    return true;
                }
            }
            catch (ArgumentException)
            {
                return false;
            }
            return false;
        }

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
    }
}
