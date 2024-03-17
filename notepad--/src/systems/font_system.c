#include "font_system.h"

#include "containers/darray.h"
#include "containers/hashtable.h"
#include "core/kmemory.h"
#include "core/kstring.h"
#include "core/logger.h"
#include "platform/filesystem.h"
#include "renderer/renderer.h"
#include "renderer/renderer_types.h"
#include "resources/resource_types.h"

// For system fonts.
#define STB_TRUETYPE_IMPLEMENTATION
#include "vendor/stb_truetype.h"

typedef struct system_font_variant_data {
    // darray
    i32* codepoints;
    f32 scale;
} system_font_variant_data;

typedef struct system_font_lookup {
    u16 id;
    u16 reference_count;
    // darray
    font_data* size_variants;
    // A copy of all this is kept for each for convenience.
    u64 binary_size;
    char* face;
    void* font_binary;
    i32 offset;
    i32 index;
    stbtt_fontinfo info;
} system_font_lookup;

struct renderer_state;
typedef struct font_system_state {
    struct renderer_state* renderer;
    font_system_config config;
    hashtable system_font_lookup;
    system_font_lookup* system_fonts;
    void* system_hashtable_block;
} font_system_state;

static b8 setup_font_data(font_data* font);
static void cleanup_font_data(font_data* font);
static b8 create_system_font_variant(system_font_lookup* lookup, u16 size, const char* font_name, font_data* out_variant);
static b8 rebuild_system_font_variant_atlas(system_font_lookup* lookup, font_data* variant);
static b8 verify_system_font_size_variant(system_font_lookup* lookup, font_data* variant, const char* text);

static font_system_state* state_ptr;

b8 font_system_initialize(u64* memory_requirement, void* memory, void* config) {
    font_system_config* typed_config = (font_system_config*)config;
    if (typed_config->max_system_font_count == 0) {
        KFATAL("font_system_initialize - config.max_system_font_count must be > 0.");
        return false;
    }

    // Block of memory will contain state structure, then blocks for arrays, then blocks for hashtables.
    u64 struct_requirement = sizeof(font_system_state);
    u64 sys_array_requirement = sizeof(system_font_lookup) * typed_config->max_system_font_count;
    u64 sys_hashtable_requirement = sizeof(u16) * typed_config->max_system_font_count;
    *memory_requirement = struct_requirement + sys_array_requirement + sys_hashtable_requirement;

    if (!memory) {
        return true;
    }

    state_ptr = (font_system_state*)memory;
    state_ptr->config = *typed_config;
    state_ptr->renderer = typed_config->renderer;

    // The array blocks are after the state. Already allocated, so just set the pointer.
    void* sys_array_block = (void*)(((u8*)memory) + struct_requirement);

    state_ptr->system_fonts = sys_array_block;

    // Hashtable blocks are after arrays.
    void* sys_hashtable_block = (void*)(((u8*)sys_array_block) + sys_array_requirement);

    // Create hashtables for font lookups.
    hashtable_create(sizeof(u16), state_ptr->config.max_system_font_count, sys_hashtable_block, false, &state_ptr->system_font_lookup);

    // Fill both hashtables with invalid references to use as a default.
    u16 invalid_id = INVALID_ID_U16;
    hashtable_fill(&state_ptr->system_font_lookup, &invalid_id);

    // Invalidate all entries in both arrays.
    u32 count = state_ptr->config.max_system_font_count;
    for (u32 i = 0; i < count; ++i) {
        state_ptr->system_fonts[i].id = INVALID_ID_U16;
        state_ptr->system_fonts[i].reference_count = 0;
    }

    // Load up any default fonts.
    // System fonts.
    for (u32 i = 0; i < state_ptr->config.default_system_font_count; ++i) {
        if (!font_system_system_font_load(&state_ptr->config.system_font_configs[i])) {
            KERROR("Failed to load system font: %s", state_ptr->config.system_font_configs[i].name);
        }
    }

    return true;
}

void font_system_shutdown(void* memory) {
    if (memory) {
        // Cleanup system fonts.
        for (u16 i = 0; i < state_ptr->config.max_system_font_count; ++i) {
            if (state_ptr->system_fonts[i].id != INVALID_ID_U16) {
                // Cleanup each variant.
                u32 variant_count = darray_length(state_ptr->system_fonts[i].size_variants);
                for (u32 j = 0; j < variant_count; ++j) {
                    font_data* data = &state_ptr->system_fonts[i].size_variants[j];
                    cleanup_font_data(data);
                }
                state_ptr->system_fonts[i].id = INVALID_ID_U16;

                darray_destroy(state_ptr->system_fonts[i].size_variants);
                state_ptr->system_fonts[i].size_variants = 0;
            }
        }
    }
}

b8 font_system_system_font_load(system_font_config* config) {
    // For system fonts, they can actually contain multiple fonts. For this reason,
    // a copy of the resource's data will be held in each resulting variant, and the
    // resource itself will be released.
    //
    // TODO: consider font config.

    // Open binary font file.
    // Open and read the font file as binary, and save into an allocated
    // buffer on the resource itself.
    const char* full_path = "../assets/fonts/MesloLGS NF Regular.ttf";
    file_handle font_binary_handle;
    if (!filesystem_open(full_path, FILE_MODE_READ, true, &font_binary_handle)) {
        KERROR("Unable to open binary font file. Load process failed.");
        return false;
    }
    u64 file_size;
    if (!filesystem_size(&font_binary_handle, &file_size)) {
        KERROR("Unable to get file size of binary font file. Load process failed.");
        return false;
    }
    void* font_binary = kallocate(file_size, MEMORY_TAG_RESOURCE);
    u64 binary_size = 0;
    if (!filesystem_read_all_bytes(&font_binary_handle, font_binary, &binary_size)) {
        KERROR("Unable to perform binary read on font file. Load process failed.");
        return false;
    }

    // Might still work anyway, so continue.
    if (binary_size != file_size) {
        KWARN("Mismatch between filesize and bytes read in font file. File may be corrupt.");
    }

    filesystem_close(&font_binary_handle);

    // End binary font file load.

    // Keep a casted pointer to the resource data for convenience.
    /* system_font_resource_data* resource_data = (system_font_resource_data*)loaded_resource.data; */

    // Loop through the faces and create one lookup for each, as well as a default size
    // variant for each lookup.
    // HACK: hardcoding this for now to get it to run.
    u32 font_face_count = 1;  // darray_length(resource_data->fonts);
    system_font_face* fonts = darray_create(system_font_face);
    string_ncopy(fonts[0].name, "MesloLGS NF", 256);
    for (u32 i = 0; i < font_face_count; ++i) {
        system_font_face* face = &fonts[i];

        // Make sure a font with this name doesn't already exist.
        u16 id = INVALID_ID_U16;
        if (!hashtable_get(&state_ptr->system_font_lookup, face->name, &id)) {
            KERROR("Hashtable lookup failed. Font will not be loaded.");
            return false;
        }
        if (id != INVALID_ID_U16) {
            KWARN("A font named '%s' already exists and will not be loaded again.", config->name);
            // Not a hard error, return success since it already exists and can be used.
            return true;
        }

        // Get a new id
        for (u16 j = 0; j < state_ptr->config.max_system_font_count; ++j) {
            if (state_ptr->system_fonts[j].id == INVALID_ID_U16) {
                id = j;
                break;
            }
        }
        if (id == INVALID_ID_U16) {
            KERROR("No space left to allocate a new font. Increase maximum number allowed in font system config.");
            return false;
        }

        // Obtain the lookup.
        system_font_lookup* lookup = &state_ptr->system_fonts[id];
        lookup->binary_size = binary_size;
        lookup->font_binary = font_binary;
        lookup->face = string_duplicate(face->name);
        lookup->index = i;
        // To hold the size variants.
        lookup->size_variants = darray_create(font_data);

        // The offset
        lookup->offset = stbtt_GetFontOffsetForIndex(lookup->font_binary, i);
        i32 result = stbtt_InitFont(&lookup->info, lookup->font_binary, lookup->offset);
        if (result == 0) {
            // Zero indicates failure.
            KERROR("Failed to init system font %s at index %i.", full_path, i);
            return false;
        }

        // Create a default size variant.
        font_data variant;
        if (!create_system_font_variant(lookup, config->default_size, face->name, &variant)) {
            KERROR("Failed to create variant: %s, index %i", face->name, i);
            continue;
        }

        // Also perform setup for the variant
        if (!setup_font_data(&variant)) {
            KERROR("Failed to setup font data");
            continue;
        }

        // Add to the lookup's size variants.
        darray_push(lookup->size_variants, variant);

        // Set the entry id here last before updating the hashtable.
        lookup->id = id;
        if (!hashtable_set(&state_ptr->system_font_lookup, face->name, &id)) {
            KERROR("Hashtable set failed on font load.");
            return false;
        }
    }

    return true;
}

font_data* font_system_acquire(const char* font_name, u16 font_size) {
    u16 id = INVALID_ID_U16;
    if (!hashtable_get(&state_ptr->system_font_lookup, font_name, &id)) {
        KERROR("System font lookup failed on acquire.");
        return false;
    }

    if (id == INVALID_ID_U16) {
        KERROR("A system font named '%s' was not found. Font acquisition failed.", font_name);
        return false;
    }

    // Get the lookup.
    system_font_lookup* lookup = &state_ptr->system_fonts[id];

    // Search the size variants for the correct size.
    u32 count = darray_length(lookup->size_variants);
    for (u32 i = 0; i < count; ++i) {
        if (lookup->size_variants[i].size == font_size) {
            // Increment the reference.
            lookup->reference_count++;
            return &lookup->size_variants[i];
        }
    }

    // If we reach this point, the size variant doesn't exist. Create it.
    font_data variant;
    if (!create_system_font_variant(lookup, font_size, font_name, &variant)) {
        KERROR("Failed to create variant: %s, index %i, size %i", lookup->face, lookup->index, font_size);
        return false;
    }

    // Also perform setup for the variant
    if (!setup_font_data(&variant)) {
        KERROR("Failed to setup font data");
    }

    // Add to the lookup's size variants.
    darray_push(lookup->size_variants, variant);
    u32 length = darray_length(lookup->size_variants);

    // Increment the reference.
    lookup->reference_count++;
    return &lookup->size_variants[length - 1];
}

b8 font_system_release(const char* name) {
    // TODO: Lookup font by name in appropriate hashtable.
    return true;
}

b8 font_system_verify_atlas(font_data* font, const char* text) {
    u16 id = INVALID_ID_U16;
    if (!hashtable_get(&state_ptr->system_font_lookup, font->face, &id)) {
        KERROR("System font lookup failed on acquire.");
        return false;
    }

    if (id == INVALID_ID_U16) {
        KERROR("A system font named '%s' was not found. Font atlas verification failed.", font->face);
        return false;
    }

    // Get the lookup.
    system_font_lookup* lookup = &state_ptr->system_fonts[id];

    return verify_system_font_size_variant(lookup, font, text);
}

vec2 font_system_measure_string(font_data* font, const char* text) {
    vec2 extents = {0};

    u32 char_length = string_length(text);
    u32 text_length_utf8 = string_utf8_length(text);

    f32 x = 0;
    f32 y = 0;

    // Take the length in chars and get the correct codepoint from it.
    for (u32 c = 0; c < char_length; ++c) {
        i32 codepoint = text[c];

        // Continue to next line for newline.
        if (codepoint == '\n') {
            if (x > extents.x) {
                extents.x = x;
            }
            x = 0;
            y += font->line_height;
            continue;
        }

        if (codepoint == '\t') {
            x += font->tab_x_advance;
            continue;
        }

        // NOTE: UTF-8 codepoint handling.
        u8 advance = 0;
        if (!bytes_to_codepoint(text, c, &codepoint, &advance)) {
            KWARN("Invalid UTF-8 found in string, using unknown codepoint of -1");
            codepoint = -1;
        }

        font_glyph* g = 0;
        for (u32 i = 0; i < font->glyph_count; ++i) {
            if (font->glyphs[i].codepoint == codepoint) {
                g = &font->glyphs[i];
                break;
            }
        }

        if (!g) {
            // If not found, use the codepoint -1
            codepoint = -1;
            for (u32 i = 0; i < font->glyph_count; ++i) {
                if (font->glyphs[i].codepoint == codepoint) {
                    g = &font->glyphs[i];
                    break;
                }
            }
        }

        if (g) {
            // Try to find kerning
            i32 kerning = 0;

            // Get the offset of the next character. If there is no advance, move forward one,
            // otherwise use advance as-is.
            u32 offset = c + advance;  //(advance < 1 ? 1 : advance);
            if (offset < text_length_utf8 - 1) {
                // Get the next codepoint.
                i32 next_codepoint = 0;
                u8 advance_next = 0;

                if (!bytes_to_codepoint(text, offset, &next_codepoint, &advance_next)) {
                    KWARN("Invalid UTF-8 found in string, using unknown codepoint of -1");
                    codepoint = -1;
                } else {
                    for (u32 i = 0; i < font->kerning_count; ++i) {
                        font_kerning* k = &font->kernings[i];
                        if (k->codepoint_0 == codepoint && k->codepoint_1 == next_codepoint) {
                            kerning = k->amount;
                        }
                    }
                }
            }

            x += g->x_advance + kerning;
        } else {
            KERROR("Unable to find unknown codepoint. Skipping.");
            continue;
        }

        // Now advance c
        c += advance - 1;  // Subtracting 1 because the loop always increments once for single-byte anyway.
    }

    // One last check in case of no more newlines.
    if (x > extents.x) {
        extents.x = x;
    }

    // Since y starts 0-based, we need to add one more to make it 1-line based.
    y += font->line_height;
    extents.y = y;

    return extents;
}

static b8 setup_font_data(font_data* font) {
    // Check for a tab glyph, as there may not always be one exported. If there is, store its
    // x_advance and just use that. If there is not, then create one based off spacex4
    if (!font->tab_x_advance) {
        for (u32 i = 0; i < font->glyph_count; ++i) {
            if (font->glyphs[i].codepoint == '\t') {
                font->tab_x_advance = font->glyphs[i].x_advance;
                break;
            }
        }
        // If still not found, use space x 4.
        if (!font->tab_x_advance) {
            for (u32 i = 0; i < font->glyph_count; ++i) {
                // Search for space
                if (font->glyphs[i].codepoint == ' ') {
                    font->tab_x_advance = font->glyphs[i].x_advance * 4;
                    break;
                }
            }
            if (!font->tab_x_advance) {
                // If _still_ not there, then a space wasn't present either, so just
                // hardcode something, in this case font size * 4.
                font->tab_x_advance = font->size * 4;
            }
        }
    }

    return true;
}

static void cleanup_font_data(font_data* font) {
    renderer_texture_destroy(state_ptr->renderer, font->atlas);
    kfree(font->atlas, sizeof(texture), MEMORY_TAG_TEXTURE);
    font->atlas = 0;
}

static b8 create_system_font_variant(system_font_lookup* lookup, u16 size, const char* font_name, font_data* out_variant) {
    kzero_memory(out_variant, sizeof(font_data));
    out_variant->atlas_size_x = 1024;  // TODO: configurable size
    out_variant->atlas_size_y = 1024;
    out_variant->size = size;
    string_ncopy(out_variant->face, font_name, 255);
    out_variant->internal_data_size = sizeof(system_font_variant_data);
    out_variant->internal_data = kallocate(out_variant->internal_data_size, MEMORY_TAG_SYSTEM_FONT);

    system_font_variant_data* internal_data = (system_font_variant_data*)out_variant->internal_data;

    // Push default codepoints (ascii 32-127) always, plus a -1 for unknown.
    internal_data->codepoints = darray_reserve(i32, 96);
    darray_push(internal_data->codepoints, -1);  // push invalid char
    for (i32 i = 0; i < 95; ++i) {
        internal_data->codepoints[i + 1] = i + 32;
    }
    darray_length_set(internal_data->codepoints, 96);

    // Create texture.
    char font_tex_name[255];
    string_format(font_tex_name, "__system_text_atlas_%s_i%i_sz%i__", font_name, lookup->index, size);
    out_variant->atlas = kallocate(sizeof(texture), MEMORY_TAG_TEXTURE);
    out_variant->atlas->width = out_variant->atlas_size_x;
    out_variant->atlas->height = out_variant->atlas_size_y;
    out_variant->atlas->channel_count = 4;
    if (!renderer_texture_create(state_ptr->renderer, out_variant->atlas)) {
        KERROR("Failed to create font texture.");
        return false;
    }

    // Obtain some metrics
    internal_data->scale = stbtt_ScaleForPixelHeight(&lookup->info, (f32)size);
    i32 ascent, descent, line_gap;
    stbtt_GetFontVMetrics(&lookup->info, &ascent, &descent, &line_gap);
    out_variant->line_height = (ascent - descent + line_gap) * internal_data->scale;

    return rebuild_system_font_variant_atlas(lookup, out_variant);
}

static b8 rebuild_system_font_variant_atlas(system_font_lookup* lookup, font_data* variant) {
    system_font_variant_data* internal_data = (system_font_variant_data*)variant->internal_data;

    u32 pack_image_size = variant->atlas_size_x * variant->atlas_size_y * sizeof(u8);
    u8* pixels = kallocate(pack_image_size, MEMORY_TAG_ARRAY);
    u32 codepoint_count = darray_length(internal_data->codepoints);
    stbtt_packedchar* packed_chars = kallocate(sizeof(stbtt_packedchar) * codepoint_count, MEMORY_TAG_ARRAY);

    // Begin packing all known characters into the atlas. This
    // creates a single-channel image with rendered glyphs at the
    // given size.
    stbtt_pack_context context;
    if (!stbtt_PackBegin(&context, pixels, variant->atlas_size_x, variant->atlas_size_y, 0, 1, 0)) {
        KERROR("stbtt_PackBegin failed");
        return false;
    }

    // Fit all codepoints into a single range for packing.
    stbtt_pack_range range;
    range.first_unicode_codepoint_in_range = 0;
    range.font_size = variant->size;
    range.num_chars = codepoint_count;
    range.chardata_for_range = packed_chars;
    range.array_of_unicode_codepoints = internal_data->codepoints;
    if (!stbtt_PackFontRanges(&context, lookup->font_binary, lookup->index, &range, 1)) {
        KERROR("stbtt_PackFontRanges failed");
        return false;
    }

    stbtt_PackEnd(&context);
    // Packing complete.

    // Convert from single-channel to RGBA, or pack_image_size * 4.
    u8* rgba_pixels = kallocate(pack_image_size * 4, MEMORY_TAG_ARRAY);
    for (u32 j = 0; j < pack_image_size; ++j) {
        rgba_pixels[(j * 4) + 0] = pixels[j];
        rgba_pixels[(j * 4) + 1] = pixels[j];
        rgba_pixels[(j * 4) + 2] = pixels[j];
        rgba_pixels[(j * 4) + 3] = pixels[j];
    }

    // Write texture data to atlas.
    if (!renderer_texture_data_set(state_ptr->renderer, variant->atlas, rgba_pixels)) {
        KERROR("Failed to write font texture data");
        return false;
    }

    // Free pixel/rgba_pixel data.
    kfree(pixels, pack_image_size, MEMORY_TAG_ARRAY);
    kfree(rgba_pixels, pack_image_size * 4, MEMORY_TAG_ARRAY);

    // Regenerate glyphs
    if (variant->glyphs && variant->glyph_count) {
        kfree(variant->glyphs, sizeof(font_glyph) * variant->glyph_count, MEMORY_TAG_ARRAY);
    }
    variant->glyph_count = codepoint_count;
    variant->glyphs = kallocate(sizeof(font_glyph) * codepoint_count, MEMORY_TAG_ARRAY);
    for (u16 i = 0; i < variant->glyph_count; ++i) {
        stbtt_packedchar* pc = &packed_chars[i];
        font_glyph* g = &variant->glyphs[i];
        g->codepoint = internal_data->codepoints[i];
        g->page_id = 0;
        g->x_offset = pc->xoff;
        g->y_offset = pc->yoff;
        g->x = pc->x0;  // xmin;
        g->y = pc->y0;
        g->width = pc->x1 - pc->x0;
        g->height = pc->y1 - pc->y0;
        g->x_advance = pc->xadvance;
    }

    // Regenerate kernings
    if (variant->kernings && variant->kerning_count) {
        kfree(variant->kernings, sizeof(font_kerning) * variant->kerning_count, MEMORY_TAG_ARRAY);
    }
    variant->kerning_count = stbtt_GetKerningTableLength(&lookup->info);
    if (variant->kerning_count) {
        variant->kernings = kallocate(sizeof(font_kerning) * variant->kerning_count, MEMORY_TAG_ARRAY);
        // Get the kerning table for the current font.
        stbtt_kerningentry* kerning_table = kallocate(sizeof(stbtt_kerningentry) * variant->kerning_count, MEMORY_TAG_ARRAY);
        u32 entry_count = stbtt_GetKerningTable(&lookup->info, kerning_table, variant->kerning_count);
        if (entry_count != variant->kerning_count) {
            KERROR("Kerning entry count mismatch: %u->%u", entry_count, variant->kerning_count);
            return false;
        }

        for (u32 i = 0; i < variant->kerning_count; ++i) {
            font_kerning* k = &variant->kernings[i];
            k->codepoint_0 = kerning_table[i].glyph1;
            k->codepoint_1 = kerning_table[i].glyph2;
            k->amount = kerning_table[i].advance;
        }

        kfree(kerning_table, sizeof(stbtt_kerningentry) * variant->kerning_count, MEMORY_TAG_ARRAY);
    } else {
        variant->kernings = 0;
    }

    return true;
}

static b8 verify_system_font_size_variant(system_font_lookup* lookup, font_data* variant, const char* text) {
    system_font_variant_data* internal_data = (system_font_variant_data*)variant->internal_data;

    u32 char_length = string_length(text);
    u32 added_codepoint_count = 0;
    for (u32 i = 0; i < char_length;) {
        i32 codepoint;
        u8 advance;
        if (!bytes_to_codepoint(text, i, &codepoint, &advance)) {
            KERROR("bytes_to_codepoint failed to get codepoint.");
            ++i;
            continue;
        } else {
            // Check if the codepoint is already contained. Note that ascii
            // codepoints are always included, so checking those may be skipped.
            i += advance;
            if (codepoint < 128) {
                continue;
            }
            u32 codepoint_count = darray_length(internal_data->codepoints);
            b8 found = false;
            for (u32 j = 95; j < codepoint_count; ++j) {
                if (internal_data->codepoints[j] == codepoint) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                darray_push(internal_data->codepoints, codepoint);
                added_codepoint_count++;
            }
        }
    }

    // If codepoints were added, rebuild the atlas.
    if (added_codepoint_count > 0) {
        return rebuild_system_font_variant_atlas(lookup, variant);
    }

    // Otherwise, proceed as normal.
    return true;
}
