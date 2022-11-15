/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) OKTET Ltd. All rights reserved. */
/** @file
 * @brief Test API to get info about RAM.
 *
 * Implementation of API to get info about RAM.
 */

#define TE_LGR_USER      "Conf RAM TAPI"

#include "te_config.h"

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif

#include "te_defs.h"
#include "te_errno.h"

#include "tapi_cfg.h"
#include "tapi_cfg_base.h"
#include "tapi_cfg_memory.h"

te_errno
tapi_cfg_get_memory(const char *ta, int node_id, uint64_t *memory)
{
    te_errno rc;

    rc = tapi_cfg_get_uint64_fmt(memory,
                                 "/agent:%s/hardware:/node:%d/memory:",
                                 ta, node_id);
    if (rc != 0)
    {
        ERROR("Failed to get memory property of node %d: error %r",
              node_id, rc);
        return rc;
    }

    return 0;
}

te_errno
tapi_cfg_get_free_memory(const char *ta, int node_id, uint64_t *avail_mem)
{
    te_errno rc;

    rc = tapi_cfg_get_uint64_fmt(avail_mem,
                                 "/agent:%s/hardware:/node:%d"
                                 "/memory:/free:", ta, node_id);
    if (rc != 0)
    {
        ERROR("Failed to get memory/free property of node %d: error %r",
              node_id, rc);
        return rc;
    }

    return 0;
}
