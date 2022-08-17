/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Proteos, TAD file protocol, NDN.
 *
 * Declarations of ASN.1 types for NDN for Ethernet protocol.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */
#ifndef __TE_NDN_BRIDGE_H__
#define __TE_NDN_BRIDGE_H__


#include "te_stdint.h"
#include "asn_usr.h"
#include "ndn.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ASN.1 tags for Bridge PDUs
 */
typedef enum {
    NDN_BRIDGE_PROTO_ID,
    NDN_BRIDGE_VERSION_ID,
    NDN_BRIDGE_BPDU_TYPE,
    NDN_BRIDGE_CONTENT,
    NDN_BRIDGE_CFG,
    NDN_BRIDGE_TCN,
    NDN_BRIDGE_FLAGS,
    NDN_BRIDGE_ROOT_ID,
    NDN_BRIDGE_PATH_COST,
    NDN_BRIDGE_BRIDGE_ID,
    NDN_BRIDGE_PORT_ID,
    NDN_BRIDGE_MESSAGE_AGE,
    NDN_BRIDGE_MAX_AGE,
    NDN_BRIDGE_HELLO_TIME,
    NDN_BRIDGE_FORWARD_DELAY,
    NDN_BRIDGE_,
} ndn_bridge_tags_t;

/** Structure represents Configuration BPDU */
typedef struct ndn_stp_cfg_bpdu_t {
    uint8_t  flags;          /**< Value of the Flags field */
    uint8_t  root_id[8];     /**< Value of the Root Identifier field */
    uint32_t root_path_cost; /**< Value of the Root Path Cost field */
    uint8_t  bridge_id[8];   /**< Value of the Bridge Identifier field */
    uint16_t port_id;        /**< Value of the Port Identifier field */
    uint16_t msg_age;        /**< Value of the Message Age field */
    uint16_t max_age;        /**< Value of the Max Age field */
    uint16_t hello_time;     /**< Value of the Hello Time field */
    uint16_t fwd_delay;      /**< Value of the Forward Delay field */
} ndn_stp_cfg_bpdu_t;

/**
 * Bit mask for Topology Change Acknowledgement flag
 * See ANSI/IEEE Std. 802.1D, 1998 Edition section 9.3.1
 */
#define CFG_BPDU_TC_ACK_FLAG 0x80

/**
 * Bit mask for Topology Change flag
 * See ANSI/IEEE Std. 802.1D, 1998 Edition section 9.3.1
 */
#define CFG_BPDU_TC_FLAG     0x01

/** Structure represents STP BPDU */
typedef struct ndn_stp_bpdu_t {
    uint8_t version;   /**< Protocol version identifier */
    uint8_t bpdu_type; /**< STP BPDU type - Configuration or Notification */

    union {
        struct ndn_stp_cfg_bpdu_t cfg; /**< Content of Configuration BPDU */
        struct ndn_stp_tcn_bpdu_t {} tcn[0];
                                       /**< Topology Change Notification
                                            BPDU has no content */
    }; /**< BPDU content */
} ndn_stp_bpdu_t;

/**
 * Value of BPDU Type field used in STP Configuration BPDU
 * See ANSI/IEEE Std. 802.1D, 1998 Edition section 9.3.1
 */
#define STP_BPDU_CFG_TYPE 0x00
/**
 * Value of BPDU Type field used in STP Topology Change Notification BPDU
 * See ANSI/IEEE Std. 802.1D, 1998 Edition section 9.3.2
 */
#define STP_BPDU_TCN_TYPE 0x80

/**
 * Convert Bridge STP PDU ASN value to plain C structure.
 *
 * @param pkt           ASN value of type Bridge-PDU or Generic-PDU with
 *                      choice "bridge".
 * @param bpdu          converted structure (OUT).
 *
 * @return zero on success or error code.
 */
extern int ndn_bpdu_asn_to_plain(const asn_value *pkt,
                                 ndn_stp_bpdu_t *bpdu);


/**
 * Convert plain C structure with Bridge STP PDU to respective ASN value.
 *
 * @param bpdu          - structure to be converted.
 *
 * @retval      pointer to created ASN value on success.
 * @retval      NULL on memory allocation error.
 */
extern asn_value * ndn_bpdu_plain_to_asn(const ndn_stp_bpdu_t *bpdu);

extern const asn_type * const ndn_bridge_pdu;
extern const asn_type * const ndn_bridge_csap;

extern asn_type ndn_bridge_pdu_s;
extern asn_type ndn_bridge_csap_s;


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_NDN_BRIDGE_H__ */
