using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO.MemoryMappedFiles;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace DevTools
{
    public partial class Injector
    {
        public static readonly string DllPath = Path.Combine(Path.GetDirectoryName(Environment.ProcessPath) ?? "", "chromium_hook.dll");

        public enum InjectStatus
        {
            NotInjected,
            Injected,
            InjectFailed,
            NotInjectable,
            IsSubprocess,
            NoAvailablePort
        }

        public Process TargetProcess { get; private set; }

        public int DebuggingPort { get; private set; }

        private InjectStatus _status;
        public InjectStatus Status
        {
            get
            {
                return _status;
            }
            private set
            {
                _status = value;
                StatusChanged?.Invoke(value);
            }
        }

        private string? _title;
        public string? Title
        {
            get
            {
                return _title;
            }
            set
            {
                var changed = _title != value;
                _title = value;
                if (changed)
                    TitleChanged?.Invoke(value);
            }
        }

        public event Action<InjectStatus>? StatusChanged;
        public event Action<int>? ProcessExited;
        public event Action<string?>? TitleChanged;

        private readonly CancellationTokenSource _cts = new();

        public Injector(Process process)
        {
            TargetProcess = process;
            if (IsSubprocess(process))
            {
                Status = InjectStatus.IsSubprocess;
                return;
            }
            DebuggingPort = GetInjectedPort(process);
            Title = process.MainWindowTitle;
            process.EnableRaisingEvents = true;
            process.Exited += OnProcessExited;
            if (DebuggingPort != 0)
            {
                Status = InjectStatus.Injected;
            }
            else
            {
                Status = InjectStatus.NotInjected;
            }
            Task.Run(async () =>
            {
                try
                {
                    while (!_cts.IsCancellationRequested)
                    {
                        Title = GetWindowTitle(process.MainWindowHandle);
                        await Task.Delay(1000, _cts.Token);
                    }
                } catch (OperationCanceledException)
                {
                    return;
                }
            });
        }

        ~Injector()
        {
            TargetProcess.Exited -= OnProcessExited;
            _cts.Cancel();
            _cts.Dispose();
        }

        private void OnProcessExited(object? sender, EventArgs e)
        {
            ProcessExited?.Invoke(TargetProcess.Id);
        }

        public bool Inject(int? debuggingPort = null)
        {
            int port;
            try
            {
                port = debuggingPort ?? FindAvailablePort();
            } catch (InvalidOperationException)
            {
                Status = InjectStatus.NoAvailablePort;
                return false;
            }
            if (!IsInjectable(TargetProcess))
            {
                Status = InjectStatus.NotInjectable;
                return false;
            }
            if (IsSubprocess(TargetProcess))
            {
                Status = InjectStatus.IsSubprocess;
                return false;
            }

            string shmName = $"GlectrionDevToolsParam_{TargetProcess.Id}";
            using var mmf = MemoryMappedFile.CreateNew(shmName, sizeof(int));
            using var accessor = mmf.CreateViewAccessor();

            accessor.Write(0, port);

            var hLib = GetModuleHandle("kernel32.dll");
            if (hLib == IntPtr.Zero)
            {
                Status = InjectStatus.InjectFailed;
                return false;
            }
            var hProc = GetProcAddress(hLib, "LoadLibraryW");
            if (hProc == IntPtr.Zero)
            {
                Status = InjectStatus.InjectFailed;
                return false;
            }

            var procHandle = OpenProcess((uint)(ProcessAccessFlags.VirtualMemoryOperation | ProcessAccessFlags.VirtualMemoryWrite | ProcessAccessFlags.CreateThread | ProcessAccessFlags.QueryInformation), false, (uint)TargetProcess.Id);
            if (procHandle == IntPtr.Zero)
            {
                Status = InjectStatus.InjectFailed;
                return false;
            }

            var len = (DllPath.Length + 1) * 2;

            var remoteAddr = VirtualAllocEx(procHandle, IntPtr.Zero, len, AllocationType.Commit | AllocationType.Reserve, MemoryProtection.ReadWrite);
            if (remoteAddr == IntPtr.Zero)
            {
                Status = InjectStatus.InjectFailed;
                return false;
            }

            var dllPathPtr = Marshal.StringToHGlobalUni(DllPath);

            bool ret = WriteProcessMemory(procHandle, remoteAddr, dllPathPtr, len, out _);
            if (!ret)
            {
                VirtualFreeEx(procHandle, remoteAddr, len, FreeType.Release);
                CloseHandle(procHandle);
                Status = InjectStatus.InjectFailed;
                return false;
            }

            int waitRet = WAIT_FAILED;
            IntPtr waitHandle = CreateRemoteThread(procHandle, IntPtr.Zero, 0, hProc, remoteAddr, 0, out _);
            if (waitHandle != IntPtr.Zero)
            {
                waitRet = WaitForSingleObject(waitHandle, 10 * 1000);
                CloseHandle(waitHandle);
            }

            VirtualFreeEx(procHandle, remoteAddr, len, FreeType.Release);
            CloseHandle(procHandle);

            if (waitRet != WAIT_OBJECT_0)
            {
                Status = InjectStatus.InjectFailed;
                return false;
            }

            DebuggingPort = port;
            Status = InjectStatus.Injected;

            return true;
        }
    }
}
