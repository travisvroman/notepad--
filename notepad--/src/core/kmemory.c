#include "kmemory.h"

#include "core/kmutex.h"
#include "core/kstring.h"
#include "core/logger.h"
#include "platform/platform.h"

// TODO: Custom string lib
#include <stdio.h>
#include <string.h>

struct memory_stats {
    u64 total_allocated;
    u64 tagged_allocations[MEMORY_TAG_MAX_TAGS];
};

static const char* memory_tag_strings[MEMORY_TAG_MAX_TAGS] = {
    "UNKNOWN    ",
    "ARRAY      ",
    "LINEAR_ALLC",
    "DARRAY     ",
    "DICT       ",
    "RING_QUEUE ",
    "BST        ",
    "STRING     ",
    "ENGINE     ",
    "JOB        ",
    "TEXTURE    ",
    "MAT_INST   ",
    "RENDERER   ",
    "GAME       ",
    "TRANSFORM  ",
    "ENTITY     ",
    "ENTITY_NODE",
    "SCENE      ",
    "RESOURCE   ",
    "VULKAN     ",
    "VULKAN_EXT ",
    "DIRECT3D   ",
    "OPENGL     ",
    "GPU_LOCAL  ",
    "BITMAP_FONT",
    "SYSTEM_FONT",
    "KEYMAP     ",
    "HASHTABLE  ",
    "UI         ",
    "AUDIO      "};

typedef struct memory_system_state {
    struct memory_stats stats;
    u64 alloc_count;
    // A mutex for allocations/frees
    kmutex allocation_mutex;
} memory_system_state;

// Pointer to system state.
static memory_system_state* state_ptr;

b8 memory_system_initialize(void) {
    // The amount needed by the system state.
    u64 state_memory_requirement = sizeof(memory_system_state);

    // Call the platform allocator to get the memory for the whole system, including the state.
    void* block = platform_allocate(state_memory_requirement, true);
    if (!block) {
        KFATAL("Memory system allocation failed and the system cannot continue.");
        return false;
    }

    // The state is in the first part of the massive block of memory.
    state_ptr = (memory_system_state*)block;
    state_ptr->alloc_count = 0;
    platform_zero_memory(&state_ptr->stats, sizeof(state_ptr->stats));

    // Create allocation mutex
    if (!kmutex_create(&state_ptr->allocation_mutex)) {
        KFATAL("Unable to create allocation mutex!");
        return false;
    }

    KDEBUG("Memory system successfully initialized.");
    return true;
}

void memory_system_shutdown(void* state) {
    if (state_ptr) {
        // Destroy allocation mutex
        kmutex_destroy(&state_ptr->allocation_mutex);

        // Free the entire block.
        platform_free(state_ptr, sizeof(memory_system_state));
    }
    state_ptr = 0;
}

void* kallocate(u64 size, memory_tag tag) {
    return kallocate_aligned(size, 1, tag);
}

void* kallocate_aligned(u64 size, u16 alignment, memory_tag tag) {
    if (tag == MEMORY_TAG_UNKNOWN) {
        KWARN("kallocate_aligned called using MEMORY_TAG_UNKNOWN. Re-class this allocation.");
    }

    if (state_ptr) {
        // Make sure multithreaded requests don't trample each other.
        if (!kmutex_lock(&state_ptr->allocation_mutex)) {
            KFATAL("Error obtaining mutex lock during allocation.");
            return 0;
        }

        state_ptr->stats.total_allocated += size;
        state_ptr->stats.tagged_allocations[tag] += size;
        state_ptr->alloc_count++;

        kmutex_unlock(&state_ptr->allocation_mutex);
    }
    void* block = platform_allocate(size, false);

    if (block) {
        platform_zero_memory(block, size);
        return block;
    }

    KFATAL("kallocate_aligned failed to allocate successfully.");
    return 0;
}

void kallocate_report(u64 size, memory_tag tag) {
    // Make sure multithreaded requests don't trample each other.
    if (!kmutex_lock(&state_ptr->allocation_mutex)) {
        KFATAL("Error obtaining mutex lock during allocation reporting.");
        return;
    }
    state_ptr->stats.total_allocated += size;
    state_ptr->stats.tagged_allocations[tag] += size;
    state_ptr->alloc_count++;
    kmutex_unlock(&state_ptr->allocation_mutex);
}

void kfree(void* block, u64 size, memory_tag tag) {
    kfree_aligned(block, size, 1, tag);
}

void kfree_aligned(void* block, u64 size, u16 alignment, memory_tag tag) {
    if (tag == MEMORY_TAG_UNKNOWN) {
        KWARN("kfree_aligned called using MEMORY_TAG_UNKNOWN. Re-class this allocation.");
    }
    if (state_ptr) {
        // Make sure multithreaded requests don't trample each other.
        if (!kmutex_lock(&state_ptr->allocation_mutex)) {
            KFATAL("Unable to obtain mutex lock for free operation. Heap corruption is likely.");
            return;
        }

        state_ptr->stats.total_allocated -= size;
        state_ptr->stats.tagged_allocations[tag] -= size;
        state_ptr->alloc_count--;

        kmutex_unlock(&state_ptr->allocation_mutex);
    }
    platform_free(block, false);
}

void kfree_report(u64 size, memory_tag tag) {
    // Make sure multithreaded requests don't trample each other.
    if (!kmutex_lock(&state_ptr->allocation_mutex)) {
        KFATAL("Error obtaining mutex lock during allocation reporting.");
        return;
    }
    state_ptr->stats.total_allocated -= size;
    state_ptr->stats.tagged_allocations[tag] -= size;
    state_ptr->alloc_count--;
    kmutex_unlock(&state_ptr->allocation_mutex);
}

void* kzero_memory(void* block, u64 size) {
    return platform_zero_memory(block, size);
}

void* kcopy_memory(void* dest, const void* source, u64 size) {
    return platform_copy_memory(dest, source, size);
}

void* kset_memory(void* dest, i32 value, u64 size) {
    return platform_set_memory(dest, value, size);
}

const char* get_unit_for_size(u64 size_bytes, f32* out_amount) {
    if (size_bytes >= GIBIBYTES(1)) {
        *out_amount = (f64)size_bytes / GIBIBYTES(1);
        return "GiB";
    } else if (size_bytes >= MEBIBYTES(1)) {
        *out_amount = (f64)size_bytes / MEBIBYTES(1);
        return "MiB";
    } else if (size_bytes >= KIBIBYTES(1)) {
        *out_amount = (f64)size_bytes / KIBIBYTES(1);
        return "KiB";
    } else {
        *out_amount = (f32)size_bytes;
        return "B";
    }
}

char* get_memory_usage_str(void) {
    char buffer[8000] = "System memory use (tagged):\n";
    u64 offset = strlen(buffer);
    for (u32 i = 0; i < MEMORY_TAG_MAX_TAGS; ++i) {
        f32 amount = 1.0f;
        const char* unit = get_unit_for_size(state_ptr->stats.tagged_allocations[i], &amount);

        i32 length = snprintf(buffer + offset, 8000, "  %s: %.2f%s\n", memory_tag_strings[i], amount, unit);
        offset += length;
    }
    {
        // Compute total usage.
        // NOTE: memory is just grabbed as needed here, so there won't ever
        // be "free" space.
        u64 total_space = state_ptr->stats.total_allocated;
        u64 used_space = state_ptr->stats.total_allocated;

        f32 used_amount = 1.0f;
        const char* used_unit = get_unit_for_size(used_space, &used_amount);

        f32 total_amount = 1.0f;
        const char* total_unit = get_unit_for_size(total_space, &total_amount);

        f64 percent_used = (f64)(used_space) / total_space;

        i32 length = snprintf(buffer + offset, 8000, "Total memory usage: %.2f%s of %.2f%s (%.2f%%)\n", used_amount, used_unit, total_amount, total_unit, percent_used);
        offset += length;
    }

    char* out_string = string_duplicate(buffer);
    return out_string;
}

u64 get_memory_alloc_count(void) {
    if (state_ptr) {
        return state_ptr->alloc_count;
    }
    return 0;
}
