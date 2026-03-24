/**
 * @file na_alloc_config.h
 * @author Levent Kaya
 * @brief General/mostly configurable fields of na_alloc
 * @version 0.1.0
 * @date 2026-03-18
 *
 */

#ifndef NA_ALLOC_CONFIG_H
#define NA_ALLOC_CONFIG_H

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

#define NA_DEFAULT_ALIGN_SIZE 16

#endif /* NA_ALLOC_CONFIG_H */
