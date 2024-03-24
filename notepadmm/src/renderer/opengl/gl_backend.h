#pragma once

#include "defines.h"
#include "math/math_types.h"
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

gl_texture gl_renderer_texture_create(gl_context* context, u32 width, u32 height);
void gl_renderer_texture_destroy(gl_context* context, gl_texture* t);
b8 gl_renderer_texture_data_set(gl_context* context, gl_texture* t, const u8* pixels);
void gl_renderer_texture_bind(gl_context* context, gl_texture* t, u32 unit);

gl_buffer gl_renderer_buffer_create(gl_context* context, u32 element_size, gl_buffer_type type);
void gl_renderer_buffer_upload_data(gl_buffer* b, u32 element_count, void* data);
void gl_renderer_buffer_bind(gl_buffer* b);
void gl_renderer_buffer_draw(gl_context* context, gl_buffer* b);

void gl_renderer_set_mvp(gl_context* context, mat4 mvp);
void gl_renderer_set_texture(gl_context* context, gl_texture* t);

u32 gl_renderer_acquire_instance(gl_context* context);
void gl_renderer_bind_instance(gl_context* context, u32 vao_instance);
void gl_renderer_unbind_instance(gl_context* context);
void gl_renderer_configure_instance_layout(gl_context* context, const gl_layout* layout);
