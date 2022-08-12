/** @file
 * @brief Home Gateway Configuration Notation
 *
 * WAN side configuration ANS.1 syntax.
 * IPCP IP address layer.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 */

#define TE_LGR_USER     "HG CN WAN IP IPCP"

#include "te_config.h"

#include "te_defs.h"
#include "logger_api.h"
#include "asn_usr.h"
#include "hg_cn.h"
#include "hg_cn_wan_layer.h"


/* WAN connection IP IPCP layer */
static asn_named_entry_t _hg_cn_wan_layer_ip_ipcp_ne_array[] = {
};

asn_type hg_cn_wan_layer_ip_ipcp_s = {
    .name   = "WAN Conn Layer IP IPCP",
    .tag    = { PRIVATE, 0 },
    .syntax = SEQUENCE,
    .len    = TE_ARRAY_LEN(_hg_cn_wan_layer_ip_ipcp_ne_array),
    .sp     = { _hg_cn_wan_layer_ip_ipcp_ne_array },
};

const asn_type * const hg_cn_wan_layer_ip_ipcp =
    &hg_cn_wan_layer_ip_ipcp_s;
