/** @file
 * @brief TAD async RCF replies
 *
 * Traffic Application Domain Command Handler.
 * Async RCF reply backend functions definition.
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
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
