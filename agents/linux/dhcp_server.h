/** @file
 * @brief Linux Test Agent: DHCP server
 *
 * Definition of DHCP server configuration notions.
 *
 */

#ifndef __TE_TA_LINUX_DHCP_SERVER_H__
#define __TE_TA_LINUX_DHCP_SERVER_H__

#include <sys/queue.h>

#include "te_defs.h"


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

typedef struct host {
    struct host  *next;
    char         *name;
    struct group *group;
    char         *chaddr;
    char         *client_id;
    char         *ip_addr;
    char         *next_server;
    char         *filename;
    te_dhcp_option  *options;
} host;

typedef struct group {
    struct group   *next;
    char           *name;
    char           *filename;
    char           *next_server;
    te_dhcp_option *options;
} group;

typedef struct te_dhcp_server_subnet {
    struct te_dhcp_server_subnet *next;

    char               *subnet;
    int                 prefix_len;
    te_dhcp_option     *options;
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

#endif /* !__TE_TA_LINUX_DHCP_SERVER_H__ */
