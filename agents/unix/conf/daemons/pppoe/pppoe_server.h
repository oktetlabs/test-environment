/** @file
 * @brief Unix Test Agent: PPPoE server
 *
 * Definition of PPPoE server configuration notions.
 *
 */

#ifndef __TE_TA_UNIX_PPPOE_SERVER_H__
#define __TE_TA_UNIX_PPPOE_SERVER_H__

#include "te_defs.h"
#include "te_queue.h"

/** Definitions of types for PPPoE configuring */
typedef struct te_pppoe_option {
    struct te_pppoe_option *next;

    char               *name;
    char               *value;
} te_pppoe_option;

typedef struct te_pppoe_server_subnet {
    TAILQ_ENTRY(te_pppoe_server_subnet)  links;

    char               *subnet;
    int                 prefix_len;
    te_pppoe_option     *options;
} te_pppoe_server_subnet;

#endif /* !__TE_TA_UNIX_DHCP_SERVER_H__ */
