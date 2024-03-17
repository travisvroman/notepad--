#pragma once

#include "defines.h"

struct platform_state;
// Platform-specific internal data.
struct gl_platform_data;

typedef struct gl_context {
    struct platform_state* platform;
    struct gl_platform_data* data;

    const char* gl_version;

    u32 framebuffer_width, framebuffer_height;
} gl_context;
