#include "kmemory.h"

#include "core/kmutex.h"
#include "core/kstring.h"
#include "core/logger.h"
#include "platform/platform.h"

#include <stdio.h>
#include <string.h>


void* kallocate(u64 size) {
    return malloc(size);
}

void kfree(void* block) {
    kfree(block);
}

void* kzero_memory(void* block, u64 size) {
    return memset(block, 0, size);
}

void* kcopy_memory(void* dest, const void* source, u64 size) {
    return memcpy(dest, source, size);
}

void* kset_memory(void* dest, i32 value, u64 size) {
    return memset(dest, value, size);
}

