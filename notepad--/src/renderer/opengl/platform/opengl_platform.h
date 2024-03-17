#pragma once

#include "renderer/opengl/gl_types.h"

b8 platform_create_rendering_context(gl_context* context);

void platform_destroy_rendering_context(gl_context* context);

b8 platform_swap_buffers(gl_context* context);

void platform_log_last_error(void);
