using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Text;
using System.Threading.Tasks;

namespace DevTools
{
    internal class JavaScriptBridge
    {
        public bool IsInjected()
        {
            return Injector.Status == InjectorStatus.Injected;
        }

        public async Task<string> PerformRequest(string path)
        {
            HttpClient hc = new();
            var res = await hc.GetAsync("http://127.0.0.1:46587" + path);
            return await res.Content.ReadAsStringAsync();
        }
    }
}
