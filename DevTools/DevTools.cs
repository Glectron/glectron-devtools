using CefSharp;
using CefSharp.WinForms;
using System.Diagnostics;

namespace DevTools
{
    public partial class DevTools : Form
    {
        readonly ChromiumWebBrowser browser;

        readonly Dictionary<int, Dictionary<string, DevToolsFrame>> devToolsWindows = [];

        public DevTools()
        {
            InitializeComponent();

            browser = new()
            {
                Dock = DockStyle.Fill,
                MenuHandler = new ContextMenuHandler(),
                DragHandler = new DragHandler()
            };
            browser.JavascriptObjectRepository.Register("devtools", new JavaScriptBridge());
            Controls.Add(browser);

            Program.InjectorAdded += Program_InjectorAdded;
            Program.InjectorRemoved += Program_InjectorRemoved;
            Program.InjectorStatusChanged += Program_InjectorStatusChanged;
            Program.InjectorTitleChanged += Program_InjectorTitleChanged;

            foreach (var injector in Program.Injectors.Values)
            {
                devToolsWindows.Add(injector.TargetProcess.Id, []);
            }
        }

        private void Program_InjectorTitleChanged(Injector arg1, string? arg2)
        {
            UpdateInjectorState();
        }

        private void Program_InjectorStatusChanged(Injector arg1, Injector.InjectStatus arg2)
        {
            UpdateInjectorState();
        }

        private void Program_InjectorRemoved(int pid)
        {
            foreach (var wnd in devToolsWindows[pid].Values)
            {
                wnd.Invoke(wnd.Close);
            }
            Invoke(() => devToolsWindows.Remove(pid));
            UpdateInjectorState();
        }

        private void Program_InjectorAdded(int pid, Injector injector)
        {
            Invoke(() => devToolsWindows.Add(pid, []));
            UpdateInjectorState();
        }

        private async void UpdateInjectorState()
        {
            if (!browser.IsBrowserInitialized) return;
            if (browser.IsLoading) await browser.WaitForNavigationAsync();
            browser.ExecuteScriptAsync("dispatchEvent(new Event('updateinjectorstate'));");
        }

        private void DevTools_Load(object sender, EventArgs e)
        {
            browser.LoadUrl("glectron://frontend/index.html");
        }

        private void DevTools_FormClosed(object sender, FormClosedEventArgs e)
        {
            Application.Exit();
        }

        public void OpenDevToolsWindow(Injector injector, string id, string ws, string? title)
        {
            Invoke(() =>
            {
                var pid = injector.TargetProcess.Id;
                if (devToolsWindows[pid].TryGetValue(id, out DevToolsFrame? value))
                {
                    value.Activate();
                    return;
                }
                var wnd = new DevToolsFrame(id, ws)
                {
                    Text = title ?? id
                };
                wnd.Text += " (#" + pid + ")";
                wnd.FormClosed += (s, e) =>
                {
                    devToolsWindows[pid].Remove(id);
                };
                devToolsWindows[pid].Add(id, wnd);
                wnd.Show();
            });
        }

        public void SetDevToolsWindowTitle(Injector injector, string id, string? title)
        {
            Invoke(() =>
            {
                var pid = injector.TargetProcess.Id;
                if (devToolsWindows[pid].TryGetValue(id, out DevToolsFrame? value))
                {
                    value.Text = title ?? id;
                    value.Text += " (#" + pid + ")";
                }
            });
        }
    }
}