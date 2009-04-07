/** @file
 * @brief Home Gateway Configuration Notation
 *
 * ATM/DSL WAN side configuration ANS.1 syntax.
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

#define TE_LGR_USER     "HG CN WAN ATM"

#include "te_config.h"

#include "te_defs.h"
#include "logger_api.h"
#include "asn_usr.h"
#include "hg_cn.h"
#include "hg_cn_wan.h"


/* Ethernet WAN - Bridge connection layers */
static asn_named_entry_t _hg_cn_wan_atm_conn_bridge_ne_array[] = {
    {
        .name   = "eth_if",
        .type   = &hg_cn_wan_layer_atm_if_s,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "bridge",
        .type   = &hg_cn_wan_layer_bridge_s,
        .tag    = { PRIVATE, 0 },
    },
};

static asn_type hg_cn_wan_atm_conn_bridge_s = {
    .name   = "WAN Ethernet Bridge layers",
    .tag    = { PRIVATE, 0 },
    .syntax = SEQUENCE,
    .len    = TE_ARRAY_LEN(_hg_cn_wan_atm_conn_bridge_ne_array),
    .sp     = { _hg_cn_wan_atm_conn_bridge_ne_array },
};


/* Ethernet WAN - Static connection layers */
static asn_named_entry_t _hg_cn_wan_atm_conn_static_ne_array[] = {
    {
        .name   = "eth_if",
        .type   = &hg_cn_wan_layer_atm_if_s,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "bridge",
        .type   = &hg_cn_wan_layer_bridge_s,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "ip_static",
        .type   = &hg_cn_wan_layer_ip_static_s,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "firewall",
        .type   = &hg_cn_wan_layer_firewall_s,
        .tag    = { PRIVATE, 0 },
    },
};

static asn_type hg_cn_wan_atm_conn_static_s = {
    .name   = "WAN Ethernet Static layers",
    .tag    = { PRIVATE, 0 },
    .syntax = SEQUENCE,
    .len    = TE_ARRAY_LEN(_hg_cn_wan_atm_conn_static_ne_array),
    .sp     = { _hg_cn_wan_atm_conn_static_ne_array },
};


/* Ethernet WAN - DHCP connection layers */
static asn_named_entry_t _hg_cn_wan_atm_conn_dhcp_ne_array[] = {
    {
        .name   = "eth_if",
        .type   = &hg_cn_wan_layer_atm_if_s,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "bridge",
        .type   = &hg_cn_wan_layer_bridge_s,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "ip_dhcp",
        .type   = &hg_cn_wan_layer_ip_dhcp_s,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "firewall",
        .type   = &hg_cn_wan_layer_firewall_s,
        .tag    = { PRIVATE, 0 },
    },
};

static asn_type hg_cn_wan_atm_conn_dhcp_s = {
    .name   = "WAN Ethernet DHCP layers",
    .tag    = { PRIVATE, 0 },
    .syntax = SEQUENCE,
    .len    = TE_ARRAY_LEN(_hg_cn_wan_atm_conn_dhcp_ne_array),
    .sp     = { _hg_cn_wan_atm_conn_dhcp_ne_array },
};


/* Ethernet WAN - PPPoE connection layers */
static asn_named_entry_t _hg_cn_wan_atm_conn_pppoe_ne_array[] = {
    {
        .name   = "eth_if",
        .type   = &hg_cn_wan_layer_atm_if_s,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "bridge",
        .type   = &hg_cn_wan_layer_bridge_s,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "ppp",
        .type   = &hg_cn_wan_layer_ppp_s,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "ip_ipcp",
        .type   = &hg_cn_wan_layer_ip_ipcp_s,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "firewall",
        .type   = &hg_cn_wan_layer_firewall_s,
        .tag    = { PRIVATE, 0 },
    },
};

static asn_type hg_cn_wan_atm_conn_pppoe_s = {
    .name   = "WAN Ethernet PPPoE layers",
    .tag    = { PRIVATE, 0 },
    .syntax = SEQUENCE,
    .len    = TE_ARRAY_LEN(_hg_cn_wan_atm_conn_pppoe_ne_array),
    .sp     = { _hg_cn_wan_atm_conn_pppoe_ne_array },
};


/* Ethernet WAN connection types */
static asn_named_entry_t _hg_cn_wan_atm_conn_layers_ne_array[] = {
    {
        .name   = "pppoe",
        .type   = &hg_cn_wan_atm_conn_pppoe_s,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "dhcp",
        .type   = &hg_cn_wan_atm_conn_dhcp_s,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "static",
        .type   = &hg_cn_wan_atm_conn_static_s,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "bridge",
        .type   = &hg_cn_wan_atm_conn_bridge_s,
        .tag    = { PRIVATE, 0 },
    },
};

static asn_type hg_cn_wan_atm_conn_s = {
    .name   = "WAN Ethernet layers",
    .tag    = { PRIVATE, 0 },
    .syntax = CHOICE,
    .len    = TE_ARRAY_LEN(_hg_cn_wan_atm_conn_layers_ne_array),
    .sp     = { _hg_cn_wan_atm_conn_layers_ne_array },
};


/* Ethernet WAN connection types */
static asn_named_entry_t _hg_cn_wan_atm_conn_ne_array[] = {
    {
        .name   = "common",
        .type   = &hg_cn_wan_conn_common_s,
        .tag    = { PRIVATE, 0 },
    },
    {
        .name   = "layer",
        .type   = &hg_cn_wan_atm_conn_layers_s,
        .tag    = { PRIVATE, 0 },
    },
};

static asn_type hg_cn_wan_atm_conn_s = {
    .name   = "WAN Ethernet connection",
    .tag    = { PRIVATE, 0 },
    .syntax = SEQUENCE,
    .len    = TE_ARRAY_LEN(_hg_cn_wan_atm_conn_ne_array),
    .sp     = { _hg_cn_wan_atm_conn_ne_array },
};


/* Ethernet WAN connections */
static asn_type hg_cn_wan_atm_conns_s = {
    .name   = "WAN Ethernet connections",
    .tag    = { PRIVATE, 0 },
    .syntax = SEQUENCE_OF,
    .len    = 0,
    .sp     = { .subtype = &hg_cn_wan_atm_conn_s },
};


/* Ethernet WAN configuration */
static asn_named_entry_t _hg_cn_wan_atm_ne_array[] = {
    {
        .name   = "conn",
        .type   = &hg_cn_wan_atm_conns_s,
        .tag    = { PRIVATE, 0 },
    },
};

asn_type hg_cn_wan_atm_s = {
    .name   = "WAN Ethernet",
    .tag    = { PRIVATE, 0 },
    .syntax = SEQUENCE,
    .len    = TE_ARRAY_LEN(_hg_cn_wan_atm_ne_array),
    .sp     = { _hg_cn_wan_atm_ne_array },
};

const asn_type * const hg_cn_wan_atm = &hg_cn_wan_atm_s;
