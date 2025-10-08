#include "cef_86_hook.h"

#include <include/capi/cef_base_capi.h>

int cef_86_initialize_hook(const void* args, void* settings, void* application, void* windows_sandbox_info)
{
    if (cef_initialize_original == NULL)
    {
        return -1;
    }
    auto s = reinterpret_cast<_cef_settings_t*>(settings);
    s->remote_debugging_port = debuggingPort;
    return cef_initialize_original(args, s, application, windows_sandbox_info);
}