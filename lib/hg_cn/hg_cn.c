/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Home Gateway Configuration Notation
 *
 * Defition of Home Gateway configuration using ASN.1 syntax.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "HG CN"

#include "te_config.h"

#include "te_defs.h"
#include "logger_api.h"
#include "asn_usr.h"
#include "hg_cn.h"
#include "hg_cn_lan.h"
#include "hg_cn_wan.h"


/* Configuration root */
static asn_named_entry_t _hg_cn_root_ne_array[] = {
    {
        .name   = "wan",
        .type   = &hg_cn_wan_s,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "lan",
        .type   = &hg_cn_lan_s,
        .tag    = { PRIVATE, 0 },
    },
};

asn_type hg_cn_root_s = {
    .name   = "Configuration",
    .tag    = { PRIVATE, 0 },
    .syntax = SEQUENCE,
    .len    = TE_ARRAY_LEN(_hg_cn_root_ne_array),
    .sp     = { _hg_cn_root_ne_array },
};

const asn_type * const hg_cn_root = &hg_cn_root_s;
