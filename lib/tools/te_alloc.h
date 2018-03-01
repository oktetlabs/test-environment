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
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
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
