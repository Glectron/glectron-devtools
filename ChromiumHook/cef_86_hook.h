#pragma once

#include "common_type.h"
#include "common_cef_type.h"

bool cef_86_prepare_hook();

int cef_86_initialize_hook(const void* args, void* settings, void* application, void* windows_sandbox_info);
int cef_86_execute_process_hook(const void* args, void* application, void* windows_sandbox_info);
