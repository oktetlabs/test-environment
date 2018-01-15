/** @file
 * @brief Test API to use malloc, calloc, realloc, memcpy and strdup
 *
 * Find details on page @ref tapi_mem.
 * @defgroup tapi_mem Test API to use memory-related functions conveniently
 * @ingroup te_ts_tapi
 * @{
 * Implementation of API to use memory-related functions in a convenient way
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Ivan Malov <Ivan.Malov@oktetlabs.ru>
 */

#ifndef __TE_TAPI_MEM_H__
#define __TE_TAPI_MEM_H__

#include "te_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @b malloc() wrapper which performs allocation status check internally
 *
 * @param size          Size of memory to allocate
 *
 * @return Pointer to an allocated memory (never returns @c NULL)
 */
extern void *tapi_malloc(size_t size);

/**
 * @b calloc() wrapper which performs allocation status check internally
 *
 * @param nmemb         Number of memory elements to allocate
 * @param size          Size of an element
 *
 * @return Pointer to an allocated memory (never returns @c NULL)
 */
extern void *tapi_calloc(size_t nmemb, size_t size);

/**
 * @b realloc() wrapper which performs allocation status check internally
 *
 * @param ptr           Pointer to a memory block to re-allocate
 * @param size          New size for a memory block to re-allocate
 *
 * @return Pointer to re-allocated memory (never returns @c NULL)
 */
extern void *tapi_realloc(void *ptr, size_t size);

/**
 * @b malloc() + memcpy() wrapper designed to simplify memory copying
 *
 * @param ptr           Source memory pointer
 * @param size          Size of the memory block pointed by @p ptr
 *
 * @return Pointer to a memory block copy (never returns @c NULL)
 */
extern void *tapi_memdup(const void *ptr, size_t size);

/**
 * @b strdup() wrapper which performs duplication status check internally
 *
 * @param s             Pointer to a source string to be duplicated
 *
 * @return Pointer to a string copy (never returns @c NULL)
 */
extern char *tapi_strdup(const char *s);

/**@} <!-- END tapi_mem --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_MEM_H__ */
