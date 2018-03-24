/** @file
 * @brief Test Environment: RGT message.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 * @author Nikolai Kondrashov <Nikolai.Kondrashov@oktetlabs.ru>
 * 
 * $Id$
 */

#ifndef __RGT_MSG_H__
#define __RGT_MSG_H__

#include <assert.h>
#include <stddef.h>
#include "te_defs.h"
#include "te_raw_log.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rgt_msg_fld rgt_msg_fld;

struct rgt_msg_fld {
    /*
     * ATTENTION
     * The manual alignment in raw2xml.c may depend
     * on the order and size of the fields.
     */
    size_t          size;   /**< Field size */
    te_log_nfl      len;    /**< Field contents length */
    uint8_t         buf[0]; /**< Field contents */
};

/**
 * Check if a message field is terminator field.
 *
 * @param fld   The message field to check.
 *
 * @return TRUE if the field is the terminator, FALSE otherwise.
 */
static inline te_bool
rgt_msg_fld_is_term(const rgt_msg_fld *fld)
{
    assert(fld != NULL);
    return fld->len == TE_LOG_RAW_EOR_LEN;
}

/**
 * Retrieve next message field.
 *
 * @param fld   The previous message field.
 *
 * @return Next message field.
 */
static inline const rgt_msg_fld *
rgt_msg_fld_next(const rgt_msg_fld *fld)
{
    assert(fld != NULL);
    assert(!rgt_msg_fld_is_term(fld));
    return (const rgt_msg_fld *)((char *)fld + fld->size);
}

/** Message */
typedef struct rgt_msg {
    te_log_ts_sec   ts_secs;    /**< Timestamp seconds */
    te_log_ts_usec  ts_usecs;   /**< Timestamp milliseconds */
    te_log_level    level;      /**< Log level */
    te_log_id       id;         /**< Node ID */
    /*
     * NOTE: the following fields reference externally allocated memory.
     */
    rgt_msg_fld    *entity;     /**< Entity name reference */
    rgt_msg_fld    *user;       /**< User name reference */
    rgt_msg_fld    *fmt;        /**< Format string reference */
    rgt_msg_fld    *args;       /**< First argument reference */
} rgt_msg;

extern te_bool rgt_msg_valid(const rgt_msg *msg);

/**
 * Check if a message has "Tester" entity and "Control" user.
 *
 * @param msg   The message to check.
 *
 * @return TRUE or FALSE.
 */
extern te_bool rgt_msg_is_tester_control(const rgt_msg *msg);

/**
 * Check if a message has "Control" user.
 *
 * @param msg   The message to check.
 *
 * @return TRUE or FALSE.
 */
extern te_bool rgt_msg_is_control(const rgt_msg *msg);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__RGT_MSG_H__ */
