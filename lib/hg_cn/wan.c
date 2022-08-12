/** @file
 * @brief Home Gateway Configuration Notation
 *
 * WAN side configuration ANS.1 syntax.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 */

#define TE_LGR_USER     "HG CN WAN"

#include "te_config.h"

#include "te_defs.h"
#include "logger_api.h"
#include "asn_usr.h"
#include "hg_cn.h"
#include "hg_cn_wan.h"


/* WAN configuration */
static asn_named_entry_t _hg_cn_wan_ne_array[] = {
    {
        .name   = "eth",
        .type   = &hg_cn_wan_eth_s,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "atm",
        .type   = &hg_cn_wan_atm_s,
        .tag    = { PRIVATE, 0 },
    },
};

asn_type hg_cn_wan_s = {
    .name   = "WAN",
    .tag    = { PRIVATE, 0 },
    .syntax = SEQUENCE,
    .len    = TE_ARRAY_LEN(_hg_cn_wan_ne_array),
    .sp     = { _hg_cn_wan_ne_array },
};

const asn_type * const hg_cn_wan = &hg_cn_wan_s;
