/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2024 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief RESTCONF agent library
 *
 * The library provides types, functions and configuration tree
 * for RESTCONF management.
 */

#define TE_LGR_USER "TA RESTCONF"

#include "te_config.h"

#include "rcf_pch.h"
#include "ta_restconf.h"
#include "ta_restconf_internal.h"


restconf_settings restconf = {};


RCF_PCH_CFG_NODE_RO(node_config, "config", NULL, NULL, NULL);

RCF_PCH_CFG_NODE_RO(node_restconf, "restconf", &node_config, NULL, NULL);


te_errno
ta_restconf_conf_init(void)
{
    te_errno rc;

    rc = rcf_pch_add_node("/agent", &node_restconf);
    if (rc != 0)
        return rc;

    rc = ta_restconf_conf_server_init();
    if (rc != 0)
        return rc;

    rc = ta_restconf_conf_search_init();
    if (rc != 0)
        return rc;

    return 0;
}
