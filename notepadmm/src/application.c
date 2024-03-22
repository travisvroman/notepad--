#include "application.h"

#include "core/kmemory.h"
#include "core/logger.h"
#include "math/kmath.h"
#include "platform/platform.h"

// GLAD import must come before GLFW.
#include <glad/glad.h>
/* // Ensure GLFW comes afterward.
#include <GLFW/glfw3.h> */
#include <stdio.h>

typedef struct vertex_2df {
    float x, y;
    float r, g, b;
} vertex_2df;

static const char* vertex_shader_text =
    "#version 330 core\n"
    "uniform mat4 MVP;\n"
    "layout (location = 0) in vec2 vPos;\n"
    "layout (location = 1) in vec3 vCol;\n"
    "out vec3 color;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = MVP * vec4(vPos, 0.0, 1.0);\n"
    "    color = vCol;\n"
    "}\n";

static const char* fragment_shader_text =
    "#version 330 core\n"
    "in vec3 color;\n"
    "out vec4 colour;\n"
    "void main()\n"
    "{\n"
    "    colour = vec4(color, 1.0);\n"
    "}\n";

b8 application_run(application_state* out_state) {
    if (!out_state) {
        KFATAL("application_create requires a valid pointer to an application state.");
        return false;
    }

    kzero_memory(out_state, sizeof(application_state));

    out_state->is_suspended = false;
    out_state->resizing = false;
    out_state->frames_since_resize = 0;

    // Setup platform.
    platform_initialize(&out_state->platform_state.state_memory_requirement, 0);
    out_state->platform_state.state = kallocate(out_state->platform_state.state_memory_requirement);
    if (!platform_initialize(&out_state->platform_state.state_memory_requirement, out_state->platform_state.state)) {
        KERROR("Failed to initialize platform layer. Aborting application.");
        return false;
    }

    // Ensure the state flag is flipped.
    out_state->is_running = true;

    // Setup renderer.
    if (!gladLoadGL()) {
        KERROR("Failed to load glad.");
        return false;
    }

    // LEFTOFF: moving renderer logic to renderer backend.
    if (!renderer_initialize()) {
        KERROR("Failed to initialize renderer.");
        return false;
    }

    while (platform_running(out_state->platform_state.state)) {
        // Prepare frame
        int width, height;
        platform_window_size(out_state->platform_state.state, &width, &height);
        glViewport(0, 0, width, height);

        // Begin frame
        glClear(GL_COLOR_BUFFER_BIT);

        mat4 model = mat4_identity();
        model = mat4_euler_z(platform_time_get(out_state->platform_state.state));
        mat4 projection = mat4_orthographic(0, width, height, 0, -100.0f, 100.0f);

        /* mat4 mvp = mat4_mul(projection, model); */
        mat4 mvp = mat4_mul(model, projection);

        glUseProgram(program);
        glBindVertexArray(vao);

        // Uniforms
        glUniformMatrix4fv(mvp_location, 1, false, (const GLfloat*)mvp.data);

        // Draw
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        if (!platform_present(out_state->platform_state.state)) {
            platform_shutdown(out_state->platform_state.state);
            return false;
        }

        if (!platform_pump_messages(out_state->platform_state.state)) {
            platform_shutdown(out_state->platform_state.state);
            return false;
        }
    }

    return true;
}
