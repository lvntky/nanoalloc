/**
 * @file nanoalloc.h
 * @author Levent
 * @brief nanoalloc public API
 * @version 0.1.0
 * @date 2026-03-17
 */
#ifndef NANOALLOC_H
#define NANOALLOC_H

#include <stddef.h>

/* ------------------------------------------------------------------ */
/* Export macro                                                         */
/* ------------------------------------------------------------------ */
#if defined(_WIN32) || defined(_WIN64)
#  define NA_EXTERN __declspec(dllexport)
#else
#  define NA_EXTERN extern
#endif

/* ------------------------------------------------------------------ */
/* Attributes                                                           */
/* ------------------------------------------------------------------ */
#if defined(__GNUC__) || defined(__clang__)
#  define NA_ATTR_MALLOC        __attribute__((__malloc__))
#  define NA_ATTR_ALLOC_SIZE1   __attribute__((__alloc_size__(1)))
#  define NA_ATTR_ALLOC_SIZE_2  __attribute__((__alloc_size__(2)))
#  define NA_ATTR_ALLOC_SIZE1_2 __attribute__((__alloc_size__(1, 2)))
#  define NA_ATTR_WUR           __attribute__((__warn_unused_result__))
#else
#  define NA_ATTR_MALLOC
#  define NA_ATTR_ALLOC_SIZE1
#  define NA_ATTR_ALLOC_SIZE_2
#  define NA_ATTR_ALLOC_SIZE1_2
#  define NA_ATTR_WUR
#endif

/* ------------------------------------------------------------------ */
/* API                                                                  */
/* ------------------------------------------------------------------ */
#if defined(NA_PREFIX) && (NA_PREFIX)

NA_EXTERN void *na_alloc(size_t size)
    NA_ATTR_MALLOC NA_ATTR_ALLOC_SIZE1 NA_ATTR_WUR;

NA_EXTERN void na_free(void *ptr);

NA_EXTERN void *na_realloc(void *ptr, size_t size)
    NA_ATTR_ALLOC_SIZE_2 NA_ATTR_WUR;              /* _2 — size param 2'de */

NA_EXTERN void *na_calloc(size_t nmemb, size_t size)
    NA_ATTR_MALLOC NA_ATTR_ALLOC_SIZE1_2 NA_ATTR_WUR; /* 1_2 — nmemb*size  */

#else /* override mode */

NA_EXTERN void *malloc(size_t size)
    NA_ATTR_MALLOC NA_ATTR_ALLOC_SIZE1 NA_ATTR_WUR;

NA_EXTERN void free(void *ptr);

NA_EXTERN void *realloc(void *ptr, size_t size)
    NA_ATTR_ALLOC_SIZE_2 NA_ATTR_WUR;

NA_EXTERN void *calloc(size_t nmemb, size_t size)
    NA_ATTR_MALLOC NA_ATTR_ALLOC_SIZE1_2 NA_ATTR_WUR;

#endif /* NA_PREFIX */

#endif /* NANOALLOC_H */
