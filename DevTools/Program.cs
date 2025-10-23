using CefSharp.WinForms;
using CefSharp;
using System.Diagnostics;

namespace DevTools
{
    internal static class Program
    {
        internal static DevTools? MainWindow = null;

        internal static Dictionary<int, Injector> Injectors = [];

        internal static event Action<int, Injector>? InjectorAdded;
        internal static event Action<int>? InjectorRemoved;
        internal static event Action<Injector, Injector.InjectStatus>? InjectorStatusChanged;
        internal static event Action<Injector, string?>? InjectorTitleChanged;

        private static Injector CreateInjector(Process proc)
        {
            var injector = new Injector(proc);
            injector.ProcessExited += (pid) =>
            {
                Injectors.Remove(pid);
                InjectorRemoved?.Invoke(pid);
            };
            injector.StatusChanged += (status) =>
            {
                InjectorStatusChanged?.Invoke(injector, status);
            };
            injector.TitleChanged += (title) =>
            {
                InjectorTitleChanged?.Invoke(injector, title);
            };
            Injectors.Add(proc.Id, injector);
            InjectorAdded?.Invoke(proc.Id, injector);
            return injector;
        }

        /// <summary>
        ///  The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();

            foreach (var proc in Process.GetProcessesByName("gmod"))
            {
                try
                {
                    var injector = CreateInjector(proc);
                    if (!Injector.HasChromiumLoaded(proc))
                    {
                        // Chromium hasn't loaded yet, try inject.
                        injector.Inject();
                    }
                } catch
                {
                }
            }

            // Keep monitoring new GMod processes.
            Task.Run(async () =>
            {
                while(true)
                {
                    var processes = Process.GetProcessesByName("gmod").Where((v) => !Injectors.ContainsKey(v.Id));
                    foreach (var proc in processes)
                    {
                        try
                        {
                            var injector = CreateInjector(proc);
                            injector.Inject();
                        } catch
                        {
                        }
                    }
                    await Task.Delay(100);
                }
            });

            var settings = new CefSettings();
            settings.RegisterScheme(new CefCustomScheme
            {
                SchemeName = "glectron",
                SchemeHandlerFactory = new SchemeHandlerFactory()
            });
            settings.CachePath = Path.Combine(Path.GetDirectoryName(Application.ExecutablePath)!, "cache");
            Cef.Initialize(settings);

            CefSharpSettings.ConcurrentTaskExecution = true;
            CefSharpSettings.RuntimeStyle = CefRuntimeStyle.Chrome;

            // To customize application configuration such as set high DPI settings or default font,
            // see https://aka.ms/applicationconfiguration.
            ApplicationConfiguration.Initialize();
            MainWindow = new DevTools();
            Application.Run(MainWindow);
        }
    }
}