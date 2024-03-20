/**
 * @file kmemory.h
 * @author Travis Vroman (travis@kohiengine.com)
 * @brief This file contains the structures and functions of the memory system.
 * This is responsible for memory interaction with the platform layer, such as
 * allocations/frees and tagging of memory allocations.
 * @note Note that reliance on this will likely be by core systems only, as items using
 * allocations directly will use allocators as they are added to the system.
 * @version 1.0
 * @date 2022-01-10
 *
 * @copyright Kohi Game Engine is Copyright (c) Travis Vroman 2021-2022
 *
 */

#pragma once

#include "defines.h"


/**
 * @brief Performs a memory allocation from the host of the given size. 
 * @param size The size of the allocation.
 * @returns If successful, a pointer to a block of allocated memory; otherwise 0.
 */
KAPI void* kallocate(u64 size);

/**
 * @brief Frees the given block, and untracks its size from the given tag.
 * @param block A pointer to the block of memory to be freed.
 */
KAPI void kfree(void* block);

/**
 * @brief Zeroes out the provided memory block.
 * @param block A pointer to the block of memory to be zeroed out.
 * @param size The size in bytes to zero out.
 * @param A pointer to the zeroed out block of memory.
 */
KAPI void* kzero_memory(void* block, u64 size);

/**
 * @brief Performs a copy of the memory at source to dest of the given size.
 * @param dest A pointer to the destination block of memory to copy to.
 * @param source A pointer to the source block of memory to copy from.
 * @param size The amount of memory in bytes to be copied over.
 * @returns A pointer to the block of memory copied to.
 */
KAPI void* kcopy_memory(void* dest, const void* source, u64 size);

/**
 * @brief Sets the bytes of memory located at dest to value over the given size.
 * @param dest A pointer to the destination block of memory to be set.
 * @param value The value to be set.
 * @param size The size in bytes to copy over to.
 * @returns A pointer to the destination block of memory.
 */
KAPI void* kset_memory(void* dest, i32 value, u64 size);

