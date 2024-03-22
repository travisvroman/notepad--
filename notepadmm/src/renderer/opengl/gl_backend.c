#include "gl_backend.h"

#include <glad/glad.h>
//
#include <gl/GL.h>

#include "core/kmemory.h"
#include "core/logger.h"
#include "gl_types.h"
#include "platform/platform.h"

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

    if (!gladLoadGL()) {
        KERROR("Failed to load glad. Renderer cannot be initialized.");
        return false;
    }

    // Save off the version for reference.
    context->gl_version = (const char*)glGetString(GL_VERSION);

    GLuint vao, vbo, ibo, vertex_shader, fragment_shader, program;
    GLint mvp_location, vpos_location, vcol_location;
    // NOTE: OpenGL error checks have been omitted for brevity

    f32 vmin = 1.0f;
    f32 vmax = 50.0f;
    vertex_2df vertices[4];
    vertices[0] = (vertex_2df){vmin, vmin, 1.f, 0.f, 0.f};
    vertices[1] = (vertex_2df){vmax, vmin, 0.f, 1.f, 0.f};
    vertices[2] = (vertex_2df){vmin, vmax, 0.f, 0.f, 1.f};
    vertices[3] = (vertex_2df){vmax, vmax, 1.f, 1.f, 1.f};

    u32 indices[6] = {
        0, 1, 2,
        2, 1, 3};

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_2df) * 4, vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u32) * 6, indices, GL_STATIC_DRAW);

    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
    glCompileShader(vertex_shader);
    GLint isCompiled = 0;
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &isCompiled);
    if (isCompiled == GL_FALSE) {
        GLint maxLength = 0;
        glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &maxLength);

        // The maxLength includes the NULL character
        char* errorLog = kallocate(sizeof(char) * maxLength);
        glGetShaderInfoLog(vertex_shader, maxLength, &maxLength, &errorLog[0]);
        KERROR("Error compiling vertex_shader: %s", errorLog);

        // Provide the infolog in whatever manor you deem best.
        // Exit with failure.
        glDeleteShader(vertex_shader);  // Don't leak the shader.
        return false;
    }

    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &isCompiled);
    if (isCompiled == GL_FALSE) {
        GLint maxLength = 0;
        glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &maxLength);

        // The maxLength includes the NULL character
        char* errorLog = kallocate(sizeof(char) * maxLength);
        glGetShaderInfoLog(fragment_shader, maxLength, &maxLength, &errorLog[0]);
        KERROR("Error compiling fragment_shader: %s", errorLog);

        // Provide the infolog in whatever manor you deem best.
        // Exit with failure.
        glDeleteShader(fragment_shader);  // Don't leak the shader.
        return false;
    }

    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    KINFO("Shader compiled and linked successfully.");

    mvp_location = glGetUniformLocation(program, "MVP");
    vpos_location = glGetAttribLocation(program, "vPos");
    vcol_location = glGetAttribLocation(program, "vCol");

    glEnableVertexAttribArray(vpos_location);
    glVertexAttribPointer(vpos_location, 2, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (void*)0);
    glEnableVertexAttribArray(vcol_location);
    glVertexAttribPointer(vcol_location, 3, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (void*)(sizeof(float) * 2));

    glClearColor(1, 0, 1, 1);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);

    return true;
}

void gl_renderer_shutdown(gl_context* context) {
    if (context) {
        // TODO:
    }
}

b8 gl_renderer_frame_prepare(gl_context* context) {
    if (!context) {
        return false;
    }

    // Make sure to bind the window frambebuffer.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // TODO: should probably only do this on resize...
    i32 width, height;
    platform_window_size(context->platform, &width, &height);

    // Ensure the viewport is correct.
    glViewport(0, 0, width, height);

    return true;
}

b8 gl_renderer_frame_begin(gl_context* context) {
    if (!context) {
        return false;
    }

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
