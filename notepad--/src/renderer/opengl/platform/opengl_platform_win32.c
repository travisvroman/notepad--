#ifdef _WIN32
#include "core/asserts.h"
#include "core/kmemory.h"
#include "core/kstring.h"
#include "core/logger.h"
#include "platform/platform.h"
#include "renderer/opengl/gl_types.h"
#include "renderer/opengl/platform/opengl_platform.h"

// platform-specific header.
#include "platform/platform_win32.h"

// gl
#include <glad/glad.h>
/* #include <gl/GL.h> */

typedef struct gl_platform_data {
    // Window handle to device context
    HDC hdc;
    HGLRC gl_rendering_context;
} gl_platform_data;

#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0X2092
#define WGL_CONTEXT_FLAGS_ARB 0X2094
#define WGL_CONTEXT_COREPROFILE_BIT_ARB 0x00000001
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126
typedef HGLRC(WINAPI* PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC hDC, HGLRC hShareContext, const int* attribList);

b8 platform_create_rendering_context(gl_context* context) {
    KASSERT_MSG(context, "platform_create_rendering_context requires a valid pointer to a gl_context.");

    platform_handle* handle = platform_handle_get(context->platform);

    context->data = kallocate(sizeof(gl_platform_data), MEMORY_TAG_RENDERER);

    // NOTE: This is specific to a window, so it will need to be
    // done once per window.
    context->data->hdc = GetDC(handle->hwnd);

    PIXELFORMATDESCRIPTOR pfd = {0};
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cAlphaBits = 8;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;

    // Let Windows choose a pixel format.
    i32 chosen_pixel_format = ChoosePixelFormat(context->data->hdc, &pfd);
    if (!chosen_pixel_format) {
        KERROR("Failed to choose pixel format for device context.");
        platform_log_last_error();
        return false;
    }

    // Use the chosen pixel format.
    if (!SetPixelFormat(context->data->hdc, chosen_pixel_format, &pfd)) {
        KERROR("Failed to set device context pixel format");
        platform_log_last_error();
        return false;
    }

    // Create temp context.
    HGLRC temp_rc = wglCreateContext(context->data->hdc);
    if (!temp_rc) {
        KERROR("Failed to create temp GL rendering context.");
        platform_log_last_error();
        return false;
    }
    wglMakeCurrent(context->data->hdc, temp_rc);
    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
    if (!wglCreateContextAttribsARB) {
        wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)GetProcAddress(GetModuleHandleA(0), "wglCreateContextAttribsARB");
    }

    const int attribList[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB,
        3,
        WGL_CONTEXT_MINOR_VERSION_ARB,
        3,
        WGL_CONTEXT_FLAGS_ARB,
        0,
        WGL_CONTEXT_PROFILE_MASK_ARB,
        WGL_CONTEXT_COREPROFILE_BIT_ARB,
        0,
    };

    // Now do the real context.
    context->data->gl_rendering_context = wglCreateContextAttribsARB(context->data->hdc, 0, attribList);
    // Disable the old context.
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(temp_rc);

    // Now do the real context.
    /* context->data->gl_rendering_context = wglCreateContext(context->data->window_handle_to_device_context); */
    // NOTE: Will likely need to change this back and forth for multi-window scenarios.
    wglMakeCurrent(context->data->hdc, context->data->gl_rendering_context);

    return true;
}

void platform_destroy_rendering_context(gl_context* context) {
    if (context) {
        platform_handle* handle = platform_handle_get(context->platform);

        glBindVertexArray(0);
        /* glDeleteVertexArrays(1, &vao); */

        wglMakeCurrent(0, 0);
        wglDeleteContext(context->data->gl_rendering_context);
        ReleaseDC(handle->hwnd, context->data->hdc);

        if (context->data) {
            // NOTE: would need to be done per window.
            wglDeleteContext(context->data->gl_rendering_context);
            kfree(context->data, sizeof(gl_platform_data), MEMORY_TAG_RENDERER);
            context->data = 0;
        }
        if (context->gl_version) {
            string_free((char*)context->gl_version);
            context->gl_version = 0;
        }
    }
}

b8 platform_swap_buffers(gl_context* context) {
    if (!context) {
        KERROR("platform_swap_buffers requires a valid pointer to context.");
        return false;
    }

    SwapBuffers(context->data->hdc);

    return true;
}

void platform_log_last_error(void) {
    DWORD last_error = GetLastError();
    LPSTR error_text = NULL;
    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        last_error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&error_text,
        0, NULL);
    if (error_text) {
        KERROR("Last error: %s", error_text);
        LocalFree(error_text);
    }
}

#endif
