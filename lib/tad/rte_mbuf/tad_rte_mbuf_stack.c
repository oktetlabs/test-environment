/** @file
 * @brief TAD RTE mbuf
 *
 * Traffic Application Domain Command Handler
 * RTE mbuf CSAP: stack-related callbacks
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Ivan Malov <Ivan.Malov@oktetlabs.ru>
 */

#define TE_LGR_USER     "TAD RTE mbuf"

#include "te_config.h"

#include "te_alloc.h"
#include "logger_api.h"
#include "logger_ta_fast.h"

#include "tad_rte_mbuf_impl.h"

te_errno
tad_rte_mbuf_rw_init_cb(csap_p  csap)
{
    te_errno                rc;
    unsigned int            layer = csap_get_rw_layer(csap);
    char                    pkt_ring[RTE_RING_NAMESIZE + 1];
    char                    pkt_pool[RTE_MEMPOOL_NAMESIZE + 1];
    size_t                  val_len;
    tad_rte_mbuf_rw_data   *spec_data;
    const asn_value        *rte_mbuf_csap_spec;

    memset(&pkt_ring, 0, RTE_RING_NAMESIZE + 1);
    memset(&pkt_pool, 0, RTE_MEMPOOL_NAMESIZE + 1);

    rte_mbuf_csap_spec = csap->layers[layer].nds;

    val_len = sizeof(pkt_ring);
    rc = asn_read_value_field(rte_mbuf_csap_spec, pkt_ring, &val_len,
                              "pkt-ring");
    if (rc != 0)
    {
        ERROR("'rte-ring-name' for RTE mbuf not found: %r", rc);
        return TE_RC(TE_TAD_CSAP, rc);
    }

    val_len = sizeof(pkt_pool);
    rc = asn_read_value_field(rte_mbuf_csap_spec, pkt_pool, &val_len,
                              "pkt-pool");
    if (rc != 0)
    {
        ERROR("'rte-mempool-name' for RTE mbuf not found: %r", rc);
        return TE_RC(TE_TAD_CSAP, rc);
    }

    /* Sanity checks */
    if (pkt_ring[RTE_RING_NAMESIZE] != '\0')
    {
        ERROR("%s(): Too long RTE ring name", __FUNCTION__);
        return TE_RC(TE_TAD_CSAP, TE_E2BIG);
    }

    if (pkt_pool[RTE_MEMPOOL_NAMESIZE] != '\0')
    {
        ERROR("%s(): Too long RTE mempool name", __FUNCTION__);
        return TE_RC(TE_TAD_CSAP, TE_E2BIG);
    }

    spec_data = TE_ALLOC(sizeof(*spec_data));
    if (spec_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    /* RTE facilities lookup and assignments */
    spec_data->sap.pkt_ring = rte_ring_lookup(pkt_ring);
    if (spec_data->sap.pkt_ring == NULL)
    {
        ERROR("%s(): No RTE ring with the specified name exists", __FUNCTION__);
        return TE_RC(TE_TAD_CSAP, TE_ENOENT);
    }

    spec_data->sap.pkt_pool = rte_mempool_lookup(pkt_pool);
    if (spec_data->sap.pkt_pool == NULL)
    {
        ERROR("%s(): No RTE pool with the specified name exists", __FUNCTION__);
        return TE_RC(TE_TAD_CSAP, TE_ENOENT);
    }

    spec_data->sap.csap = csap;

    csap_set_rw_data(csap, spec_data);

    csap->layers[layer].rw_use_tad_pkt_seg_tagging = TRUE;

    return 0;
}

te_errno
tad_rte_mbuf_rw_destroy_cb(csap_p  csap)
{
    tad_rte_mbuf_rw_data   *spec_data = csap_get_rw_data(csap);

    if (spec_data == NULL)
    {
        WARN("No RTE mbuf CSAP %d special data found!", csap->id);
        return 0;
    }
    csap_set_rw_data(csap, NULL);

    free(spec_data);

    return 0;
}

te_errno
tad_rte_mbuf_read_cb(csap_p          csap,
                     unsigned int    timeout,
                     tad_pkt        *pkt,
                     size_t         *pkt_len)
{
    unsigned                pend = 0;
    te_errno                rc;
    tad_rte_mbuf_rw_data   *spec_data = csap_get_rw_data(csap);

    assert(spec_data != NULL);

    UNUSED(timeout);

    rc = tad_rte_mbuf_sap_read(&spec_data->sap, pkt, pkt_len, &pend);

    if (rc == 0)
        if (pend == 0)
            csap->state |= CSAP_STATE_STOP;

    return rc;
}

te_errno
tad_rte_mbuf_write_cb(csap_p          csap,
                      const tad_pkt  *pkt)
{
    tad_rte_mbuf_rw_data *spec_data = csap_get_rw_data(csap);

    assert(spec_data != NULL);

    return tad_rte_mbuf_sap_write(&spec_data->sap, pkt);
}
