using CefSharp.WinForms;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace DevTools
{
    public partial class DevToolsFrame : Form
    {
        private readonly string ws;
        private readonly ChromiumWebBrowser browser;

        public DevToolsFrame(string ws)
        {
            InitializeComponent();

            this.ws = ws;

            browser = new ChromiumWebBrowser()
            {
                Dock = DockStyle.Fill,
                MenuHandler = new ContextMenuHandler(),
                DragHandler = new DragHandler()
            };
            Controls.Add(browser);
        }

        private void DevToolsFrame_Load(object sender, EventArgs e)
        {
            browser.Load("devtools://devtools/bundled/inspector.html?ws=" + ws);
        }
    }
}
