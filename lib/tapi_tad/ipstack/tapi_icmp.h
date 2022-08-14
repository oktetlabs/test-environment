/** @file
 * @brief TAPI TAD ICMP
 *
 * @defgroup tapi_tad_icmp ICMP
 * @ingroup tapi_tad_ipstack
 * @{
 *
 * Copyright (C) 2019-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAPI_ICMP_H__
#define __TE_TAPI_ICMP_H__


#include "tapi_icmp4.h"
#include "tapi_icmp6.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create 'udp.ip.icmp.ip.eth' CSAP on the specified Agent,
 * IPv4 or IPv6 version depends on the @p af.
 *
 * @param ta_name           Test Agent name
 * @param sid               RCF SID
 * @param eth_dev           Name of Ethernet interface
 * @param receive_mode      Bitmask with receive mode, see 'enum
 *                          tad_eth_recv_mode' in tad_common.h.
 *                          Use TAD_ETH_RECV_DEF by default.
 * @param loc_eth           Local MAC address (or NULL)
 * @param rem_eth           Remote MAC address (or NULL)
 * @param loc_saddr         Local IP address (or NULL)
 * @param rem_saddr         Remote IP address (or NULL)
 * @param msg_loc_saddr     Local IP address/port of ICMP Error message
 *                          (or NULL)
 * @param msg_rem_saddr     Remote IP address/port of ICMP Error message
 *                          (or NULL)
 * @param af                Address family
 * @param udp_csap          Location for the CSAP handle (OUT)
 *
 * @return Status code
 */
extern te_errno tapi_udp_ip_icmp_ip_eth_csap_create(
                                const char            *ta_name,
                                int                    sid,
                                const char            *eth_dev,
                                unsigned int           receive_mode,
                                const uint8_t         *loc_eth,
                                const uint8_t         *rem_eth,
                                const struct sockaddr *loc_saddr,
                                const struct sockaddr *rem_saddr,
                                const struct sockaddr *msg_loc_saddr,
                                const struct sockaddr *msg_rem_saddr,
                                int                    af,
                                csap_handle_t         *udp_csap);

/**
 * Create 'tcp.ip.icmp.ip.eth' CSAP on the specified Agent,
 * IPv4 or IPv6 version depends on the @p af.
 *
 * @param ta_name           Test Agent name
 * @param sid               RCF SID
 * @param eth_dev           Name of Ethernet interface
 * @param receive_mode      Bitmask with receive mode, see 'enum
 *                          tad_eth_recv_mode' in tad_common.h.
 *                          Use TAD_ETH_RECV_DEF by default.
 * @param loc_eth           Local MAC address (or NULL)
 * @param rem_eth           Remote MAC address (or NULL)
 * @param loc_saddr         Local IP address (or NULL)
 * @param rem_saddr         Remote IP address (or NULL)
 * @param msg_loc_saddr     Local IP address/port of ICMP Error message
 *                          (or NULL)
 * @param msg_rem_saddr     Remote IP address/port of ICMP Error message
 *                          (or NULL)
 * @param af                Address family
 * @param tcp_csap          Location for the CSAP handle (OUT)
 *
 * @return Status code
 */
extern te_errno tapi_tcp_ip_icmp_ip_eth_csap_create(
                                const char            *ta_name,
                                int                    sid,
                                const char            *eth_dev,
                                unsigned int           receive_mode,
                                const uint8_t         *loc_eth,
                                const uint8_t         *rem_eth,
                                const struct sockaddr *loc_saddr,
                                const struct sockaddr *rem_saddr,
                                const struct sockaddr *msg_loc_saddr,
                                const struct sockaddr *msg_rem_saddr,
                                int                    af,
                                csap_handle_t         *tcp_csap);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_ICMP_H__ */

/**@} <!-- END tapi_tad_icmp --> */
