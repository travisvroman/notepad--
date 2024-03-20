#include "kmemory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/kstring.h"
#include "core/logger.h"

void* kallocate(u64 size) {
    return malloc(size);
}

void kfree(void* block) {
    free(block);
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
