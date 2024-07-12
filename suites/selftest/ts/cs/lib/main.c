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

RCF_PCH_CFG_NODE_RO(helloworld, "ts_lib_helloworld", NULL, NULL, helloworld_get);

TE_CONSTRUCTOR
static void
pch_init(void)
{
    te_errno rc;

    rc = rcf_pch_add_node("/agent", &helloworld);
    if (rc != 0)
        ERROR("Failed to init the PCH node: %r", rc);
}

#endif /* TE_CONSTRUCTOR_AVAILABLE */
