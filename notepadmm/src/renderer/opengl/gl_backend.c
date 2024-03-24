#include "gl_backend.h"

#include <glad/glad.h>
//
#include <gl/GL.h>

#include "containers/darray.h"
#include "core/kmemory.h"
#include "core/logger.h"
#include "gl_types.h"
#include "math/math_types.h"
#include "platform/platform.h"

static const char* vertex_shader_text =
    "#version 330 core\n"
    "uniform mat4 u_mvp;\n"
    "layout (location = 0) in vec2 a_position;\n"
    "layout (location = 1) in vec2 a_texcoord;\n"
    "layout (location = 2) in vec4 a_colour;\n"
    "out vec4 frag_colour;\n"
    "out vec2 frag_texcoord;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = u_mvp * vec4(a_position, 0.0, 1.0);\n"
    "    frag_colour = a_colour;\n"
    "    frag_texcoord = a_texcoord;\n"
    "}\n";

static const char* fragment_shader_text =
    "#version 330 core\n"
    "uniform sampler2D diffuse;\n"
    "in vec4 frag_colour;\n"
    "in vec2 frag_texcoord;\n"
    "out vec4 out_colour;\n"
    "void main()\n"
    "{\n"
    "    out_colour = frag_colour + texture(diffuse, frag_texcoord.xy);\n"
    "}\n";

static i32 compile_shader(const char* source, GLenum shader_type) {
    u32 shader = glCreateShader(shader_type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint isCompiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
    if (isCompiled == GL_FALSE) {
        GLint maxLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

        // The maxLength includes the NULL character
        char* errorLog = kallocate(sizeof(char) * maxLength);
        glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);
        KERROR("Error compiling %s shader: %s", shader_type == GL_VERTEX_SHADER ? "vertex" : "fragment", errorLog);

        // Provide the infolog in whatever manor you deem best.
        // Exit with failure.
        glDeleteShader(shader);  // Don't leak the shader.
        return -1;
    }
    return shader;
}

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

    // Some global configuration.
    glClearColor(1, 0, 1, 1);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);

    // Layout for textured 2d vertices.
    context->textured_vertex_2d_layout.attributes = darray_create(gl_vertex_attribute);
    context->textured_vertex_2d_layout.stride = sizeof(vertex_2d);

    // Position
    gl_vertex_attribute attribute_position = {0};
    attribute_position.name = "a_position";
    attribute_position.location = 0;
    attribute_position.offset = 0;
    attribute_position.size = 2;
    attribute_position.type = GL_FLOAT;
    darray_push(context->textured_vertex_2d_layout.attributes, attribute_position);

    // texcoord
    gl_vertex_attribute attribute_texcoord = {0};
    attribute_texcoord.name = "a_texcoord";
    attribute_texcoord.location = 1;
    attribute_texcoord.offset = sizeof(f32) * 2;
    attribute_texcoord.size = 2;
    attribute_texcoord.type = GL_FLOAT;
    darray_push(context->textured_vertex_2d_layout.attributes, attribute_texcoord);

    // colour
    gl_vertex_attribute attribute_colour = {0};
    attribute_colour.name = "a_colour";
    attribute_colour.location = 2;
    attribute_colour.offset = sizeof(f32) * 4;
    attribute_colour.size = 4;
    attribute_colour.type = GL_FLOAT;
    darray_push(context->textured_vertex_2d_layout.attributes, attribute_colour);

    // Initialize

    // Load the main shader.
    {
        i32 vertex_shader = compile_shader(vertex_shader_text, GL_VERTEX_SHADER);
        if (vertex_shader == -1) {
            return false;
        }

        i32 fragment_shader = compile_shader(fragment_shader_text, GL_FRAGMENT_SHADER);
        if (fragment_shader == -1) {
            return false;
        }

        context->main_shader.program = glCreateProgram();
        glAttachShader(context->main_shader.program, vertex_shader);
        glAttachShader(context->main_shader.program, fragment_shader);
        glLinkProgram(context->main_shader.program);

        GLint is_linked = 0;
        glGetProgramiv(context->main_shader.program, GL_LINK_STATUS, &is_linked);
        if (is_linked == GL_FALSE) {
            GLint maxLength = 0;
            glGetProgramiv(context->main_shader.program, GL_INFO_LOG_LENGTH, &maxLength);

            // The maxLength includes the NULL character
            char* errorLog = kallocate(sizeof(char) * maxLength);
            glGetProgramInfoLog(context->main_shader.program, maxLength, &maxLength, &errorLog[0]);
            KERROR("Error linking program: %s", errorLog);

            // Provide the infolog in whatever manor you deem best.
            // Exit with failure.
            glDeleteProgram(context->main_shader.program);  // Don't leak the shader.
            return false;
        }
        glUseProgram(context->main_shader.program);

        KINFO("Main shader compiled and linked successfully.");

        context->mvp_location = glGetUniformLocation(context->main_shader.program, "u_mvp");
        context->diffuse_location = glGetUniformLocation(context->main_shader.program, "diffuse");
        context->position_location = glGetAttribLocation(context->main_shader.program, "a_position");
        context->texcoord_location = glGetAttribLocation(context->main_shader.program, "a_texcoord");
        context->colour_location = glGetAttribLocation(context->main_shader.program, "a_colour");
    }

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

    glUseProgram(context->main_shader.program);

    return true;
}

b8 gl_renderer_frame_end(gl_context* context) {
    if (!context) {
        return false;
    }

    glBindVertexArray(0);

    return true;
}

void gl_renderer_on_resize(gl_context* context, u32 width, u32 height) {
    if (context) {
        /* context->framebuffer_width = width; */
        /* context->framebuffer_height = height; */
    }
}

gl_texture gl_renderer_texture_create(gl_context* context, u32 width, u32 height) {
    gl_texture t = {0};

    glGenTextures(1, &t.texture);
    glBindTexture(GL_TEXTURE_2D, t.texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Unbind the texture
    glBindTexture(GL_TEXTURE_2D, 0);

    return t;
}

void gl_renderer_texture_destroy(gl_context* context, gl_texture* t) {
    if (context && t) {
        glDeleteTextures(1, &t->texture);

        t->width = t->height = 0;
    }
}

b8 gl_renderer_texture_data_set(gl_context* context, gl_texture* t, const u8* pixels) {
    if (!t) {
        KERROR("gl_renderer_texture_data_set requires a valid pointer to a texture.");
        return false;
    }

    glBindTexture(GL_TEXTURE_2D, t->texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->width, t->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

gl_buffer gl_renderer_buffer_create(gl_context* context, u32 element_size, gl_buffer_type type) {
    gl_buffer b = {0};
    b.element_size = element_size;
    b.target = type == GL_BUFFER_TYPE_INDEX ? GL_ELEMENT_ARRAY_BUFFER : GL_ARRAY_BUFFER;
    b.usage = GL_STATIC_DRAW;

    glGenBuffers(1, &b.buffer);
    glBindBuffer(b.target, b.buffer);

    return b;
}

void gl_renderer_buffer_upload_data(gl_buffer* b, u32 element_count, void* data) {
    if (b) {
        b->element_count = element_count;
        glBindBuffer(b->target, b->buffer);
        glBufferData(b->target, b->element_size * element_count, data, b->usage);
    }
}

void gl_renderer_buffer_bind(gl_buffer* b) {
    if (b) {
        glBindBuffer(b->target, b->buffer);
    }
}

void gl_renderer_buffer_draw(gl_context* context, gl_buffer* b) {
    if (b) {
        if (b->target == GL_ELEMENT_ARRAY_BUFFER) {
            glDrawElements(GL_TRIANGLES, b->element_count, GL_UNSIGNED_INT, 0);
        } else {
            glDrawArrays(GL_TRIANGLES, b->target, b->buffer);
        }
    }
}

void gl_renderer_set_mvp(gl_context* context, mat4 mvp) {
    if (context) {
        glUniformMatrix4fv(context->mvp_location, 1, false, (const GLfloat*)mvp.data);
    }
}

u32 gl_renderer_acquire_instance(gl_context* context) {
    u32 vao_instance;
    glGenVertexArrays(1, &vao_instance);
    return vao_instance;
}

void gl_renderer_bind_instance(gl_context* context, u32 vao_instance) {
    glBindVertexArray(vao_instance);
}
void gl_renderer_unbind_instance(gl_context* context) {
    glBindVertexArray(0);
}

void gl_renderer_configure_instance_layout(gl_context* context, const gl_layout* layout) {
    u32 attribute_count = darray_length(layout->attributes);
    for (u32 a = 0; a < attribute_count; ++a) {
        gl_vertex_attribute* attrib = &layout->attributes[a];
        glVertexAttribPointer(attrib->location, attrib->size, GL_FLOAT, attrib->type, layout->stride, (void*)(attrib->offset));
        glEnableVertexAttribArray(attrib->location);
    }
}
