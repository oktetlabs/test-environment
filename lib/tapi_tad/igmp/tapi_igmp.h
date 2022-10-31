/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief TAPI TAD IGMP
 *
 * @defgroup tapi_tad_igmp IGMP
 * @ingroup tapi_tad_main
 * @{
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAPI_IGMP2_H__
#define __TE_TAPI_IGMP2_H__


#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#if 1 /* HAVE_LINUX_IGMP_H */
#include <linux/igmp.h>
#endif

#include "te_stdint.h"
#include "te_defs.h"
#include "tad_common.h"
#include "asn_usr.h"
#include "tapi_tad.h"
#include "tapi_ip4.h"
#include "tapi_eth.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * IGMP protocol versions definition
 */
typedef enum {
    TAPI_IGMP_INVALID = -1,   /**< Uninitialised value */
    TAPI_IGMP_VERSION_0 = 0,  /**< IGMP version 0, see RFC 988 */
    TAPI_IGMP_VERSION_1 = 1,  /**< IGMP version 1, see RFC 1112 */
    TAPI_IGMP_VERSION_2 = 2,  /**< IGMP version 2, see RFC 2236 */
    TAPI_IGMP_VERSION_3 = 3,  /**< IGMP version 3, see RFC 3376 */
} tapi_igmp_version;


/**
 * IGMP message types definition
 */
/** General/Group Query message*/
#define TAPI_IGMP_TYPE_QUERY IGMP_HOST_MEMBERSHIP_QUERY

/** IGMPv1 Membership report*/
#define TAPI_IGMP1_TYPE_REPORT IGMP_HOST_MEMBERSHIP_REPORT

/** IGMPv2 Membership report */
#define TAPI_IGMP2_TYPE_REPORT IGMPV2_HOST_MEMBERSHIP_REPORT

/**< Group Leave message */
#define TAPI_IGMP2_TYPE_LEAVE IGMP_HOST_LEAVE_MESSAGE

/** IGMPv3 Membership report */
#define TAPI_IGMP3_TYPE_REPORT IGMPV3_HOST_MEMBERSHIP_REPORT

typedef int tapi_igmp_msg_type;


typedef enum {
    TAPI_IGMP_QUERY_TYPE_UNUSED,
    TAPI_IGMP_QUERY_TYPE_GENERAL,
    TAPI_IGMP_QUERY_TYPE_GROUP,
} tapi_igmp_query_type;

/** IPv4 Multicast Address of All-Hosts group: 224.0.0.1 */
#define TAPI_MCAST_ADDR_ALL_HOSTS    IGMP_ALL_HOSTS

/** IPv4 Multicast Address of All-Routers group: 224.0.0.2 */
#define TAPI_MCAST_ADDR_ALL_ROUTERS  IGMP_ALL_ROUTER

/** IPv4 Multicast Address of All-Multicast-Routers IGMPv3 group: 224.0.0.22 */
#define TAPI_MCAST_ADDR_ALL_MCR      IGMPV3_ALL_MCR

/** Default TTL for IGMP messages is 1 */
#define TAPI_IGMP_IP4_TTL_DEFAULT 1

/** Default ToS for IGMPv2 messages is not stricted to any value */
#define TAPI_IGMP_IP4_TOS_DEFAULT    0xc0

/** Default ToS for IGMPv3 messages is 0xc0 */
#define TAPI_IGMP3_IP4_TOS_DEFAULT   0xc0

#define TAPI_IGMP3_GROUP_RECORD_HDR_LEN 8

/** Maximum number of Source Addresses in list (see RFC 3376) */
#define TAPI_IGMP3_SRC_LIST_SIZE_MAX    65535

/** Maximum number of Group Records in list (see RFC 3376) */
#define TAPI_IGMP3_GROUP_LIST_SIZE_MAX  65535

/**
 * Max Response Time default value in seconds, can be converted to
 * Max Resp Code (see RFC 3376/RFC 2236, 8.3)
 */
#define TAPI_IGMP_QUERY_MAX_RESP_TIME_DEFAULT_S 10

/**
 * Max Response Time maximum value in seconds for IGMPv3 Query message
 * (see RFC 3376, 4.1.1)
 */
#define TAPI_IGMP3_QUERY_MAX_RESP_TIME_MAX_S    3174

/** Suppress Router-Side Processing flag (see RFC 3376, 4.1.5) */
#define TAPI_IGMP3_QUERY_S_DEFAULT 0

/** Querier's Robustness Variable (see RFC 3376, 4.1.6) */
#define TAPI_IGMP3_QUERY_QRV_DEFAULT 2

/**
 * Querier's Query Interval (QQI) default value in seconds, can be converted to
 * Querier's Query Interval Code (QQIС)(see RFC 3376, 4.1.7)
 */
#define TAPI_IGMP3_QUERY_QQI_DEFAULT_S 125

/** Querier's Query Interval (QQI) maximum value (see RFC 3376, 4.1.7) */
#define TAPI_IGMP3_QUERY_QQI_MAX_S     31744


/**
 * IGMPv3 Source Address List (simple array) storage
 */
typedef struct tapi_igmp3_src_list_s {
    in_addr_t  *src_addr;    /**< Array of source addresses */
    int         src_no;      /**< Number of sources */
    int         src_no_max;  /**< Maximum number of sources pre-allocated */
} tapi_igmp3_src_list_t;

/**
 * IGMPv3 Group Record structure
 */
typedef struct tapi_igmp3_group_record_s {
    int                     record_type;   /**< Record type of Group Record */
    in_addr_t               group_address; /**< Multicast Address which this
                                                Group Record relays to */
    int                     aux_data_len;  /**< Length of auxiliary data in
                                                32-bit words */
    void                   *aux_data;      /**< Pointer to auxiliaty data
                                                buffer */
    tapi_igmp3_src_list_t   src_list;      /**< Source Address List storage */
} tapi_igmp3_group_record_t;

/**
 * IGMPv3 Group Record List storage
 */
typedef struct tapi_igmp3_group_list_s {
    tapi_igmp3_group_record_t **groups;        /**< Array of Group Records */
    int                         groups_no;     /**< Number of Group Records */
    int                         groups_no_max; /**< Size of pre-allocated
                                                    Group Records array */
} tapi_igmp3_group_list_t;

/**
 * Calculate MAC address corresponding to a given IPv4 multicast
 * address.
 *
 * @param ip4_addr        IPv4 multicast address.
 * @param eth_addr        Where to save corresponding MAC address.
 */
extern void tapi_ip4_to_mcast_mac(in_addr_t ip4_addr, uint8_t *eth_addr);

/**
 * Add IGMP layer in CSAP specification.
 *
 * @param csap_spec     Location of CSAP specification pointer.
 *
 * @retval Status code.
 */
extern te_errno tapi_igmp_add_csap_layer(asn_value **csap_spec);

/**
 * Add IGMPv2 PDU as the last PDU to the last unit of the traffic
 * template or pattern.
 *
 * @param tmpl_or_ptrn  Location of ASN.1 value with traffic template or
 *                      pattern
 * @param pdu           Location for ASN.1 value pointer with added PDU
 * @param is_pattern    Is the first argument template or pattern
 * @param type          Type of IGMPv2 message or negative to keep
 *                      unspecified.
 * @param max_resp_time IGMP message maximum response   time,
 *                      or negative to keep unspecified.
 * @param group_addr    Multicast Group Address field of IGMPv2 message.
 *
 * @return              Status code.
 */
extern te_errno tapi_igmp2_add_pdu(asn_value          **tmpl_or_ptrn,
                                   asn_value          **pdu,
                                   te_bool              is_pattern,
                                   tapi_igmp_msg_type   type,
                                   int                  max_resp_time,
                                   in_addr_t            group_addr);


/**
 * Create 'igmp.ip4.eth' CSAP on the specified Agent
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param ifname        Network interface name
 * @param receive_mode  Bitmask with receive mode, see 'enum
 *                      tad_eth_recv_mode' in tad_common.h.
 *                      Use TAD_ETH_RECV_DEF by default.
 * @param eth_src       Local MAC address (or @c NULL)
 * @param src_addr      Local IP address in network byte order (or @c NULL)
 * @param igmp_csap     Location for the CSAP handle (OUT)
 *
 * @return Zero on success or error code
 */
extern te_errno tapi_igmp_ip4_eth_csap_create(const char    *ta_name,
                                              int            sid,
                                              const char    *ifname,
                                              unsigned int   receive_mode,
                                              const uint8_t *eth_src,
                                              in_addr_t      src_addr,
                                              csap_handle_t *igmp_csap);

/**
 * Add IPv4 layer to PDU, used for ppp connections
 *
 * @param tmpl_or_ptrn  Location of ASN.1 value with traffic template or
 *                      pattern
 * @param pdu           Location for ASN.1 value pointer with added PDU
 * @param is_pattern    Is the first argument template or pattern
 * @param dst_addr      IPv4 layer Destination Multicast address (also used
 *                      for generating Ethernet multicast address)
 * @param src_addr      IPv4 layer Source Address field or @c INADDR_ANY to
 *                      keep unspecified.
 *
 * @return              Status code.
 */
extern te_errno tapi_igmp_add_ip4_pdu(asn_value **tmpl_or_ptrn,
                      asn_value **pdu,
                      te_bool     is_pattern,
                      in_addr_t   dst_addr,
                      in_addr_t   src_addr);

/**
 * Add IPv4 layer to PDU, used for ppp connections
 *
 * @param tmpl_or_ptrn  Location of ASN.1 value with traffic template or
 *                      pattern
 * @param pdu           Location for ASN.1 value pointer with added PDU
 * @param is_pattern    Is the first argument template or pattern
 * @param dst_addr      IPv4 layer Destination Multicast address (also used
 *                      for generating Ethernet multicast address)
 * @param src_addr      IPv4 layer Source Address field or @c INADDR_ANY to
 *                      keep unspecified.
 * @param ttl           IP TTL
 * @param tos           IP ToS
 *
 * @return              Status code.
 */
extern te_errno tapi_igmp_add_ip4_pdu_gen(asn_value **tmpl_or_ptrn,
                        asn_value **pdu,
                        te_bool     is_pattern,
                        in_addr_t   dst_addr,
                        in_addr_t   src_addr,
                        int         ttl,
                        int         tos);

/**
 * Add IPv4.Eth layers to PDU
 *
 * @param tmpl_or_ptrn  Location of ASN.1 value with traffic template or
 *                      pattern
 * @param pdu           Location for ASN.1 value pointer with added PDU
 * @param is_pattern    Is the first argument template or pattern
 * @param dst_addr      IPv4 layer Destination Multicast address (also used
 *                      for generating Ethernet multicast address)
 * @param src_addr      IPv4 layer Source Address field or @c INADDR_ANY to
 *                      keep unspecified.
 * @param eth_src       Ethernet layer Source Address field or @c NULL to keep
 *                      unspecified.
 * @param tos           IP ToS field
 *
 * @note  Destination address Ethernet layers is calculated from
 *        @p dst_address value.
 *
 * @return              Status code.
 */
extern te_errno tapi_igmp_add_ip4_eth_pdu(asn_value **tmpl_or_ptrn,
                          asn_value **pdu,
                          te_bool     is_pattern,
                          in_addr_t   dst_addr,
                          in_addr_t   src_addr,
                          uint8_t    *eth_src);

/**
 * Add IPv4.Eth layers to PDU,
 *
 * @param tmpl_or_ptrn  Location of ASN.1 value with traffic template or
 *                      pattern
 * @param pdu           Location for ASN.1 value pointer with added PDU
 * @param is_pattern    Is the first argument template or pattern
 * @param dst_addr      IPv4 layer Destination Multicast address (also used
 *                      for generating Ethernet multicast address)
 * @param src_addr      IPv4 layer Source Address field or @c INADDR_ANY to
 *                      keep unspecified.
 * @param eth_src       Ethernet layer Source Address field or @c NULL to keep
 *                      unspecified.
 * @param ttl           IP TTL field
 * @param tos           IP ToS field
 *
 * @note  Destination address Ethernet layers is calculated from
 *        @p dst_address value.
 *
 * @return              Status code.
 */
extern te_errno tapi_igmp_add_ip4_eth_pdu_gen(
                            asn_value **tmpl_or_ptrn,
                            asn_value **pdu,
                            te_bool     is_pattern,
                            in_addr_t   dst_addr,
                            in_addr_t   src_addr,
                            uint8_t    *eth_src,
                            int         ttl,
                            int         tos);

/**
 * Send IGMPv1 report message
 *
 * @param ta_name       Test Agent name
 * @param session       RCF SID
 * @param csap          igmp.ip4.eth CSAP handle to send IGMP message through
 * @param group_addr    Multicast Group Address field of IGMPv2 message.
 * @param src_addr      IPv4 layer Source Address field or @c INADDR_ANY to
 *                      keep unspecified.
 * @param eth_src       Ethernet layer Source Address field or @c NULL to keep
 *                      unspecified.
 *
 * @return              Status code.
 */
extern te_errno
tapi_igmp1_ip4_eth_send_report(const char    *ta_name,
                               int            session,
                               csap_handle_t  csap,
                               in_addr_t      group_addr,
                               in_addr_t      src_addr,
                               uint8_t       *eth_src);

/**
 * Send IGMPv2 report message
 *
 * @param ta_name       Test Agent name
 * @param session       RCF SID
 * @param csap          igmp.ip4.eth CSAP handle to send IGMP message through
 * @param group_addr    Multicast Group Address field of IGMPv2 message.
 * @param src_addr      IPv4 layer Source Address field or @c INADDR_ANY to
 *                      keep unspecified.
 * @param eth_src       Ethernet layer Source Address field or @c NULL to keep
 *                      unspecified.
 *
 * @return              Status code.
 */
extern te_errno
tapi_igmp2_ip4_eth_send_report(const char    *ta_name,
                               int            session,
                               csap_handle_t  csap,
                               in_addr_t      group_addr,
                               in_addr_t      src_addr,
                               uint8_t       *eth_src);

/**
 * Send IGMPv2 Group Membership Leave message
 *
 * @param ta_name       Test Agent name
 * @param session       RCF SID
 * @param csap          igmp.ip4.eth CSAP handle to send IGMP message through
 * @param group_addr    Multicast Group Address field of IGMPv2 message.
 * @param src_addr      IPv4 layer Source Address field or @c INADDR_ANY to
 *                      keep unspecified.
 * @param eth_src       Ethernet layer Source Address field or @c NULL to keep
 *                      unspecified.
 *
 * @return              Status code.
 */
extern te_errno
tapi_igmp2_ip4_eth_send_leave(const char    *ta_name,
                              int            session,
                              csap_handle_t  csap,
                              in_addr_t      group_addr,
                              in_addr_t      src_addr,
                              uint8_t       *eth_src);

/**
 * Send IGMPv2 Query message
 *
 * @param ta_name       Test Agent name
 * @param session       RCF SID
 * @param csap          igmp.ip4.eth CSAP handle to send IGMP message through
 * @param max_resp_time IGMP message maximum response time,
 *                      or negative to keep unspecified.
 * @param group_addr    Multicast Group Address field of IGMPv2 message.
 *                      For General Query should be 0.0.0.0 (@c INADDR_ANY)
 * @param src_addr      IPv4 layer Source Address field or @c INADDR_ANY to
 *                      keep unspecified.
 * @param skip_eth      Do not add @p eth_src
 * @param eth_src       Ethernet layer Source Address field or @c NULL to keep
 *                      unspecified.
 *
 * @return              Status code.
 */
extern te_errno
tapi_igmp2_ip4_eth_send_query(const char    *ta_name,
                              int            session,
                              csap_handle_t  csap,
                              int            max_resp_time,
                              in_addr_t      group_addr,
                              in_addr_t      src_addr,
                              te_bool        skip_eth,
                              uint8_t       *eth_src);

/**
 * Add IGMPv3 Report PDU as the last PDU to the last unit of the traffic
 * template or pattern.
 *
 * @param tmpl_or_ptrn  Location of ASN.1 value with traffic template or
 *                      pattern
 * @param pdu           Location for ASN.1 value pointer with added PDU
 * @param is_pattern    Is the first argument template or pattern
 * @param group_list    List of Group Records to be sent in this PDU
 *
 * @return              Status code.
 */
extern te_errno
tapi_igmp3_add_report_pdu(asn_value               **tmpl_or_ptrn,
                          asn_value               **pdu,
                          te_bool                   is_pattern,
                          tapi_igmp3_group_list_t  *group_list);

/**
 * Send IGMPv3 Group Membership report message
 *
 * @param ta_name       Test Agent name
 * @param session       RCF SID
 * @param csap          igmp.ip4.eth CSAP handle to send IGMP message through
 * @param group_list    Group Record List to be sent
 * @param src_addr      IPv4 layer Source Address field or @c INADDR_ANY to
 *                      keep unspecified.
 * @param eth_src       Ethernet layer Source Address field or @c NULL to keep
 *                      unspecified.
 *
 * @return              Status code.
 */
extern te_errno
tapi_igmp3_ip4_eth_send_report(const char      *ta_name,
                               int              session,
                               csap_handle_t    csap,
                               tapi_igmp3_group_list_t  *group_list,
                               in_addr_t        src_addr,
                               uint8_t         *eth_src);

/**
 * Convert Max Response Time to IGMPv3 Max Response Code.
 * See RFC 3376, section 4.1.1.
 *
 * @param max_resp_time  Max Response Time in seconds.
 *
 * @note  Predefined @p max_resp_time values:
 *        @ref TAPI_IGMP_QUERY_MAX_RESP_TIME_DEFAULT_S
 *        @ref TAPI_IGMP3_QUERY_MAX_RESP_TIME_MAX_S
 *
 * @return               Max Response Code suitable for writing to
 *                       IGMPv3 Query message.
 */
extern uint8_t tapi_igmp3_max_response_time_to_code(unsigned max_resp_time);

/**
 * Convert IGMPv3 Max Response Code to Max Response Time.
 * See RFC 3376, section 4.1.1.
 *
 * @param max_resp_code  Max Response Code in exponential form
 *                       from IGMPv3 Query message.
 *
 * @return               Max Response Time in seconds.
 */
extern unsigned tapi_igmp3_max_response_code_to_time(uint8_t max_resp_code);

/**
 * Convert Querier's Query Interval to Querier's Query Interval Code.
 * See RFC 3376, section 4.1.7.
 *
 * @param qqi  Querier's Query Interval (QQI) in seconds.
 *
 * @note  Predefined @p qqi values:
 *        @ref TAPI_IGMP3_QUERY_QQI_DEFAULT_S
 *        @ref TAPI_IGMP3_QUERY_QQI_MAX_S
 *
 * @return     Querier's Query Interval Code (QQIC) suitable for writing to
 *             IGMPv3 Query message.
 */
extern uint8_t tapi_igmp3_qqi_to_qqic(unsigned qqi);

/**
 * Convert Querier's Query Interval Code to Querier's Query Interval.
 * See RFC 3376, section 4.1.7.
 *
 * @param qqic  Querier's Query Interval Code (QQIC) in exponential form
 *              from IGMPv3 Query message.
 *
 * @return      Querier's Query Interval (QQI) in seconds.
 */
extern unsigned tapi_igmp3_qqic_to_qqi(uint8_t qqic);

/**
 * Add IGMPv3 Query PDU as the last PDU to the last unit of the traffic
 * template or pattern.
 *
 * @param tmpl_or_ptrn  Location of ASN.1 value with traffic template or
 *                      pattern
 * @param pdu           Location for ASN.1 value pointer with added PDU
 * @param is_pattern    Is the first argument template or pattern
 * @param max_resp_code IGMP message maximum response code,
 *                      or negative to keep unspecified.
 * @param group_addr    Multicast Group Address field of IGMPv3 Query message.
 * @param s_flag        S Flag (Suppress Router-Side Processing) field
 *                      of IGMPv3 Query message.
 * @param qrv           QRV (Querier's Robustness Variable) field
 *                      of IGMPv3 Query message.
 * @param qqic          QQIC (Querier's Query Interval Code) field
 *                      of IGMPv3 Query message.
 * @param src_list      List of source addresses to be sent in this PDU,
 *                      or @c NULL
 *
 * @return              Status code.
 */
extern te_errno
tapi_igmp3_add_query_pdu(asn_value               **tmpl_or_ptrn,
                         asn_value               **pdu,
                         te_bool                   is_pattern,
                         int                       max_resp_code,
                         in_addr_t                 group_addr,
                         int                       s_flag,
                         int                       qrv,
                         int                       qqic,
                         tapi_igmp3_src_list_t    *src_list);

/**
 * Send IGMPv3 Group Membership Query message
 *
 * @param ta_name       Test Agent name
 * @param session       RCF SID
 * @param csap          igmp.ip4.eth CSAP handle to send IGMP message through
 * @param max_resp_code IGMP message maximum response code,
 *                      or negative to keep unspecified.
 * @param group_addr    Multicast Group Address field of IGMPv3 Query message.
 * @param s_flag        S Flag (Suppress Router-Side Processing) field
 *                      of IGMPv3 Query message.
 * @param qrv           QRV (Querier's Robustness Variable) field
 *                      of IGMPv3 Query message.
 * @param qqic          QQIC (Querier's Query Interval Code) field
 *                      of IGMPv3 Query message.
 * @param src_list      List of source addresses to be sent in this PDU,
 *                      or @c NULL
 * @param src_addr      IPv4 layer Source Address field or @c INADDR_ANY to
 *                      keep unspecified.
 * @param skip_eth      Do not add @p eth_src
 * @param eth_src       Ethernet layer Source Address field or @c NULL to keep
 *                      unspecified.
 *
 * @note                To specify the case of General Query, @p group_addr
 *                      should be @c INADDR_ANY and message will be
 *                      sent to @c ALL_HOSTS (224.0.0.1)
 *
 * @return              Status code.
 */
extern te_errno
tapi_igmp3_ip4_eth_send_query(const char            *ta_name,
                              int                    session,
                              csap_handle_t          csap,
                              int                    max_resp_code,
                              in_addr_t              group_addr,
                              int                    s_flag,
                              int                    qrv,
                              int                    qqic,
                              tapi_igmp3_src_list_t *src_list,
                              in_addr_t              src_addr,
                              te_bool                skip_eth,
                              uint8_t               *eth_src);

/**
 * Send IGMPv3 Group Membership Query message with default timeouts and flags.
 *
 * @param ta_name       Test Agent name.
 * @param session       RCF SID.
 * @param csap          igmp.ip4.eth CSAP handle to send IGMP message through.
 * @param group_addr    Multicast Group Address field of IGMPv3 Query message.
 * @param src_list      List of source addresses to be sent in this PDU,
 *                      or @c NULL.
 * @param src_addr      IPv4 layer Source Address field or @c INADDR_ANY to
 *                      keep unspecified.
 * @param skip_eth      Do not add @p eth_src.
 * @param eth_src       Ethernet layer Source Address field or @c NULL to keep
 *                      unspecified.
 *
 * @return              Status code.
 */
extern te_errno
tapi_igmp3_ip4_eth_send_query_default(const char            *ta_name,
                                      int                    session,
                                      csap_handle_t          csap,
                                      in_addr_t              group_addr,
                                      tapi_igmp3_src_list_t *src_list,
                                      in_addr_t              src_addr,
                                      te_bool                skip_eth,
                                      uint8_t               *eth_src);

/**
 * Initialise Source Address List instance
 *
 * @param src_list      Pointer to Source Address List to initialise
 *                      with default values
 *
 * @return              Status code.
 */
extern te_errno
tapi_igmp3_src_list_init(tapi_igmp3_src_list_t *src_list);

/**
 * Free resources allocated by Source Address List instance
 *
 * @param src_list      Pointer to Source Address List to finalise
 *
 * @return              N/A
 */
extern void
tapi_igmp3_src_list_free(tapi_igmp3_src_list_t *src_list);

/**
 * Add source address to the list
 *
 * @param src_list      Source Address List to add to
 * @param addr          IPv4 address to add
 *
 * @return              Status code.
 */
extern te_errno
tapi_igmp3_src_list_add(tapi_igmp3_src_list_t *src_list, in_addr_t addr);

/**
 * Calculate the binary length of the Source Address List
 * stored in IGMPv3 message
 *
 * @param src_list      Source Address List
 *
 * @return              Length in bytes.
 */
extern int
tapi_igmp3_src_list_length(tapi_igmp3_src_list_t *src_list);

/**
 * Pack the source list to store in IGMPv3 message
 *
 * @param[in]     src_list  Source Address List to pack
 * @param[in]     buf       Buffer to pack the Source Address List at
 * @param[in]     buf_size  Size of pre-allocated buffer
 * @param[in,out] offset    Buffer offset to start packing from
 *
 * @return                  Status code.
 */
extern te_errno
tapi_igmp3_src_list_gen_bin(tapi_igmp3_src_list_t *src_list,
                            void *buf, int buf_size, int *offset);

/**
 * Calculate the binary length of the Group Record
 * stored in IGMPv3 message
 *
 * @param group_record  Group Record to pack
 *
 * @return              Length in bytes.
 */
extern int
tapi_igmp3_group_record_length(tapi_igmp3_group_record_t *group_record);

/**
 * Pack the Group Record structure with Source Address List
 * to store in IGMPv3 message
 *
 * @param[in]     group_record  Group Record to pack
 * @param[in]     buf           Buffer to pack the Group Record List at
 * @param[in]     buf_size      Size of pre-allocated buffer
 * @param[in,out] offset        Buffer offset to start packing from
 *
 * @return                      Status code.
 */
extern te_errno
tapi_igmp3_group_record_gen_bin(tapi_igmp3_group_record_t *group_record,
                                void *buf, int buf_size, int *offset);

/**
 * Calculate the binary length of the Group Record List
 * packed in IGMPv3 message
 *
 * @param group_list    Group Record List to pack
 *
 * @return              Length in bytes.
 */
extern int
tapi_igmp3_group_list_length(tapi_igmp3_group_list_t *group_list);

/**
 * Pack the Group Record List
 * to store in IGMPv3 message
 *
 * @param[in]     group_list  Group Record List to pack
 * @param[in]     buf         Buffer to pack the Group Record List at
 * @param[in]     buf_size    Size of pre-allocated buffer
 * @param[in,out] offset      Buffer offset to start packing from
 *
 * @return                    Status code.
 */
extern te_errno
tapi_igmp3_group_list_gen_bin(tapi_igmp3_group_list_t *group_list,
                              void *buf, int buf_size, int *offset);

/**
 * Initialise pre-allocated structure of Group Record with default values
 *
 * @param group_record   Group Record to initialise
 * @param group_type     Type of the Group Record
 * @param group_address  Multicast Group Address
 * @param aux_data       Pointer to auxiliary data to be packed into the
 *                       record group.
 * @param aux_data_len   Length of the auxiliary data in 32-bit words
 *
 * @return               Status code.
 */
extern te_errno
tapi_igmp3_group_record_init(tapi_igmp3_group_record_t *group_record,
                             int group_type, in_addr_t group_address,
                             void *aux_data, int aux_data_len);

/**
 * Free system resources allocated by group_record
 *
 * @param group_record  Group Record to free
 *
 * @return              Status code.
 */
extern void
tapi_igmp3_group_record_free(tapi_igmp3_group_record_t *group_record);

/**
 * Add source address to the group_record
 *
 * @param group_record  Group Record to add to
 * @param src_addr      IPv4 source address to add
 *
 * @return              Status code.
 */
extern te_errno
tapi_igmp3_group_record_add_source(tapi_igmp3_group_record_t *group_record,
                                   in_addr_t src_addr);

/**
 * Initialise the Group Record List with initial values
 *
 * @param group_list  Group Record List structure to initialise
 *
 * @return            Status code.
 */
extern te_errno
tapi_igmp3_group_list_init(tapi_igmp3_group_list_t *group_list);

/**
 * Free system resources allocated by Group Record List
 *
 * @param group_list  Group Record List to free
 *
 * @return            Status code.
 */
extern void
tapi_igmp3_group_list_free(tapi_igmp3_group_list_t *group_list);


/**
 * Add Group Record to the Group Record List
 *
 * @param group_list    Group Record List to add to
 * @param group_record  Group Record structure to add
 *
 * @return              Status code.
 */
extern te_errno
tapi_igmp3_group_list_add(tapi_igmp3_group_list_t *group_list,
                          tapi_igmp3_group_record_t *group_record);


/**
 * Allocate, initialise and fill source list structure
 *
 * @param src_list    Source list to initialise and fill
 *                    Memory allocation will be skipped if this parameter
 *                    is not @c NULL
 * @param ...         List of source addresses to be added to the list
 *                    (list should end with 0 value (@c INADDR_ANY))
 *
 * @return            Initialised source list, or @c NULL if failed.
 */
extern tapi_igmp3_src_list_t *
tapi_igmp3_src_list_new(tapi_igmp3_src_list_t *src_list, ...);


/**
 * Allocate, initialise and fill Group Record structure
 *
 * @param group_record   Group Record to initialise and fill
 *                       Memory allocation will be skipped if this parameter is
 *                       not @c NULL
 * @param group_type     Type of the Group Record
 * @param group_address  Multicast Group Address
 * @param aux_data       Pointer to auxiliary data to be packed into the
 *                       record group.
 * @param aux_data_len   Length of the auxiliary data in 32-bit words
 * @param ...            List of source addresses to be added to the record
 *                       (list should end with 0 value (@c INADDR_ANY))
 *
 * @return               Initialised Group Record, or @c NULL if failed.
 */
extern tapi_igmp3_group_record_t *
tapi_igmp3_group_record_new(tapi_igmp3_group_record_t *group_record,
                            int group_type, in_addr_t group_address,
                            void *aux_data, int aux_data_len, ...);


/**
 * Allocate, initialise and fill Group Record List structure
 *
 * @param group_list  Group Record List to initialise and fill
 *                    Memory allocation will be skipped if this parameter
 *                    is not @c NULL
 * @param ...         List of Group Records to be added to the list
 *                    (list should end with @c NULL value)
 *
 * @return            Initialised Group Record List, or @c NULL if failed.
 */
extern tapi_igmp3_group_list_t *
tapi_igmp3_group_list_new(tapi_igmp3_group_list_t *group_list, ...);


#define IGMP1_SEND_JOIN(_pco, _csap, _group_addr, _src_addr, _src_mac) \
    CHECK_RC(tapi_igmp1_ip4_eth_send_report((_pco)->ta, (_pco)->sid,    \
                                            (_csap), (_group_addr),    \
                                            (_src_addr), (_src_mac)))

#define IGMP2_SEND_JOIN(_pco, _csap, _group_addr, _src_addr, _src_mac) \
    CHECK_RC(tapi_igmp2_ip4_eth_send_report((_pco)->ta, (_pco)->sid,    \
                                            (_csap), (_group_addr),    \
                                            (_src_addr), (_src_mac)))

#define IGMP2_SEND_LEAVE(_pco, _csap, _group_addr, _src_addr, _src_mac) \
    CHECK_RC(tapi_igmp2_ip4_eth_send_leave((_pco)->ta, (_pco)->sid,    \
                                           (_csap), (_group_addr),    \
                                           (_src_addr), (_src_mac)))

#define IGMP2_SEND_QUERY(_pco,_csap,_group_addr,_src_addr,_skip_eth,_src_mac) \
    CHECK_RC(tapi_igmp2_ip4_eth_send_query((_pco)->ta, (_pco)->sid,           \
                                           (_csap), 0, (_group_addr),         \
                                           (_src_addr), (_skip_eth),          \
                                           (_src_mac)))


/**
 * Allocate, initialise and fill Source Address List structure.
 *
 * @param ...  List of source addresses (in_addr_t) to be added to the list.
 *
 * @return     Initialised Source Address List. May be omitted.
 */
#define IGMP3_SRC_LIST(...) \
    tapi_igmp3_src_list_new(NULL, __VA_ARGS__ __VA_OPT__(,) 0)

/**
 * Allocate, initialise and fill Group Record List structure.
 *
 * @param ...  List of Group Records (tapi_igmp3_group_record_t *) to be added
 *             to the list. May be omitted.
 *
 * @return     Initialised Group Record List.
 */
#define IGMP3_GROUP_LIST(...) \
    tapi_igmp3_group_list_new(NULL, __VA_ARGS__ __VA_OPT__(,) NULL)

/**
 * Allocate, initialise and fill Group Record structure.
 *
 * @param _group_type     Type of the Group Record.
 * @param _group_address  Multicast Group Address.
 * @param ...             List of source addresses (in_addr_t) to be added to
 *                        the record. May be omitted.
 *
 * @return                Initialised Group Record.
 */
#define IGMP3_GROUP_RECORD(_group_type, _group_address, ...)                \
    tapi_igmp3_group_record_new(NULL, _group_type, _group_address, NULL, 0, \
                                __VA_ARGS__ __VA_OPT__(,) 0)

/**
 * Send IGMPv3 report and free group list.
 *
 * @param _pco         RPC server.
 * @param _csap        igmp.ip4.eth CSAP handle to send IGMP message through.
 * @param _group_list  Group Record List to be sent.
 * @param _src_addr    IPv4 layer Source Address field or @c INADDR_ANY to
 *                     keep unspecified.
 * @param _src_mac     Ethernet layer Source Address field or @c NULL to keep
 *                     unspecified.
 *
 * @note  @p _group_list will be freed after sending.
 */
#define IGMP3_SEND_REPORT(_pco, _csap, _group_list, _src_addr, _src_mac)   \
    do {                                                                   \
        tapi_igmp3_group_list_t *gl_magic_3f859 = (_group_list);           \
                                                                           \
        CHECK_RC(tapi_igmp3_ip4_eth_send_report((_pco)->ta, (_pco)->sid,   \
                                                (_csap), gl_magic_3f859,   \
                                                (_src_addr), (_src_mac))); \
        tapi_igmp3_group_list_free(gl_magic_3f859);                        \
        free(gl_magic_3f859);                                              \
    } while (0)

/**
 * Send IGMPv3 report with one multicast Group Record.
 *
 * @param _pco         RPC server.
 * @param _csap        igmp.ip4.eth CSAP handle to send IGMP message through.
 * @param _group_type  Type of the Group Record.
 * @param _group_addr  Multicast Group Address.
 * @param _src_addr    IPv4 layer Source Address field or @c INADDR_ANY to
 *                     keep unspecified.
 * @param _src_mac     Ethernet layer Source Address field or @c NULL to keep
 *                     unspecified.
 * @param ...          List of source addresses to be added to the record.
 *                     May be omitted.
 */
#define IGMP3_SEND_SINGLE_REPORT(_pco, _csap, _group_type, _group_addr,   \
                                 _src_addr, _src_mac, ...)                \
    IGMP3_SEND_REPORT((_pco), (_csap),                                    \
        IGMP3_GROUP_LIST(                                                 \
            IGMP3_GROUP_RECORD((_group_type),                             \
                               (_group_addr) __VA_OPT__(,) __VA_ARGS__)), \
        (_src_addr), (_src_mac))

/**
 * Send IGMPv3 report with one Group Record to recieve multicast traffic
 * from any source (same as IGMPv2 Membership Report).
 *
 * @param _pco         RPC server.
 * @param _csap        igmp.ip4.eth CSAP handle to send IGMP message through.
 * @param _group_addr  Multicast Group Address.
 * @param _src_addr    IPv4 layer Source Address field or @c INADDR_ANY to
 *                     keep unspecified.
 * @param _src_mac     Ethernet layer Source Address field or @c NULL to keep
 *                     unspecified.
 */
#define IGMP3_SEND_JOIN(_pco, _csap, _group_addr, _src_addr, _src_mac)  \
    IGMP3_SEND_SINGLE_REPORT((_pco), (_csap), IGMPV3_CHANGE_TO_EXCLUDE, \
                             (_group_addr), (_src_addr), (_src_mac))

/**
 * Send IGMPv3 report with one Group Record to block multicast traffic
 * from any source (same as IGMPv2 Leave Group).
 *
 * @param _pco         RPC server.
 * @param _csap        igmp.ip4.eth CSAP handle to send IGMP message through.
 * @param _group_addr  Multicast Group Address.
 * @param _src_addr    IPv4 layer Source Address field or @c INADDR_ANY to
 *                     keep unspecified.
 * @param _src_mac     Ethernet layer Source Address field or @c NULL to keep
 *                     unspecified.
 */
#define IGMP3_SEND_LEAVE(_pco, _csap, _group_addr, _src_addr, _src_mac) \
    IGMP3_SEND_SINGLE_REPORT((_pco), (_csap), IGMPV3_CHANGE_TO_INCLUDE, \
                             (_group_addr), (_src_addr), (_src_mac))

/**
 * Send IGMPv3 report with one Group Record to allow multicast traffic.
 *
 * @param _pco         RPC server.
 * @param _csap        igmp.ip4.eth CSAP handle to send IGMP message through.
 * @param _group_addr  Multicast Group Address.
 * @param _src_addr    IPv4 layer Source Address field or @c INADDR_ANY to
 *                     keep unspecified.
 * @param _src_mac     Ethernet layer Source Address field or @c NULL to keep
 *                     unspecified.
 * @param _addr1       First source address to be added to the record.
 * @param ...          List of source addresses to be added to the record.
 *                     May be omitted.
 */
#define IGMP3_SEND_ALLOW(_pco, _csap, _group_addr, _src_addr, _src_mac,      \
                         _addr1...)                                          \
    IGMP3_SEND_SINGLE_REPORT((_pco), (_csap), IGMPV3_ALLOW_NEW_SOURCES,      \
                             (_group_addr), (_src_addr), (_src_mac), _addr1)

/**
 * Send IGMPv3 report with one Group Record to block multicast traffic.
 *
 * @param _pco         RPC server.
 * @param _csap        igmp.ip4.eth CSAP handle to send IGMP message through.
 * @param _group_addr  Multicast Group Address.
 * @param _src_addr    IPv4 layer Source Address field or @c INADDR_ANY to
 *                     keep unspecified.
 * @param _src_mac     Ethernet layer Source Address field or @c NULL to keep
 *                     unspecified.
 * @param _addr1       First source address to be added to the record.
 * @param ...          List of source addresses to be added to the record.
 *                     May be omitted.
 */
#define IGMP3_SEND_BLOCK(_pco, _csap, _group_addr, _src_addr, _src_mac,      \
                         _addr1...)                                          \
    IGMP3_SEND_SINGLE_REPORT((_pco), (_csap), IGMPV3_BLOCK_OLD_SOURCES,      \
                             (_group_addr), (_src_addr), (_src_mac), _addr1)

/**
 * Send IGMPv3 report with one Group Record of MODE_IS_INCLUDE type.
 *
 * @param _pco         RPC server.
 * @param _csap        igmp.ip4.eth CSAP handle to send IGMP message through.
 * @param _group_addr  Multicast Group Address.
 * @param _src_addr    IPv4 layer Source Address field or @c INADDR_ANY to
 *                     keep unspecified.
 * @param _src_mac     Ethernet layer Source Address field or @c NULL to keep
 *                     unspecified.
 * @param ...          List of source addresses to be added to the record.
 *                     May be omitted.
 */
#define IGMP3_SEND_IS_INCLUDE(_pco, _csap, _group_addr,               \
                              _src_addr, _src_mac, ...)               \
    IGMP3_SEND_SINGLE_REPORT((_pco), (_csap), IGMPV3_MODE_IS_INCLUDE, \
                             (_group_addr), (_src_addr), (_src_mac)   \
                             __VA_OPT__(,) __VA_ARGS__)

/**
 * Send IGMPv3 report with one Group Record of MODE_IS_EXCLUDE type.
 *
 * @param _pco         RPC server.
 * @param _csap        igmp.ip4.eth CSAP handle to send IGMP message through.
 * @param _group_addr  Multicast Group Address.
 * @param _src_addr    IPv4 layer Source Address field or @c INADDR_ANY to
 *                     keep unspecified.
 * @param _src_mac     Ethernet layer Source Address field or @c NULL to keep
 *                     unspecified.
 * @param ...          List of source addresses to be added to the record.
 *                     May be omitted.
 */
#define IGMP3_SEND_IS_EXCLUDE(_pco, _csap, _group_addr,               \
                              _src_addr, _src_mac, ...)               \
    IGMP3_SEND_SINGLE_REPORT((_pco), (_csap), IGMPV3_MODE_IS_EXCLUDE, \
                             (_group_addr), (_src_addr), (_src_mac)   \
                             __VA_OPT__(,) __VA_ARGS__)

/**
 * Send IGMPv3 report with one Group Record of CHANGE_TO_INCLUDE type.
 *
 * @param _pco         RPC server.
 * @param _csap        igmp.ip4.eth CSAP handle to send IGMP message through.
 * @param _group_addr  Multicast Group Address.
 * @param _src_addr    IPv4 layer Source Address field or @c INADDR_ANY to
 *                     keep unspecified.
 * @param _src_mac     Ethernet layer Source Address field or @c NULL to keep
 *                     unspecified.
 * @param ...          List of source addresses to be added to the record.
 *                     May be omitted.
 */
#define IGMP3_SEND_TO_INCLUDE(_pco, _csap, _group_addr,                 \
                              _src_addr, _src_mac, ...)                 \
    IGMP3_SEND_SINGLE_REPORT((_pco), (_csap), IGMPV3_CHANGE_TO_INCLUDE, \
                             (_group_addr), (_src_addr), (_src_mac)     \
                             __VA_OPT__(,) __VA_ARGS__)

/**
 * Send IGMPv3 report with one Group Record of CHANGE_TO_EXCLUDE type.
 *
 * @param _pco         RPC server.
 * @param _csap        igmp.ip4.eth CSAP handle to send IGMP message through.
 * @param _group_addr  Multicast Group Address.
 * @param _src_addr    IPv4 layer Source Address field or @c INADDR_ANY to
 *                     keep unspecified.
 * @param _src_mac     Ethernet layer Source Address field or @c NULL to keep
 *                     unspecified.
 * @param ...          List of source addresses to be added to the record.
 *                     May be omitted.
 */
#define IGMP3_SEND_TO_EXCLUDE(_pco, _csap, _group_addr,                 \
                              _src_addr, _src_mac, ...)                 \
    IGMP3_SEND_SINGLE_REPORT((_pco), (_csap), IGMPV3_CHANGE_TO_EXCLUDE, \
                             (_group_addr), (_src_addr), (_src_mac)     \
                             __VA_OPT__(,) __VA_ARGS__)

/**
 * Send IGMPv3 query.
 *
 * @param _pco         RPC server.
 * @param _csap        igmp.ip4.eth CSAP handle to send IGMP message through.
 * @param _group_addr  Multicast Group Address field of IGMPv3 Query message.
 * @param _src_addr    IPv4 layer Source Address field or @c INADDR_ANY to
 *                     keep unspecified.
 * @param _skip_eth    Do not add @p _src_mac
 * @param _src_mac     Ethernet layer Source Address field or @c NULL to keep
 *                     unspecified.
 * @param ...          List of source addresses to be added to the record.
 *                     May be omitted.
 */
#define IGMP3_SEND_QUERY(_pco, _csap, _group_addr,               \
                         _src_addr, _skip_eth, _src_mac, ...)    \
    do {                                                         \
        tapi_igmp3_src_list_t *sl_magic_5201 = NULL;             \
                                                                 \
        __VA_OPT__(sl_magic_5201 = IGMP3_SRC_LIST(__VA_ARGS__);) \
        CHECK_RC(tapi_igmp3_ip4_eth_send_query_default(          \
                     (_pco)->ta, (_pco)->sid,                    \
                     (_csap),                                    \
                     (_group_addr),                              \
                     sl_magic_5201,                              \
                     (_src_addr),                                \
                     (_skip_eth), (_src_mac)));                  \
        tapi_igmp3_src_list_free(sl_magic_5201);                 \
        free(sl_magic_5201);                                     \
    } while (0)

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_IGMP2_H__ */

/**@} <!-- END tapi_tad_igmp --> */
