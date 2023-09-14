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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Allocate @p size bytes and fill allocated memory with zeroes.
 *
 * This function should never be called directly,
 * use TE_ALLOC() macro instead.
 *
 * @param size     Number of bytes to allocate.
 * @param filename Caller's filename.
 * @param line     Caller's line.
 *
 * @return A pointer to a fresh memory block (never @c NULL).
 *
 * @exception TE_FATAL_ERROR in an unlikely case of a memory allocation
 *            failure.
 *
 * @note Unlike system malloc(), this function does not permit allocating
 *       zero-sized memory blocks.
 */
extern void *te_alloc_internal(size_t size, const char *filename, int line);


/**
 * A wrapper for te_alloc_internal() tracking source location.
 *
 * @param _size     Number of bytes to allocate
 */
#define TE_ALLOC(_size) (te_alloc_internal((_size), __FILE__, __LINE__))

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TOOLS_ALLOC_H__ */
/**@} <!-- END te_tools_te_alloc --> */
