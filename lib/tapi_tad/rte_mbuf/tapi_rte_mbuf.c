/** @file
 * @brief TAPI TAD RTE mbuf
 *
 * Implementation of test API for RTE mbuf TAD
 *
 *
 * Copyright (C) 2003-2006 Test Environment authors (see file AUTHORS in
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

#define TE_LGR_USER     "TAPI RTE mbuf"

#include "te_config.h"

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "logger_api.h"
#include "asn_usr.h"
#include "ndn_rte_mbuf.h"
#include "tapi_tad.h"
#include "tapi_ndn.h"
#include "tapi_rte_mbuf.h"

#include "tapi_test.h"

te_errno
tapi_rte_mbuf_add_csap_layer(asn_value **csap_spec,
                             const char *pkt_ring,
                             const char *pkt_pool)
{
    asn_value  *layer;

    CHECK_RC(tapi_tad_csap_add_layer(csap_spec, ndn_rte_mbuf_csap,
                                     "#rtembuf", &layer));

    if (pkt_ring != NULL)
        CHECK_RC(asn_write_string(layer, pkt_ring, "pkt-ring.#plain"));

    if (pkt_pool != NULL)
        CHECK_RC(asn_write_string(layer, pkt_pool, "pkt-pool.#plain"));

    return 0;
}
