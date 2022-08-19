/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Home Gateway Configuration Notation
 *
 * WAN side configuration ANS.1 syntax.
 * Static IP address layer.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "HG CN WAN IP Static"

#include "te_config.h"

#include "te_defs.h"
#include "logger_api.h"
#include "asn_usr.h"
#include "hg_cn.h"
#include "hg_cn_wan_layer.h"


/* WAN connection IP static layer */
static asn_named_entry_t _hg_cn_wan_layer_ip_static_ne_array[] = {
    {
        .name   = "addr",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "prefix",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "gateway",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "hostname",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "domain",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "dns1",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "dns2",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "dns3",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
};

asn_type hg_cn_wan_layer_ip_static_s = {
    .name   = "WAN Conn Layer IP Static",
    .tag    = { PRIVATE, 0 },
    .syntax = SEQUENCE,
    .len    = TE_ARRAY_LEN(_hg_cn_wan_layer_ip_static_ne_array),
    .sp     = { _hg_cn_wan_layer_ip_static_ne_array },
};

const asn_type * const hg_cn_wan_layer_ip_static =
    &hg_cn_wan_layer_ip_static_s;
