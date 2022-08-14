/** @file
 * @brief RPC client API for DPDK
 *
 * RPC client API for DPDK internal definitions and functions.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_RPCC_DPDK_H__
#define __TE_RPCC_DPDK_H__

#include "te_rpc_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Negative errno with RPC sanity failure indication */
#define RETVAL_ECORRUPTED (-TE_RC(TE_TAPI, TE_ECORRUPTED))

/**
 * Generic format string for negative errno logging.
 *
 * @note It is used with #NEG_ERRNO_ARGS.
 */
#define NEG_ERRNO_FMT "%s"

/**
 * Negative errno format string values.
 *
 * @note It is used with #NEG_ERRNO_FMT.
 */
#define NEG_ERRNO_ARGS(_val) \
    neg_errno_rpc2str(_val)

/**
 * Convert negative RPC errno to string.
 *
 * Non-negative is printed as decimal number.
 *
 * @note The function uses static buffer and therefore non thread-safe
 * and non-reentrant.
 */
static inline const char *
neg_errno_rpc2str(int nerrno)
{
    static char buf[64];

    if (nerrno >= 0)
        snprintf(buf, sizeof(buf), "%d", nerrno);
    else
        snprintf(buf, sizeof(buf), "-%s", errno_rpc2str(-nerrno));

    return buf;
}

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_RPCC_DPDK_H__ */
