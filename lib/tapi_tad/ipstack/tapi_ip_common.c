/** @file
 * @brief Test API for TAD. Common functions for IP CSAP.
 *
 * Implementation of common functions for IPv4/IPv6 CSAP.
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 */

#include "te_config.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "logger_api.h"
#include "tapi_ip4.h"
#include "tapi_ip6.h"
#include "tapi_ip_common.h"

/* See description in tapi_ip_common.h */
te_errno
tapi_ip_eth_csap_create(const char *ta_name, int sid,
                        const char *eth_dev, unsigned int receive_mode,
                        const uint8_t *loc_mac_addr,
                        const uint8_t *rem_mac_addr,
                        int af,
                        const void *loc_ip_addr,
                        const void *rem_ip_addr,
                        int ip_proto,
                        csap_handle_t *ip_csap)
{
    switch (af)
    {
        case AF_INET:
        {
            in_addr_t loc_ip4_addr = htonl(INADDR_ANY);
            in_addr_t rem_ip4_addr = htonl(INADDR_ANY);

            if (loc_ip_addr != NULL)
                loc_ip4_addr = *(in_addr_t *)loc_ip_addr;
            if (rem_ip_addr != NULL)
                rem_ip4_addr = *(in_addr_t *)rem_ip_addr;

            return tapi_ip4_eth_csap_create(
                        ta_name, sid, eth_dev, receive_mode,
                        loc_mac_addr, rem_mac_addr,
                        loc_ip4_addr, rem_ip4_addr,
                        ip_proto, ip_csap);
        }

        case AF_INET6:
            return tapi_ip6_eth_csap_create(
                        ta_name, sid, eth_dev, receive_mode,
                        loc_mac_addr, rem_mac_addr,
                        (const uint8_t *)loc_ip_addr,
                        (const uint8_t *)rem_ip_addr,
                        ip_proto, ip_csap);
    }

    ERROR("%s(): address family %d is not supported",
          __FUNCTION__, af);
    return TE_RC(TE_TAPI, TE_EINVAL);
}
