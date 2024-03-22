#include "platform.h"

#include "core/logger.h"

//
#include <GLFW/glfw3.h>
#include <stdio.h>

typedef struct platform_state {
    GLFWwindow* window;
    i16 width;
    i16 height;
} platform_state;

static void error_callback(int error, const char* description) {
    fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

b8 platform_initialize(u64* memory_requirement, struct platform_state* out_state) {
    *memory_requirement = sizeof(platform_state);
    if (!out_state) {
        return true;
    }

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

    out_state->width = 1280;
    out_state->height = 720;
    out_state->window = glfwCreateWindow(out_state->width, out_state->height, "Notepad--", NULL, NULL);

    if (!out_state->window) {
        glfwTerminate();
        return false;
    }

    glfwSetKeyCallback(out_state->window, key_callback);

    glfwMakeContextCurrent(out_state->window);

    // Vsync TODO: this was after gladLoadGL() - make sure this works
    glfwSwapInterval(1);

    return true;
}

void platform_shutdown(struct platform_state* state) {
    if (state) {
        glfwDestroyWindow(state->window);
        glfwTerminate();
    }
}

b8 platform_running(struct platform_state* state) {
    if (!state) {
        return false;
    }
    return !glfwWindowShouldClose(state->window);
}

void platform_window_size(struct platform_state* state, i32* out_width, i32* out_height) {
    glfwGetFramebufferSize(state->window, out_width, out_height);
}

f32 platform_time_get(struct platform_state* state) {
    return glfwGetTime();
}

b8 platform_pump_messages(struct platform_state* state) {
    if (!state) {
        return false;
    }

    glfwPollEvents();

    return true;
}

b8 platform_present(struct platform_state* state) {
    if (!state) {
        return false;
    }

    glfwSwapBuffers(state->window);

    return true;
}
