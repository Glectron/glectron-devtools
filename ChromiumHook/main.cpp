// Special thanks to @wolf109909, who created the first implementation of 64-bit cef_initialize injection.

#define x86 _WIN32 and !_WIN64

#include <Windows.h>

#include "common_cef_type.h"
#include "cef_86_hook.h"
#if _WIN64
#include "cef_137_hook.h"
#endif
#include "detours/detours.h"
#include "memory.h"

typedef HMODULE(__stdcall*LoadLibraryExAType)(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
HMODULE __stdcall LoadLibraryExAHook(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);

HANDLE stateMapFile;

cef_initializeType cef_initialize_original = NULL;
int debuggingPort = 46587;

LoadLibraryExAType LoadLibraryExAOriginal = LoadLibraryExA;

HMODULE __stdcall LoadLibraryExAHook(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
    HMODULE moduleAddress = LoadLibraryExAOriginal(lpLibFileName, hFile, dwFlags);

    if (moduleAddress)
    {
        if (strstr(lpLibFileName, "html_chromium.dll") != NULL)
        {
            auto hook_func = cef_86_initialize_hook;
            auto module = CModule("libcef.dll");
#if _WIN64
            CMemAddr addr = module.FindPatternSIMD("41 57 41 56 41 54 56 57 55 53 48 81 EC 10 02 00 00 48 8B 05 ?? ?? ?? ?? 48 31 E0 48 89 84 24 08 02 00 00 31");
            if (addr.GetPtr() == 0) {
                // not CEF 86, maybe GModPatchTool, try again
                addr = module.FindPatternSIMD("41 57 41 56 56 57 53 48 81 EC ?? ?? 00 00 4C 89 C7 48 89 D3 48 8B 05 ?? ?? ?? ?? 48 31 E0");
                hook_func = cef_137_initialize_hook;
            }
            if (addr.GetPtr() == 0) {
                // give up
                return moduleAddress;
            }
#else
            CMemAddr addr = module.FindPatternSIMD("55 89 E5 53 57 56 81 EC 0C 01 00 00 8B 45 08 8B 0D ?? ?? ?? ?? 31 E9 89 4D F0 31");
#endif
            cef_initialize_original = reinterpret_cast<cef_initializeType>(addr.GetPtr());
            DetourTransactionBegin();
            DetourUpdateThread(GetCurrentThread());
            DetourAttach(&(PVOID&)cef_initialize_original, hook_func);
            DetourTransactionCommit();
        }
    }

    return moduleAddress;
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved)
{
    if (dwReason == DLL_PROCESS_ATTACH) {
		// Read injector parameter from shared memory.
        char shmName[256];
        sprintf_s(shmName, "GlectrionDevToolsParam_%lu", GetCurrentProcessId());

        HANDLE hMapFile = OpenFileMappingA(FILE_MAP_READ, FALSE, shmName);
        if (hMapFile != NULL)
        {
            int* pParam = (int*)MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, sizeof(int));
            if (pParam != NULL)
            {
                debuggingPort = *pParam;

                UnmapViewOfFile(pParam);
            }
            CloseHandle(hMapFile);
        }

        // Create shared memory to store the DevTools state.
        sprintf_s(shmName, "GlectrionDevTools_%lu", GetCurrentProcessId());

        hMapFile = CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            NULL,
            PAGE_READWRITE,
            0,
            sizeof(int),
            shmName);

        if (hMapFile == NULL)
        {
            return NULL;
        }

        int* pBuf = (int*)MapViewOfFile(
            hMapFile,
            FILE_MAP_ALL_ACCESS,
            0,
            0,
            sizeof(int));

        if (pBuf == NULL)
        {
            CloseHandle(hMapFile);
            return NULL;
        }

        // Write the value
        *pBuf = debuggingPort;

        UnmapViewOfFile(pBuf);

		stateMapFile = hMapFile;

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID&)LoadLibraryExAOriginal, LoadLibraryExAHook);
        DetourTransactionCommit();
    }

    return TRUE;
}