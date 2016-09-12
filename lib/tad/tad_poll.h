/** @file
 * @brief TAD Poll Support
 *
 * Traffic Application Domain Command Handler.
 * Declarations of types and functions for TAD poll support.
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAD_POLL_H__
#define __TE_TAD_POLL_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <semaphore.h>

#include "te_errno.h"
#include "te_queue.h"

#include "tad_types.h"
#include "tad_reply.h"
#include "tad_send_recv.h"


#ifdef __cplusplus
extern "C" {
#endif


/** TAD poll context data. */
typedef struct tad_poll_context {
    LIST_ENTRY(tad_poll_context)    links;  /**< List links */

    tad_reply_context   reply_ctx;  /**< Reply context */
    csap_p              csap;       /**< CSAP instance */
    unsigned int        id;         /**< Poll request ID */
    unsigned int        timeout;    /**< Poll request timeout */
    pthread_t           thread;     /**< Thread ID */
    te_errno            status;     /**< Poll request status */
    sem_t               sem;
} tad_poll_context;


/**
 * Enqueue TAD poll request.
 *
 * @param csap          CSAP instance
 * @param timeout       Timeout in milliseconds
 * @param reply_ctx     TAD async reply context
 *
 * @return Status code.
 */
extern te_errno tad_poll_enqueue(csap_p                     csap,
                                 unsigned int               timeout,
                                 const tad_reply_context   *reply_ctx);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /*  __TE_TAD_POLL_H__ */
