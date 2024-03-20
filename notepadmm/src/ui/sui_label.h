#pragma once

#include "math/math_types.h"

typedef struct sui_label {
    vec2i size;
    vec4 colour;
    u32 instance_id;
    u64 frame_number;
    u8 draw_index;

    struct font_data* data;
    u64 vertex_buffer_offset;
    u64 index_buffer_offset;
    u64 vertex_buffer_size;
    u64 index_buffer_size;
    char* text;
    u32 max_text_length;
    u32 quad_count;
    u32 max_quad_count;

    b8 is_dirty;
} sui_label;

struct frame_data;

KAPI b8 sui_label_control_create(const char* name, const char* font_name, u16 font_size, const char* text, struct sui_label* out_control);
KAPI void sui_label_control_destroy(struct sui_label* self);
KAPI b8 sui_label_control_load(struct sui_label* self);
KAPI void sui_label_control_unload(struct sui_label* self);
KAPI b8 sui_label_control_update(struct sui_label* self, struct frame_data* p_frame_data);
KAPI b8 sui_label_control_render(struct sui_label* self, struct frame_data* p_frame_data);

/**
 * @brief Sets the text on the given label object.
 *
 * @param u_text A pointer to the label whose text will be set.
 * @param text The text to be set.
 */
KAPI void sui_label_text_set(struct sui_label* self, const char* text);

KAPI const char* sui_label_text_get(struct sui_label* self);
