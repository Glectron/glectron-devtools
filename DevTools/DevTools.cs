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
            };
            browser.AddressChanged += Browser_AddressChanged;
            browser.MenuHandler = new ContextMenuHandler();
            Controls.Add(browser);

            Injector.OnInjected += Injector_OnInjected;
            Injector.OnInjectInvalid += Injector_OnInjectInvalid;
            Injector.OnInjectorStatusChanged += Injector_OnInjectorStatusChanged;

            if (Program.WasGModRunning)
            {
                Process.Start(Program.GModPath);
            }
        }

        private void Injector_OnInjected(object sender, EventArgs e)
        {
            if (IsBrowserAtHomepage())
            {
                browser.ExecuteScriptAsync("dispatchEvent(new Event('injected'));");
            }
        }

        private void Injector_OnInjectInvalid(object sender, EventArgs e)
        {
            if (IsBrowserAtHomepage())
            {
                browser.ExecuteScriptAsync("dispatchEvent(new Event('uninjected'));");
            }
        }

        private void Injector_OnInjectorStatusChanged(object sender, EventArgs e)
        {
            if (IsBrowserAtHomepage())
            {
                var stat = (InjectorStatus)sender;
                browser.ExecuteScriptAsync("dispatchEvent(new CustomEvent('injectorstatus', {detail: " + (int)stat + "}))");
            }
        }

        private bool IsBrowserAtHomepage()
        {
            var uri = new Uri(browser.Address);
            return uri.Scheme == "glectron" && uri.Host == "frontend" && uri.AbsolutePath == "/index.html";
        }

        private void Browser_AddressChanged(object? sender, EventArgs e)
        {
            Invoke(() =>
            {
                Text = IsBrowserAtHomepage() ? "Glectron DevTools" : "Inspecting Glectron application (click close to return homepage)";
            });
            if (IsBrowserAtHomepage())
            {
                if (!browser.JavascriptObjectRepository.IsBound("devtools"))
                    browser.JavascriptObjectRepository.Register("devtools", new JavaScriptBridge());
            }
            else
            {
                browser.JavascriptObjectRepository.UnRegisterAll();
            }
        }

        private void DevTools_Load(object sender, EventArgs e)
        {
            browser.LoadUrl("glectron://frontend/index.html");
        }

        private void DevTools_FormClosing(object sender, FormClosingEventArgs e)
        {
            if (!IsBrowserAtHomepage())
            {
                browser.LoadUrl("glectron://frontend/index.html");
                e.Cancel = true;
            }
        }
    }
}