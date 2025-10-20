#include "global.h"

#include <include/capi/cef_base_capi.h>
#include <include/capi/cef_app_capi.h>
#include <include/cef_app.h>

typedef int (*FnCefStringUtf16Set)(const char16_t* src, size_t src_len, cef_string_t* output, int copy);
typedef void (*FnCefStringUtf16Clear)(cef_string_t* str);

typedef void(__stdcall* FnOnBeforeCommandLineProcessing)(
    _cef_app_t* self,
    const cef_string_t* process_type,
    _cef_command_line_t* command_line
);

typedef void(__stdcall* FnOnRegisterCustomSchemes)(
    _cef_app_t* self,
    _cef_scheme_registrar_t* registrar
);

typedef int(__stdcall* FnAddCustomScheme)(
    _cef_scheme_registrar_t* self,
    const cef_string_t* scheme_name,
    int options
);

static FnCefStringUtf16Set pfn_cef_string_utf16_set = nullptr;
static FnCefStringUtf16Clear pfn_cef_string_utf16_clear = nullptr;

static FnOnBeforeCommandLineProcessing original_OnBeforeCommandLineProcessing = nullptr;
static FnOnRegisterCustomSchemes original_OnRegisterCustomSchemes = nullptr;
static FnAddCustomScheme original_AddCustomScheme = nullptr;

static void __stdcall OnBeforeCommandLineProcessingDetour(_cef_app_t* self, const cef_string_t* process_type, _cef_command_line_t* command_line) {
    if (!pfn_cef_string_utf16_clear || !pfn_cef_string_utf16_set) return;

    cef_string_t cef_key = {};
    cef_string_t cef_value = {};

    const char16_t* key = u"remote-allow-origins";
    const char16_t* value = u"devtools://devtools";

    pfn_cef_string_utf16_set(key, wcslen((wchar_t*)key), &cef_key, 1);
    pfn_cef_string_utf16_set(value, wcslen((wchar_t*)value), &cef_value, 1);

    command_line->append_switch_with_value(command_line, &cef_key, &cef_value);

    pfn_cef_string_utf16_clear(&cef_key);
    pfn_cef_string_utf16_clear(&cef_value);

    if (original_OnBeforeCommandLineProcessing) {
        original_OnBeforeCommandLineProcessing(self, process_type, command_line);
    }
}

static int __stdcall AddCustomSchemeDetour(_cef_scheme_registrar_t* self,
    const cef_string_t* scheme_name,
    int options) {
    if (wcscmp((wchar_t*)scheme_name->str, (wchar_t*)u"asset") == 0) {
        options |= CEF_SCHEME_OPTION_SECURE;
    }
    return original_AddCustomScheme(self, scheme_name, options);
}

static void __stdcall OnRegisterCustomSchemesDetour(_cef_app_t* self, _cef_scheme_registrar_t* registrar) {
    original_AddCustomScheme = registrar->add_custom_scheme;
    registrar->add_custom_scheme = AddCustomSchemeDetour;
    if (original_OnRegisterCustomSchemes) {
        original_OnRegisterCustomSchemes(self, registrar);
    }
}

bool PREPARE_HOOK_FUNC()
{
    auto module = GetModuleHandleW(L"libcef.dll");
    if (!module) return false;

    pfn_cef_string_utf16_set = (FnCefStringUtf16Set)GetProcAddress(module, "cef_string_utf16_set");
    pfn_cef_string_utf16_clear = (FnCefStringUtf16Clear)GetProcAddress(module, "cef_string_utf16_clear");

    return true;
}

int INITIALIZE_HOOK_FUNC(const void* args, void* settings, void* application, void* windows_sandbox_info)
{
    if (cef_initialize_original == NULL)
    {
        return -1;
    }

    auto s = reinterpret_cast<_cef_settings_t*>(settings);
    s->remote_debugging_port = g_debuggingPort;
    
    if (g_injectorSettings.DisableSandbox) {
        s->no_sandbox = true;
    }

    _cef_app_t* app = reinterpret_cast<_cef_app_t*>(application);

    original_OnBeforeCommandLineProcessing = app->on_before_command_line_processing;
    app->on_before_command_line_processing = OnBeforeCommandLineProcessingDetour;

    if (g_injectorSettings.DisableSandbox && g_injectorSettings.RegisterAssetAsSecured) {
        original_OnRegisterCustomSchemes = app->on_register_custom_schemes;
        app->on_register_custom_schemes = OnRegisterCustomSchemesDetour;
    }

    return cef_initialize_original(args, s, application, windows_sandbox_info);
}

int EXECUTE_PROCESS_HOOK_FUNC(const void* args, void* application, void* windows_sandbox_info)
{
    if (cef_execute_process_original == NULL)
    {
        return -1;
    }

    _cef_app_t* app = reinterpret_cast<_cef_app_t*>(application);

    original_OnBeforeCommandLineProcessing = app->on_before_command_line_processing;
    app->on_before_command_line_processing = OnBeforeCommandLineProcessingDetour;

    if (g_injectorSettings.DisableSandbox && g_injectorSettings.RegisterAssetAsSecured) {
        original_OnRegisterCustomSchemes = app->on_register_custom_schemes;
        app->on_register_custom_schemes = OnRegisterCustomSchemesDetour;
    }
    return cef_execute_process_original(args, application, windows_sandbox_info);
}