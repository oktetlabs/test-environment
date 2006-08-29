/** @file
 * @brief Tail queue of strings (char *).
 *
 * Definition of API for working with tail queue of strings.
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TOOLS_TQ_STRING_H__
#define __TE_TOOLS_TQ_STRING_H__

#include "te_queue.h"

#ifdef __cplusplus
extern "C" {
#endif


/** Element of the list of strings */
typedef struct tqe_string {
    TAILQ_ENTRY(tqe_string)     links;  /**< List links */
    char                       *v;      /**< Value */
} tqe_string;

/** Head of the list of strings */
typedef TAILQ_HEAD(tqh_strings, tqe_string) tqh_strings;


/**
 * Free list of allocated strings.
 *
 * @param head          Head of the list
 * @param value_free    Function to be called to free value or NULL
 */
extern void tq_strings_free(tqh_strings *head, void (*value_free)(void *));



#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TOOLS_TQ_STRING_H__ */
