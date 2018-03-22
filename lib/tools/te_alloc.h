/** @file
 * @brief API to safely allocate memory
 *
 * @defgroup te_tools_te_alloc Safe memory allocation
 * @ingroup te_tools
 * @{
 *
 * Safe memory allocation
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
 *
 *
 * @author Artem Andreev <Artem.Andreev@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TOOLS_ALLOC_H__
#define __TE_TOOLS_ALLOC_H__

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Allocate @p size bytes and fill allocated memory with zeroes.
 * Logs an error if the memory cannot be allocated.
 * This function should never be called directly,
 * use TE_ALLOC() macro instead
 *
 * @param size     Number of bytes to allocate
 * @param filename Caller's filename
 * @param line     Caller's line
 */
extern void *te_alloc_internal(size_t size, const char *filename, int line);


/**
 * It is a wrapper for te_alloc_internal()
 *
 * @param _size     Number of bytes to allocate
 */
#define TE_ALLOC(_size) (te_alloc_internal((_size), __FILE__, __LINE__))

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TOOLS_ALLOC_H__ */
/**@} <!-- END te_tools_te_alloc --> */
