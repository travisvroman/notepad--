#pragma once

#include "math/math_types.h"
#include "renderer/opengl/gl_types.h"

typedef struct sui_label {
    gl_context* context;
    b8 is_visible;
    vec2i size;
    u32 instance_id;

    struct font_data* data;
    u64 index_buffer_offset;
    u64 index_buffer_size;
    char* text;
    u32 max_text_length;
    u32 quad_count;
    u32 max_quad_count;

    gl_buffer vertex_buffer;
    gl_buffer index_buffer;

    b8 is_dirty;
} sui_label;

struct frame_data;

KAPI b8 sui_label_control_create(gl_context* context, const char* text, struct sui_label* out_control);
KAPI void sui_label_control_destroy(struct sui_label* self);
KAPI b8 sui_label_control_load(struct sui_label* self);
KAPI void sui_label_control_unload(struct sui_label* self);
KAPI b8 sui_label_control_update(struct sui_label* self, struct frame_data* p_frame_data);

void sui_label_control_render_frame_prepare(struct sui_label* self);
KAPI b8 sui_label_control_render(struct sui_label* self, mat4 projection);

/**
 * @brief Sets the text on the given label object.
 *
 * @param u_text A pointer to the label whose text will be set.
 * @param text The text to be set.
 */
KAPI void sui_label_text_set(struct sui_label* self, const char* text);

KAPI const char* sui_label_text_get(struct sui_label* self);
