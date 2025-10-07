#pragma once

#include "common_cef_type.h"

extern cef_initializeType cef_initialize_original;

int cef_86_initialize_hook(const void* args, void* settings, void* application, void* windows_sandbox_info);
