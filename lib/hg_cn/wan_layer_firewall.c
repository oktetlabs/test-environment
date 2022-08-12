/** @file
 * @brief Home Gateway Configuration Notation
 *
 * WAN side configuration ANS.1 syntax.
 * Firewall layer.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 */

#define TE_LGR_USER     "HG CN WAN Firewall"

#include "te_config.h"

#include "te_defs.h"
#include "logger_api.h"
#include "asn_usr.h"
#include "hg_cn.h"
#include "hg_cn_wan_layer.h"


/* WAN connection Firewall layer */
static asn_named_entry_t _hg_cn_wan_layer_firewall_ne_array[] = {
    {
        .name   = "firewall",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "nat",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "enforce_mss",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "dmz",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "dmz_host",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
};

asn_type hg_cn_wan_layer_firewall_s = {
    .name   = "WAN Conn Layer Firewall",
    .tag    = { PRIVATE, 0 },
    .syntax = SEQUENCE,
    .len    = TE_ARRAY_LEN(_hg_cn_wan_layer_firewall_ne_array),
    .sp     = { _hg_cn_wan_layer_firewall_ne_array },
};

const asn_type * const hg_cn_wan_layer_firewall =
    &hg_cn_wan_layer_firewall_s;
