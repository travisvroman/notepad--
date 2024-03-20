#include "application.h"

#include "core/kmemory.h"
#include "core/logger.h"
#include "math/kmath.h"

//
#include <glad/glad.h>
//
#include <GLFW/glfw3.h>
#include <stdio.h>

static const f32 vmod = 1000.0f;
// static const f32 vmod = 1.0f;

static const struct
{
    float x, y;
    float r, g, b;
} vertices[3] =
    {
        {-0.6f * vmod, -0.4f * vmod, 1.f, 0.f, 0.f},
        {+0.6f * vmod, -0.4f * vmod, 0.f, 1.f, 0.f},
        {+0.0f * vmod, +0.6f * vmod, 0.f, 0.f, 1.f}};

static const char* vertex_shader_text =
    "#version 110\n"
    "uniform mat4 MVP;\n"
    "attribute vec3 vCol;\n"
    "attribute vec2 vPos;\n"
    "varying vec3 color;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = MVP * vec4(vPos, 0.0, 1.0);\n"
    "    color = vCol;\n"
    "}\n";

static const char* fragment_shader_text =
    "#version 110\n"
    "varying vec3 color;\n"
    "void main()\n"
    "{\n"
    "    gl_FragColor = vec4(color, 1.0);\n"
    "}\n";

static void error_callback(int error, const char* description) {
    fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

b8 application_run(application_state* out_state) {
    if (!out_state) {
        KFATAL("application_create requires a valid pointer to an application state.");
        return false;
    }

    kzero_memory(out_state, sizeof(application_state));

    out_state->is_suspended = false;
    out_state->width = 1280;
    out_state->height = 720;
    out_state->resizing = false;
    out_state->frames_since_resize = 0;

    // Ensure the state flag is flipped.
    out_state->is_running = true;

    GLFWwindow* window;
    GLuint vertex_buffer, vertex_shader, fragment_shader, program;
    GLint mvp_location, vpos_location, vcol_location;

    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) {
        KERROR("Failed to intialize glfw");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    window = glfwCreateWindow(out_state->width, out_state->height, "Notepad--", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return false;
    }

    glfwSetKeyCallback(window, key_callback);

    glfwMakeContextCurrent(window);
    // gladLoadGL(glfwGetProcAddress);
    if (!gladLoadGL()) {
        KERROR("Failed to load glad.");
        return false;
    }
    glfwSwapInterval(1);

    // NOTE: OpenGL error checks have been omitted for brevity

    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
    glCompileShader(vertex_shader);

    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
    glCompileShader(fragment_shader);

    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    mvp_location = glGetUniformLocation(program, "MVP");
    vpos_location = glGetAttribLocation(program, "vPos");
    vcol_location = glGetAttribLocation(program, "vCol");

    glEnableVertexAttribArray(vpos_location);
    glVertexAttribPointer(vpos_location, 2, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (void*)0);
    glEnableVertexAttribArray(vcol_location);
    glVertexAttribPointer(vcol_location, 3, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (void*)(sizeof(float) * 2));

    glClearColor(1, 0, 1, 1);
    // glCullFace(GL_NONE);

    while (!glfwWindowShouldClose(window)) {
        int width, height;
        mat4 m, p, mvp;

        glfwGetFramebufferSize(window, &width, &height);

        glViewport(0, height, width, -height);
        glClear(GL_COLOR_BUFFER_BIT);

        m = mat4_identity();
        // m = mat4_euler_z((float)glfwGetTime());
        //
#if 0
        float ratio;
        ratio = width / (float)height;
        p = mat4_orthographic(-ratio, ratio, -1.f, 1.f, 1.f, -1.f);
#else
        p = mat4_orthographic(0, width, height, 0, 100.0f, -100.0f);
#endif
        mvp = mat4_mul(p, m);
        // mvp = mat4_mul(m, p);

        glUseProgram(program);
        glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*)mvp.data);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();

    return true;
}
