
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
#include "renderer/opengl/gl_backend.h"
#include "renderer/opengl/gl_types.h"
#include "resources/resource_types.h"

typedef struct sui_label_pending_data {
    u32 quad_count;
    u64 vertex_buffer_size;
    u64 index_buffer_size;
    vertex_2d* vertex_buffer_data;
    u32* index_buffer_data;
} sui_label_pending_data;

b8 sui_label_control_create(gl_context* context, const char* text, struct sui_label* out_control) {
    if (!out_control) {
        return false;
    }

    out_control->context = context;

    // Set all controls to visible by default.
    out_control->is_visible = true;

    // Acquire the font of the correct type and assign its internal data.
    // This also gets the atlas texture.
    out_control->data = font_system_acquire(20);  // TODO: load this globally somewhere.
    if (!out_control->data) {
        KERROR("Unable to acquire font. ui_text cannot be created.");
        return false;
    }

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

    // Init "vao"
    {
        out_control->instance_id = gl_renderer_acquire_instance(context);
        gl_renderer_bind_instance(context, out_control->instance_id);

        // Create buffers.
        out_control->vertex_buffer = gl_renderer_buffer_create(context, sizeof(vertex_2d), GL_BUFFER_TYPE_VERTEX);
        out_control->index_buffer = gl_renderer_buffer_create(context, sizeof(u32), GL_BUFFER_TYPE_INDEX);

        gl_renderer_configure_instance_layout(context, &context->textured_vertex_2d_layout);
        gl_renderer_unbind_instance(context);
    }

    // Verify atlas has the glyphs needed.
    if (!font_system_verify_atlas(out_control->data, text)) {
        KERROR("Font atlas verification failed.");
        return false;
    }

    return true;
}

void sui_label_control_destroy(struct sui_label* self) {
    // TODO: destroy this
}

b8 sui_label_control_load(struct sui_label* self) {
    if (self->text && self->text[0] != 0) {
        // Flag it as dirty to ensure it gets updated on the next frame.
        self->is_dirty = true;
    }

    return true;
}

void sui_label_control_unload(struct sui_label* self) {
    if (self->text) {
        kfree(self->text);
        self->text = 0;
    }

    // TODO: Unload stuff.
}

b8 sui_label_control_update(struct sui_label* self, struct frame_data* p_frame_data) {
    return true;
}

b8 sui_label_control_render(struct sui_label* self, mat4 projection) {
    mat4 model = mat4_translation((vec3){0, 20, 0});
    mat4 mvp = mat4_mul(model, projection);
    // Uniforms
    gl_renderer_set_mvp(self->context, mvp);

    // Set diffuse
    gl_renderer_texture_bind(self->context, &self->data->atlas, 0);
    gl_renderer_set_texture(self->context, &self->data->atlas);

    // Draw
    // NOTE: Every "instance" of a thing will need to bind the instance first before draw.
    gl_renderer_bind_instance(self->context, self->instance_id);
    gl_renderer_buffer_draw(self->context, &self->index_buffer);

    return true;
}

void sui_label_text_set(struct sui_label* self, const char* text) {
    if (self) {
        // If strings are already equal, don't do anything.
        if (self->text && strings_equal(text, self->text)) {
            return;
        }

        if (self->text) {
            kfree(self->text);
        }

        self->text = string_duplicate(text);

        // NOTE: Only bother with verification and setting the dirty flag for non-empty strings.
        u32 length = string_length(self->text);
        if (length > 0) {
            // Verify atlas has the glyphs needed.
            if (!font_system_verify_atlas(self->data, text)) {
                KERROR("Font atlas verification failed.");
            }

            self->is_dirty = true;
        } else {
            self->is_dirty = false;
        }
    }
}

const char* sui_label_text_get(struct sui_label* self) {
    if (self) {
        return self->text;
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

static b8 regenerate_label_geometry(sui_label* self, sui_label_pending_data* pending_data) {
    sui_label* out_control = self;

    // Get the UTF-8 string length
    u32 text_length_utf8 = string_utf8_length(out_control->text);
    u32 char_length = string_length(out_control->text);

    // Iterate the string once and count how many quads are required. This allows
    // characters which don't require rendering (spaces, tabs, etc.) to be skipped.
    pending_data->quad_count = 0;
    i32* codepoints = kallocate(sizeof(i32) * text_length_utf8);
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
    pending_data->vertex_buffer_data = kallocate(pending_data->vertex_buffer_size);
    pending_data->index_buffer_data = kallocate(pending_data->index_buffer_size);

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
            // Flip the y axis for system text. NOTE: Apparently not for GL?
            /* if (out_control->type == FONT_TYPE_SYSTEM) { */
            /* tminy = 1.0f - tminy;
            tmaxy = 1.0f - tmaxy; */
            /* } */

            // TODO: only setting all colours to white for now...
            // This will need to be updated once colour schemes and tokenization are working.
            vertex_2d p0 = (vertex_2d){vec2_create(minx, miny), vec2_create(tminx, tminy), vec4_one()};
            vertex_2d p1 = (vertex_2d){vec2_create(maxx, miny), vec2_create(tmaxx, tminy), vec4_one()};
            vertex_2d p2 = (vertex_2d){vec2_create(maxx, maxy), vec2_create(tmaxx, tmaxy), vec4_one()};
            vertex_2d p3 = (vertex_2d){vec2_create(minx, maxy), vec2_create(tminx, tmaxy), vec4_one()};

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
        kfree(codepoints);
    }

    return true;
}

void sui_label_control_render_frame_prepare(struct sui_label* self) {
    if (self) {
        sui_label* out_control = self;
        if (out_control->is_dirty) {
            sui_label_pending_data pending_data = {0};
            if (!regenerate_label_geometry(self, &pending_data)) {
                KERROR("Error regenerating label geometry.");
                out_control->quad_count = 0;  // Keep it from drawing.
                goto sui_label_frame_prepare_cleanup;
            }

            // Upload it
            gl_renderer_buffer_upload_data(&self->vertex_buffer, pending_data.quad_count * 4, pending_data.vertex_buffer_data);
            gl_renderer_buffer_upload_data(&self->index_buffer, pending_data.quad_count * 6, pending_data.index_buffer_data);

            out_control->quad_count = pending_data.quad_count;

            // Update the max length if the string is now longer.
            if (pending_data.quad_count > out_control->max_quad_count) {
                out_control->max_quad_count = pending_data.quad_count;
            }

            // No longer dirty.
            out_control->is_dirty = false;

        sui_label_frame_prepare_cleanup:
            if (pending_data.vertex_buffer_data) {
                kfree(pending_data.vertex_buffer_data);
            }
            if (pending_data.index_buffer_data) {
                kfree(pending_data.index_buffer_data);
            }
        }
    }
}
