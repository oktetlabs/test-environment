/** @file
 * @brief Home Gateway Configuration Notation
 *
 * LAN side configuration ANS.1 syntax.
 * LAN groups.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 */

#define TE_LGR_USER     "HG CN LAN group"

#include "te_config.h"

#include "te_defs.h"
#include "logger_api.h"
#include "asn_usr.h"
#include "hg_cn.h"
#include "hg_cn_lan.h"


/* LAN group virtual interface configuration */
static asn_named_entry_t _hg_cn_lan_group_vif_ne_array[] = {
    {
        .name   = "if",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "vlan_id",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
};

asn_type hg_cn_lan_group_vif_s = {
    .name   = "LAN group VIF",
    .tag    = { PRIVATE, 0 },
    .syntax = SEQUENCE,
    .len    = TE_ARRAY_LEN(_hg_cn_lan_group_vif_ne_array),
    .sp     = { &_hg_cn_lan_group_vif_ne_array },
};

const asn_type * const hg_cn_lan_group_vif = &hg_cn_lan_group_vif_s;


/* LAN group virtual interfaces */
asn_type hg_cn_lan_group_vifs_s = {
    .name   = "LAN group VIFs",
    .tag    = { PRIVATE, 0 },
    .syntax = SEQUENCE_OF,
    .len    = 0,
    .sp     = { .subtype = &hg_cn_lan_group_vif_s },
};

const asn_type * const hg_cn_lan_group_vifs = &hg_cn_lan_group_vifs_s;


/* LAN group configuration */
static asn_named_entry_t _hg_cn_lan_group_ne_array[] = {
    {
        .name   = "name",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "description",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "priority",
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
        .name   = "rt_conn",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "ppp_pt_conn",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "ppp_pt_oper",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "vif",
        .type   = &hg_cn_lan_group_vifs_s,
        .tag    = { PRIVATE, 0 },
    },
};

asn_type hg_cn_lan_group_s = {
    .name   = "LAN group",
    .tag    = { PRIVATE, 0 },
    .syntax = SEQUENCE,
    .len    = TE_ARRAY_LEN(_hg_cn_lan_group_ne_array),
    .sp     = { &_hg_cn_lan_group_ne_array },
};

const asn_type * const hg_cn_lan_group = &hg_cn_lan_group_s;


/* LAN groups array */
asn_type hg_cn_lan_groups_s = {
    .name   = "LAN groups",
    .tag    = { PRIVATE, 0 },
    .syntax = SEQUENCE_OF,
    .len    = 0,
    .sp     = { .subtype = &hg_cn_lan_group_s },
};

const asn_type * const hg_cn_lan_groups = &hg_cn_lan_groups_s;
