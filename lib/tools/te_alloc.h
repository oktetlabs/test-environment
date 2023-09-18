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

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TOOLS_ALLOC_H__ */
/**@} <!-- END te_tools_te_alloc --> */
