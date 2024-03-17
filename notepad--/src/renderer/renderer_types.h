#pragma once

#include "math/math_types.h"

struct texture_internal_data;

typedef struct texture {
    u32 width;
    u32 height;
    u8 channel_count;
    struct texture_internal_data* internal;
} texture;
