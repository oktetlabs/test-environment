/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Proteos, TAD file protocol, NDN.
 *
 * Declarations of ASN.1 types for NDN for Ethernet protocol.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_NDN_ETH_H__
#define __TE_NDN_ETH_H__

#include "te_stdint.h"
#include "te_ethernet.h"
#include "asn_usr.h"
#include "ndn.h"


#define NDN_ETH_VLAN_TCI_MASK_PRIO 0xe000
#define NDN_ETH_VLAN_TCI_MASK_CFI 0x1000
#define NDN_ETH_VLAN_TCI_MASK_ID 0xfff

#ifdef __cplusplus
extern "C" {
#endif


/* Structure for Ethernet frame header. written according to IEEE 802.3 */
typedef struct ndn_eth_header_plain {

    uint8_t  dst_addr[ETHER_ADDR_LEN];  /**< Destination MAC address */
    uint8_t  src_addr[ETHER_ADDR_LEN];  /**< Source MAC address */

    uint16_t len_type;      /**< Ethernet Length/Type */
    int      is_tagged;     /**< BOOL, flag - is frame tagged. */
    int      cfi;           /**< BOOL, Canonical Format Indicator*/
    uint8_t  priority;      /**< Priority */
    uint16_t vlan_id;       /**< VLAN tag */

} ndn_eth_header_plain;


typedef enum {

    NDN_TAG_ETH_DEVICE,
    NDN_TAG_ETH_RECV_MODE,
    NDN_TAG_ETH_LOCAL,
    NDN_TAG_ETH_REMOTE,

    NDN_TAG_802_3_DST,
    NDN_TAG_802_3_SRC,
    NDN_TAG_802_3_LENGTH_TYPE,
    NDN_TAG_802_3_ETHER_TYPE,

    NDN_TAG_ETH_UNTAGGED,
    NDN_TAG_VLAN_TAGGED,
    NDN_TAG_VLAN_TAG_HEADER,
    NDN_TAG_VLAN_TAG_HEADER_PRIO,
    NDN_TAG_VLAN_TAG_HEADER_CFI,
    NDN_TAG_VLAN_TAG_HEADER_VID,
    NDN_TAG_VLAN_TAG_HEADER_ERIF_RC_RT,
    NDN_TAG_VLAN_TAG_HEADER_ERIF_RC_LTH,
    NDN_TAG_VLAN_TAG_HEADER_ERIF_RC_D,
    NDN_TAG_VLAN_TAG_HEADER_ERIF_RC_LF,
    NDN_TAG_VLAN_TAG_HEADER_ERIF_RC_NCFI,
    NDN_TAG_VLAN_TAG_HEADER_ERIF_RD,
    NDN_TAG_VLAN_DOUBLE_TAGGED,
    NDN_TAG_VLAN_DOUBLE_TAG_HEADER,
    NDN_TAG_VLAN_HEADER_TPID,
    NDN_TAG_VLAN_HEADER_PCP,
    NDN_TAG_VLAN_HEADER_DEI,
    NDN_TAG_VLAN_HEADER_VID,
    NDN_TAG_VLAN_HEADER,
    NDN_TAG_VLAN_HEADER_OUTER,
    NDN_TAG_VLAN_HEADER_INNER,

    NDN_TAG_802_3_ENCAP,
    NDN_TAG_ETHERNET2,

    NDN_TAG_LLC_HEADER,

} ndn_eth_tags_t;


/**
 * Convert Ethernet-Header ASN value to plain C structure.
 *
 * @param pkt           ASN value of type Ethernet Header or
 *                      Generic-PDU with choice "eth".
 * @param eth_header    location for converted structure (OUT).
 *
 * @return zero on success or error code.
 */
extern int ndn_eth_packet_to_plain(const asn_value *pkt,
                                   ndn_eth_header_plain *eth_header);


/**
 * Convert plain C structure to Ethernet-Header ASN value.
 *
 * @param eth_header    structure to be converted.
 *
 * @return pointer to new ASN value object or NULL.
 */
extern asn_value *ndn_eth_plain_to_packet(const ndn_eth_header_plain
                                          *eth_header);


extern const asn_type * const ndn_eth_snap;
extern const asn_type * const ndn_vlan_tagged;
extern const asn_type * const ndn_vlan_tag_header;
extern const asn_type * const ndn_vlan_header;
extern const asn_type * const ndn_vlan_double_tag_header;

extern const asn_type * const ndn_eth_header;
extern const asn_type * const ndn_eth_csap;
extern const asn_type * const ndn_data_unit_eth_address;
extern const asn_type * const ndn_eth_address;

extern asn_type ndn_eth_header_s;
extern asn_type ndn_eth_csap_s;
extern asn_type ndn_data_unit_eth_address_s;
extern asn_type ndn_eth_address_s;
extern asn_type ndn_vlan_header_s;
extern asn_type ndn_vlan_double_tag_header_s;


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_NDN_ETH_H__ */
