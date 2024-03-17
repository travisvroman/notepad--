#pragma once

#include "defines.h"

typedef struct system_state_info {
    u64 state_memory_requirement;
    void* state;
} system_state_info;

struct platform_state;
typedef struct platform_system_state_info {
    u64 state_memory_requirement;
    struct platform_state* state;
} platform_system_state_info;

struct renderer_state;
typedef struct renderer_system_state_info {
    u64 state_memory_requirement;
    struct renderer_state* state;
} renderer_system_state_info;

typedef struct applicaton_state {
    b8 is_running;

    b8 is_suspended;
    i16 width;
    i16 height;

    // Indicates if the window is currently being resized.
    b8 resizing;
    // The current number of frames since the last resize operation.
    // Only set if resizing = true. Otherwise 0.
    u8 frames_since_resize;

    system_state_info event_state;
    platform_system_state_info platform_state;
    system_state_info input_state;
    renderer_system_state_info renderer_state;
    system_state_info font_state;
} application_state;

b8 application_create(application_state* out_state);

void application_destroy(application_state* state);

b8 application_update(application_state* state);
