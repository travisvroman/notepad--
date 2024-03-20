/*
#include "sui_label.h"

#include <containers/darray.h>
#include <core/asserts.h>
#include <core/kmemory.h>
#include <core/kstring.h>
#include <core/logger.h>
#include <math/kmath.h>
#include <math/transform.h>
#include <systems/font_system.h>

#include "defines.h"
#include "resources/resource_types.h"

typedef struct sui_label_pending_data {
    u32 quad_count;
    u64 vertex_buffer_size;
    u64 vertex_buffer_offset;
    u64 index_buffer_size;
    u64 index_buffer_offset;
    vertex_2d* vertex_buffer_data;
    u32* index_buffer_data;
} sui_label_pending_data;

static void sui_label_control_render_frame_prepare(struct sui_label* self, const struct frame_data* p_frame_data);

b8 sui_label_control_create(const char* name, const char* font_name, u16 font_size, const char* text, struct sui_label* out_control) {
    if (!out_control) {
        return false;
    }

    // Set all controls to visible by default.
    out_control->is_visible = true;

    // Reasonable defaults.
    out_control->colour = vec4_one();

    out_control->name = string_duplicate(name);

    // Assign the type first
    out_control->type = type;

    // Acquire the font of the correct type and assign its internal data.
    // This also gets the atlas texture.
    out_control->data = font_system_acquire(font_name, font_size);
    if (!out_control->data) {
        KERROR("Unable to acquire font: '%s'. ui_text cannot be created.", font_name);
        return false;
    }

    out_control->vertex_buffer_offset = INVALID_ID_U64;
    out_control->vertex_buffer_size = INVALID_ID_U64;
    out_control->index_buffer_offset = INVALID_ID_U64;
    out_control->index_buffer_size = INVALID_ID_U64;

    // Default quad count is 0 until the first geometry regeneration happens.
    out_control->quad_count = 0;

    // Set text if applicable.
    if (text && string_length(text) > 0) {
        sui_label_text_set(out_control, text);
    } else {
        sui_label_text_set(out_control, "");
    }

    out_control->instance_id = INVALID_ID;
    out_control->frame_number = INVALID_ID_U64;

    // Acquire resources for font texture map.
    // TODO: Offload this somewhere else.
    texture_map* maps[1] = {&out_control->data->atlas};
    shader* s = shader_system_get("Shader.StandardUI");
    u16 atlas_location = s->uniforms[s->instance_sampler_indices[0]].index;
    shader_instance_resource_config instance_resource_config = {0};
    // Map count for this type is known.
    shader_instance_uniform_texture_config atlas_texture = {0};
    atlas_texture.uniform_location = atlas_location;
    atlas_texture.texture_map_count = 1;
    atlas_texture.texture_maps = maps;

    instance_resource_config.uniform_config_count = 1;
    instance_resource_config.uniform_configs = &atlas_texture;

    if (!renderer_shader_instance_resources_acquire(s, &instance_resource_config, &out_control->instance_id)) {
        KFATAL("Unable to acquire shader resources for font texture map.");
        return false;
    }

    // Verify atlas has the glyphs needed.
    if (!font_system_verify_atlas(out_control->data, text)) {
        KERROR("Font atlas verification failed.");
        return false;
    }

    return true;
}

void sui_label_control_destroy(struct sui_label* self) {
    sui_base_control_destroy(self);
}

b8 sui_label_control_load(struct sui_label* self) {
    if (!sui_base_control_load(self)) {
        return false;
    }

    sui_label* out_control = self->internal_data;

    if (out_control->text && typed_data->text[0] != 0) {
        // Flag it as dirty to ensure it gets updated on the next frame.
        out_control->is_dirty = true;
    }

    return true;
}

void sui_label_control_unload(struct sui_label* self) {
    sui_label* out_control = self->internal_data;

    if (out_control->text) {
        u32 text_length = string_length(out_control->text);
        kfree(out_control->text, sizeof(char) * text_length + 1, MEMORY_TAG_STRING);
        out_control->text = 0;
    }

    // Free from the vertex buffer.
    renderbuffer* vertex_buffer = renderer_renderbuffer_get(RENDERBUFFER_TYPE_VERTEX);
    if (out_control->vertex_buffer_offset != INVALID_ID_U64) {
        if (out_control->max_text_length > 0) {
            renderer_renderbuffer_free(vertex_buffer, sizeof(vertex_2d) * 4 * out_control->max_quad_count, typed_data->vertex_buffer_offset);
        }
        out_control->vertex_buffer_offset = INVALID_ID_U64;
    }

    // Free from the index buffer.
    if (out_control->index_buffer_offset != INVALID_ID_U64) {
        static const u64 quad_index_size = (sizeof(u32) * 6);
        renderbuffer* index_buffer = renderer_renderbuffer_get(RENDERBUFFER_TYPE_INDEX);
        if (out_control->max_text_length > 0 || typed_data->index_buffer_offset != INVALID_ID_U64) {
            renderer_renderbuffer_free(index_buffer, quad_index_size * out_control->max_quad_count, typed_data->index_buffer_offset);
        }
        out_control->index_buffer_offset = INVALID_ID_U64;
    }

    // Release resources for font texture map.
    shader* ui_shader = shader_system_get("Shader.StandardUI");  // TODO: text shader.
    if (!renderer_shader_instance_resources_release(ui_shader, out_control->instance_id)) {
        KFATAL("Unable to release shader resources for font texture map.");
    }
    out_control->instance_id = INVALID_ID;
}

b8 sui_label_control_update(struct sui_label* self, struct frame_data* p_frame_data) {
    if (!sui_base_control_update(self, p_frame_data)) {
        return false;
    }

    //

    return true;
}

b8 sui_label_control_render(struct sui_label* self, struct frame_data* p_frame_data, standard_ui_render_data* render_data) {
    if (!sui_base_control_render(self, p_frame_data, render_data)) {
        return false;
    }

    sui_label* out_control = self->internal_data;

    if (out_control->quad_count && typed_data->vertex_buffer_offset != INVALID_ID_U64) {
        standard_ui_renderable renderable = {0};
        renderable.render_data.unique_id = self->id.uniqueid;
        renderable.render_data.material = 0;
        renderable.render_data.vertex_count = out_control->quad_count * 4;
        renderable.render_data.vertex_buffer_offset = out_control->vertex_buffer_offset;
        renderable.render_data.vertex_element_size = sizeof(vertex_2d);
        renderable.render_data.index_count = out_control->quad_count * 6;
        renderable.render_data.index_buffer_offset = out_control->index_buffer_offset;
        renderable.render_data.index_element_size = sizeof(u32);

        // NOTE: Override the default UI atlas and use that of the loaded font instead.
        renderable.atlas_override = &out_control->data->atlas;

        renderable.render_data.model = transform_world_get(&self->xform);
        renderable.render_data.diffuse_colour = out_control->colour;

        renderable.instance_id = &out_control->instance_id;
        renderable.frame_number = &out_control->frame_number;
        renderable.draw_index = &out_control->draw_index;

        darray_push(render_data->renderables, renderable);
    }

    return true;
}

void sui_label_text_set(struct sui_label* self, const char* text) {
    if (self) {
        sui_label* out_control = self->internal_data;

        // If strings are already equal, don't do anything.
        if (out_control->text && strings_equal(text, typed_data->text)) {
            return;
        }

        if (out_control->text) {
            u32 text_length = string_length(out_control->text);
            kfree(out_control->text, sizeof(char) * text_length + 1, MEMORY_TAG_STRING);
        }

        out_control->text = string_duplicate(text);

        // NOTE: Only bother with verification and setting the dirty flag for non-empty strings.
        u32 length = string_length(out_control->text);
        if (length > 0) {
            // Verify atlas has the glyphs needed.
            if (!font_system_verify_atlas(out_control->data, text)) {
                KERROR("Font atlas verification failed.");
            }

            out_control->is_dirty = true;
        } else {
            out_control->is_dirty = false;
        }
    }
}

const char* sui_label_text_get(struct sui_label* self) {
    if (self && self->internal_data) {
        sui_label* out_control = self->internal_data;
        return out_control->text;
    }
    return 0;
}

static font_glyph* glyph_from_codepoint(font_data* font, i32 codepoint) {
    for (u32 i = 0; i < font->glyph_count; ++i) {
        if (font->glyphs[i].codepoint == codepoint) {
            return &font->glyphs[i];
        }
    }

    KERROR("Unable to find font glyph for codepoint: %s", codepoint);
    return 0;
}

static font_kerning* kerning_from_codepoints(font_data* font, i32 codepoint_0, i32 codepoint_1) {
    for (u32 i = 0; i < font->kerning_count; ++i) {
        font_kerning* k = &font->kernings[i];
        if (k->codepoint_0 == codepoint_0 && k->codepoint_1 == codepoint_1) {
            return k;
        }
    }

    // No kerning found. This is okay, not necessarily an error.
    return 0;
}

static b8 regenerate_label_geometry(const sui_label* self, sui_label_pending_data* pending_data) {
    sui_label* out_control = self->internal_data;

    // Get the UTF-8 string length
    u32 text_length_utf8 = string_utf8_length(out_control->text);
    u32 char_length = string_length(out_control->text);

    // Iterate the string once and count how many quads are required. This allows
    // characters which don't require rendering (spaces, tabs, etc.) to be skipped.
    pending_data->quad_count = 0;
    i32* codepoints = kallocate(sizeof(i32) * text_length_utf8, MEMORY_TAG_ARRAY);
    for (u32 c = 0, cp_idx = 0; c < char_length;) {
        i32 codepoint = out_control->text[c];
        u8 advance = 1;

        // Ensure the propert UTF-8 codepoint is being used.
        if (!bytes_to_codepoint(out_control->text, c, &codepoint, &advance)) {
            KWARN("Invalid UTF-8 found in string, using unknown codepoint of -1");
            codepoint = -1;
        }

        // Whitespace codepoints do not need to be included in the quad count.
        if (!codepoint_is_whitespace(codepoint)) {
            pending_data->quad_count++;
        }

        c += advance;

        // Add to the codepoint list.
        codepoints[cp_idx] = codepoint;
        cp_idx++;
    }

    // Calculate buffer sizes.
    static const u64 verts_per_quad = 4;
    static const u8 indices_per_quad = 6;

    // Save the data off to a pending structure.
    pending_data->vertex_buffer_size = sizeof(vertex_2d) * verts_per_quad * pending_data->quad_count;
    pending_data->index_buffer_size = sizeof(u32) * indices_per_quad * pending_data->quad_count;
    // Temp arrays to hold vertex/index data.
    pending_data->vertex_buffer_data = kallocate(pending_data->vertex_buffer_size, MEMORY_TAG_ARRAY);
    pending_data->index_buffer_data = kallocate(pending_data->index_buffer_size, MEMORY_TAG_ARRAY);

    // Generate new geometry for each character.
    f32 x = 0;
    f32 y = 0;

    // Iterate the codepoints list.
    for (u32 c = 0, q_idx = 0; c < text_length_utf8; ++c) {
        i32 codepoint = codepoints[c];

        // Whitespace doesn't get a quad created for it.
        if (codepoint == '\n') {
            // Newline needs to move to the next line and restart x position.
            x = 0;
            y += out_control->data->line_height;
            // No further processing needed.
            continue;
        } else if (codepoint == '\t') {
            // Manually move over by the configured tab advance amount.
            x += out_control->data->tab_x_advance;
            // No further processing needed.
            continue;
        }

        // Obtain the glyph.
        font_glyph* g = glyph_from_codepoint(out_control->data, codepoint);
        if (!g) {
            KERROR("Unable to find unknown codepoint. Using '?' instead.");
            g = glyph_from_codepoint(out_control->data, '?');
        }

        // If not on the last codepoint, try to find kerning between this and the next codepoint.
        i32 kerning_amount = 0;
        if (c < text_length_utf8 - 1) {
            i32 next_codepoint = codepoints[c + 1];
            // Try to find kerning
            font_kerning* kerning = kerning_from_codepoints(out_control->data, codepoint, next_codepoint);
            if (kerning) {
                kerning_amount = kerning->amount;
            }
        }

        // Only generate a quad for non-whitespace characters.
        if (!codepoint_is_whitespace(codepoint)) {
            // Generate points for the quad.
            f32 minx = x + g->x_offset;
            f32 miny = y + g->y_offset;
            f32 maxx = minx + g->width;
            f32 maxy = miny + g->height;
            f32 tminx = (f32)g->x / out_control->data->atlas_size_x;
            f32 tmaxx = (f32)(g->x + g->width) / out_control->data->atlas_size_x;
            f32 tminy = (f32)g->y / out_control->data->atlas_size_y;
            f32 tmaxy = (f32)(g->y + g->height) / out_control->data->atlas_size_y;
            // Flip the y axis for system text
            if (out_control->type == FONT_TYPE_SYSTEM) {
                tminy = 1.0f - tminy;
                tmaxy = 1.0f - tmaxy;
            }

            vertex_2d p0 = (vertex_2d){vec2_create(minx, miny), vec2_create(tminx, tminy)};
            vertex_2d p1 = (vertex_2d){vec2_create(maxx, miny), vec2_create(tmaxx, tminy)};
            vertex_2d p2 = (vertex_2d){vec2_create(maxx, maxy), vec2_create(tmaxx, tmaxy)};
            vertex_2d p3 = (vertex_2d){vec2_create(minx, maxy), vec2_create(tminx, tmaxy)};

            // Vertex data
            pending_data->vertex_buffer_data[(q_idx * 4) + 0] = p0;  // 0    3
            pending_data->vertex_buffer_data[(q_idx * 4) + 1] = p2;  //
            pending_data->vertex_buffer_data[(q_idx * 4) + 2] = p3;  //
            pending_data->vertex_buffer_data[(q_idx * 4) + 3] = p1;  // 2    1

            // Index data 210301
            pending_data->index_buffer_data[(q_idx * 6) + 0] = (q_idx * 4) + 2;
            pending_data->index_buffer_data[(q_idx * 6) + 1] = (q_idx * 4) + 1;
            pending_data->index_buffer_data[(q_idx * 6) + 2] = (q_idx * 4) + 0;
            pending_data->index_buffer_data[(q_idx * 6) + 3] = (q_idx * 4) + 3;
            pending_data->index_buffer_data[(q_idx * 6) + 4] = (q_idx * 4) + 0;
            pending_data->index_buffer_data[(q_idx * 6) + 5] = (q_idx * 4) + 1;

            // Increment quad index.
            q_idx++;
        }

        // Advance by the glyph's advance and kerning.
        x += g->x_advance + kerning_amount;
    }

    // Clean up.
    if (codepoints) {
        kfree(codepoints, sizeof(i32) * text_length_utf8, MEMORY_TAG_ARRAY);
    }

    return true;
}

static void sui_label_control_render_frame_prepare(struct sui_label* self, const struct frame_data* p_frame_data) {
    if (self) {
        sui_label* out_control = self->internal_data;
        if (out_control->is_dirty) {
            sui_label_pending_data pending_data = {0};
            if (!regenerate_label_geometry(self, &pending_data)) {
                KERROR("Error regenerating label geometry.");
                out_control->quad_count = 0;  // Keep it from drawing.
                goto sui_label_frame_prepare_cleanup;
            }

            renderbuffer* vertex_buffer = renderer_renderbuffer_get(RENDERBUFFER_TYPE_VERTEX);
            renderbuffer* index_buffer = renderer_renderbuffer_get(RENDERBUFFER_TYPE_INDEX);

            u64 old_vertex_size = out_control->vertex_buffer_size;
            u64 old_vertex_offset = out_control->vertex_buffer_offset;
            u64 old_index_size = out_control->index_buffer_size;
            u64 old_index_offset = out_control->index_buffer_offset;

            // Use the new offsets unless a realloc is needed.
            u64 new_vertex_size = pending_data.vertex_buffer_size;
            u64 new_vertex_offset = old_vertex_offset;
            u64 new_index_size = pending_data.index_buffer_size;
            u64 new_index_offset = old_index_offset;

            // A reallocation is required if the text is longer than it previously was.
            b8 needs_realloc = pending_data.quad_count > out_control->max_quad_count;
            if (needs_realloc) {
                if (!renderer_renderbuffer_allocate(vertex_buffer, new_vertex_size, &pending_data.vertex_buffer_offset)) {
                    KERROR(
                        "sui_label_control_render_frame_prepare failed to allocate from the renderer's vertex buffer: size=%u, offset=%u",
                        new_vertex_size,
                        pending_data.vertex_buffer_offset);
                    out_control->quad_count = 0;  // Keep it from drawing.
                    goto sui_label_frame_prepare_cleanup;
                }
                new_vertex_offset = pending_data.vertex_buffer_offset;

                if (!renderer_renderbuffer_allocate(index_buffer, new_index_size, &pending_data.index_buffer_offset)) {
                    KERROR(
                        "sui_label_control_render_frame_prepare failed to allocate from the renderer's index buffer: size=%u, offset=%u",
                        new_index_size,
                        pending_data.index_buffer_offset);
                    out_control->quad_count = 0;  // Keep it from drawing.
                    goto sui_label_frame_prepare_cleanup;
                }
                new_index_offset = pending_data.index_buffer_offset;
            }

            // Load up the data, if there is data to load.
            if (pending_data.vertex_buffer_data) {
                if (!renderer_renderbuffer_load_range(vertex_buffer, new_vertex_offset, new_vertex_size, pending_data.vertex_buffer_data, true)) {
                    KERROR("sui_label_control_render_frame_prepare failed to load data into vertex buffer range: size=%u, offset=%u", new_vertex_size, new_vertex_offset);
                }
            }
            if (pending_data.index_buffer_data) {
                if (!renderer_renderbuffer_load_range(index_buffer, new_index_offset, new_index_size, pending_data.index_buffer_data, true)) {
                    KERROR("sui_label_control_render_frame_prepare failed to load data into index buffer range: size=%u, offset=%u", new_index_size, new_index_offset);
                }
            }

            if (needs_realloc) {
                // Release the old vertex/index data from the buffers and update the sizes/offsets.
                if (old_vertex_offset != INVALID_ID_U64 && old_vertex_size != INVALID_ID_U64) {
                    if (!renderer_renderbuffer_free(vertex_buffer, old_vertex_size, old_vertex_offset)) {
                        KERROR("Failed to free from renderer vertex buffer: size=%u, offset=%u", old_vertex_size, old_vertex_offset);
                    }
                }
                if (old_index_offset != INVALID_ID_U64 && old_index_size != INVALID_ID_U64) {
                    if (!renderer_renderbuffer_free(index_buffer, old_index_size, old_index_offset)) {
                        KERROR("Failed to free from renderer index buffer: size=%u, offset=%u", old_index_size, old_index_offset);
                    }
                }

                out_control->vertex_buffer_offset = new_vertex_offset;
                out_control->vertex_buffer_size = new_vertex_size;
                out_control->index_buffer_offset = new_index_offset;
                out_control->index_buffer_size = new_index_size;
            }

            out_control->quad_count = pending_data.quad_count;

            // Update the max length if the string is now longer.
            if (pending_data.quad_count > out_control->max_quad_count) {
                out_control->max_quad_count = pending_data.quad_count;
            }

            // No longer dirty.
            out_control->is_dirty = false;

        sui_label_frame_prepare_cleanup:
            if (pending_data.vertex_buffer_data) {
                kfree(pending_data.vertex_buffer_data, pending_data.vertex_buffer_size, MEMORY_TAG_ARRAY);
            }
            if (pending_data.index_buffer_data) {
                kfree(pending_data.index_buffer_data, pending_data.index_buffer_size, MEMORY_TAG_ARRAY);
            }
        }
    }
}
*/
