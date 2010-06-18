/** @file
 * @brief Test Environment: RGT message.
 *
 * Copyright (C) 2010 Test Environment authors (see file AUTHORS
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
 * @author Nikolai Kondrashov <Nikolai.Kondrashov@oktetlabs.ru>
 * 
 * $Id$
 */

#ifndef __TE_RGT_MSG_H__
#define __TE_RGT_MSG_H__

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
 * Retrieve next message field.
 *
 * @param arg   The previous message field.
 *
 * @return Next message field.
 */
static inline rgt_msg_fld *
rgt_msg_fld_next(rgt_msg_fld *fld)
{
    return (rgt_msg_fld *)((char *)fld + fld->size);
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

#endif /* !__TE_RGT_MSG_H__ */
