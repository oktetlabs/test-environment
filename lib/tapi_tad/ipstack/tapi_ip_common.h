/** @file
 * @brief Test API for TAD. Common functions for IP CSAP.
 *
 * @defgroup tapi_tad_ip_common Common functions for IPv4 and IPv6
 * @ingroup tapi_tad_ipstack
 * @{
 *
 * Declaration of common functions for IPv4/IPv6 CSAP.
 *
 * Copyright (C) 2019-2022 OKTET Labs. All rights reserved.
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

/** Specification of IPv4 or IPv6 fragment */
typedef struct tapi_ip_frag_spec {
    uint32_t    hdr_offset;     /**< Value for "offset" in IP header */
    uint32_t    real_offset;    /**< Begin of frag data in real payload */
    size_t      hdr_length;     /**< Value for "length" in IP header */
    size_t      real_length;    /**< Length of frag data in real payload */
    te_bool     more_frags;     /**< Value for "more frags" flag */
    te_bool     dont_frag;      /**< Value for "don't frag" flag */
    int64_t     id;             /**< Value for ID field; ignored if
                                     negative */
} tapi_ip_frag_spec;

/**
 * Initialize array of IP fragment specifications.
 *
 * @param frags       Pointer to array.
 * @param num         Number of fragment specifications.
 */
extern void tapi_ip_frag_specs_init(tapi_ip_frag_spec *frags,
                                    unsigned int num);

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

/**
 * Add fragments specification to IPv4 or IPv6 PDU.
 *
 * @param tmpl          @c NULL or location of ASN.1 value with traffic
 *                      template where IP PDU should be added
 * @param pdu           If @p tmpl is @c NULL, this parameter must
 *                      point to IP PDU where to add fragments
 *                      specification; on return, if this parameter is
 *                      not @c NULL, pointer to IP PDU will be saved
 *                      in it
 * @param ipv4          If @c TRUE, IPv4 PDU is processed, otherwise
 *                      IPv6 PDU
 * @param fragments     Array with IP fragments specifications
 * @param num_frags     Number of IP fragments
 *
 * @return Status code.
 */
extern te_errno tapi_ip_pdu_tmpl_fragments(asn_value **tmpl,
                                           asn_value **pdu,
                                           te_bool ipv4,
                                           tapi_ip_frag_spec *fragments,
                                           unsigned int num_frags);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_TAPI_IP_COMMON_H__ */

/**@} <!-- END tapi_tad_ip_common --> */
