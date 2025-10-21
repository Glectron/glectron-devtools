#include "hook.h"

#include <Windows.h>

#include "wchar.h"
#include "global.h"
#include "cef_86_hook.h"
#include "cef_137_hook.h"
#include "detours/detours.h"

static int GetCEFVersion() {
    auto mod = GetModuleHandleW(L"libcef.dll");
    if (mod == NULL) return -1;

	auto cef_version_info_addr = GetProcAddress(mod, "cef_version_info");
    if (cef_version_info_addr == NULL) return -1;

	auto cef_version_info = reinterpret_cast<cef_versionInfoType>(cef_version_info_addr);
    
    return cef_version_info(4); // CHROME_VERSION_MAJOR
}

bool HookCEFInitialize() {
    auto mod = GetModuleHandleW(L"libcef.dll");
    if (mod == NULL) return false;

    auto cef_initialize_addr = GetProcAddress(mod, "cef_initialize");
    if (cef_initialize_addr == NULL) return false;

    auto hook_func = cef_86_initialize_hook;

    if (GetCEFVersion() >= 137) {
        cef_137_prepare_hook();
        hook_func = cef_137_initialize_hook;
    } else {
        cef_86_prepare_hook();
	}

    cef_initialize_original = reinterpret_cast<cef_initializeType>(cef_initialize_addr);
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)cef_initialize_original, hook_func);
    DetourTransactionCommit();

    return true;
}

bool HookCEFExecuteProcess() {
    auto mod = GetModuleHandleW(L"libcef.dll");
    if (mod == NULL) {
        // Workaround: CEF 86's subprocesses don't load libcef.dll so early.
        mod = LoadLibraryW(L"libcef.dll");
        if (mod == NULL) return false;
    }
    auto cef_execute_process_addr = GetProcAddress(mod, "cef_execute_process");
    if (cef_execute_process_addr == NULL) return false;

    auto hook_func = cef_86_execute_process_hook;

    if (GetCEFVersion() >= 137) {
        cef_137_prepare_hook();
        hook_func = cef_137_execute_process_hook;
    }
    else {
        cef_86_prepare_hook();
    }

    cef_execute_process_original = reinterpret_cast<cef_executeProcessType>(cef_execute_process_addr);
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)cef_execute_process_original, hook_func);
    DetourTransactionCommit();

    return true;
}

HMODULE __stdcall LoadLibraryExAHook(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
    HMODULE moduleAddress = LoadLibraryExAOriginal(lpLibFileName, hFile, dwFlags);

    if (moduleAddress)
    {
        if (strstr(lpLibFileName, "html_chromium.dll") != NULL)
        {
            HookCEFInitialize();
        }
    }

    return moduleAddress;
}

BOOL __stdcall CreateProcessWHook(
    LPCWSTR lpApplicationName,
    LPWSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
)
{
    bool shouldInject = false;

    // Check if is a Chromium subprocess
    if (lpCommandLine != NULL) {
        if (wcsstr(lpCommandLine, L"--type=") != NULL) {
            shouldInject = true;
        }
    }

    if (shouldInject) {
        char dllPath[MAX_PATH];
        GetModuleFileNameA(dllInstance, dllPath, MAX_PATH);

        char* dlls[1]{
            dllPath
        };

        // Build suffix with current process id
        DWORD pid = GetCurrentProcessId();
        wchar_t suffix[64];
        swprintf_s(suffix, _countof(suffix), L" --glectron-devtools-parent=%lu", (unsigned long)pid);

        // Prepare modified command line (writable)
        size_t origLen = lpCommandLine ? wcslen(lpCommandLine) : 0;
        size_t suffixLen = wcslen(suffix);
        size_t totalLen = origLen + suffixLen + 1;

        wchar_t* modCmd = (wchar_t*)malloc(totalLen * sizeof(wchar_t));
        if (modCmd != NULL) {
            if (origLen > 0) {
                wcscpy_s(modCmd, totalLen, lpCommandLine);
            }
            else {
                modCmd[0] = L'\0';
            }
            wcscat_s(modCmd, totalLen, suffix);
        }

        // Call DetourCreateProcessWithDllsW using modified command line if available
        BOOL result = DetourCreateProcessWithDllsW(
            lpApplicationName,
            modCmd ? modCmd : lpCommandLine,
            lpProcessAttributes,
            lpThreadAttributes,
            bInheritHandles,
            dwCreationFlags,
            lpEnvironment,
            lpCurrentDirectory,
            lpStartupInfo,
            lpProcessInformation,
            1,
            (LPCSTR*)dlls,
            CreateProcessWOriginal
        );

        if (modCmd) {
            free(modCmd);
        }

        return result;
    }
    else
    {
        return CreateProcessWOriginal(
            lpApplicationName,
            lpCommandLine,
            lpProcessAttributes,
            lpThreadAttributes,
            bInheritHandles,
            dwCreationFlags,
            lpEnvironment,
            lpCurrentDirectory,
            lpStartupInfo,
            lpProcessInformation
        );
    }
}