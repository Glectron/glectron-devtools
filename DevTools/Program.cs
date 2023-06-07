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

            var gmod = Process.GetProcessesByName("gmod");
            bool injected = false;
            foreach (var p in gmod)
                if (Injector.IsProcessInjected(p)) injected = true;
            if (gmod.Length > 0 && !injected)
            {
                var result = MessageBox.Show("Garry's Mod is running, would you like to restart it in order to use the DevTools?", "Question", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
                if (result == DialogResult.Yes)
                {
                    WasGModRunning = true;
                    foreach (var proc in gmod)
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