#pragma once

#include "common_type.h"
#include "common_cef_type.h"
#include "hook.h"

extern HINSTANCE dllInstance;
extern LoadLibraryExAType LoadLibraryExAOriginal;
extern CreateProcessWType CreateProcessWOriginal;

extern cef_initializeType cef_initialize_original;
extern cef_executeProcessType cef_execute_process_original;

extern int g_debuggingPort;
extern InjectorSettings g_injectorSettings;