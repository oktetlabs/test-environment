/** @file
 * @brief TAD async RCF replies
 *
 * Traffic Application Domain Command Handler.
 * Async RCF reply backend functions definition.
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS
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
 */

#ifndef __TE_TAD_REPLY_RCF__
#define __TE_TAD_REPLY_RCF__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "te_errno.h"
#include "comm_agent.h"
#include "tad_reply.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize TAD RCF reply context.
 *
 * @param ctx           TAD async reply context to be initialized
 * @param rcfc          RCF connection handle
 * @param answer_pfx    Answer prefix
 * @param pfx           Answer prefix length
 *
 * @return Status code.
 */
extern te_errno tad_reply_rcf_init(tad_reply_context   *ctx,
                                   rcf_comm_connection *rcfc,
                                   const char          *answer_pfx,
                                   size_t               pfx_len);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /*  __TE_TAD_REPLY_RCF__ */
