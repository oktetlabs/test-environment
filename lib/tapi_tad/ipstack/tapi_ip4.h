/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief TAPI TAD IPv4
 *
 * @defgroup tapi_tad_ip4 IPv4
 * @ingroup tapi_tad_ipstack
 * @{
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAPI_IP_H__
#define __TE_TAPI_IP_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "te_stdint.h"
#include "te_defs.h"
#include "tad_common.h"
#include "tapi_ip_common.h"
#include "asn_usr.h"
#include "tapi_tad.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tapi_ip4_packet_t {
    in_addr_t   src_addr;
    in_addr_t   dst_addr;
    int         hlen;
    int         len;
    uint8_t     ip_proto;

    uint8_t     ttl;
    uint8_t     tos;

    uint8_t    *payload;
    size_t      pld_len;

    size_t      ip_ident;
    te_bool     more_frags;
    size_t      offset;
} tapi_ip4_packet_t;


/**
 * Add IPv4 layer in CSAP specification.
 *
 * @param csap_spec     Location of CSAP specification pointer.
 * @param local_addr    Default local IPv4 address in network byte order
 *                      or htonl(INADDR_ANY)
 * @param remote_addr   Default remote IPv4 address in network byte order
 *                      or htonl(INADDR_ANY)
 * @param ip_proto      Protocol or negative to keep unspecified.
 * @param ttl           Time-to-live or negative to keep unspecified.
 * @param tos           Type-of-service or negative to keep unspecified.
 *
 * @retval Status code.
 */
extern te_errno tapi_ip4_add_csap_layer(asn_value **csap_spec,
                                        in_addr_t   local_addr,
                                        in_addr_t   remote_addr,
                                        int         ip_proto,
                                        int         ttl,
                                        int         tos);

/**
 * Add IPv4 PDU as the last PDU to the last unit of the traffic
 * template or pattern.
 *
 * @param tmpl_or_ptrn  Location of ASN.1 value with traffic template or
 *                      pattern
 * @param pdu           Location for ASN.1 value pointer with added PDU
 * @param is_pattern    Is the first argument template or pattern
 * @param src_addr      Source IPv4 address in network byte order or
 *                      htonl(INADDR_ANY). If htonl(INADDR_ANY), default
 *                      value is specified during CSAP creation (as
 *                      local address for sending, as remote address for
 *                      receiving).
 * @param dst_addr      Destination IPv4 address in network byte order or
 *                      htonl(INADDR_ANY). If htonl(INADDR_ANY), default
 *                      value is specified during CSAP creation (as
 *                      remote address for sending, as local address for
 *                      receiving).
 * @param ip_proto      Protocol or negative to keep unspecified.
 * @param ttl           Time-to-live or negative to keep unspecified.
 * @param tos           Type-of-service or negative to keep unspecified.
 *
 * @return Status code.
 */
extern te_errno tapi_ip4_add_pdu(asn_value **tmpl_or_ptrn,
                                 asn_value **pdu,
                                 te_bool     is_pattern,
                                 in_addr_t   src_addr,
                                 in_addr_t   dst_addr,
                                 int         ip_proto,
                                 int         ttl,
                                 int         tos);

/**
 * Add fragments specification to IPv4 PDU.
 *
 * @param tmpl          @c NULL or location of ASN.1 value with traffic
 *                      template where IPv4 PDU should be added
 * @param pdu           If @p tmpl is @c NULL, this parameter must
 *                      point to IPv4 PDU where to add fragments
 *                      specification; on return, if this parameter is
 *                      not @c NULL, pointer to IPv4 PDU will be saved
 *                      in it
 * @param fragments     Array with IP fragments specifictaions
 * @param num_frags     Number of IP fragments (if 0, nothing is done)
 *
 * @return Status code.
 */
extern te_errno tapi_ip4_pdu_tmpl_fragments(asn_value         **tmpl,
                                            asn_value         **pdu,
                                            tapi_ip_frag_spec  *fragments,
                                            unsigned int        num_frags);

/**
 * Create 'ip4.eth' CSAP
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param eth_dev       Name of Ethernet interface
 * @param receive_mode  Receive mode for Ethernet Layer on the Interface
 * @param loc_mac_addr  Local MAC address (or NULL)
 * @param rem_mac_addr  Remote MAC address (or NULL)
 * @param loc_ip4_addr  Local IPv4 address in network order or
 *                      htonl(INADDR_ANY)
 * @param rem_ip4_addr  Remote IPv4 address in network order or
 *                      htonl(INADDR_ANY)
 * @param proto         Protocol over IPv4 or negative number
 * @param ip4_csap      Location for the IPv4 CSAP handle (OUT)
 *
 * @return Status code.
 */
extern te_errno tapi_ip4_eth_csap_create(const char    *ta_name,
                                         int            sid,
                                         const char    *eth_dev,
                                         unsigned int   receive_mode,
                                         const uint8_t *loc_mac_addr,
                                         const uint8_t *rem_mac_addr,
                                         in_addr_t      loc_ip4_addr,
                                         in_addr_t      rem_ip4_addr,
                                         int            ip_proto,
                                         csap_handle_t *ip4_csap);

/**
 * Creates 'ip4' CSAP
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param loc_ip4_addr  Local IPv4 address in network order or
 *                      htonl(INADDR_ANY)
 * @param rem_ip4_addr  Remote IPv4 address in network order or
 *                      htonl(INADDR_ANY)
 * @param proto         Protocol over IPv4 or negative number
 * @param ip4_csap      Location for the IPv4 CSAP handle (OUT)
 *
 * @return Status code.
 */
extern te_errno tapi_ip4_csap_create(const char    *ta_name,
                                     int            sid,
                                     in_addr_t      loc_ip4_addr,
                                     in_addr_t      rem_ip4_addr,
                                     int            ip_proto,
                                     csap_handle_t *ip4_csap);

/**
 * Callback function for the receiving IP datagrams.
 *
 * Neither pkt, nor pkt->payload MAY NOT be stored for future use
 * by this callback: they are freed just after callback return.
 *
 * @param pkt           Received IP packet.
 * @param userdata      Parameter, provided by the caller.
 */
typedef void (*ip4_callback)(const tapi_ip4_packet_t *pkt, void *userdata);

extern tapi_tad_trrecv_cb_data *tapi_ip4_eth_trrecv_cb_data(
                                    ip4_callback  callback,
                                    void         *user_data);



/**
 * Prepare ASN Traffic-Template value for CSAP with 'ip4' layer.
 *
 * @param fragments     array with IP fragments specifictaions, or NULL;
 * @param num_frags     number of IP fragments;
 * @param ttl           time-to-live field, or negative for CSAP default;
 * @param protocol      protocol field, or negative for CSAP default;
 * @param payload       payload of IPv4 packet, or NULL;
 * @param pld_len       length of payload;
 * @param result_value  location for pointer to new ASN value (OUT);
 *
 * @return status code
 *
 * @deprecated Avoid usage of this function, since it should be removed
 *             in the future.
 */
extern te_errno tapi_ip4_template(tapi_ip_frag_spec *fragments,
                                  unsigned int num_frags,
                                  int ttl, int protocol,
                                  const uint8_t *payload,
                                  size_t pld_len,
                                  asn_value **result_value);


/**
 * Compute IPv4 packet payload length from IP header length fields.
 *
 * @param pdu         IPv4 PDU.
 * @param len         Where to save computed length.
 *
 * @return Status code.
 */
extern te_errno tapi_ip4_get_payload_len(asn_value *pdu, size_t *len);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_IP_H__ */

/**@} <!-- END tapi_tad_ip4 --> */
