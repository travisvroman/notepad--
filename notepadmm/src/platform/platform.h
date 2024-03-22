#pragma once

#include "defines.h"

struct platform_state;
struct platform_window;

b8 platform_initialize(u64* memory_requirement, struct platform_state* out_state);

void platform_shutdown(struct platform_state* state);

b8 platform_running(struct platform_state* state);

void platform_window_size(struct platform_state* state, i32* out_width, i32* out_height);

f32 platform_time_get(struct platform_state* state);

b8 platform_pump_messages(struct platform_state* state);

b8 platform_pump_messages(struct platform_state* state);

b8 platform_present(struct platform_state* state);
