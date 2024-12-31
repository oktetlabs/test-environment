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
 * Ensure that @p offset + @p extent is not out of bounds.
 *
 * The function adjusts the value in @p extent so that a chunk
 * starting at @p extent lie completely within a buffer of
 * @p total_length bytes.
 *
 * The function handles unsigned overflows correctly, so e.g.
 * @p extent may contain @c SIZE_MAX.
 *
 * @param[in]     total_length   Total length of a buffer.
 * @param[in]     offset         Offset of a chunk within the buffer.
 * @param[in,out] extent         Length of the chunk.
 *
 * @return @c true if @p extent was reduced.
 *
 * @pre @p offset should be within @p total_length.
 */
static inline bool
te_alloc_adjust_extent(size_t total_length, size_t offset, size_t *extent)
{
    bool adjusted = false;

    assert(offset < total_length);
    if (offset + *extent > total_length ||
        offset + *extent < offset)
    {
        *extent = total_length - offset;
        adjusted = true;
    }

    return adjusted;
}

/**
 * Allocate @p size bytes and optionally fill allocated memory with zeroes.
 *
 * This function should never be called directly,
 * use TE_ALLOC() or TE_ALLOC_UNINITIALIZED() macros instead.
 *
 * @param size       Number of bytes to allocate.
 * @param initialize Zero the allocated memory if @c true.
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
extern void *te_alloc_internal(size_t size, bool initialize,
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
#define TE_ALLOC(size_) (te_alloc_internal((size_), true, __FILE__, __LINE__))

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
    (te_alloc_internal((size_), false, __FILE__, __LINE__))

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
 * Check whether an array of @p nmemb elements of @p size bytes
 * can be allocated.
 *
 * Basically, it checks that @p nmemb * @p size would fit into
 * @c size_t; it does not check for available memory and so on.
 *
 * The primary purpose of this function is to ensure that
 * `TE_ALLOC(x * y)` would not erroneously return a small buffer
 * due to overflow:
 * @code
 * if (!te_is_valid_alloc(nmemb, size))
 *     TEST_FAIL("Array is too large");
 * arr = TE_ALLOC(nmemb * size);
 * @endcode
 *
 * @alg See <cite>Hacker's Delight, 2nd Edition, section 2.12.4</cite>
 *
 * @param nmemb  Number of elements.
 * @param size   Size of an element.
 *
 * @return @c true if an array of @p nmemb elements may be allocated.
 */
static inline bool
te_is_valid_alloc(size_t nmemb, size_t size)
{
    return size <= 1 || nmemb <= 1 || nmemb < (SIZE_MAX / size);
}

/**
 * Copy a block of memory @p src.
 *
 * This function should never be called directly,
 *
 * @param src                Source buffer (may be @c NULL).
 * @param zero_terminated    If @c true, only copy bytes up to the first
 *                           binary zero.
 * @param maxsize            Maximum number of bytes to copy
 *                           (may be less if @p zero_terminated is @c true).
 * @param filename           Source filename.
 * @param line               Source line.
 *
 * @return A pointer to a fresh copy of @p src.
 * @retval @c NULL  if and only if @p src is @c NULL.
 *
 * @exception TE_FATAL_ERROR in an unlikely case of a memory allocation
 *            failure.
 */
extern void *te_memdup_internal(const void *src, bool zero_terminated,
                                size_t maxsize, const char *filename, int line);

/**
 * Make a copy of @p size_ bytes of memory starting from @p src_.
 *
 * This macro is similar to @c memdup function found in OpenBSD
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
    (te_memdup_internal((src_), false, (size_), __FILE__, __LINE__))

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
    ((char *)te_memdup_internal((src_), true, SIZE_MAX, __FILE__, __LINE__))

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
    ((char *)te_memdup_internal((src_), true, (maxsize_), __FILE__, __LINE__))


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TOOLS_ALLOC_H__ */
/**@} <!-- END te_tools_te_alloc --> */
