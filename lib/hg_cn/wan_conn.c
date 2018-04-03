/** @file
 * @brief Home Gateway Configuration Notation
 *
 * WAN side configuration ANS.1 syntax.
 * WAN connection common configuration.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "HG CN WAN Conn"

#include "te_config.h"

#include "te_defs.h"
#include "logger_api.h"
#include "asn_usr.h"
#include "hg_cn.h"
#include "hg_cn_wan.h"


/* WAN connection common configuration */
static asn_named_entry_t _hg_cn_wan_conn_common_ne_array[] = {
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
        .name   = "admin",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
};

asn_type hg_cn_wan_conn_common_s = {
    .name   = "WAN Conn Common",
    .tag    = { PRIVATE, 0 },
    .syntax = SEQUENCE,
    .len    = TE_ARRAY_LEN(_hg_cn_wan_conn_common_ne_array),
    .sp     = { _hg_cn_wan_conn_common_ne_array },
};

const asn_type * const hg_cn_wan_conn_common = &hg_cn_wan_conn_common_s;
