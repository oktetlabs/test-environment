/** @file
 * @brief Home Gateway Configuration Notation
 *
 * ATM/DSL WAN side configuration ANS.1 syntax.
 *
 *
 * Copyright (C) 2004-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 */

#define TE_LGR_USER     "HG CN WAN ATM"

#include "te_config.h"

#include "te_defs.h"
#include "logger_api.h"
#include "asn_usr.h"
#include "hg_cn.h"
#include "hg_cn_wan.h"


/* ATM WAN configuration */
static asn_named_entry_t _hg_cn_wan_atm_ne_array[] = {
    {
        .name   = "conn",
        .type   = &hg_cn_wan_atm_conns_s,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "vc",
        .type   = &hg_cn_wan_atm_vcs_s,
        .tag    = { PRIVATE, 0 },
    },
};

asn_type hg_cn_wan_atm_s = {
    .name   = "WAN ATM",
    .tag    = { PRIVATE, 0 },
    .syntax = SEQUENCE,
    .len    = TE_ARRAY_LEN(_hg_cn_wan_atm_ne_array),
    .sp     = { _hg_cn_wan_atm_ne_array },
};

const asn_type * const hg_cn_wan_atm = &hg_cn_wan_atm_s;
