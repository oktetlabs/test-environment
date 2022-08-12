/** @file
 * @brief Home Gateway Configuration Notation
 *
 * WAN side configuration ANS.1 syntax.
 * Ethernet interface layer.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 */

#define TE_LGR_USER     "HG CN WAN EthIf"

#include "te_config.h"

#include "te_defs.h"
#include "logger_api.h"
#include "asn_usr.h"
#include "hg_cn.h"
#include "hg_cn_wan_layer_eth_if.h"


/* WAN connection Ethernet interface layer */
static asn_named_entry_t _hg_cn_wan_layer_eth_if_ne_array[] = {
    {
        .name   = "vlan_id",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "priority",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
};

asn_type hg_cn_wan_layer_eth_if_s = {
    .name   = "WAN Conn Layer Eth If",
    .tag    = { PRIVATE, 0 },
    .syntax = SEQUENCE,
    .len    = TE_ARRAY_LEN(_hg_cn_wan_layer_eth_if_ne_array),
    .sp     = { _hg_cn_wan_layer_eth_if_ne_array },
};

const asn_type * const hg_cn_wan_layer_eth_if = &hg_cn_wan_layer_eth_if_s;
