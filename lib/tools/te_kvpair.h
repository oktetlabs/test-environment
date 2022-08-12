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
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
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
#include "te_queue.h"
#include "te_string.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TE_KVPAIR_STR_DELIMITER ":"

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
 * Add key-value pair using variadic list.
 *
 * @param head          Head of the list
 * @param key           Key of value
 * @param value_fmt     Format (*printf-like) string for value string
 * @param ap            List of arguments
 *
 * @retval TE_EEXIST    The key already exists
 */
extern te_errno te_kvpair_add_va(te_kvpair_h *head, const char *key,
                                 const char *value_fmt, va_list ap);

/**
 * Remove key-value pair by key
 *
 * @param head          Head of the list
 * @param key           Key of value
 *
 * @retval TE_ENOENT    No entry with such key
 * @retval 0            Pair removed successfully
 */
extern te_errno te_kvpairs_del(te_kvpair_h *head, const char *key);

/**
 * Get value associated with the @p key
 *
 * @param key    Key of required value
 *
 * @retval       Requested @p key value or @c NULL if there is no such key
 */
extern const char *te_kvpairs_get(const te_kvpair_h *head, const char *key);

/**
 * Convert list of kv_pairs to string representation (i.e. key1=val1:key2=val2)
 * @sa TE_KVPAIR_STR_DELIMITER
 *
 * @param[in] head  Head of the list
 * @param[out] str  Pointer to string
 *
 * @retval          Status code
 */
extern te_errno te_kvpair_to_str(const te_kvpair_h *head, te_string *str);

/**
 * Convert string to list of kv_pairs,
 * @sa TE_KVPAIR_STR_DELIMITER
 *
 * @param[in] str   Pointer to string
 * @param[out] head Head of the kv_pair list
 *
 * @retval          Status code
 */
extern te_errno te_kvpair_from_str(const char *str, te_kvpair_h *head);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TOOLS_KV_PAIRS_H__ */
/**@} <!-- END te_tools_te_kvpair --> */
