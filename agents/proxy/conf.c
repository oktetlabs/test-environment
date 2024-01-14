/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Proxy Test Agent
 *
 * Configuration tree support.
 *
 *
 * Copyright (C) 2024 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER      "Configurator"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "proxy_internal.h"

/* See description in rcf_ch_api.h */
int
rcf_ch_conf_init(void)
{
    /*
     * We do not export/support any configuration object,
     * so we do not register any subtree with rcf_pch_add_node().
     */
    return 0;
}

/* See description in rcf_ch_api.h */
const char *
rcf_ch_conf_agent(void)
{
    return ta_name;
}

/* See description in rcf_ch_api.h */
void
rcf_ch_conf_fini(void)
{
}
