/** @file
 * @brief TAD async replies
 *
 * Traffic Application Domain Command Handler.
 * Declarations of types and functions to used by TAD to reply
 * asynchronously.
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

#ifndef __TE_TAD_REPLY__
#define __TE_TAD_REPLY__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "te_errno.h"
#include "asn_usr.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Just status report without any additional information */
typedef te_errno (tad_reply_op_status)(void *, te_errno);

/** Report poll completion or error */
typedef te_errno (tad_reply_op_poll)(void *, te_errno, unsigned int);

/** Report status and number of sent/received packets */
typedef te_errno (tad_reply_op_pkts)(void *, te_errno, unsigned int);

/** Report received packet */
typedef te_errno (tad_reply_op_pkt)(void *, const asn_value *);

/** TAD async reply backend specification */
typedef struct tad_reply_spec {
    size_t                  opaque_size;
    tad_reply_op_status    *status;
    tad_reply_op_poll      *poll;
    tad_reply_op_pkts      *pkts;
    tad_reply_op_pkt       *pkt;
} tad_reply_spec;


/** TAD async reply context */
typedef struct tad_reply_context {
    const tad_reply_spec   *spec;       /**< Reply backend spec */
    void                   *opaque;     /**< Reply backend data */
} tad_reply_context;

/** Clone TAD reply context. */
static inline te_errno
tad_reply_clone(tad_reply_context *dst, const tad_reply_context *src)
{
    memset(dst, 0, sizeof(*dst));

    dst->opaque = malloc(src->spec->opaque_size);
    if (dst->opaque == NULL)
        return TE_OS_RC(TE_TAD_CH, errno);

    dst->spec = src->spec;
    memcpy(dst->opaque, src->opaque, src->spec->opaque_size);

    return 0;
}

/** Clean up TAD reply context. */
static inline void
tad_reply_cleanup(tad_reply_context *ctx)
{
    ctx->spec = NULL;
    free(ctx->opaque);
    ctx->opaque = NULL;
}


/**
 * Async status reply.
 *
 * @param ctx           TAD async reply context
 * @param rc            Status to be reported
 */
static inline te_errno
tad_reply_status(tad_reply_context *ctx, te_errno rc)
{
    return (ctx != NULL && ctx->spec != NULL && ctx->spec->status != NULL) ?
                ctx->spec->status(ctx->opaque, rc) : 0;
}

/**
 * Async report poll completion.
 *
 * @param ctx           TAD async reply context
 * @param rc            Status to be reported
 * @param poll_id       Poll ID
 */
static inline te_errno
tad_reply_poll(tad_reply_context *ctx, te_errno rc, unsigned int poll_id)
{
    return (ctx != NULL && ctx->spec != NULL && ctx->spec->poll != NULL) ?
                ctx->spec->poll(ctx->opaque, rc, poll_id) : 0;
}

/**
 * Async report number of sent/received packets.
 *
 * @param ctx           TAD async reply context
 * @param rc            Status to be reported
 * @param num           Number of sent or received packets
 */
static inline te_errno
tad_reply_pkts(tad_reply_context *ctx, te_errno rc, unsigned int num)
{
    return (ctx != NULL && ctx->spec != NULL && ctx->spec->pkts != NULL) ?
                ctx->spec->pkts(ctx->opaque, rc, num) : 0;
}

/**
 * Async report received packet.
 *
 * @param ctx           TAD async reply context
 * @param pkt           Packet in ASN.1 value
 */
static inline te_errno
tad_reply_pkt(tad_reply_context *ctx, const asn_value *pkt)
{
    return (ctx != NULL && ctx->spec != NULL && ctx->spec->pkt != NULL) ?
                ctx->spec->pkt(ctx->opaque, pkt) : 0;
}

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /*  __TE_TAD_REPLY__ */
