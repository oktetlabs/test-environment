/* SPDX-License-Identifier: Apache-2.0 */
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

/**
 * Encapsulate packet template in ICMP header.
 *
 * @param[in]  tmpl         Packet template to encapsulate.
 * @param[in]  src_eth      Source MAC address for ETH header.
 * @param[in]  dst_eth      Destination address for ETH header.
 * @param[in]  src_addr     Source IP address for IP header.
 * @param[in]  dst_addr     Destination IP address for IP header.
 * @param[in]  ttl_hoplimit TTL or Hop Limit for IP header.
 * @param[in]  ip4_tos      TOS field for IP header. For IPv4 only.
 *                          (may be @c -1 for the default value).
 * @param[in]  af           Address family.
 * @param[in]  icmp_type    ICMP type.
 * @param[in]  icmp_code    ICMP code.
 * @param[in]  msg_body     ICMP message body. For IPv6 only.
 *                          (may be @c NULL for the default value).
 * @param[out] icmp_tmpl    Resulting packet template.
 *
 * @return Status code
 */
extern te_errno tapi_icmp_tmpl_encap_ext(
                    const asn_value *tmpl,
                    const void *src_eth, const void *dst_eth,
                    const struct sockaddr *src_addr,
                    const struct sockaddr *dst_addr,
                    int ttl_hoplimit, int ip4_tos, int af,
                    uint8_t icmp_type, uint8_t icmp_code,
                    icmp6_msg_body *msg_body,
                    asn_value **icmp_tmpl);

/**
 * Wrapper for @b tapi_icmp_tmpl_encap_ext with default options.
 *
 * @param[in]  tmpl         Packet template to encapsulate.
 * @param[in]  src_eth      Source MAC address for ETH header.
 * @param[in]  dst_eth      Destination address for ETH header.
 * @param[in]  src_addr     Source IP address for IP header.
 * @param[in]  dst_addr     Destination IP address for IP header.
 * @param[in]  af           Address family.
 * @param[in]  icmp_type    ICMP type.
 * @param[in]  icmp_code    ICMP code.
 * @param[out] icmp_tmpl    Resulting packet template.
 *
 * @return Status code
 */
static inline te_errno
tapi_icmp_tmpl_encap(const asn_value *tmpl,
                      const void *src_eth, const void *dst_eth,
                      const struct sockaddr *src_addr,
                      const struct sockaddr *dst_addr,
                      int af, uint8_t icmp_type, uint8_t icmp_code,
                      asn_value **icmp_tmpl)
{
    return tapi_icmp_tmpl_encap_ext(tmpl, src_eth, dst_eth, src_addr,
                                     dst_addr, -1, -1, af, icmp_type,
                                     icmp_code, NULL, icmp_tmpl);
}

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_ICMP_H__ */

/**@} <!-- END tapi_tad_icmp --> */
