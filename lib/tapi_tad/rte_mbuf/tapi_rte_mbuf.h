/** @file
 * @brief TAPI TAD RTE mbuf
 *
 * @defgroup tapi_tad_rte_mbuf RTE mbuf
 * @ingroup tapi_tad_main
 * @{
 *
 * Declarations of test API for RTE mbuf TAD.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAPI_RTE_MBUF_H__
#define __TE_TAPI_RTE_MBUF_H__

#include "te_defs.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "tad_common.h"
#include "asn_usr.h"
#include "ndn_rte_mbuf.h"
#include "tapi_tad.h"

/**
 * Add RTE mbuf pseudo-layer to CSAP specification
 *
 * @param[out] csap_spec     Location of CSAP specification pointer
 * @param      pkt_ring      Name of RTE ring to be used by CSAP to enqueue and
 *                           dequeue packets in RTE mbuf representation
 * @param      pkt_pool      Name of RTE mempool to be used by CSAP on write
 *                           for allocation of buffers in order to represent
 *                           TAD packets as RTE mbuf chains
 *
 * @retval Status code
 */
extern te_errno tapi_rte_mbuf_add_csap_layer(asn_value      **csap_spec,
                                             const char      *pkt_ring,
                                             const char      *pkt_pool);

#endif /* __TE_TAPI_RTE_MBUF_H__ */

/**@} <!-- END tapi_tad_rte_mbuf --> */
