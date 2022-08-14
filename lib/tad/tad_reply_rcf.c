/** @file
 * @brief TAD async RCF replies
 *
 * Traffic Application Domain Command Handler.
 * Async RCF reply backend implementation.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include <stdio.h>
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_STDARG_H
#include <stdarg.h>
#endif

#include "te_errno.h"
#include "logger_api.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "asn_usr.h"
#include "tad_reply_rcf.h"


/* TODO: this constant should be placed to more appropriate header! */
/**
 * Maximum length of the test protocol answer to be sent by TAD.
 */
#define TAD_ANSWER_LEN  0x100


/** Reply to RCF context */
typedef struct tad_reply_rcf_ctx  {

    rcf_comm_connection    *rcfc;       /**< RCF connection to answer */

    char    answer_buf[TAD_ANSWER_LEN]; /**< Prefix for test-protocol
                                             answer to the current
                                             command */
    size_t  prefix_len;                 /**< Length of the Test Protocol
                                             answer prefix */
} tad_reply_rcf_ctx;


static te_errno
tad_reply_rfc_ctx_alloc(rcf_comm_connection *rcfc,
                        const char *answer_pfx, size_t pfx_len,
                        tad_reply_rcf_ctx **ctxp)
{
    tad_reply_rcf_ctx  *ctx;

    if (pfx_len >= sizeof(ctx->answer_buf))
    {
        ERROR("Too small buffer for Test Protocol command answer in TAD "
              "RCF reply structure");
        return TE_RC(TE_TAD_CH, TE_ESMALLBUF);
    }

    ctx = malloc(sizeof(*ctx));
    if (ctx == NULL)
        return TE_OS_RC(TE_TAD_CH, errno);

    ctx->rcfc = rcfc;
    ctx->prefix_len = pfx_len;
    memcpy(ctx->answer_buf, answer_pfx, pfx_len);

    *ctxp = ctx;

    return 0;
}


static te_errno
tad_reply_rfc_fmt(void *opaque, const char *fmt, ...)
{
    tad_reply_rcf_ctx  *ctx = opaque;
    te_errno            rc;
    va_list             ap;
    int                 buf_len = sizeof(ctx->answer_buf) - ctx->prefix_len;

    va_start(ap, fmt);
    if (vsnprintf(ctx->answer_buf + ctx->prefix_len, buf_len,
                  fmt, ap) >= buf_len)
    {
        ERROR("TE protocol answer is truncated");
        /* Try to continue */
    }
    INFO("Sending reply: '%s'", ctx->answer_buf);
    RCF_CH_SAFE_LOCK;
    rc = rcf_comm_agent_reply(ctx->rcfc, ctx->answer_buf,
                              strlen(ctx->answer_buf) + 1);
    RCF_CH_SAFE_UNLOCK;

    return rc;
}

static tad_reply_op_status tad_reply_rcf_status;
static te_errno
tad_reply_rcf_status(void *opaque, te_errno rc)
{
    return tad_reply_rfc_fmt(opaque, "%u", (unsigned int)rc);
}

static tad_reply_op_poll tad_reply_rcf_poll;
static te_errno
tad_reply_rcf_poll(void *opaque, te_errno rc, unsigned int poll_id)
{
    return tad_reply_rfc_fmt(opaque, "%u %u", (unsigned int)rc, poll_id);
}

static tad_reply_op_pkts tad_reply_rcf_pkts;
static te_errno
tad_reply_rcf_pkts(void *opaque, te_errno rc, unsigned int num)
{
    return tad_reply_rfc_fmt(opaque, "%u %u", (unsigned int)rc, num);
}

static tad_reply_op_pkt tad_reply_rcf_pkt;
static te_errno
tad_reply_rcf_pkt(void *opaque, const asn_value *pkt)
{
/*
 * It is an upper estimation for "attach" and decimal presentation
 * of attach length.
 */
#define EXTRA_BUF_SPACE     20

    tad_reply_rcf_ctx  *ctx = opaque;
    te_errno            rc;
    int                 ret;
    size_t              attach_len;
    int                 attach_rlen;
    char               *buffer;
    size_t              cmd_len;

    assert(pkt != NULL);

    attach_len = asn_count_txt_len(pkt, 0) + 1;
    VERB("%s(): attach len %u", __FUNCTION__, (unsigned)attach_len);

    buffer = calloc(1, ctx->prefix_len + EXTRA_BUF_SPACE + attach_len);
    if (buffer == NULL)
        return TE_ENOMEM;

    memcpy(buffer, ctx->answer_buf, ctx->prefix_len);
    ret = snprintf(buffer + ctx->prefix_len, EXTRA_BUF_SPACE, " attach %u",
                   (unsigned)attach_len);
    if (ret >= EXTRA_BUF_SPACE)
    {
        ERROR("%s(): Upper estimation on required buffer space is wrong",
              __FUNCTION__);
        free(buffer);
        return TE_ESMALLBUF;
    }
    cmd_len = strlen(buffer) + 1;

    if ((attach_rlen =
         asn_sprint_value(pkt, buffer + cmd_len, attach_len, 0))
        != (int)(attach_len - 1))
    {
        ERROR("%s(): asn_sprint_value() returns unexpected number: "
              "expected %u, got %d",
              __FUNCTION__, (unsigned)(attach_len - 1), attach_rlen);
        free(buffer);
        return TE_EFAULT;
    }

    RCF_CH_SAFE_LOCK;
    rc = rcf_comm_agent_reply(ctx->rcfc, buffer, cmd_len + attach_len);
    RCF_CH_SAFE_UNLOCK;
    free(buffer);

    return rc;

#undef EXTRA_BUF_SPACE
}

/** Reply to RCF backend specification */
static const tad_reply_spec tad_reply_rfc = {
    .opaque_size    = sizeof(tad_reply_rcf_ctx),
    .status         = tad_reply_rcf_status,
    .poll           = tad_reply_rcf_poll,
    .pkts           = tad_reply_rcf_pkts,
    .pkt            = tad_reply_rcf_pkt,
};


/* See the description in tad_reply_rcf.h */
te_errno
tad_reply_rcf_init(tad_reply_context *ctx, rcf_comm_connection *rcfc,
                   const char *answer_pfx, size_t pfx_len)
{
    te_errno            rc;
    tad_reply_rcf_ctx  *rcf_ctx = NULL; /* Just to make compiler happy */

    rc = tad_reply_rfc_ctx_alloc(rcfc, answer_pfx, pfx_len, &rcf_ctx);
    if (rc != 0)
        return rc;

    ctx->spec = &tad_reply_rfc;
    ctx->opaque = rcf_ctx;

    return 0;
}
