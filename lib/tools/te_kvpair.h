/** @file
 * @brief Key-value pairs API
 *
 * @defgroup te_tools_te_kvpair Key-value pairs
 * @ingroup te_tools
 * @{
 *
 * Definition of API for working with key-value pairs
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
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
/**@} <!-- END te_tools_te_kvpair --> */
