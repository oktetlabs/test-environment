/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief TAPI TAD ARP
 *
 * @defgroup tapi_tad_arp ARP
 * @ingroup tapi_tad_main
 * @{
 *
 * Declarations of test API for ARP TAD.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAPI_ARP_H__
#define __TE_TAPI_ARP_H__

#include <stdio.h>
#if HAVE_ASSERT_H
#include <assert.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif
#if HAVE_NET_IF_ARP_H
#include <net/if_arp.h>
#endif

#include "te_stdint.h"
#include "te_defs.h"
#include "tad_common.h"
#include "asn_usr.h"
#include "ndn_eth.h"
#include "ndn_arp.h"
#include "tapi_tad.h"


/** Structure that represents ARP frame: Ethernet header and ARP header */
typedef struct tapi_arp_frame {
    ndn_eth_header_plain  eth_hdr;  /**< Ethernet header */
    ndn_arp_header_plain  arp_hdr;  /**< ARP header */

    uint32_t              data_len; /**< Data length */
    uint8_t              *data;     /**< The data that goes after
                                         ARP header */
} tapi_arp_frame_t;

/**
 * Fills in Ethernet header of ARP frame with 802.3 source and destination
 * MAC addresses.
 *
 * @param arp_frame_  ARP frame to be updated
 * @param src_mac_    Source MAC address
 * @param dst_mac_    Destination MAC address
 */
#define TAPI_ARP_FILL_ETH_HDR(arp_frame_, src_mac_, dst_mac_) \
    do {                                                                  \
        memset(&((arp_frame_)->eth_hdr), 0,                               \
                sizeof((arp_frame_)->eth_hdr));                           \
        assert(sizeof((arp_frame_)->eth_hdr.src_addr) == ETHER_ADDR_LEN); \
        memcpy((arp_frame_)->eth_hdr.src_addr, src_mac_, ETHER_ADDR_LEN); \
        assert(sizeof((arp_frame_)->eth_hdr.dst_addr) == ETHER_ADDR_LEN); \
        memcpy((arp_frame_)->eth_hdr.dst_addr, dst_mac_, ETHER_ADDR_LEN); \
        (arp_frame_)->eth_hdr.len_type = ETHERTYPE_ARP;                   \
        (arp_frame_)->eth_hdr.is_tagged = FALSE;                          \
    } while (0)

/**
 * Fills in ARP frame header
 *
 * @param  arp_frame_    structure storing arp frame
 * @param  op_           operation type
 * @param  snd_hw_       sender hardware address
 * @param  snd_proto_    sender protocol address
 * @param  tgt_hw_       target hardware address
 * @param  tgt_proto_    target protocol address
 *
 */
#define TAPI_ARP_FILL_HDR(arp_frame_, op_, \
                          snd_hw_, snd_proto_, tgt_hw_, tgt_proto_) \
    do {                                                                \
        memset(&((arp_frame_)->arp_hdr), 0,                             \
               sizeof((arp_frame_)->arp_hdr));                          \
        (arp_frame_)->arp_hdr.hw_type = ARPHRD_ETHER;                   \
        (arp_frame_)->arp_hdr.proto_type = ETHERTYPE_IP;                \
        (arp_frame_)->arp_hdr.hw_size = ETHER_ADDR_LEN;                 \
        (arp_frame_)->arp_hdr.proto_size = sizeof(struct in_addr);      \
        (arp_frame_)->arp_hdr.opcode = (op_);                           \
        if ((snd_hw_) != NULL)                                          \
            memcpy((arp_frame_)->arp_hdr.snd_hw_addr, snd_hw_,          \
                   ETHER_ADDR_LEN);                                     \
        if ((snd_proto_) != NULL)                                       \
            memcpy((arp_frame_)->arp_hdr.snd_proto_addr, snd_proto_,    \
                   sizeof(struct in_addr));                             \
        if ((tgt_hw_) != NULL)                                          \
            memcpy((arp_frame_)->arp_hdr.tgt_hw_addr, tgt_hw_,          \
                   ETHER_ADDR_LEN);                                     \
        if ((tgt_proto_) != NULL)                                       \
            memcpy((arp_frame_)->arp_hdr.tgt_proto_addr, tgt_proto_,    \
                   sizeof(struct in_addr));                             \
                                                                        \
        (arp_frame_)->data = NULL;                                      \
        (arp_frame_)->data_len = 0;                                     \
    } while (0)


/**
 * Create ARP CSAP that runs over Ethernet RFC 894.
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF session
 * @param device        interface name on TA host
 * @param receive_mode  Receive mode for Ethernet CSAP on the Interface
 * @param remote_addr   default remote MAC address, may be NULL - in this
 *                      case frames will be sent only if dst is specified in
 *                      template, and frames from all src's will be catched.
 *                      If NULL, CSAP will have remote address unconfigured
 *                      and will require it in traffic template.
 * @param local_addr    default local MAC address. may be NULL - in this
 *                      case frames will be sent with src specifed in
 *                      template or native for outgoing device (if not
 *                      present in template).
 * @param hw_type       Pointer to ARP header hardware type or NULL
 * @param proto_type    Pointer to ARP header protocol type or NULL
 * @param hw_size       Pointer to ARP header hardware address size or NULL
 * @param proto_size    Pointer to ARP header protocol address size or NULL
 * @param arp_csap      Identifier of created CSAP (OUT)
 *
 * @return zero on success, otherwise standard or common TE error code.
 */
extern te_errno tapi_arp_eth_csap_create(const char     *ta_name,
                                         int             sid,
                                         const char     *device,
                                         unsigned int    receive_mode,
                                         const uint8_t  *remote_addr,
                                         const uint8_t  *local_addr,
                                         const uint16_t *hw_type,
                                         const uint16_t *proto_type,
                                         const uint8_t  *hw_size,
                                         const uint8_t  *proto_size,
                                         csap_handle_t  *arp_csap);

/**
 * Create 'arp.eth' CSAP to deal with IPv4 over Ethernet.
 *
 * See tapi_arp_eth_csap_create() for details.
 */
static inline te_errno
tapi_arp_eth_csap_create_ip4(const char     *ta_name,
                             int             sid,
                             const char     *device,
                             unsigned int    receive_mode,
                             const uint8_t  *remote_addr,
                             const uint8_t  *local_addr,
                             csap_handle_t  *arp_csap)
{
    uint16_t    hw_type = ARPHRD_ETHER;
    uint16_t    proto_type = ETHERTYPE_IP;
    uint8_t     hw_size = ETHER_ADDR_LEN;
    uint8_t     proto_size = sizeof(in_addr_t);

    return tapi_arp_eth_csap_create(ta_name, sid, device,
                                    receive_mode,
                                    remote_addr, local_addr,
                                    &hw_type, &proto_type,
                                    &hw_size, &proto_size,
                                    arp_csap);
}

/**
 * Add Ethernet layer for ARP protocol in CSAP specification.
 *
 * @param csap_spec     Location of CSAP specification pointer.
 * @param device        Interface name on TA host or NULL (have to
 *                      be not-NULL, if Ethernet is the bottom layer)
 * @param receive_mode  Receive mode for Ethernet CSAP on the Interface
 * @param remote_addr   Default remote MAC address, may be NULL - in this
 *                      case frames will be sent only dst is specified in
 *                      template, and frames from all src's will be
 *                      catched.
 * @param local_addr    Default local MAC address, may be NULL - in this
 *                      case frames will be sent with src specifed in
 *                      template or native for outgoing device (if not
 *                      present in template), frames to all dst's will
 *                      be caugth.
 *
 * @retval Status code.
 *
 * @sa tapi_eth_add_csap_layer, tapi_eth_add_csap_layer_tagged
 */
extern te_errno tapi_arp_add_csap_layer_eth(asn_value      **csap_spec,
                                            const char      *device,
                                            unsigned int     receive_mode,
                                            const uint8_t   *remote_addr,
                                            const uint8_t   *local_addr);

/**
 * Add ARP layer in CSAP specification.
 *
 * @param csap_spec     Location of CSAP specification pointer.
 * @param hw_type       Pointer to Hardware Type or NULL. If NULL, it
 *                      have to be specified in traffic templates and
 *                      match any, if it is not specified in traffic
 *                      pattern.
 * @param proto_type    Pointer to Protocol Type or NULL. If NULL, it
 *                      have to be specified in traffic templates and
 *                      match any, if it is not specified in traffic
 *                      pattern.
 * @param hw_size       Pointer to Hardware Address Length or NULL.
 *                      If NULL, it have to be specified in traffic
 *                      templates and match any, if it is not
 *                      specified in traffic pattern.
 * @param proto_size    Pointer to Protocol Address Length or NULL.
 *                      If NULL, it have to be specified in traffic
 *                      templates and match any, if it is not
 *                      specified in traffic pattern.
 *
 * @retval Status code.
 *
 * @sa tapi_arp_add_csap_layer_eth_ip4
 */
extern te_errno tapi_arp_add_csap_layer(asn_value      **csap_spec,
                                        const uint16_t  *hw_type,
                                        const uint16_t  *proto_type,
                                        const uint8_t   *hw_size,
                                        const uint8_t   *proto_size);

/**
 * Add ARP layer for IPv4 over Ethernet in CSAP specification.
 *
 * @param csap_spec     Location of CSAP specification pointer.
 *
 * @retval Status code.
 *
 * @sa tapi_arp_add_csap_layer
 */
extern te_errno tapi_arp_add_csap_layer_eth_ip4(asn_value **csap_spec);


/**
 * Add ARP PDU as the last PDU to the last unit of the traffic
 * template or pattern.
 *
 * @param tmpl_or_ptrn  Location of ASN.1 value with traffic template or
 *                      pattern
 * @param is_pattern    Is the first argument template or pattern
 * @param hw_type       Pointer to Hardware Type field value or NULL
 *                      (default value for template have to be specified
 *                      during CSAP creation)
 * @param proto_type    Pointer to Protocol Type field value or NULL
 *                      (default value for template have to be specified
 *                      during CSAP creation)
 * @param hw_size       Pointer to Hardware Address Length field value or
 *                      NULL (default value for template have to be
 *                      specified during CSAP creation)
 * @param proto_size    Pointer to Protocol Address Length field value or
 *                      NULL (default value for template have to be
 *                      specified during CSAP creation)
 * @param opcode        Pointer to OpCode field value or NULL (cannot be
 *                      used in the case of template, match any on
 *                      receive)
 *
 * @param hw_addr_len       Sender/Target hardware address real length
 * @param proto_addr_len    Sender/Target protocol address real length
 * @param snd_hw_addr       Sender hardware address or NULL (cannot be
 *                          used on transmit, match any on receive)
 * @param snd_proto_addr    Sender protocol address or NULL (cannot be
 *                          used on transmit, match any on receive)
 * @param tgt_hw_addr       Target hardware address or NULL (cannot be
 *                          used on transmit, match any on receive)
 * @param tgt_proto_addr    Target protocol address or NULL (cannot be
 *                          used on transmit, match any on receive)
 *
 * @return Status code.
 *
 * @sa tapi_arp_add_csap_layer, tapi_arp_add_pdu_eth_ip4
 */
extern te_errno tapi_arp_add_pdu(asn_value      **tmpl_or_ptrn,
                                 te_bool          is_pattern,
                                 const uint16_t  *hw_type,
                                 const uint16_t  *proto_type,
                                 const uint8_t   *hw_size,
                                 const uint8_t   *proto_size,
                                 const uint16_t  *opcode,
                                 size_t           hw_addr_len,
                                 size_t           proto_addr_len,
                                 const uint8_t   *snd_hw_addr,
                                 const uint8_t   *snd_proto_addr,
                                 const uint8_t   *tgt_hw_addr,
                                 const uint8_t   *tgt_proto_addr);

/**
 * Add ARP PDU for IPv4 over Ethernet as the last PDU to the last unit
 * of the traffic template or pattern.
 *
 * It is assumed that CSAP is created using
 * tapi_arp_add_csap_layer_eth_ip4() function, length of hardware
 * address is equal to ETHER_ADDR_LEN, length of protocol address is
 * equal to sizeof(in_addr_t).
 *
 * @param tmpl_or_ptrn      Location of ASN.1 value with traffic template
 *                          or pattern
 * @param is_pattern        Is the first argument template or pattern
 * @param opcode            Pointer to OpCode field value or NULL (cannot
 *                          be used in the case of template, match any on
 *                          receive)
 * @param snd_hw_addr       Sender hardware address or NULL (cannot be
 *                          used on transmit, match any on receive)
 * @param snd_proto_addr    Sender protocol address or NULL (cannot be
 *                          used on transmit, match any on receive)
 * @param tgt_hw_addr       Target hardware address or NULL (cannot be
 *                          used on transmit, match any on receive)
 * @param tgt_proto_addr    Target protocol address or NULL (cannot be
 *                          used on transmit, match any on receive)
 *
 * @return Status code.
 *
 * @sa tapi_arp_add_csap_layer_eth_ip4, tapi_arp_add_pdu
 */
extern te_errno tapi_arp_add_pdu_eth_ip4(asn_value      **tmpl_or_ptrn,
                                         te_bool          is_pattern,
                                         const uint16_t  *opcode,
                                         const uint8_t   *snd_hw_addr,
                                         const uint8_t   *snd_proto_addr,
                                         const uint8_t   *tgt_hw_addr,
                                         const uint8_t   *tgt_proto_addr);


/**
 * Creates traffic template for a single ARP frame.
 *
 * @param frame  ARP frame data structure as the source of ARP frame values
 * @param templ  Placeholder for ARP template (OUT)
 *
 * @returns Status code.
 */
extern te_errno tapi_arp_prepare_template(const tapi_arp_frame_t *frame,
                                          asn_value **templ);


/**
 * Callback function for the tapi_arp_recv_start() routine, it is called
 * for each packet received on CSAP.
 *
 * @param header        Structure with ARP and Ethernet header of the frame
 * @param payload       Payload of the frame
 * @param plen          Length of the frame payload
 * @param userdata      Pointer to user data, provided by  the caller of
 *                      tapi_arp_recv_start
 */
typedef void (*tapi_arp_frame_callback)(const tapi_arp_frame_t *header,
                                        void *userdata);

/**
 * Prepare callback data to be passed in tapi_tad_trrecv_{wait,stop,get}
 * to process received ARP frames.
 *
 * @param callback        Callback for ARP frames handling
 * @param user_data       User-supplied data to be passed to @a callback
 *
 * @return Pointer to allocated callback data or NULL.
 */
extern tapi_tad_trrecv_cb_data *tapi_arp_trrecv_cb_data(
                                    tapi_arp_frame_callback  callback,
                                    void                    *user_data);


/**
 * Receives specified number of ARP frames matched with the pattern.
 * The function blocks the caller until all the frames are received or
 * timeout occurred.
 *
 * @param ta_name    Test Agent name
 * @param sid        RCF session
 * @param arp_csap   CSAP handle
 * @param pattern    ASN value with receive pattern
 * @param timeout    Timeout for receiving of packets, measured in
 *                   milliseconds
 * @param frames     Pointer to the array of packets (OUT)
 *                   the function allocates memory under the packets that
 *                   should be freed with free() function
 * @param num        Number of packets caller wants to receive (IN)
 *                   number of received packets (OUT)
 *
 * @return zero on success, otherwise standard or common TE error code.
 */
extern te_errno tapi_arp_recv(const char *ta_name, int sid,
                              csap_handle_t arp_csap,
                              const asn_value *pattern,
                              unsigned int timeout,
                              tapi_arp_frame_t **frames,
                              unsigned int *num);


#endif /* __TE_TAPI_ARP_H__ */

/**@} <!-- END tapi_tad_arp --> */
