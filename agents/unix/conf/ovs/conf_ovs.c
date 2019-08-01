/** @file
 * @brief Unix Test Agent
 *
 * Unix TA Open vSwitch deployment.
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 * @author Ivan Malov <Ivan.Malov@oktetlabs.ru>
 */

#define TE_LGR_USER "TA Unix OVS"

#include "te_config.h"

#if HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "te_errno.h"

#include "logger_api.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"

#include "conf_ovs.h"

RCF_PCH_CFG_NODE_NA(node_ovs, "ovs", NULL, NULL);

te_errno
ta_unix_conf_ovs_init(void)
{
    INFO("Initialising the facility");

    return rcf_pch_add_node("/agent", &node_ovs);
}
