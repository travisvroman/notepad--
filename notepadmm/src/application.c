#include "application.h"

#include "core/kmemory.h"
#include "core/logger.h"
#include "math/kmath.h"
#include "math/math_types.h"
#include "platform/platform.h"
#include "renderer/opengl/gl_backend.h"
#include "renderer/opengl/gl_types.h"

// GLAD import must come before GLFW.
/* #include <glad/glad.h> */
/* // Ensure GLFW comes afterward.
#include <GLFW/glfw3.h> */
#include <stdio.h>
#include <stdlib.h>

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
    gl_renderer_config renderer_config = {0};
    renderer_config.platform = out_state->platform_state.state;
    if (!gl_renderer_initialize(&out_state->gl, renderer_config)) {
        KERROR("Failed to initialize gl renderer.");
        return false;
    }

    // NOTE: Every "instance" of something (i.e. text) will need one of these,
    // and to be created in the acquire/bind/create vertex/create index/configure/unbind order.
    {
        u32 instance = gl_renderer_acquire_instance(&out_state->gl);
        gl_renderer_bind_instance(&out_state->gl, instance);

        // Create buffers.
        out_state->vertex_buffer = gl_renderer_buffer_create(&out_state->gl, sizeof(vertex_2d), GL_BUFFER_TYPE_VERTEX);
        out_state->index_buffer = gl_renderer_buffer_create(&out_state->gl, sizeof(u32), GL_BUFFER_TYPE_INDEX);

        gl_renderer_configure_instance_layout(&out_state->gl, &out_state->gl.textured_vertex_2d_layout);
        gl_renderer_unbind_instance(&out_state->gl);
    }

    // We can then upload the data.
    f32 vmin = 1.0f;
    f32 vmax = 50.0f;
    f32 tmin = 0.0f;
    f32 tmax = 1.0f;
    vertex_2d vertices[4];
    vertices[0] = (vertex_2d){vmin, vmin, tmin, tmin, 1.f, 0.f, 0.0f, 1.0f};
    vertices[1] = (vertex_2d){vmax, vmin, tmax, tmin, 0.f, 1.f, 0.0f, 1.0f};
    vertices[2] = (vertex_2d){vmin, vmax, tmin, tmax, 0.f, 0.f, 1.0f, 1.0f};
    vertices[3] = (vertex_2d){vmax, vmax, tmax, tmax, 1.f, 1.f, 1.0f, 1.0f};

    u32 indices[6] = {
        0, 1, 2,
        2, 1, 3};

    // Upload vertex data.
    gl_renderer_buffer_bind(&out_state->vertex_buffer);
    gl_renderer_buffer_upload_data(&out_state->vertex_buffer, 4, vertices);

    // Upload index data.
    gl_renderer_buffer_bind(&out_state->index_buffer);
    gl_renderer_buffer_upload_data(&out_state->index_buffer, 6, indices);

    // Main application loop.
    while (platform_running(out_state->platform_state.state)) {
        // Prepare frame
        int width, height;
        platform_window_size(out_state->platform_state.state, &width, &height);

        if (!gl_renderer_frame_prepare(&out_state->gl)) {
            KERROR("GL frame prepare failed!");
            return false;
        }

        // Begin
        if (!gl_renderer_frame_begin(&out_state->gl)) {
            KERROR("Failed to begin frame!");
            return false;
        }

        mat4 model = mat4_identity();
        model = mat4_euler_z(platform_time_get(out_state->platform_state.state));
        mat4 projection = mat4_orthographic(0, width, height, 0, -100.0f, 100.0f);

        /* mat4 mvp = mat4_mul(projection, model); */
        mat4 mvp = mat4_mul(model, projection);

        // Uniforms
        gl_renderer_set_mvp(&out_state->gl, mvp);

        // Draw
        // NOTE: Every "instance" of a thing will need to bind the instance first before draw.
        gl_renderer_bind_instance(&out_state->gl, instance);
        gl_renderer_buffer_draw(&out_state->gl, &out_state->index_buffer);

        gl_renderer_frame_end(&out_state->gl);

        // Present
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
