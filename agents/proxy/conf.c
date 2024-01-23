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

#ifdef WITH_LIBTA_RESTCONF
#include "ta_restconf.h"
#endif

#include "proxy_internal.h"

/* See description in rcf_ch_api.h */
int
rcf_ch_conf_init(void)
{
#ifdef WITH_LIBTA_RESTCONF
    if (ta_restconf_conf_init() != 0)
    {
        ERROR("Failed to add RESTCONF configuration subtree");
        return -1;
    }
#endif

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
