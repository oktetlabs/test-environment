/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Unix Test Agent: DHCP server
 *
 * Definition of DHCP server configuration notions.
 *
 * Copyright (C) 2006-2022 OKTET Labs Ltd. All rights reserved.
 */
#ifndef __TE_TA_UNIX_DHCP_SERVER_H__
#define __TE_TA_UNIX_DHCP_SERVER_H__

#include "te_defs.h"
#include "te_queue.h"

enum {
    TE_DHCPS_ALLOW,
    TE_DHCPS_DENY,
    TE_DHCPS_IGNORE,
};

enum {
    TE_DHCPS_DDNS_UPDATE_NONE,
    TE_DHCPS_DDNS_UPDATE_AD_HOC,
    TE_DHCPS_DDNS_UPDATE_INTERIM,
};

/** Definitions of types for DHCP configuring */
typedef struct te_dhcp_option {
    struct te_dhcp_option *next;

    char               *name;
    char               *value;
} te_dhcp_option;

/** Definitions for option spaces for DHCP configuring */
typedef struct te_dhcp_space_opt {
    struct te_dhcp_space_opt *next;

    char   *name;
    int     code;
    char   *type;
} te_dhcp_space_opt;

typedef struct space {
    struct space      *next;

    char              *name;
    te_dhcp_space_opt *options;

} space;

typedef struct host {
    struct host    *next;
    char           *name;
    struct group   *group;
    char           *chaddr;
    char           *client_id;
    char           *ip_addr;
    char           *next_server;
    char           *filename;
    char           *flags;
    char           *host_id;
    char           *prefix6;
    te_dhcp_option *options;
} host;

typedef struct group {
    struct group   *next;
    char           *name;
    char           *filename;
    char           *next_server;
    te_dhcp_option *options;
} group;

typedef struct te_dhcp_server_subnet {
    TAILQ_ENTRY(te_dhcp_server_subnet)  links;

    char           *subnet;
    int             prefix_len;
    char           *range;
    te_dhcp_option *options;
    char           *vos;
} te_dhcp_server_subnet;

typedef struct te_dhcp_server_shared_net {
    char   *name;
} te_dhcp_server_shared_net;

/** DHCP server configuration */
typedef struct te_dhcp_server_cfg {
    TAILQ_HEAD(te_dhcp_server_shared_nets, te_dhcp_server_shared_net)
    shared_nets;
} te_dhcp_server_cfg;

extern int isc_dhcp_server_cfg_parse(const char *filename);

#endif /* !__TE_TA_UNIX_DHCP_SERVER_H__ */
