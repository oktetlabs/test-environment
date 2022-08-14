/** @file
 * @brief TAPI TAD ICMP6
 *
 * @defgroup tapi_tad_icmp6 ICMP6
 * @ingroup tapi_tad_ipstack
 * @{
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAPI_ICMP6_H__
#define __TE_TAPI_ICMP6_H__


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
#include "tapi_ip6.h"
#include "tapi_eth.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ICMPv6 message type enumeration
 */
typedef enum {
    ICMP6_DEST_UNREACH = 1,
    ICMP6_PACKET_TOO_BIG,
    ICMP6_TIME_EXCEEDED,
    ICMP6_PARAM_PROB,
    ICMP6_ECHO_REQUEST = 128,
    ICMP6_ECHO_REPLY,
    ICMP6_MLD_QUERY,
    ICMP6_MLD_REPORT,
    ICMP6_MLD_DONE,
    ICMP6_ROUTER_SOL,
    ICMP6_ROUTER_ADV,
    ICMP6_NEIGHBOR_SOL,
    ICMP6_NEIGHBOR_ADV,
} icmp6_msg_type;

/**
 * ICMPv6 option type enumeration
 */
typedef enum {
    ICMP6_OPT_SOURCE_LL_ADDR = 1,
    ICMP6_OPT_TARGET_LL_ADDR = 2,
    ICMP6_OPT_PREFIX_INFO = 3,
} icmp6_opt_type;

/**
 * ICMPv6 router advertisement message body
 */
struct radv_body {
    uint8_t     cur_hop_limit;
    uint8_t     flags;
    uint16_t    lifetime;
    uint32_t    reachable_time;
    uint32_t    retrans_timer;
};

/**
 * ICMPv6 neighbor solicitation message body
 */
struct nsol_body {
    uint32_t    nsol_reserved;
    uint8_t     tgt_addr[16];
};

/**
 * ICMPv6 neighbor advertisement message body
 */
struct nadv_body {
    uint32_t    flags;
    uint8_t     tgt_addr[16];
};

/**
 * ICMPv6 echo request/reply message body
 */
struct echo_body {
    uint16_t    id;
    uint16_t    seq;
};

/**
 * Structure to keep ICMPv6 message body information
 */
typedef struct icmp6_msg_body {
    icmp6_msg_type  msg_type;
    union {
        uint32_t            dunr_unused;
        uint32_t            packet_too_big_mtu;
        uint32_t            time_exceeded_unused;
        uint32_t            param_prob_ptr;
        uint32_t            rsol_reserved;
        struct radv_body    radv;
        struct nsol_body    nsol;
        struct nadv_body    nadv;
        struct echo_body    echo;
    } msg_body;
} icmp6_msg_body;

/**
 * ICMPv6 prefix info option body
 */
struct prefix_info {
    uint8_t     prefix_length;
    uint8_t     flags;
    uint32_t    valid_lifetime;
    uint32_t    preferred_lifetime;
    uint8_t     prefix[16];
};

/**
 * Structure to keep list of ICMPv6 options
 */
typedef struct icmp6_msg_option {
    icmp6_opt_type  opt_type;
    union {
        uint8_t                 source_ll_addr[ETHER_ADDR_LEN];
        struct prefix_info      prefix_info;
    } opt_body;
    struct icmp6_msg_option *next;
} icmp6_msg_option;

/**
 * Create 'icmp6.ip6.eth' CSAP on the specified Agent
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param eth_dev       Name of Ethernet interface
 * @param receive_mode  Bitmask with receive mode, see 'enum
 *                      tad_eth_recv_mode' in tad_common.h.
 *                      Use TAD_ETH_RECV_DEF by default.
 * @param loc_hwaddr    Local MAC address (or NULL)
 * @param rem_hwaddr    Remote MAC address (or NULL)
 * @param loc_addr      Local IP address in network byte order (or NULL)
 * @param rem_addr      Remote IP address in network byte order (or NULL)
 * @param icmp_csap     Location for the CSAP handle (OUT)
 *
 * @return Zero on success or error code
 */
extern te_errno tapi_icmp_ip6_eth_csap_create(const char    *ta_name,
                                              int            sid,
                                              const char    *eth_dev,
                                              unsigned int   receive_mode,
                                              const uint8_t *loc_hwaddr,
                                              const uint8_t *rem_hwaddr,
                                              const uint8_t *loc_addr,
                                              const uint8_t *rem_addr,
                                              csap_handle_t *icmp_csap);

/**
 * Add ICMPv6 PDU as the last PDU to the last unit of the traffic
 * template or pattern.
 *
 * @param tmpl_or_ptrn  Location of ASN.1 value with traffic template or
 *                      pattern
 * @param pdu           Location for ASN.1 value pointer with added PDU
 * @param is_pattern    Is the first argument template or pattern
 * @param type          Type of ICMPv6 message or negative to keep
 *                      unspecified
 * @param code          ICMPv6 message code or negative to keep unspecified
 * @param body          ICMPv6 message body or NULL to keep unspecified
 * @param optlist       List of ICPMv6 options or NULL to keep unspecified
 *
 * @return Status code.
 */
extern te_errno tapi_icmp6_add_pdu(asn_value **tmpl_or_ptrn,
                                   asn_value **pdu,
                                   te_bool is_pattern,
                                   int type, int code,
                                   icmp6_msg_body *body,
                                   icmp6_msg_option *optlist);

/**
 * Add ICMPv6 layer in CSAP specification.
 *
 * @param csap_spec     Location of CSAP specification pointer.
 *
 * @retval Status code.
 */
extern te_errno tapi_icmp6_add_csap_layer(asn_value **csap_spec);

/**
 * Create '{udp,tcp}.ip6.icmp.ip6.eth' CSAP on the specified Agent
 *
 * @param ta_name           Test Agent name
 * @param sid               RCF SID
 * @param eth_dev           Name of Ethernet interface
 * @param receive_mode      Bitmask with receive mode, see 'enum
 *                          tad_eth_recv_mode' in tad_common.h.
 *                          Use TAD_ETH_RECV_DEF by default.
 * @param loc_eth           Local MAC address (or NULL)
 * @param rem_eth           Remote MAC address (or NULL)
 * @param loc_addr          Local IPv6 address (or NULL)
 * @param rem_addr          Remote IPv6 address (or NULL)
 * @param msg_loc_saddr     Local IPv6 address/port of ICMP Error message
 *                          (or NULL)
 * @param msg_rem_saddr     Remote IPv6 address/port of ICMP Error message
 *                          (or NULL)
 * @param ip_proto          @c IPPROTO_UDP or @c IPPROTO_TCP
 * @param ip_proto_csap     Location for the CSAP handle (OUT)
 *
 * @return Status code
 */
extern te_errno tapi_ipproto_ip6_icmp_ip6_eth_csap_create(
                            const char                 *ta_name,
                            int                         sid,
                            const char                 *eth_dev,
                            unsigned int                receive_mode,
                            const uint8_t              *loc_eth,
                            const uint8_t              *rem_eth,
                            const uint8_t              *loc_addr,
                            const uint8_t              *rem_addr,
                            const struct sockaddr_in6  *msg_loc_saddr,
                            const struct sockaddr_in6  *msg_rem_saddr,
                            int                         ip_proto,
                            csap_handle_t              *ip_proto_csap);

/**
 * Create 'udp.ip6.icmp.ip6.eth' CSAP on the specified Agent
 *
 * @param ta_name           Test Agent name
 * @param sid               RCF SID
 * @param eth_dev           Name of Ethernet interface
 * @param receive_mode      Bitmask with receive mode, see 'enum
 *                          tad_eth_recv_mode' in tad_common.h.
 *                          Use TAD_ETH_RECV_DEF by default.
 * @param loc_eth           Local MAC address (or NULL)
 * @param rem_eth           Remote MAC address (or NULL)
 * @param loc_addr          Local IPv6 address (or NULL)
 * @param rem_addr          Remote IPv6 address (or NULL)
 * @param msg_loc_saddr     Local IPv6 address/port of ICMP Error message
 *                          (or NULL)
 * @param msg_rem_saddr     Remote IPv6 address/port of ICMP Error message
 *                          (or NULL)
 * @param udp_csap          Location for the CSAP handle (OUT)
 *
 * @return Status code
 */
extern te_errno tapi_udp_ip6_icmp_ip6_eth_csap_create(
                            const char                 *ta_name,
                            int                         sid,
                            const char                 *eth_dev,
                            unsigned int                receive_mode,
                            const uint8_t              *loc_eth,
                            const uint8_t              *rem_eth,
                            const uint8_t              *loc_addr,
                            const uint8_t              *rem_addr,
                            const struct sockaddr_in6  *msg_loc_saddr,
                            const struct sockaddr_in6  *msg_rem_saddr,
                            csap_handle_t              *udp_csap);

/**
 * Create 'tcp.ip6.icmp.ip6.eth' CSAP on the specified Agent
 *
 * @param ta_name           Test Agent name
 * @param sid               RCF SID
 * @param eth_dev           Name of Ethernet interface
 * @param receive_mode      Bitmask with receive mode, see 'enum
 *                          tad_eth_recv_mode' in tad_common.h.
 *                          Use TAD_ETH_RECV_DEF by default.
 * @param loc_eth           Local MAC address (or NULL)
 * @param rem_eth           Remote MAC address (or NULL)
 * @param loc_addr          Local IPv6 address (or NULL)
 * @param rem_addr          Remote IPv6 address (or NULL)
 * @param msg_loc_saddr     Local IPv6 address/port of ICMP Error message
 *                          (or NULL)
 * @param msg_rem_saddr     Remote IPv6 address/port of ICMP Error message
 *                          (or NULL)
 * @param tcp_csap          Location for the CSAP handle (OUT)
 *
 * @return Status code
 */
extern te_errno tapi_tcp_ip6_icmp_ip6_eth_csap_create(
                            const char                 *ta_name,
                            int                         sid,
                            const char                 *eth_dev,
                            unsigned int                receive_mode,
                            const uint8_t              *loc_eth,
                            const uint8_t              *rem_eth,
                            const uint8_t              *loc_addr,
                            const uint8_t              *rem_addr,
                            const struct sockaddr_in6  *msg_loc_saddr,
                            const struct sockaddr_in6  *msg_rem_saddr,
                            csap_handle_t              *tcp_csap);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_ICMP6_H__ */

/**@} <!-- END tapi_tad_icmp6 --> */
