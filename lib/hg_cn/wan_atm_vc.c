/** @file
 * @brief Home Gateway Configuration Notation
 *
 * ATM/DSL WAN side configuration ANS.1 syntax.
 * ATM virtual curcuits.
 *
 *
 * Copyright (C) 2009 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
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
