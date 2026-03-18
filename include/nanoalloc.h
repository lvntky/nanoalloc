/**
 * @file nanoalloc.h
 * @author Levent 
 * @brief nanoalloc public API
 * @version 0.1.0
 * @date 2026-03-17
 *
 * nanoalloc public API
 */

#ifndef _NANOALLOC_H
#define _NANOALLOC_H 1

#include <features.h>
#include <stddef.h>
#include <stdio.h>

#if defined(_WIN32)
#define NA_EXTERN __declspec(dllexport)
#else
#define NA_EXTERN extern
#endif

NA_EXTERN void *na_alloc(size_t __size) __attribute_malloc__
	__attribute_alloc_size__((1)) __wur;

#endif //_NANOALLOC_H
