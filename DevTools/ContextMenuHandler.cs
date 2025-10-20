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
            if (frame.Url.StartsWith("devtools://")) return; // Use default context menus in DevTools
            model.Clear();
#if DEBUG
            model.AddItem(CefMenuCommand.UserFirst, "Reload");
            model.AddItem((CefMenuCommand)(int)CefMenuCommand.UserFirst + 1, "Inspect");
#endif
        }

        public bool OnContextMenuCommand(IWebBrowser chromiumWebBrowser, IBrowser browser, IFrame frame, IContextMenuParams parameters, CefMenuCommand commandId, CefEventFlags eventFlags)
        {
#if DEBUG
            if (commandId == CefMenuCommand.UserFirst)
            {
                browser.Reload(true);
            }
            else if (commandId == (CefMenuCommand)(int)CefMenuCommand.UserFirst + 1)
            {
                chromiumWebBrowser.ShowDevTools(null, parameters.XCoord, parameters.YCoord);
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
