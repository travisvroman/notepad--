/*
#include "gl_backend.h"

#include <glad/glad.h>
//
#include <gl/GL.h>

#include "core/kmemory.h"
#include "core/logger.h"
#include "gl_types.h"
#include "renderer/opengl/platform/opengl_platform.h"
#include "renderer/renderer_types.h"

typedef struct texture_internal_data {
    u32 texture_id;
} texture_internal_data;

b8 gl_renderer_initialize(gl_context* context, gl_renderer_config config) {
    if (!context) {
        KERROR("context is required for gl renderer initialization.");
        return false;
    }
    if (!config.platform) {
        KERROR("A valid pointer to platform state is required for gl renderer initialization.");
        return false;
    }

    context->platform = config.platform;

    if (!platform_create_rendering_context(context)) {
        KERROR("Failed to create platform rendering context. Renderer cannot be initialized.");
        return false;
    }

    if (!gladLoadGL()) {
        KERROR("Failed to load glad. Renderer cannot be initialized.");
        return false;
    }

    // Save off the version for reference.
    context->gl_version = (const char*)glGetString(GL_VERSION);

    glClearColor(0.0f, 0.0f, 0.2f, 1.0f);

    return true;
}

void gl_renderer_shutdown(gl_context* context) {
    if (context) {
        platform_destroy_rendering_context(context);
    }
}

b8 gl_renderer_frame_prepare(gl_context* context) {
    if (!context) {
        return false;
    }

    return true;
}

b8 gl_renderer_frame_begin(gl_context* context) {
    if (!context) {
        return false;
    }
    // Make sure to bind the window frambebuffer.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Ensure the viewport is correct.
    glViewport(0, 0, context->framebuffer_width, context->framebuffer_height);

    // Clear the screen.
    glClear(GL_COLOR_BUFFER_BIT);

    return true;
}

b8 gl_renderer_frame_end(gl_context* context) {
    if (!context) {
        return false;
    }

    if (!platform_swap_buffers(context)) {
        KERROR("Platform buffer swap failed!");
        return false;
    }

    return true;
}

void gl_renderer_on_resize(gl_context* context, u32 width, u32 height) {
    if (context) {
        context->framebuffer_width = width;
        context->framebuffer_height = height;
    }
}

b8 gl_renderer_texture_create(gl_context* context, struct texture* out_texture) {
    if (!out_texture) {
        KERROR("gl_renderer_texture_create requires a valid pointer to a texture.");
        return false;
    }

    out_texture->internal = kallocate(sizeof(texture_internal_data), MEMORY_TAG_TEXTURE);

    glGenTextures(1, &out_texture->internal->texture_id);
    glBindTexture(GL_TEXTURE_2D, out_texture->internal->texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Unbind the texture
    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

void gl_renderer_texture_destroy(gl_context* context, struct texture* t) {
    if (context && t) {
        if (t->internal) {
            glDeleteTextures(1, &t->internal->texture_id);
            kfree(t->internal, sizeof(texture_internal_data), MEMORY_TAG_TEXTURE);
            t->internal = 0;
        }

        t->width = t->height = 0;
    }
}

b8 gl_renderer_texture_data_set(gl_context* context, struct texture* t, const u8* pixels) {
    if (!t) {
        KERROR("gl_renderer_texture_data_set requires a valid pointer to a texture.");
        return false;
    }

    glBindTexture(GL_TEXTURE_2D, t->internal->texture_id);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->width, t->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}
*/
