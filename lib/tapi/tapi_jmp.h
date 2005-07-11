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

    jmp_buf         env;        /**< Stack context, if jumps are
                                     enabled */
    const char     *file;       /**< Name of the file where the point
                                     is added */
    unsigned int    lineno;     /**< Line number in the file where the
                                     point is added */
} tapi_jmp_point;


/**
 * Create jump point with actions to be done in the case of jump.
 *
 * @param x         Actions to be done in the case of long jump
 *                  (it is assumed that it does return, goto or exit)
 */
#define TAPI_ON_JMP(x) \
    do {                                                        \
        tapi_jmp_point *p = tapi_jmp_push(__FILE__, __LINE__);  \
        int             jmp_rc = EFAULT;                        \
                                                                \
        if (p == NULL || (jmp_rc = setjmp(p->env)) != 0)        \
        {                                                       \
            x;                                                  \
            ERROR("%s:%u: Unexpected after jump actions");      \
        }                                                       \
    } while (0)

/**
 * Allocate and push long jump point in a stack (saved stack context
 * for non-local goto).
 *
 * The routine is thread-safe using per thread stacks of jump points.
 *
 * @param file      Name of the file where jump point is set
 * @param lineno    Number of line in the file
 *
 * @return Pointer to allocated long jump point.
 *
 * @sa tapi_jmp_do
 */
extern tapi_jmp_point *tapi_jmp_push(const char *file,
                                     unsigned int lineno);


/**
 * Macro for convinient use of tapi_jmp_do().
 */
#define TAPI_JMP_DO(_val) tapi_jmp_do((_val), __FILE__, __LINE__)

/**
 * Do non-local goto a stack context saved using tapi_jmp_push().
 *
 * The routine is thread-safe using per thread stacks of jump points.
 *
 * @param val       Positive value to be returned by tapi_jmp_push
 *                  (0 is mapped to ETEOK)
 *
 * @return Status code
 * @retval ENOENT   Stack of of jump points is empty
 *
 * @sa tapi_jmp_push
 */
extern int tapi_jmp_do(int val, const char *file, unsigned int lineno);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_JMP_H__ */
