using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Text;
using System.Threading.Tasks;

namespace DevTools
{
    internal class JavaScriptBridge
    {
        public class InjectorState
        {
            public int ProcessId;
            public Injector.InjectStatus Status;
            public string? Title;
            public int DebuggingPort;
        }

        public InjectorState[] GetInjectorState()
        {
            return [.. Program.Injectors.Select((v) =>
            {
                return new InjectorState
                {
                    ProcessId = v.Key,
                    Status = v.Value.Status,
                    Title = v.Value.Title,
                    DebuggingPort = v.Value.DebuggingPort
                };
            })];
        }

        public async Task<string> PerformRequest(int processId, string path)
        {
            var injector = Program.Injectors[processId] ?? throw new ArgumentException("Invalid process ID.");
            if (injector.DebuggingPort == 0)
                throw new ArgumentException($"{processId} isn't injected.");
            HttpClient hc = new();
            var res = await hc.GetAsync("http://localhost:" + injector.DebuggingPort + path);
            return await res.Content.ReadAsStringAsync();
        }

        public void OpenDevTools(int processId, string id, string ws, string? title)
        {
            if (Program.MainWindow == null) throw new InvalidOperationException("Main window is not available.");
            var injector = Program.Injectors[processId] ?? throw new ArgumentException("Invalid process ID.");
            Program.MainWindow.OpenDevToolsWindow(injector, id, ws, title);
        }
    }
}
