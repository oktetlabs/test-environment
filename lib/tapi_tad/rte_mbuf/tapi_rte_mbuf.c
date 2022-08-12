/** @file
 * @brief TAPI TAD RTE mbuf
 *
 * Implementation of test API for RTE mbuf TAD
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
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
