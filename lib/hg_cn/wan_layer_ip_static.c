/** @file
 * @brief Home Gateway Configuration Notation
 *
 * WAN side configuration ANS.1 syntax.
 * Static IP address layer.
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
