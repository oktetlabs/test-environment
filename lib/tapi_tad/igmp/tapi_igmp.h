/** @file
 * @brief Test API for TAD. IGMP CSAP
 *
 * Implementation of Test API
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/ or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 *
 * $Id: $
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

/** Default ToS for IGMP messages is 0xc0 */
#define TAPI_IGMP_IP4_TOS_DEFAULT   0xc0

/** Pre-allocated size for source addresses list */
#define TAPI_IGMP3_SRC_LIST_SIZE_MIN    16

/** Pre-allocated size for group records list */
#define TAPI_IGMP3_GROUP_LIST_SIZE_MIN  16

#define TAPI_IGMP3_GROUP_RECORD_HDR_LEN 8


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
    tapi_igmp3_src_list_t   src_list;      /**< Source Address list storage */
} tapi_igmp3_group_record_t;

/**
 * IGMPv3 Group Records List storage
 */
typedef struct tapi_igmp3_group_list_s {
    tapi_igmp3_group_record_t **groups;        /**< Array of Group Records */
    int                         groups_no;     /**< Number of Group Records */
    int                         groups_no_max; /**< Size of pre-allocated
                                                    Group Records array */
} tapi_igmp3_group_list_t;



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
 * @param eth_src       Local MAC address (or NULL)
 * @param src_addr      Local IP address in network byte order (or NULL)
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
 * @param src_addr      IPv4 layer Source Address field or INADDR_ANY to
 *                      keep unspecified.
 *
 * @return              Status code.
 */
extern te_errno
tapi_igmp_add_ip4_pdu(asn_value **tmpl_or_ptrn,
                      asn_value **pdu,
                      te_bool     is_pattern,
                      in_addr_t   dst_addr,
                      in_addr_t   src_addr);


/**
 * Add IPv4.Eth layers to PDU
 *
 * @param tmpl_or_ptrn  Location of ASN.1 value with traffic template or
 *                      pattern
 * @param pdu           Location for ASN.1 value pointer with added PDU
 * @param is_pattern    Is the first argument template or pattern
 * @param dst_addr      IPv4 layer Destination Multicast address (also used
 *                      for generating Ethernet multicast address)
 * @param src_addr      IPv4 layer Source Address field or INADDR_ANY to
 *                      keep unspecified.
 * @param eth_src       Ethernet layer Source Address field or NULL to keep
 *                      unspecified.
 *
 * @note  Destination address Ethernet layers is calculated from
 *        @p dst_address value.
 *
 * @return              Status code.
 */
extern te_errno
tapi_igmp_add_ip4_eth_pdu(asn_value **tmpl_or_ptrn,
                          asn_value **pdu,
                          te_bool     is_pattern,
                          in_addr_t   dst_addr,
                          in_addr_t   src_addr,
                          uint8_t    *eth_src);

/**
 * Send IGMPv1 report message
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param csap          igmp.ip4.eth CSAP handle to send IGMP message through
 * @param group_addr    Multicast Group Address field of IGMPv2 message.
 * @param src_addr      IPv4 layer Source Address field or INADDR_ANY to
 *                      keep unspecified.
 * @param eth_src       Ethernet layer Source Address field or NULL to keep
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
 * @param sid           RCF SID
 * @param csap          igmp.ip4.eth CSAP handle to send IGMP message through
 * @param group_addr    Multicast Group Address field of IGMPv2 message.
 * @param src_addr      IPv4 layer Source Address field or INADDR_ANY to
 *                      keep unspecified.
 * @param eth_src       Ethernet layer Source Address field or NULL to keep
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
 * @param sid           RCF SID
 * @param csap          igmp.ip4.eth CSAP handle to send IGMP message through
 * @param group_addr    Multicast Group Address field of IGMPv2 message.
 * @param src_addr      IPv4 layer Source Address field or INADDR_ANY to
 *                      keep unspecified.
 * @param eth_src       Ethernet layer Source Address field or NULL to keep
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
 * @param sid           RCF SID
 * @param csap          igmp.ip4.eth CSAP handle to send IGMP message through
 * @param max_resp_time IGMP message maximum response time,
 *                      or negative to keep unspecified.
 * @param group_addr    Multicast Group Address field of IGMPv2 message.
 *                      For General Query should be 0.0.0.0 (INADDR_ANY)
 * @param src_addr      IPv4 layer Source Address field or INADDR_ANY to
 *                      keep unspecified.
 * @param eth_src       Ethernet layer Source Address field or NULL to keep
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
 * @param group_list    List of group records to be sent in this PDU
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
 * @param sid           RCF SID
 * @param csap          igmp.ip4.eth CSAP handle to send IGMP message through
 * @param group_list    List of group records to be sent in this PDU
 * @param src_addr      IPv4 layer Source Address field or INADDR_ANY to
 *                      keep unspecified.
 * @param eth_src       Ethernet layer Source Address field or NULL to keep
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
 * Add IGMPv3 Query PDU as the last PDU to the last unit of the traffic 
 * template or pattern.
 *
 * @param tmpl_or_ptrn  Location of ASN.1 value with traffic template or
 *                      pattern
 * @param pdu           Location for ASN.1 value pointer with added PDU
 * @param is_pattern    Is the first argument template or pattern
 * @param max_resp_time IGMP message maximum response code,
 *                      or negative to keep unspecified.
 * @param group_addr    Multicast Group Address field of IGMPv3 Query message.
 * @param s_flag        S Flag (Suppress Router-Side Processing) field
 *                      of IGMPv3 Query message.
 * @param qrv           QRV (Querier's Robustness Variable) field
 *                      of IGMPv3 Query message.
 * @param qqic          QQIC (Querier's Query Interval Code) field
 *                      of IGMPv3 Query message.
 * @param src_list      List of source addresses to be sent in this PDU,
 *                      or NULL
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
 * @param sid           RCF SID
 * @param csap          igmp.ip4.eth CSAP handle to send IGMP message through
 * @param max_resp_time IGMP message maximum response code,
 *                      or negative to keep unspecified.
 * @param group_addr    Multicast Group Address field of IGMPv3 Query message.
 * @param s_flag        S Flag (Suppress Router-Side Processing) field
 *                      of IGMPv3 Query message.
 * @param qrv           QRV (Querier's Robustness Variable) field
 *                      of IGMPv3 Query message.
 * @param qqic          QQIC (Querier's Query Interval Code) field
 *                      of IGMPv3 Query message.
 * @param src_list      List of source addresses to be sent in this PDU,
 *                      or NULL
 * @param src_addr      IPv4 layer Source Address field or INADDR_ANY to
 *                      keep unspecified.
 * @param eth_src       Ethernet layer Source Address field or NULL to keep
 *                      unspecified.
 *
 * @note                To specify the case of General Query, @p group_addr
 *                      should be INADDR_ANY and message will be
 *                      sent to ALL_HOSTS (224.0.0.1)
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
 * Initialise source address list instance
 *
 * @param src_list      Pointer to source address list to initialise
 *                      with default values
 *
 * @return              Status code.
 */
extern te_errno
tapi_igmp3_src_list_init(tapi_igmp3_src_list_t *src_list);

/**
 * Free resources allocated by source address list instance
 *
 * @param src_list      Pointer to source address list to finalise
 *
 * @return              N/A
 */
extern void
tapi_igmp3_src_list_free(tapi_igmp3_src_list_t *src_list);

/**
 * Add source address to the list
 *
 * @param src_list      Source address list to add to
 * @param addr          IPv4 address to add
 *
 * @return              Status code.
 */
extern te_errno
tapi_igmp3_src_list_add(tapi_igmp3_src_list_t *src_list, in_addr_t addr);

/**
 * Calculate the binary length of the source address list
 * stored in IGMPv3 message
 *
 * @param src_list      Source address list to add to
 *
 * @return              Length in bytes.
 */
extern int
tapi_igmp3_src_list_length(tapi_igmp3_src_list_t *src_list);

/**
 * Pack the source list to store in IGMPv3 message
 *
 * @param src_list      Source address list to add to
 * @param buf           Buffer to pack the source address list at
 * @param buf_size      Size of pre-allocated buffer
 * @param offset        Buffer offset to start packing from (IN/OUT)
 *
 * @return              Status Code.
 */
extern te_errno
tapi_igmp3_src_list_gen_bin(tapi_igmp3_src_list_t *src_list,
                            void *buf, int buf_size, int *offset);

/**
 * Calculate the binary length of the Group Record
 * stored in IGMPv3 message
 *
 * @param group_record  Group record to pack
 *
 * @return              Length in bytes.
 */
extern int
tapi_igmp3_group_record_length(tapi_igmp3_group_record_t *group_record);

/**
 * Pack the groups record structure with source addresses list
 * to store in IGMPv3 message
 *
 * @param group_record  Group record to pack
 * @param buf           Buffer to pack the source address list at
 * @param buf_size      Size of pre-allocated buffer
 * @param offset        Buffer offset to start packing from (IN/OUT)
 *
 * @return              Status Code.
 */
extern te_errno
tapi_igmp3_group_record_gen_bin(tapi_igmp3_group_record_t *group_record,
                                void *buf, int buf_size, int *offset);

/**
 * Calculate the binary length of the Group Records list
 * packed in IGMPv3 message
 *
 * @param group_list    Group Records list to pack to pack
 *
 * @return              Length in bytes.
 */
extern int
tapi_igmp3_group_list_length(tapi_igmp3_group_list_t *group_list);

/**
 * Pack the Group Records list
 * to store in IGMPv3 message
 *
 * @param group_list    Group Records list to pack
 * @param buf           Buffer to pack the source address list at
 * @param buf_size      Size of pre-allocated buffer
 * @param offset        Buffer offset to start packing from (IN/OUT)
 *
 * @return              Status Code.
 */
extern te_errno
tapi_igmp3_group_list_gen_bin(tapi_igmp3_group_list_t *group_list,
                              void *buf, int buf_size, int *offset);

/**
 * Initialise pre-allocated structure of Group Record with default values
 *
 * @param group_record  Group Record to initialise list to pack
 * @param group_type    Type of the group record
 * @param group_address Multicast Group Address
 * @param aux_data      Pointer to auxiliary data to be packed into the
 *                      record group)
 * @param aux_data_len  Length of the auxiliary data in 32-bit words
 *
 * @return              Status Code.
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
 * @return              Status Code.
 */
extern void
tapi_igmp3_group_record_free(tapi_igmp3_group_record_t *group_record);

/**
 * Free system resources allocated by group_record
 *
 * @param group_record  Group Record to free
 *
 * @return              Status Code.
 */
extern te_errno
tapi_igmp3_group_record_add_source(tapi_igmp3_group_record_t *group_record,
                                   in_addr_t src_addr);

/**
 * Initialise the group records list with initial values
 *
 * @param group_list    Group Records list structure to initialise
 *
 * @return              Status Code.
 */
extern te_errno
tapi_igmp3_group_list_init(tapi_igmp3_group_list_t *group_list);

/**
 * Free system resources allocated by group records list
 *
 * @param group_list    Group Record to free
 *
 * @return              Status Code.
 */
extern void
tapi_igmp3_group_list_free(tapi_igmp3_group_list_t *group_list);


/**
 * Add group record to the Group Records list
 *
 * @param group_list    Group Records list to add to
 * @param group_recordd Group Record structure to add
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
 *                    Memory allocation will be skipped if this parameter is not NULL
 * @param ...         List of source addresses to be added to the list
 *                    (list should be ended with 0 value (INADDR_ANY))
 *
 * @return            Initialised source list, or NULL if failed.
 */
extern tapi_igmp3_src_list_t *
tapi_igmp3_src_list_new(tapi_igmp3_src_list_t *src_list, ...);


extern tapi_igmp3_group_record_t *
tapi_igmp3_group_record_new(tapi_igmp3_group_record_t *group_record,
                            int group_type, in_addr_t group_address,
                            void *aux_data, int aux_data_len, ...);


/**
 * Allocate, initialise and fill group list structure
 *
 * @param group_list  Group Records list to initialise and fill (i
 *                    Memory allocation will be skipped if this parameter is not NULL
 * @param ...         List of group records to be added to the list
 *                    (list should be ended with NULL value)
 *
 * @return            Initialised source list, or NULL if failed.
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


#define IGMP3_SRC_LIST(_list...)                \
    tapi_igmp3_src_list_new(_list)

#define IGMP3_GROUP_LIST(_group_list...) \
    tapi_igmp3_group_list_new(_group_list)

#define IGMP3_GROUP_RECORD(_group_record, _group_type, _group_address, _aux_data, _aux_data_len...) \
    tapi_igmp3_group_record_new(_group_record, _group_type, _group_address, \
                                _aux_data, _aux_data_len)

#define IGMP3_SEND_REPORT(_pco, _csap, _group_list, _src_addr, _src_mac) \
    CHECK_RC(tapi_igmp3_ip4_eth_send_report((_pco)->ta, (_pco)->sid,     \
                                            (_csap), (_group_list),      \
                                            (_src_addr), (_src_mac)))

#define IGMP3_SEND_SINGLE_REPORT(_pco, _csap, _group_type, _group_addr, _src_addr, _src_mac, _addr1...) \
    IGMP3_SEND_REPORT((_pco), (_csap),          \
        IGMP3_GROUP_LIST(NULL,                  \
            IGMP3_GROUP_RECORD(NULL,            \
                (_group_type), (_group_addr),   \
                NULL, 0, _addr1, 0),            \
            NULL),                              \
        (_src_addr), (_src_mac))

#define IGMP3_SEND_JOIN(_pco, _csap, _group_addr, _src_addr, _src_mac) \
    IGMP3_SEND_REPORT((_pco), (_csap),          \
        IGMP3_GROUP_LIST(NULL,                  \
            IGMP3_GROUP_RECORD(NULL,            \
                IGMPV3_CHANGE_TO_EXCLUDE,       \
                (_group_addr), NULL, 0, 0),     \
            NULL),                              \
        (_src_addr), (_src_mac))

#define IGMP3_SEND_LEAVE(_pco, _csap, _group_addr, _src_addr, _src_mac) \
    IGMP3_SEND_REPORT((_pco), (_csap),          \
        IGMP3_GROUP_LIST(NULL,                  \
            IGMP3_GROUP_RECORD(NULL,            \
                IGMPV3_CHANGE_TO_INCLUDE,       \
                (_group_addr), NULL, 0, 0),     \
            NULL),                              \
        (_src_addr), (_src_mac))

#define IGMP3_SEND_ALLOW(_pco, _csap, _group_addr, _src_addr, _src_mac, _addr1,...) \
    IGMP3_SEND_REPORT((_pco), (_csap),              \
        IGMP3_GROUP_LIST(NULL,                      \
            IGMP3_GROUP_RECORD(NULL,                \
                IGMPV3_ALLOW_NEW_SOURCES,           \
                (_group_addr), NULL, 0, _addr1, 0), \
            NULL),                                  \
        (_src_addr), (_src_mac))

#define IGMP3_SEND_BLOCK(_pco, _csap, _group_addr, _src_addr, _src_mac, _addr1,...) \
    IGMP3_SEND_REPORT((_pco), (_csap),              \
        IGMP3_GROUP_LIST(NULL,                      \
            IGMP3_GROUP_RECORD(NULL,                \
                IGMPV3_BLOCK_OLD_SOURCES,           \
                (_group_addr), NULL, 0, _addr1, 0), \
            NULL),                                  \
        (_src_addr), (_src_mac))


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_IGMP2_H__ */
