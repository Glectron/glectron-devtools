using CefSharp.WinForms;
using CefSharp;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Security.Cryptography.X509Certificates;
using CefSharp.Handler;
using System.IO;

namespace DevTools
{
    public class JavaScriptInjector : RequestHandler
    {
        protected override IResourceRequestHandler? GetResourceRequestHandler(IWebBrowser chromiumWebBrowser, IBrowser browser, IFrame frame, IRequest request, bool isNavigation, bool isDownload, string requestInitiator, ref bool disableDefaultHandling)
        {
            if (frame.IsMain && request.ResourceType == ResourceType.MainFrame && request.Url.Contains("devtools.html"))
            {
                return new JavaScriptRequestHandler();
            }
            return null;
        }
    }

    public class JavaScriptRequestHandler : ResourceRequestHandler
    {
        protected override IResponseFilter GetResourceResponseFilter(IWebBrowser chromiumWebBrowser, IBrowser browser, IFrame frame, IRequest request, IResponse response)
        {
            return new JavaScriptInjectionFilter("glectron://awesomiumpolyfill");
        }
    }
}
