/** @file
 * @brief Key-value pairs API
 *
 * Definition of API for working with key-value pairs
 *
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS
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
 * @author Denis Pryazhennikov <Denis.Pryazhennikov@oktetlabs.ru>
 */

#ifndef __TE_TOOLS_KV_PAIRS_H__
#define __TE_TOOLS_KV_PAIRS_H__

#include "te_config.h"

#include "te_defs.h"
#include "te_errno.h"

#if HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#else
#include "te_queue.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif

/** Element of the list of key-value pair */
typedef struct te_kvpair {
    TAILQ_ENTRY(te_kvpair)   links;
    char                    *key;
    char                    *value;
} te_kvpair;

/** Head of the list of key-value pair */
typedef TAILQ_HEAD(te_kvpair_h, te_kvpair) te_kvpair_h;

/**
 * Initializes empty key-value pairs list
 *
 * @param head      Head of the list
 */
extern void te_kvpair_init(te_kvpair_h *head);

/**
 * Free resources allocated for key-value pairs list
 *
 * @param head      Head of the list
 */
extern void te_kvpair_fini(te_kvpair_h *head);

/**
 * Add key-value pair
 *
 * @param head          Head of the list
 * @param key           Key of value
 * @param value_fmt     Format (*printf-like) string for value string
 * @param ...           Respective parameters for format string
 *
 * @retval TE_EEXIST    The key already exists
 */
extern te_errno te_kvpair_add(te_kvpair_h *head, const char *key,
                              const char *value_fmt, ...);

/**
 * Get value associated with the @p key
 *
 * @param key    Key of required value
 *
 * @retval       Requested @p key value or @c NULL if there is no such key
 */
extern const char *te_kvpairs_get(te_kvpair_h *head, const char *key);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TOOLS_KV_PAIRS_H__ */
