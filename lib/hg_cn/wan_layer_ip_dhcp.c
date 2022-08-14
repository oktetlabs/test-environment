/** @file
 * @brief Home Gateway Configuration Notation
 *
 * WAN side configuration ANS.1 syntax.
 * DHCP IP address layer.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "HG CN WAN IP DHCP"

#include "te_config.h"

#include "te_defs.h"
#include "logger_api.h"
#include "asn_usr.h"
#include "hg_cn.h"
#include "hg_cn_wan_layer.h"


/* WAN connection IP DHCP layer */
static asn_named_entry_t _hg_cn_wan_layer_ip_dhcp_ne_array[] = {
    {
        .name   = "vendor_id",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
};

asn_type hg_cn_wan_layer_ip_dhcp_s = {
    .name   = "WAN Conn Layer IP DHCP",
    .tag    = { PRIVATE, 0 },
    .syntax = SEQUENCE,
    .len    = TE_ARRAY_LEN(_hg_cn_wan_layer_ip_dhcp_ne_array),
    .sp     = { _hg_cn_wan_layer_ip_dhcp_ne_array },
};

const asn_type * const hg_cn_wan_layer_ip_dhcp =
    &hg_cn_wan_layer_ip_dhcp_s;
