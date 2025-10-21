#pragma once

typedef int (*cef_initializeType)(const void* args, void* settings, void* application, void* windows_sandbox_info);
typedef int (*cef_executeProcessType)(const void* args, void* application, void* windows_sandbox_info);
typedef int (*cef_versionInfoType)(int entry);