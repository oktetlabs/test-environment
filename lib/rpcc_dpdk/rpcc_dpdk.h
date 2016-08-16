/** @file
 * @brief RPC client API for DPDK
 *
 * RPC client API for DPDK internal definitions and functions.
 *
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS in
 * the root directory of the distribution).
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
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
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
