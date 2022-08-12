/** @file
 * @brief Home Gateway Configuration Notation
 *
 * LAN side configuration ANS.1 syntax.
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

#define TE_LGR_USER     "HG CN LAN"

#include "te_config.h"

#include "te_defs.h"
#include "logger_api.h"
#include "asn_usr.h"
#include "hg_cn.h"
#include "hg_cn_lan.h"


/* LAN configuration */
static asn_named_entry_t _hg_cn_lan_ne_array[] = {
    {
        .name   = "group",
        .type   = &hg_cn_lan_groups_s,
        .tag    = { PRIVATE, 0 },
    },
};

asn_type hg_cn_lan_s = {
    .name   = "LAN",
    .tag    = { PRIVATE, 0 },
    .syntax = SEQUENCE,
    .len    = TE_ARRAY_LEN(_hg_cn_lan_ne_array),
    .sp     = { _hg_cn_lan_ne_array },
};

const asn_type * const hg_cn_lan = &hg_cn_lan_s;
