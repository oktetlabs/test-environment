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

#define TE_LGR_USER     "HG CN WAN Bridge"

#include "te_config.h"

#include "te_defs.h"
#include "logger_api.h"
#include "asn_usr.h"
#include "hg_cn.h"
#include "hg_cn_wan_layer.h"


/* WAN connection Ethernet bridging layer */
static asn_named_entry_t _hg_cn_wan_layer_bridge_ne_array[] = {
    {
        .name   = "ppp_pt",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "lan_group",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
};

asn_type hg_cn_wan_layer_bridge_s = {
    .name   = "WAN Conn Layer Bridge",
    .tag    = { PRIVATE, 0 },
    .syntax = SEQUENCE,
    .len    = TE_ARRAY_LEN(_hg_cn_wan_layer_bridge_ne_array),
    .sp     = { _hg_cn_wan_layer_bridge_ne_array },
};

const asn_type * const hg_cn_wan_layer_bridge = &hg_cn_wan_layer_bridge_s;
