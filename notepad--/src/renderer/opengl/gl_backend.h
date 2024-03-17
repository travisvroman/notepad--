#pragma once

#include "defines.h"
#include "renderer/opengl/gl_types.h"

struct platform_state;
struct texture;

typedef struct gl_renderer_config {
    struct platform_state* platform;
} gl_renderer_config;

b8 gl_renderer_initialize(gl_context* context, gl_renderer_config config);
void gl_renderer_shutdown(gl_context* context);

b8 gl_renderer_frame_prepare(gl_context* context);

b8 gl_renderer_frame_begin(gl_context* context);
b8 gl_renderer_frame_end(gl_context* context);

void gl_renderer_on_resize(gl_context* context, u32 width, u32 height);

b8 gl_renderer_texture_create(gl_context* context, struct texture* out_texture);
void gl_renderer_texture_destroy(gl_context* context, struct texture* t);
b8 gl_renderer_texture_data_set(gl_context* context, struct texture* t, const u8* pixels);
