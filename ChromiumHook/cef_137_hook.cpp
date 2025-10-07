#include "cef_137_hook.h"

// Windows's min/max macro can mess around with std:min/std:max, which are used by CEF headers
#define NOMINMAX
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

FnCefStringUtf16Set pfn_cef_string_utf16_set = nullptr;
FnCefStringUtf16Clear pfn_cef_string_utf16_clear = nullptr;

FnOnBeforeCommandLineProcessing original_OnBeforeCommandLineProcessing = nullptr;

static void __stdcall OnBeforeCommandLineProcessingDetour(_cef_app_t* self, const cef_string_t* process_type, _cef_command_line_t* command_line) {
    if (!pfn_cef_string_utf16_clear || !pfn_cef_string_utf16_set) return;

    cef_string_t cef_key = {};
    cef_string_t cef_value = {};

    const char16_t* key = u"remote-allow-origins";
    const char16_t* value = u"devtools://devtools";

    pfn_cef_string_utf16_set(key, wcslen((wchar_t*)key), &cef_key, 1);
    pfn_cef_string_utf16_set(value, wcslen((wchar_t*)value), &cef_value, 1);

    // Allow remote debugging from DevTools
    // --remote-allow-origins devtools://devtools
    command_line->append_switch_with_value(command_line, &cef_key, &cef_value);

    pfn_cef_string_utf16_clear(&cef_key);
    pfn_cef_string_utf16_clear(&cef_value);
    
    if (original_OnBeforeCommandLineProcessing) {
        original_OnBeforeCommandLineProcessing(self, process_type, command_line);
    }
}

int cef_137_initialize_hook(const void* args, void* settings, void* application, void* windows_sandbox_info)
{
    if (cef_initialize_original == NULL)
    {
        return -1;
    }
    
    auto s = reinterpret_cast<_cef_settings_t*>(settings);
    s->remote_debugging_port = 46587;

	auto module = GetModuleHandleW(L"libcef.dll");
	if (!module) return cef_initialize_original(args, s, application, windows_sandbox_info);

	pfn_cef_string_utf16_set = (FnCefStringUtf16Set)GetProcAddress(module, "cef_string_utf16_set");
	pfn_cef_string_utf16_clear = (FnCefStringUtf16Clear)GetProcAddress(module, "cef_string_utf16_clear");

	_cef_app_t* app = reinterpret_cast<_cef_app_t*>(application);

	original_OnBeforeCommandLineProcessing = app->on_before_command_line_processing;
    app->on_before_command_line_processing = OnBeforeCommandLineProcessingDetour;

    return cef_initialize_original(args, s, application, windows_sandbox_info);
}