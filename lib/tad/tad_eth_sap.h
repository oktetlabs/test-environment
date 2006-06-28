/** @file
 * @brief TAD Ethernet Service Access Point 
 *
 * Declaration of Traffic Application Domain interface to Ethernet.
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAD_ETH_SAP_H__
#define __TE_TAD_ETH_SAP_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "te_stdint.h"
#include "te_ethernet.h"
#include "te_errno.h"

#include "tad_pkt.h"

#ifdef __cplusplus
extern "C" {
#endif

/**< Send modes */
typedef enum tad_eth_sap_send_mode {
    TAD_ETH_SAP_SEND_NORMAL = 0x01, /**< Normal mode of sending */
} tad_eth_sap_send_mode;

/**< Receive modes */
typedef enum tad_eth_sap_recv_mode {
    TAD_ETH_SAP_RECV_HOST  = 0x01,  /**< Receive frames destine to the
                                         host */
    TAD_ETH_SAP_RECV_BCAST = 0x02,  /**< Receive broadcast frames */
    TAD_ETH_SAP_RECV_MCAST = 0x04,  /**< Receive multicast frames */
    TAD_ETH_SAP_RECV_OTHER = 0x08,  /**< Receive frames destine to other
                                         hosts */
    TAD_ETH_SAP_RECV_OUT   = 0x10,  /**< Receive outgoing frames */
} tad_eth_sap_recv_mode;


/** Maximum length of the Ethernet interface (service provider) name */
#define TAD_ETH_SAP_IFNAME_SIZE 256

/** Ethernet service access point data */
typedef struct tad_eth_sap {
    /* Configuration parameters */
    char    name[TAD_ETH_SAP_IFNAME_SIZE];  /**< Name of the interface/
                                                 service */

    /* Ancillary information */
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
 *
 * @param sap           SAP description structure
 * @param mode          Send mode
 *
 * @return Status code.
 *
 * @sa tad_eth_sap_send_close(), tad_eth_sap_recv_open()
 */
extern te_errno tad_eth_sap_send_open(tad_eth_sap           *sap,
                                      tad_eth_sap_send_mode  mode);

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
 *
 * @param sap           SAP description structure
 * @param mode          Receive mode
 *
 * @return Status code.
 *
 * @sa tad_eth_sap_recv_close(), tad_eth_sap_send_open()
 */
extern te_errno tad_eth_sap_recv_open(tad_eth_sap           *sap,
                                      tad_eth_sap_recv_mode  mode);

/**
 * Receive Ethernet frame using service access point opened for
 * receiving.
 *
 * @param sap           SAP description structure
 * @param timeout       Receive operation timeout
 * @param pkt           Frame to be sent
 * @param pkt_len       Location for real packet length
 *
 * @return Status code.
 *
 * @sa tad_eth_sap_recv_open(), tad_eth_sap_send()
 */
extern te_errno tad_eth_sap_recv(tad_eth_sap *sap, unsigned int timeout,
                                 const tad_pkt *pkt, size_t *pkt_len);

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
