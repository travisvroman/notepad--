#include "application.h"

#include "core/kmemory.h"
#include "core/logger.h"
#include "math/kmath.h"

// GLAD import must come before GLFW.
#include <glad/glad.h>
// Ensure GLFW comes afterward.
#include <GLFW/glfw3.h>
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
    GLuint vao, vbo, ibo, vertex_shader, fragment_shader, program;
    GLint mvp_location, vpos_location, vcol_location;

    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) {
        KERROR("Failed to intialize glfw");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);  // uncomment this statement to fix compilation on OS X
#endif

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

    while (!glfwWindowShouldClose(window)) {
        // Prepare frame
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        // Begin frame
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        mat4 model = mat4_identity();
        model = mat4_euler_z((float)glfwGetTime());
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

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();

    return true;
}
