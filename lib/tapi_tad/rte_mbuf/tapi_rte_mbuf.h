/** @file
 * @brief TAPI TAD RTE mbuf
 *
 * Declarations of test API for RTE mbuf TAD
 *
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS in
 * the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Ivan Malov <Ivan.Malov@oktetlabs.ru>
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
