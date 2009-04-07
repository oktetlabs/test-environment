/** @file
 * @brief Home Gateway Configuration Notation
 *
 * WAN side configuration ANS.1 syntax.
 * PPP layer.
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

#define TE_LGR_USER     "HG CN WAN PPP"

#include "te_config.h"

#include "te_defs.h"
#include "logger_api.h"
#include "asn_usr.h"
#include "hg_cn.h"
#include "hg_cn_wan_layer.h"


/* WAN connection Ethernet interface layer */
static asn_named_entry_t _hg_cn_wan_layer_ppp_ne_array[] = {
    {
        .name   = "username",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "password",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "service_name",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "ac_name",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "auth_type",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "auth_type",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "mru",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "persist",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "maxfail",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "demand",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "idle",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "proxyarp",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "debug",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "keepalive_retry",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "keepalive_interval",
        .type   = &,
        .tag    = { PRIVATE, 0 },
    },
};

asn_type hg_cn_wan_layer_ppp_s = {
    .name   = "WAN Conn Layer PPP",
    .tag    = { PRIVATE, 0 },
    .syntax = SEQUENCE,
    .len    = TE_ARRAY_LEN(_hg_cn_wan_layer_ppp_ne_array),
    .sp     = { _hg_cn_wan_layer_ppp_ne_array },
};

const asn_type * const hg_cn_wan_layer_ppp =
    &hg_cn_wan_layer_ppp_s;
