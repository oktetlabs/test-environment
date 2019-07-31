/** @file
 * @brief Test API for TAD. Common functions for IP CSAP.
 *
 * @defgroup tapi_tad_ip_common Common functions for IPv4 and IPv6
 * @ingroup tapi_tad_ipstack
 * @{
 *
 * Declaration of common functions for IPv4/IPv6 CSAP.
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 */

#ifndef __TE_TAPI_IP_COMMON_H__
#define __TE_TAPI_IP_COMMON_H__

#include "te_stdint.h"
#include "te_defs.h"
#include "tad_common.h"
#include "tapi_tad.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create CSAP for IPv4 or IPv6 traffic.
 *
 * @param ta_name       Test Agent name.
 * @param sid           RCF SID.
 * @param eth_dev       Name of Ethernet interface.
 * @param receive_mode  Receive mode for Ethernet Layer on the interface.
 * @param loc_mac_addr  Local MAC address (or @c NULL).
 * @param rem_mac_addr  Remote MAC address (or @c NULL).
 * @param af            Address family (@c AF_INET or @c AF_INET6).
 * @param loc_ip_addr   Pointer to local IPv4 or IPv6 address (or @c NULL).
 * @param rem_ip_addr   Pointer to remote IPv4 or IPv6 address (or @c NULL).
 * @param proto         IP protocol or negative number.
 * @param ip_csap       Where to save pointer to created CSAP.
 *
 * @return Status code.
 */
extern te_errno tapi_ip_eth_csap_create(
                        const char *ta_name, int sid,
                        const char *eth_dev, unsigned int receive_mode,
                        const uint8_t *loc_mac_addr,
                        const uint8_t *rem_mac_addr,
                        int af,
                        const void *loc_ip_addr,
                        const void *rem_ip_addr,
                        int ip_proto,
                        csap_handle_t *ip_csap);

/**
 * Create CSAP for TCP or UDP IPv4 or IPv6 traffic.
 *
 * @param ta_name       Test Agent name.
 * @param sid           RCF SID.
 * @param eth_dev       Name of Ethernet interface.
 * @param receive_mode  Receive mode for Ethernet Layer on the interface.
 * @param loc_mac_addr  Local MAC address (or @c NULL).
 * @param rem_mac_addr  Remote MAC address (or @c NULL).
 * @param af            Address family (@c AF_INET or @c AF_INET6).
 * @param ip_proto      @c IPPROTO_TCP or @c IPPROTO_UDP.
 * @param loc_ip_addr   Pointer to local IPv4 or IPv6 address (or @c NULL).
 * @param rem_ip_addr   Pointer to remote IPv4 or IPv6 address (or @c NULL).
 * @param loc_port      Local port number in network byte order or @c -1.
 * @param rem_port      Remote port number in network byte order or @c -1.
 * @param csap          Where to save pointer to created CSAP.
 *
 * @return Status code.
 */
extern te_errno tapi_tcp_udp_ip_eth_csap_create(
                        const char *ta_name, int sid,
                        const char *eth_dev, unsigned int receive_mode,
                        const uint8_t *loc_mac_addr,
                        const uint8_t *rem_mac_addr,
                        int af,
                        int ip_proto,
                        const void *loc_ip_addr,
                        const void *rem_ip_addr,
                        int loc_port,
                        int rem_port,
                        csap_handle_t *csap);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_TAPI_IP_COMMON_H__ */

/**@} <!-- END tapi_tad_ip_common --> */
