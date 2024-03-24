#pragma once

#include "defines.h"

struct platform_state;
// Platform-specific internal data.
struct gl_platform_data;

typedef enum gl_buffer_type {
    GL_BUFFER_TYPE_VERTEX,
    GL_BUFFER_TYPE_INDEX
} gl_buffer_type;

typedef struct gl_buffer {
    gl_buffer_type type;
    u32 buffer;
    u32 target;
    u32 usage;
    u32 element_size;
    u32 element_count;
} gl_buffer;

typedef struct gl_uniform {
    i32 location;
    const char* name;
} gl_uniform;

typedef struct gl_vertex_attribute {
    i32 location;
    u64 offset;
    // Number of floats/ints/etc for this attribute.
    u32 size;
    u32 type;
    const char* name;
} gl_vertex_attribute;

// Defines a vertex/attribute layout.
typedef struct gl_layout {
    // Element stride.
    u32 stride;
    // darray
    gl_vertex_attribute* attributes;
} gl_layout;

// Defines a shader and its uniforms.
typedef struct gl_shader {
    const char* name;
    u32 program;
    // darray
    gl_uniform* uniforms;
} gl_shader;

typedef struct gl_texture {
    u32 texture;
    u32 width;
    u32 height;
} gl_texture;

typedef struct gl_context {
    struct platform_state* platform;
    struct gl_platform_data* data;

    const char* gl_version;

    // Textured vertex 2d layout. Created during renderer init.
    gl_layout textured_vertex_2d_layout;

    gl_shader main_shader;

    i32 mvp_location;
    i32 diffuse_location;
    i32 position_location;
    i32 texcoord_location;
    i32 colour_location;
} gl_context;
