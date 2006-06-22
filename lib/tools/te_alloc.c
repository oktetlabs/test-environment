/** @file
 * @brief API to safely allocate memory
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

#define TE_LGR_USER     "TE Alloc"

#include "te_config.h"

#ifndef WINDOWS
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#endif
#endif

#include "logger_api.h"
#include "te_alloc.h"

#if SIZEOF_SIZE_T > SIZEOF_LONG
#define SIZE_T_FMT "%Lu"
#define SIZE_T_CAST(x) ((unsigned long long)(x))
#else
#define SIZE_T_FMT "%lu" 
#define SIZE_T_CAST(x) ((unsigned long)(x))
#endif

/* See description in te_alloc.h */
void *
te_alloc_internal(size_t size, const char *filename, int line)
{
    void *result = calloc(1, size);
    
    if (result == NULL)
    {
        ERROR("Cannot allocate memory of size " SIZE_T_FMT " at %s:%d: %s",
              SIZE_T_CAST(size), 
              filename, line,
#ifndef WINDOWS
              strerror(errno)
#else
              "No enough memory"
#endif
              );
    }
    return result;
}
