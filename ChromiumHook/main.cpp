#include <Windows.h>

#include <include/capi/cef_base_capi.h>
#include "detours/detours.h"
#include "memory.h"

typedef int (*cef_initializeType)(const cef_main_args_t* args, struct _cef_settings_t* settings, void* application, void* windows_sandbox_info);
int cef_initialize_hook(const cef_main_args_t* args, struct _cef_settings_t* settings, void* application, void* windows_sandbox_info);

typedef HMODULE(*LoadLibraryExAType)(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
HMODULE LoadLibraryExAHook(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);

cef_initializeType cef_initialize_original = NULL;

LoadLibraryExAType LoadLibraryExAOriginal = LoadLibraryExA;

int cef_initialize_hook(const cef_main_args_t* args, struct _cef_settings_t* settings, void* application, void* windows_sandbox_info)
{
    if (cef_initialize_original == NULL)
    {
        return -1;
    }
    settings->remote_debugging_port = 46587;
    return cef_initialize_original(args, settings, application, windows_sandbox_info);
}

HMODULE LoadLibraryExAHook(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
    HMODULE moduleAddress = LoadLibraryExAOriginal(lpLibFileName, hFile, dwFlags);

    if (moduleAddress)
    {
        if (strstr(lpLibFileName, "html_chromium.dll") != NULL)
        {
            CMemAddr addr = CModule("libcef.dll").FindPatternSIMD("41 57 41 56 41 54 56 57 55 53 48 81 EC 10 02 00 00 48 8B 05 ?? ?? ?? ?? 48 31 E0 48 89 84 24 08 02 00 00 31");
            cef_initialize_original = reinterpret_cast<cef_initializeType>(addr.GetPtr());
            DetourTransactionBegin();
            DetourUpdateThread(GetCurrentThread());
            DetourAttach(&(PVOID&)cef_initialize_original, cef_initialize_hook);
            DetourTransactionCommit();
        }
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