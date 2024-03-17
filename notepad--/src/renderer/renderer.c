#include "renderer.h"

#include "core/logger.h"
#include "renderer/opengl/gl_backend.h"

b8 renderer_initialize(u64* memory_requirement, renderer_state* state, renderer_config config) {
    if (!memory_requirement) {
        KERROR("renderer_initialize requires a valid pointer to memory_requirement.");
        return false;
    }

    *memory_requirement = sizeof(renderer_state) + sizeof(struct gl_context*);

    if (!state) {
        return true;
    }

    state->context = (struct gl_context*)(((u64)state) + sizeof(renderer_state));

    gl_renderer_config gl_config = {0};
    gl_config.platform = config.platform;

    return gl_renderer_initialize(state->context, gl_config);
}

void renderer_shutdown(renderer_state* state) {
    if (state) {
        gl_renderer_shutdown(state->context);
    }
}

b8 renderer_frame_prepare(renderer_state* state) {
    if (!state || !state->context) {
        return false;
    }

    return gl_renderer_frame_prepare(state->context);
}

b8 renderer_frame_begin(renderer_state* state) {
    if (!state || !state->context) {
        return false;
    }

    return gl_renderer_frame_begin(state->context);
}

b8 renderer_frame_end(renderer_state* state) {
    if (!state || !state->context) {
        return false;
    }

    return gl_renderer_frame_end(state->context);
}

b8 renderer_texture_create(renderer_state* state, struct texture* out_texture) {
    return gl_renderer_texture_create(state->context, out_texture);
}

void renderer_texture_destroy(renderer_state* state, struct texture* t) {
    gl_renderer_texture_destroy(state->context, t);
}

b8 renderer_texture_data_set(renderer_state* state, struct texture* t, const u8* pixels) {
    return gl_renderer_texture_data_set(state->context, t, pixels);
}
