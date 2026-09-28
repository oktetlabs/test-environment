/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2024 OKTET Ltd. All rights reserved. */
/** @file
 * @brief TS-provided subtree
 */

#define TE_LGR_USER     "CS Lib Hello World"

#include "te_compiler.h"

#if TE_CONSTRUCTOR_AVAILABLE

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "te_stdint.h"
#include "te_errno.h"
#include "logger_api.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "logger_api.h"
#include "logger_file.h"

static te_errno
helloworld_get(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    strcpy(value, "Hello, world!");

    return 0;
}

/**
 * Log and return the maximum size_t value.
 *
 * Logging is performed in the TA process to check handling of the @c z
 * length modifier by the TA logger.
 */
static te_errno
size_max_get(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    RING("TA %%zi format check: %zi", (ssize_t)(-1));
    RING("TA %%zd format check: %zd", (ssize_t)(-1));
    RING("TA %%zo format check: %zo", SIZE_MAX);
    RING("TA %%zu format check: %zu", SIZE_MAX);
    RING("TA %%zx format check: %zx", SIZE_MAX);
    RING("TA %%zX format check: %zX", SIZE_MAX);

    snprintf(value, RCF_MAX_VAL, "%zu", SIZE_MAX);

    return 0;
}

RCF_PCH_CFG_NODE_RO(helloworld, "ts_lib_helloworld", NULL, NULL, helloworld_get);
RCF_PCH_CFG_NODE_RO(size_max, "ts_lib_size_max", NULL, NULL, size_max_get);

TE_CONSTRUCTOR
static void
pch_init(void)
{
    te_errno rc;

    rc = rcf_pch_add_node("/agent", &helloworld);
    if (rc != 0)
        ERROR("Failed to init the ts_lib_helloworld PCH node: %r", rc);

    rc = rcf_pch_add_node("/agent", &size_max);
    if (rc != 0)
        ERROR("Failed to init the ts_lib_size_max PCH node: %r", rc);
}

#endif /* TE_CONSTRUCTOR_AVAILABLE */
