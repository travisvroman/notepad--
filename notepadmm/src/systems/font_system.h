#pragma once

#include "math/math_types.h"
#include "resources/resource_types.h"

struct renderer_state;

b8 font_system_initialize(u64* memory_requirement, void* memory, void* config);
void font_system_shutdown(void* memory);

KAPI b8 font_system_system_font_load(const char* full_path, u16 default_size);

KAPI font_data* font_system_acquire(u16 font_size);

KAPI b8 font_system_release(const char* name);

KAPI b8 font_system_verify_atlas(font_data* font, const char* text);

KAPI vec2 font_system_measure_string(font_data* font, const char* text);
