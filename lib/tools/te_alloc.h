/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2004-2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief API to safely allocate memory
 *
 * @defgroup te_tools_te_alloc Safe memory allocation
 * @ingroup te_tools
 * @{
 *
 * Safe memory allocation.
 */

#ifndef __TE_TOOLS_ALLOC_H__
#define __TE_TOOLS_ALLOC_H__

#include "te_config.h"

#include <stddef.h>
#include "te_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Allocate @p size bytes and optionally fill allocated memory with zeroes.
 *
 * This function should never be called directly,
 * use TE_ALLOC() or TE_ALLOC_UNINITIALIZED() macros instead.
 *
 * @param size       Number of bytes to allocate.
 * @param initialize Zero the allocated memory if @c TRUE.
 * @param filename   Caller's filename.
 * @param line       Caller's line.
 *
 * @return A pointer to a fresh memory block (never @c NULL).
 *
 * @exception TE_FATAL_ERROR in an unlikely case of a memory allocation
 *            failure.
 *
 * @note On requesting zero bytes, the function actually returns a 1-byte
 *       buffer capturing ISO C permitted behaviour of recent glibc.
 */
extern void *te_alloc_internal(size_t size, te_bool initialize,
                               const char *filename, int line);

/**
 * Allocate @p size_ bytes and fill allocated memory with zeroes.
 *
 * @param size_     Number of bytes to allocate.
 *
 * @return A pointer to a fresh memory block (never @c NULL).
 *
 * @exception TE_FATAL_ERROR in an unlikely case of a memory allocation
 *            failure.
 *
 * @note On requesting zero bytes, the macro actually returns a 1-byte
 *       buffer capturing ISO C permitted behaviour of recent glibc.
 */
#define TE_ALLOC(size_) (te_alloc_internal((size_), TRUE, __FILE__, __LINE__))

/**
 * Allocate @p size_ bytes without initializing it.
 *
 * In most cases TE_ALLOC() shall be used instead.
 * This macro is intended for performance-critical cases where
 * the caller would immediately initialize the memory block itself,
 * e.g. with @c memset or @c memcpy.
 *
 * @param size_     Number of bytes to allocate.
 *
 * @return A pointer to a fresh uninitialized memory block (never @c NULL).
 *
 * @exception TE_FATAL_ERROR in an unlikely case of a memory allocation
 *            failure.
 *
 * @note On requesting zero bytes, the macro actually returns a 1-byte
 *       buffer capturing ISO C permitted behaviour of recent glibc.
 */
#define TE_ALLOC_UNINITIALIZED(size_) \
    (te_alloc_internal((size_), FALSE, __FILE__, __LINE__))

/**
 * Reallocate @p oldptr to have the size of @p newsize.
 *
 * If @p oldptr is @c NULL, it is an exact equivalent of
 * te_alloc_internal().
 *
 * This function should never be called directly,
 * use TE_REALLOC() macro instead.
 *
 * @param oldptr     An existing memory block or @c NULL.
 * @param newsize    New size of the block.
 * @param filename   Caller's filename.
 * @param line       Caller's line.
 *
 * @return A pointer to a reallocated block
 *        (never @c NULL, may be the same as @p oldptr).
 *
 * @exception TE_FATAL_ERROR in an unlikely case of a memory allocation
 *            failure or zero-size reallocation.
 *
 * @warning Unlike system @c realloc, this function aborts
 *          if a non-null @p oldptr is tried to resize to
 *          zero bytes -- the behaviour of system @c realloc
 *          is really ill-defined.
 */
extern void *te_realloc_internal(void *oldptr, size_t newsize,
                                 const char *filename, int line);

/**
 * Reallocate @p ptr_ to have the size of @p newsize_.
 *
 * If @p ptr_ contains @c NULL, it is an exact equivalent of
 * TE_ALLOC().
 *
 * @p ptr_ must be an lvalue and its content is replaced upon
 * reallocation, so the old invalidated address is not accessible
 * anymore.
 *
 * @param[in,out] ptr_      An lvalue holding an existing memory block
 *                          or @c NULL.
 * @param[in]     newsize_  New size of the block.
 *
 * @exception TE_FATAL_ERROR in an unlikely case of a memory allocation
 *            failure or zero-size reallocation.
 *
 * @warning Unlike system @c realloc, this function aborts
 *          if a non-null @p ptr_ is tried to resize to
 *          zero bytes -- the behaviour of system @c realloc
 *          is really ill-defined.
 */
#define TE_REALLOC(ptr_, newsize_) \
    ((void)((ptr_) = te_realloc_internal((ptr_), (newsize_),    \
                                         __FILE__, __LINE__)))

/**
 * Copy a block of memory @p src.
 *
 * This function should never be called directly,
 *
 * @param src                Source buffer (may be @c NULL).
 * @param zero_terminated    If @c TRUE, only copy bytes up to the first
 *                           binary zero.
 * @param maxsize            Maximum number of bytes to copy
 *                           (may be less if @p zero_terminated is @c TRUE).
 * @param filename           Source filename.
 * @param line               Source line.
 *
 * @return A pointer to a fresh copy of @p src.
 * @retval @c NULL  if and only if @p src is @c NULL.
 *
 * @exception TE_FATAL_ERROR in an unlikely case of a memory allocation
 *            failure.
 */
extern void *te_memdup_internal(const void *src, te_bool zero_terminated,
                                size_t maxsize, const char *filename, int line);

/**
 * Make a copy of @p size_ bytes of memory starting from @p src_.
 *
 * This macro is similiar to @c memdup function found in OpenBSD
 * and some other OSes.
 *
 * @param src_  Source buffer (may be @c NULL).
 * @param size_ Size of the source buffer.
 *
 * @return A pointer to a fresh copy of @p src_.
 * @retval @c NULL  if and only if @p src_ is @c NULL.
 *
 * @exception TE_FATAL_ERROR in an unlikely case of a memory allocation
 *            failure.
 *
 * @note Refer to TE_ALLOC() on behaviour when @p size_ is zero.
 */
#define TE_MEMDUP(src_, size_) \
    (te_memdup_internal((src_), FALSE, (size_), __FILE__, __LINE__))

/**
 * Make a copy of zero-terminated @p src_ like system @c strdup.
 *
 * @param src_  Zero-terminated string (may be @c NULL).
 *
 * @return A pointer to a fresh copy of @p src_.
 * @retval @c NULL  if and only if @p src_ is @c NULL.
 *
 * @exception TE_FATAL_ERROR in an unlikely case of a memory allocation
 *            failure.
 */
#define TE_STRDUP(src_) \
    ((char *)te_memdup_internal((src_), TRUE, SIZE_MAX, __FILE__, __LINE__))

/**
 * Make a copy of possibly zero-terminated @p src_ like system @c strndup.
 *
 * The macro copies the content of @p src_ up to and including the first
 * binary zero or the first @p maxsize_ bytes, whichever comes first.
 *
 * The result is always zero-terminated.
 *
 * @param src_      A possibly zero-terminated string (may be @c NULL).
 * @param maxsize_  Max number of bytes to search for a trailing zero.
 *
 * @return A pointer to a fresh copy of @p src_.
 * @retval @c NULL  if and only if @p src_ is @c NULL.
 *
 * @exception TE_FATAL_ERROR in an unlikely case of a memory allocation
 *            failure.
 */
#define TE_STRNDUP(src_, maxsize_) \
    ((char *)te_memdup_internal((src_), TRUE, (maxsize_), __FILE__, __LINE__))


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TOOLS_ALLOC_H__ */
/**@} <!-- END te_tools_te_alloc --> */
