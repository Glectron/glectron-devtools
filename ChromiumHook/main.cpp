// Special thanks to @wolf109909, who created the first implementation of 64-bit cef_initialize injection.

#define x86 _WIN32 and !_WIN64

#include <Windows.h>

#include "hook.h"
#include "common_type.h"
#include "common_cef_type.h"
#include "detours/detours.h"
#include "memory.h"

HINSTANCE dllInstance = NULL;
LoadLibraryExAType LoadLibraryExAOriginal = LoadLibraryExA;
CreateProcessWType CreateProcessWOriginal = CreateProcessW;

cef_initializeType cef_initialize_original = NULL;
cef_executeProcessType cef_execute_process_original = NULL;

bool g_isSubprocess = false;
int g_debuggingPort = 46587;
InjectorSettings g_injectorSettings = { false };

__declspec(dllexport) void DummyEmptyFunction() { }

static int ParseParentPidFromCommandLine()
{
    const wchar_t* key = L"--glectron-devtools-parent=";
    const wchar_t* cmd = GetCommandLineW();
    if (!cmd) return 0;

    const wchar_t* found = wcsstr(cmd, key);
    if (!found) return 0;

    found += wcslen(key);
    wchar_t* endptr = nullptr;
    unsigned long long val = _wcstoui64(found, &endptr, 10);
    if (endptr == found) return 0;
    if (val == 0 || val > 0xFFFFFFFFULL) return 0;

    return (int)val;
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved)
{
    if (dwReason == DLL_PROCESS_ATTACH) {
        dllInstance = hinst;

        int parentPid = ParseParentPidFromCommandLine();
        g_isSubprocess = parentPid != 0;

        // Read injector parameter from shared memory. Read from parent process if is a subprocess.
        char shmName[256];
        sprintf_s(shmName, g_isSubprocess ? "GlectronDevTools_%lu" : "GlectronDevToolsParam_%lu", g_isSubprocess ? parentPid : GetCurrentProcessId());

        HANDLE hMapFile = OpenFileMappingA(FILE_MAP_READ, FALSE, shmName);
        if (hMapFile != NULL)
        {
            SIZE_T paramMappingSize = sizeof(int) + sizeof(InjectorSettings);
            void* pParamView = MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, paramMappingSize);
            if (pParamView != NULL)
            {
                // Read port
                int* pParam = (int*)pParamView;
                g_debuggingPort = *pParam;

                // Read settings
                InjectorSettings* pSettings = (InjectorSettings*)((char*)pParamView + sizeof(int));
                g_injectorSettings = *pSettings;

                UnmapViewOfFile(pParamView);
            }

            CloseHandle(hMapFile);
        }

        if (!g_isSubprocess) {
            // Create shared memory to store the DevTools state (port + InjectorSettings).
            sprintf_s(shmName, "GlectronDevTools_%lu", GetCurrentProcessId());

            // compute mapping size: int (port) + native InjectorSettings
            SIZE_T mappingSize = sizeof(int) + sizeof(InjectorSettings);

            hMapFile = CreateFileMappingA(
                INVALID_HANDLE_VALUE,
                NULL,
                PAGE_READWRITE,
                (DWORD)((mappingSize >> 32) & 0xFFFFFFFF),
                (DWORD)(mappingSize & 0xFFFFFFFF),
                shmName);

            if (hMapFile == NULL)
            {
                return NULL;
            }

            void* pView = MapViewOfFile(
                hMapFile,
                FILE_MAP_ALL_ACCESS,
                0,
                0,
                mappingSize);

            if (pView == NULL)
            {
                CloseHandle(hMapFile);
                return NULL;
            }

            int* pPort = (int*)pView;
            *pPort = g_debuggingPort;

            InjectorSettings* pSettings = (InjectorSettings*)((char*)pView + sizeof(int));
            *pSettings = g_injectorSettings;

            UnmapViewOfFile(pView);
        }

        if (!g_isSubprocess) {
            DetourTransactionBegin();
            DetourUpdateThread(GetCurrentThread());
            DetourAttach(&(PVOID&)LoadLibraryExAOriginal, LoadLibraryExAHook);
            if (g_injectorSettings.DisableSandbox && g_injectorSettings.RegisterAssetAsSecured) {
                DetourAttach(&(PVOID&)CreateProcessWOriginal, CreateProcessWHook);
            }
            DetourTransactionCommit();
        }
        else {
            HookCEFExecuteProcess();
        }
    }

    return TRUE;
}