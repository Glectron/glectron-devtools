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
        public int[] InjectionStatus()
        {
            return [(int)Injector.Status, Injector.DebuggingPort];
        }

        public async Task<string> PerformRequest(string path)
        {
            HttpClient hc = new();
            var res = await hc.GetAsync("http://127.0.0.1:" + Injector.DebuggingPort + path);
            return await res.Content.ReadAsStringAsync();
        }

        public void OpenDevTools(string ws, string? title)
        {
            if (Program.MainWindow == null) throw new InvalidOperationException("Main window is not available.");
            Program.MainWindow.Invoke(() =>
            {
                var wnd = new DevToolsFrame(ws);
                if (title != null) wnd.Text = title;
                wnd.Show();
            });
        }
    }
}
