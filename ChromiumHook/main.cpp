// Special thanks to @wolf109909, who created the first implementation of 64-bit cef_initialize injection.

#define x86 _WIN32 and !_WIN64

#include <Windows.h>

#include "common_cef_type.h"
#include "cef_86_hook.h"
#if !(x86)
#include "cef_137_hook.h"
#endif
#include "detours/detours.h"
#include "memory.h"

#if x86
#include <Awesomium/WebCore.h>
#include <Awesomium/WebConfig.h>
#endif

#if x86
typedef Awesomium::WebCore* (*awesomium_webcore_initializeType)(const Awesomium::WebConfig& config);
Awesomium::WebCore* awesomium_webcore_initialize_hook(Awesomium::WebConfig& config);
#endif

typedef HMODULE(__stdcall*LoadLibraryExAType)(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
HMODULE __stdcall LoadLibraryExAHook(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);

cef_initializeType cef_initialize_original = NULL;
#if x86
awesomium_webcore_initializeType awesomium_webcore_initialize_original = NULL;
#endif

LoadLibraryExAType LoadLibraryExAOriginal = LoadLibraryExA;

#if x86
Awesomium::WebCore* awesomium_webcore_initialize_hook(Awesomium::WebConfig& config) {
    if (awesomium_webcore_initialize_original == NULL) {
        return nullptr;
    }
    config.remote_debugging_port = 46587;
    return awesomium_webcore_initialize_original(config);
}
#endif

HMODULE __stdcall LoadLibraryExAHook(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
    HMODULE moduleAddress = LoadLibraryExAOriginal(lpLibFileName, hFile, dwFlags);

    if (moduleAddress)
    {
        if (strstr(lpLibFileName, "html_chromium.dll") != NULL)
        {
            auto hook_func = cef_86_initialize_hook;
#if _WIN64
            CMemAddr addr = CModule("libcef.dll").FindPatternSIMD("41 57 41 56 41 54 56 57 55 53 48 81 EC 10 02 00 00 48 8B 05 ?? ?? ?? ?? 48 31 E0 48 89 84 24 08 02 00 00 31");
            if (addr.GetPtr() == 0) {
                // not CEF 86, maybe GModPatchTool, try again
                addr = CModule("libcef.dll").FindPatternSIMD("41 57 41 56 56 57 53 48 81 EC ?? ?? 00 00 4C 89 C7 48 89 D3 48 8B 05 ?? ?? ?? ?? 48 31 E0");
                hook_func = cef_137_initialize_hook;
            }
            if (addr.GetPtr() == 0) {
                // give up
                return moduleAddress;
            }
#else
            CMemAddr addr = CModule("libcef.dll").FindPatternSIMD("55 89 E5 53 57 56 81 EC 0C 01 00 00 8B 45 08 8B 0D ?? ?? ?? ?? 31 E9 89 4D F0 31");
#endif
            cef_initialize_original = reinterpret_cast<cef_initializeType>(addr.GetPtr());
            DetourTransactionBegin();
            DetourUpdateThread(GetCurrentThread());
            DetourAttach(&(PVOID&)cef_initialize_original, hook_func);
            DetourTransactionCommit();
        }
#if x86
        else if (strstr(lpLibFileName, "gmhtml.dll") != NULL) {
            CMemAddr addr = CModule("Awesomium.dll").FindPatternSIMD("55 8B EC 51 83 65 FC 00 83 3D 30 ?? ?? ?? ?? 74 05");
            awesomium_webcore_initialize_original = reinterpret_cast<awesomium_webcore_initializeType>(addr.GetPtr());
            DetourTransactionBegin();
            DetourUpdateThread(GetCurrentThread());
            DetourAttach(&(PVOID&)awesomium_webcore_initialize_original, awesomium_webcore_initialize_hook);
            DetourTransactionCommit();
        }
#endif
    }

    return moduleAddress;
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved)
{
    if (dwReason == DLL_PROCESS_ATTACH) {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID&)LoadLibraryExAOriginal, LoadLibraryExAHook);
        DetourTransactionCommit();
    }

    return TRUE;
}