using CefSharp;
using CefSharp.Callback;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace DevTools
{
    internal class SchemeHandler : ResourceHandler
    {
        readonly string FrontendPath = Path.Combine(Path.GetDirectoryName(Environment.ProcessPath) ?? "", "frontend");

        public override CefReturnValue ProcessRequestAsync(IRequest request, ICallback callback)
        {
            Task.Run(() =>
            {
                var uri = new Uri(request.Url);
                if (uri.Host == "frontend")
                {
                    var file = Path.Combine(FrontendPath, uri.AbsolutePath[1..]);
                    if (File.Exists(file))
                    {
                        var cont = File.ReadAllBytes(file);
                        Stream = new MemoryStream(cont);
                        MimeType = GetMimeType(Path.GetExtension(file));

                        callback.Continue();
                    } else
                    {
                        StatusCode = 404;
                        callback.Continue();
                    }
                } else
                {
                    callback.Cancel();
                }
            });
            return CefReturnValue.ContinueAsync;
        }
    }
}
