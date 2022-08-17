/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief TAD Ethernet Service Access Point
 *
 * Declaration of Traffic Application Domain interface to Ethernet.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAD_ETH_SAP_H__
#define __TE_TAD_ETH_SAP_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "te_stdint.h"
#include "te_ethernet.h"
#include "te_errno.h"

#include "tad_types.h"
#include "tad_pkt.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Auxiliary structure to represent VLAN tag */
struct tad_vlan_tag {
    uint16_t vlan_tpid; /**< Tag protocol ID (network byte order) */
    uint16_t vlan_tci;  /**< VLAN TCI (network byte order) */
};

/** Length of VLAN tag */
#define TAD_VLAN_TAG_LEN 4

/** Maximum length of the Ethernet interface (service provider) name */
#define TAD_ETH_SAP_IFNAME_SIZE 256

/** Ethernet service access point data */
typedef struct tad_eth_sap {
    /* Configuration parameters */
    char    name[TAD_ETH_SAP_IFNAME_SIZE];  /**< Name of the interface/
                                                 service */

    /* Ancillary information */
    csap_p  csap;                           /**< CSAP handle */
    uint8_t addr[ETHER_ADDR_LEN];           /**< Local address */

    void   *data;   /**< Provider-specific data */

} tad_eth_sap;


/**
 * Attach Ethernet service access point to provider and extract
 * ancillary information. SAP is neigher sending nor receiving after
 * attach.
 *
 * @note It is assumed that ancillary information is constant and will
 *       not be modified before close.
 *
 * @param ifname        Name of the interface/service
 * @param sap           Location for SAP description structure
 *
 * @return Status code.
 *
 * @sa tad_eth_sap_send_open(), tad_eth_sap_recv_open(),
 *     tad_eth_sap_detach()
 */
extern te_errno tad_eth_sap_attach(const char *ifname, tad_eth_sap *sap);

/**
 * Open Ethernet service access point for sending.
 * The function does nothing, if Ethernet service access point has
 * already been open for sending.
 *
 * @param sap           SAP description structure
 * @param mode          Send mode
 *
 * @return Status code.
 *
 * @sa tad_eth_sap_send_close(), tad_eth_sap_recv_open()
 */
extern te_errno tad_eth_sap_send_open(tad_eth_sap  *sap,
                                      unsigned int  mode);

/**
 * Send Ethernet frame using service access point opened for sending.
 *
 * @param sap           SAP description structure
 * @param pkt           Frame to be sent
 *
 * @return Status code.
 *
 * @sa tad_eth_sap_send_open(), tad_eth_sap_recv()
 */
extern te_errno tad_eth_sap_send(tad_eth_sap *sap, const tad_pkt *pkt);

/**
 * Close Ethernet service access point for sending.
 *
 * @param sap           SAP description structure
 *
 * @return Status code.
 */
extern te_errno tad_eth_sap_send_close(tad_eth_sap *sap);

/**
 * Open Ethernet service access point for receiving.
 * The function does nothing, if Ethernet service access point has
 * already been open for receiving.
 *
 * @param sap           SAP description structure
 * @param mode          Receive mode (see enum tad_eth_recv_mode in
 *                      ndn_eth.h)
 *
 * @return Status code.
 *
 * @sa tad_eth_sap_recv_close(), tad_eth_sap_send_open()
 */
extern te_errno tad_eth_sap_recv_open(tad_eth_sap  *sap,
                                      unsigned int  mode);

/**
 * Receive Ethernet frame using service access point opened for
 * receiving.
 *
 * @param sap           SAP description structure
 * @param timeout       Receive operation timeout
 * @param pkt           Frame to receive
 * @param pkt_len       Location for real packet length
 *
 * @return Status code.
 *
 * @sa tad_eth_sap_recv_open(), tad_eth_sap_send()
 */
extern te_errno tad_eth_sap_recv(tad_eth_sap *sap, unsigned int timeout,
                                 tad_pkt *pkt, size_t *pkt_len);

/**
 * Close Ethernet service access point for receiving.
 *
 * @param sap           SAP description structure
 *
 * @return Status code.
 */
extern te_errno tad_eth_sap_recv_close(tad_eth_sap *sap);

/**
 * Detach Ethernet service access point from service provider and
 * free all allocated resources.
 *
 * @param sap           SAP description structure
 *
 * @return Status code.
 */
extern te_errno tad_eth_sap_detach(tad_eth_sap *sap);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAD_ETH_SAP_H__ */
