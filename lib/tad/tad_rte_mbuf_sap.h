/** @file
 * @brief TAD RTE mbuf Service Access Point
 *
 * Declaration of Traffic Application Domain RTE mbuf interface
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 */

#ifndef __TE_TAD_RTE_MBUF_SAP_H__
#define __TE_TAD_RTE_MBUF_SAP_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "te_stdint.h"
#include "te_errno.h"

#include "tad_types.h"
#include "tad_pkt.h"

#include "rte_config.h"
#include "rte_ring.h"
#include "rte_mempool.h"
#include "rte_mbuf.h"

#ifdef __cplusplus
extern "C" {
#endif

/** RTE mbuf service access point data */
typedef struct tad_rte_mbuf_sap {
    /* Main SAP parameters */
    struct rte_ring    *pkt_ring;  /**< RTE ring to store the queue of mbufs */
    struct rte_mempool *pkt_pool;  /**< RTE mempool to store mbufs */

    /* Ancillary information */
    csap_p  csap;                  /**< CSAP handle */
} tad_rte_mbuf_sap;

/**
 * Take RTE mbuf from RTE ring and convert it to TAD packet
 *
 * @param            sap           SAP description structure
 * @param[in,out]    pkt           TAD packet to be produced
 * @param[out]       pkt_len       Location for TAD packet length
 * @param[out]       pend          Location for the residual of RTE ring entries
 *
 * @return Status code
 */
te_errno tad_rte_mbuf_sap_read(tad_rte_mbuf_sap   *sap,
                               tad_pkt            *pkt,
                               size_t             *pkt_len,
                               unsigned           *pend);

/**
 * Convert TAD packet to RTE mbuf and put the latter into RTE ring
 *
 * @param sap           SAP description structure
 * @param pkt           Packet to be converted
 *
 * @return Status code
 */
extern te_errno tad_rte_mbuf_sap_write(tad_rte_mbuf_sap   *sap,
                                       const tad_pkt      *pkt);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAD_RTE_MBUF_SAP_H__ */
