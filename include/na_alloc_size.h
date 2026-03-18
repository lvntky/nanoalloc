/**
 * @file na_alloc_size.h
 * @author Levent Kaya
 * @brief Internal size abstraction for nanoalloc
 * @version 0.1.0
 * @date 2026-03-18
 *
 * Inspired by glibc's INTERNAL_SIZE_T concept,
 * but simplified and made portable.
 */

#ifndef NA_ALLOC_SIZE_H
#define NA_ALLOC_SIZE_H

#include <stddef.h>
#include <stdint.h>

/*
 * Internal size type used by the allocator.
 *
 * You can change this later if you want:
 * - smaller headers (uint32_t)
 * - specific alignment tricks
 *
 * For now, use size_t for maximum portability.
 */
#define INTERNAL_SIZE_T size_t

/* Size of the internal size type (word size) */
#define NA_SIZE_SZ (sizeof(INTERNAL_SIZE_T))


#endif /* NA_ALLOC_SIZE_H */
