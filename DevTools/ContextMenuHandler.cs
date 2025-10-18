using CefSharp;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace DevTools
{
    internal class ContextMenuHandler : IContextMenuHandler
    {
        public void OnBeforeContextMenu(IWebBrowser chromiumWebBrowser, IBrowser browser, IFrame frame, IContextMenuParams parameters, IMenuModel model)
        {
            //if (frame.Url.StartsWith("devtools://")) return; // Context Menu handling is broken in CEF if loaded manually.
            model.Clear();
#if DEBUG
            model.AddItem(CefMenuCommand.Reload, "Reload");
            model.AddItem(CefMenuCommand.UserFirst, "Inspect");
#endif
        }

        public bool OnContextMenuCommand(IWebBrowser chromiumWebBrowser, IBrowser browser, IFrame frame, IContextMenuParams parameters, CefMenuCommand commandId, CefEventFlags eventFlags)
        {
#if DEBUG
            if (commandId == CefMenuCommand.UserFirst)
            {
                chromiumWebBrowser.ShowDevTools();
                return true;
            }
#endif

            return false;
        }

        public void OnContextMenuDismissed(IWebBrowser chromiumWebBrowser, IBrowser browser, IFrame frame)
        {
        }

        public bool RunContextMenu(IWebBrowser chromiumWebBrowser, IBrowser browser, IFrame frame, IContextMenuParams parameters, IMenuModel model, IRunContextMenuCallback callback)
        {
            return false;
        }
    }
}
