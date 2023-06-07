using CefSharp.WinForms;
using CefSharp;
using System.Diagnostics;

namespace DevTools
{
    internal static class Program
    {
        public static bool WasGModRunning = false;
        public static string GModPath = "";

        /// <summary>
        ///  The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();

            List<Process> gmodList = new();
            bool injected = false;
            var gmods = Process.GetProcessesByName("gmod");
            foreach (var proc in gmods)
            {
                if (Injector.IsProcessInjectable(proc)) gmodList.Add(proc);
                if (Injector.IsProcessInjected(proc)) injected = true;
            }
#if !X64
            var hl2s = Process.GetProcessesByName("hl2");
            foreach (var hl2 in hl2s)
            {
                if (!Injector.IsHL2GMod(hl2)) continue;
                if (Injector.IsProcessInjectable(hl2)) gmodList.Add(hl2);
                if (Injector.IsProcessInjected(hl2)) injected = true;
            }
#endif

            if (gmodList.Count > 0 && !injected)
            {
                var result = MessageBox.Show("Garry's Mod is running, would you like to restart it in order to use the DevTools?", "Question", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
                if (result == DialogResult.Yes)
                {
                    WasGModRunning = true;
                    foreach (var proc in gmodList)
                    {
                        try
                        {
                            if (proc.MainModule?.FileName != null)
                                GModPath = proc.MainModule.FileName;
                            proc.Kill();
                            proc.WaitForExit();
                        }
                        catch { }
                    }
                }
                else
                    return;
            }

            var settings = new CefSettings();
            settings.RegisterScheme(new CefCustomScheme
            {
                SchemeName = "glectron",
                SchemeHandlerFactory = new SchemeHandlerFactory()
            });
            Cef.Initialize(settings);

            CefSharpSettings.ConcurrentTaskExecution = true;

            // To customize application configuration such as set high DPI settings or default font,
            // see https://aka.ms/applicationconfiguration.
            ApplicationConfiguration.Initialize();
            Application.Run(new DevTools());
        }
    }
}