/** @file
 * @brief Home Gateway Configuration Notation
 *
 * ATM/DSL WAN side configuration ANS.1 syntax.
 * ATM virtual curcuits.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "HG CN WAN ATM VC"

#include "te_config.h"

#include "te_defs.h"
#include "logger_api.h"
#include "asn_usr.h"
#include "hg_cn.h"
#include "hg_cn_wan.h"


/* ATM virtual curcuit */
static asn_named_entry_t _hg_cn_wan_atm_vc_ne_array[] = {
    {
        .name   = "name",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "admin",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "oper",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "auto",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "vpi",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "vci",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "encaps",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "category",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "pcr",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "scr",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "mbs",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "cdvt",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
};

static asn_type hg_cn_wan_atm_vc_s = {
    .name   = "WAN ATM VC",
    .tag    = { PRIVATE, 0 },
    .syntax = SEQUENCE,
    .len    = TE_ARRAY_LEN(_hg_cn_wan_atm_vc_ne_array),
    .sp     = { _hg_cn_wan_atm_vc_ne_array },
};


/* ATM virtual curcuits */
asn_type hg_cn_wan_atm_vcs_s = {
    .name   = "WAN ATM VCs",
    .tag    = { PRIVATE, 0 },
    .syntax = SEQUENCE_OF,
    .len    = 0,
    .sp     = { .subtype = &hg_cn_wan_atm_vc_s },
};

const asn_type * const hg_cn_wan_atm_vcs = &hg_cn_wan_atm_vcs_s;
