/** @file
 * @brief Test API to use setjmp/longjmp.
 *
 * Definition of API to deal with thread-safe stack of jumps.
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAPI_JMP_H__
#define __TE_TAPI_JMP_H__

#if HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#else
#error Required sys/queue.h not found
#endif
#if HAVE_SETJMP_H
#include <setjmp.h>
#else
#error Required setjmp.h not found
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** Jump point (stack context to jump to) */
typedef struct tapi_jmp_point {
    LIST_ENTRY(tapi_jmp_point)  links;      /**< List links */

    te_bool         enabled;    /** Are jumps enalbed or disabled? */
    jmp_buf         env;        /**< Stack context, if jumps are
                                     enabled */
    const char     *file;       /**< Name of the file where the point
                                     is added */
    unsigned int    lineno;     /**< Line number in the file where the
                                     point is added */
} tapi_jmp_point;

#define TAPI_ON_JMP(rc, x) \
    do  {                                                               \
        tapi_jmp_point *p = tapi_jmp_alloc(TRUE, __FILE__, __LINE__);   \
                                                                        \
        if (p == NULL || (rc = setjmp(p->env)) != 0)                    \
        {                                                               \
            x;                                                          \
        }                                                               \
    } while (0)

/**
 * Allocate long jump point (saved stack context for non-local goto).
 *
 * The routine is thread-safe using per thread stacks of jump points.
 *
 * @param enabled   Point with enabled/disabled jump
 * @param file      Name of the file where jump point is set
 * @param lineno    Number of line in the file
 *
 * @return Pointer to allocated long jump point.
 *
 * @sa tapi_jmp_do, tapi_jmp_pop
 */
extern tapi_jmp_point *tapi_jmp_alloc(te_bool enabled, const char *file,
                                      unsigned int lineno);

/**
 * Do does non-local goto a stack context saved using tapi_jmp_push().
 *
 * The routine is thread-safe using per thread stacks of jump points.
 *
 * @param val       Positive value to be returned by tapi_jmp_push
 *                  (0 is mapped to ETEOK)
 *
 * @return Status code
 * @retval ENOENT   Stack of of jump points is empty
 *
 * @sa tapi_jmp_push, tapi_jmp_pop
 */
extern int tapi_jmp_do(int val);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_JMP_H__ */
