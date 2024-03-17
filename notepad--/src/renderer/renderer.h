#pragma once

#include "renderer/renderer_types.h"

struct gl_context;
struct platform_state;

typedef struct renderer_state {
    struct gl_context* context;
} renderer_state;

typedef struct renderer_config {
    struct platform_state* platform;
} renderer_config;

b8 renderer_initialize(u64* memory_requirement, renderer_state* state, renderer_config config);
void renderer_shutdown(renderer_state* state);

b8 renderer_frame_prepare(renderer_state* state);

b8 renderer_frame_begin(renderer_state* state);
b8 renderer_frame_end(renderer_state* state);

b8 renderer_texture_create(renderer_state* state, struct texture* out_texture);
void renderer_texture_destroy(renderer_state* state, struct texture* t);
b8 renderer_texture_data_set(renderer_state* state, struct texture* t, const u8* pixels);
