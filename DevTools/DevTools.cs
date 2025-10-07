using CefSharp;
using CefSharp.WinForms;
using System.Diagnostics;

namespace DevTools
{
    public partial class DevTools : Form
    {
        readonly ChromiumWebBrowser browser;

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

            Injector.OnInjected += Injector_OnInjected;
            Injector.OnInjectInvalid += Injector_OnInjectInvalid;
            Injector.OnInjectorStatusChanged += Injector_OnInjectorStatusChanged;

            if (Program.WasGModRunning)
            {
                Process.Start(Program.GModPath);
            }
        }

        private async void Injector_OnInjected(object sender, EventArgs e)
        {
            if (!browser.IsBrowserInitialized) return;
            if (browser.IsLoading) await browser.WaitForNavigationAsync();
            browser.ExecuteScriptAsync("dispatchEvent(new Event('injected'));");
        }

        private async void Injector_OnInjectInvalid(object sender, EventArgs e)
        {
            if (!browser.IsBrowserInitialized) return;
            if (browser.IsLoading) await browser.WaitForNavigationAsync();
            browser.ExecuteScriptAsync("dispatchEvent(new Event('uninjected'));");
        }

        private async void Injector_OnInjectorStatusChanged(object sender, EventArgs e)
        {
            if (!browser.IsBrowserInitialized) return;
            if (browser.IsLoading) await browser.WaitForNavigationAsync();
            var stat = (InjectorStatus)sender;
            browser.ExecuteScriptAsync("dispatchEvent(new CustomEvent('injectorstatus', {detail: " + (int)stat + "}))");
        }

        private void DevTools_Load(object sender, EventArgs e)
        {
            browser.LoadUrl("glectron://frontend/index.html");
        }
    }
}