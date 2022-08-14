/** @file
 * @brief TAPI TAD ICMP4
 *
 * @defgroup tapi_tad_icmp4 ICMP4
 * @ingroup tapi_tad_ipstack
 * @{
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAPI_ICMP4_H__
#define __TE_TAPI_ICMP4_H__


#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "te_stdint.h"
#include "te_defs.h"
#include "tad_common.h"
#include "asn_usr.h"
#include "tapi_tad.h"
#include "tapi_udp.h"
#include "tapi_tcp.h"
#include "tapi_ip4.h"
#include "tapi_eth.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Add ICMPv4 layer in CSAP specification.
 *
 * @param csap_spec     Location of CSAP specification pointer.
 *
 * @retval Status code.
 */
extern te_errno tapi_icmp4_add_csap_layer(asn_value **csap_spec);

/**
 * Add ICMPv4 PDU as the last PDU to the last unit of the traffic
 * template or pattern.
 *
 * @param tmpl_or_ptrn  Location of ASN.1 value with traffic template or
 *                      pattern
 * @param pdu           Location for ASN.1 value pointer with added PDU
 * @param is_pattern    Is the first argument template or pattern
 * @param type          Type of ICMP message or negative to keep
 *                      unspecified.
 * @param code          ICMP message code or negative to keep unspecified.
 *
 * @return Status code.
 */
extern te_errno tapi_icmp4_add_pdu(asn_value **tmpl_or_ptrn,
                                   asn_value **pdu,
                                   te_bool     is_pattern,
                                   int         type,
                                   int         code);

/**
 * Create '{udp,tcp}.ip4.icmp.ip4.eth' CSAP on the specified Agent
 *
 * @param ta_name           Test Agent name
 * @param sid               RCF SID
 * @param eth_dev           Name of Ethernet interface
 * @param receive_mode      Bitmask with receive mode, see 'enum
 *                          tad_eth_recv_mode' in tad_common.h.
 *                          Use TAD_ETH_RECV_DEF by default.
 * @param src_mac           Local MAC address (or NULL)
 * @param dst_mac           Remote MAC address (or NULL)
 * @param src_addr          Local IP address in network byte order (or NULL)
 * @param dst_addr          Remote IP address in network byte order (or NULL)
 * @param msg_src_addr      Source IPv4 Address of ICMP Error message
 * @param msg_dst_addr      Destination IPv4 Address of ICMP Error message
 * @param msg_src_port      Source UDP port in network byte order or -1
 * @param msg_dst_port      Destination UDP port in network byte order or -1
 * @param ip_proto          @c IPPROTO_UDP or @c IPPROTO_TCP
 * @param ip_proto_csap     Location for the CSAP handle (OUT)
 *
 * @return Statuc code
 */
extern te_errno tapi_ipproto_ip4_icmp_ip4_eth_csap_create(
                    const char    *ta_name,
                    int            sid,
                    const char    *eth_dev,
                    unsigned int   receive_mode,
                    const uint8_t *eth_src,
                    const uint8_t *eth_dst,
                    in_addr_t      src_addr,
                    in_addr_t      dst_addr,
                    in_addr_t      msg_src_addr,
                    in_addr_t      msg_dst_addr,
                    int            msg_src_port,
                    int            msg_dst_port,
                    int            ip_proto,
                    csap_handle_t *ip_proto_csap);


/**
 * Create 'tcp.ip4.icmp.ip4.eth' CSAP on the specified Agent
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param eth_dev       Name of Ethernet interface
 * @param receive_mode  Bitmask with receive mode, see 'enum
 *                      tad_eth_recv_mode' in tad_common.h.
 *                      Use TAD_ETH_RECV_DEF by default.
 * @param src_mac       Local MAC address (or NULL)
 * @param dst_mac       Remote MAC address (or NULL)
 * @param src_addr      Local IP address in network byte order (or NULL)
 * @param dst_addr      Remote IP address in network byte order (or NULL)
 * @param msg_src_addr  Source IPv4 Address of ICMP Error message
 * @param msg_dst_addr  Destination IPv4 Address of ICMP Error message
 * @param msg_src_port  Source UDP port in network byte order or -1
 * @param msg_dst_port  Destination UDP port in network byte order or -1
 * @param tcp_csap      Location for the CSAP handle (OUT)
 *
 * @return Zero on success or error code
 */
extern te_errno tapi_tcp_ip4_icmp_ip4_eth_csap_create(
                    const char    *ta_name,
                    int            sid,
                    const char    *eth_dev,
                    unsigned int   receive_mode,
                    const uint8_t *eth_src,
                    const uint8_t *eth_dst,
                    in_addr_t      src_addr,
                    in_addr_t      dst_addr,
                    in_addr_t      msg_src_addr,
                    in_addr_t      msg_dst_addr,
                    int            msg_src_port,
                    int            msg_dst_port,
                    csap_handle_t *tcp_csap);


/**
 * Create 'udp.ip4.icmp.ip4.eth' CSAP on the specified Agent
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param eth_dev       Name of Ethernet interface
 * @param receive_mode  Bitmask with receive mode, see 'enum
 *                      tad_eth_recv_mode' in tad_common.h.
 *                      Use TAD_ETH_RECV_DEF by default.
 * @param src_mac       Local MAC address (or NULL)
 * @param dst_mac       Remote MAC address (or NULL)
 * @param src_addr      Local IP address in network byte order (or NULL)
 * @param dst_addr      Remote IP address in network byte order (or NULL)
 * @param msg_src_addr  Source IPv4 Address of ICMP Error message
 * @param msg_dst_addr  Destination IPv4 Address of ICMP Error message
 * @param msg_src_port  Source UDP port in network byte order or -1
 * @param msg_dst_port  Destination UDP port in network byte order or -1
 * @param udp_csap      Location for the CSAP handle (OUT)
 *
 * @return Zero on success or error code
 */
extern te_errno tapi_udp_ip4_icmp_ip4_eth_csap_create(
                    const char    *ta_name,
                    int            sid,
                    const char    *eth_dev,
                    unsigned int   receive_mode,
                    const uint8_t *eth_src,
                    const uint8_t *eth_dst,
                    in_addr_t      src_addr,
                    in_addr_t      dst_addr,
                    in_addr_t      msg_src_addr,
                    in_addr_t      msg_dst_addr,
                    int            msg_src_port,
                    int            msg_dst_port,
                    csap_handle_t *udp_csap);

/**
 * Create 'icmp.ip4.eth' CSAP on the specified Agent
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param eth_dev       Name of Ethernet interface
 * @param receive_mode  Bitmask with receive mode, see 'enum
 *                      tad_eth_recv_mode' in tad_common.h.
 *                      Use TAD_ETH_RECV_DEF by default.
 * @param eth_src       Local MAC address (or NULL)
 * @param eth_dst       Remote MAC address (or NULL)
 * @param src_addr      Local IP address in network byte order (or NULL)
 * @param dst_addr      Remote IP address in network byte order (or NULL)
 * @param icmp_csap     Location for the CSAP handle (OUT)
 *
 * @return Zero on success or error code
 */
extern te_errno tapi_icmp_ip4_eth_csap_create(
                    const char    *ta_name,
                    int            sid,
                    const char    *eth_dev,
                    unsigned int   receive_mode,
                    const uint8_t *eth_src,
                    const uint8_t *eth_dst,
                    in_addr_t      src_addr,
                    in_addr_t      dst_addr,
                    csap_handle_t *icmp_csap);


/**
 * Create 'icmp.ip4' CSAP on the specified Agent
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param ifname        Name of interface
 * @param src_addr      Local IP address in network byte order (or NULL)
 * @param dst_addr      Remote IP address in network byte order (or NULL)
 * @param icmp_csap     Location for the CSAP handle (OUT)
 *
 * @return Zero on success or error code
 */
te_errno
tapi_icmp_ip4_csap_create(const char    *ta_name,
                          int            sid,
                          const char    *ifname,
                          in_addr_t      src_addr,
                          in_addr_t      dst_addr,
                          csap_handle_t *icmp_csap);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_ICMP4_H__ */

/**@} <!-- END tapi_tad_icmp4 --> */
