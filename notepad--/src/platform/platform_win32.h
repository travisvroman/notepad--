#pragma once

#include "defines.h"

// Windows platform layer.
#if KPLATFORM_WINDOWS

#include "containers/darray.h"
#include "core/event.h"
#include "core/input.h"
#include "core/kmemory.h"
#include "core/kmutex.h"
#include "core/ksemaphore.h"
#include "core/kstring.h"
#include "core/kthread.h"
#include "core/logger.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/** Win32-specific platform handle type to be used with rendering backends. */
typedef struct platform_handle {
    HINSTANCE h_instance;
    HWND hwnd;
} platform_handle;

#endif
