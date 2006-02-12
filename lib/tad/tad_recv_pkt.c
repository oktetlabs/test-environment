/** @file
 * @brief TAD Receiver
 *
 * Traffic Application Domain Command Handler.
 * Implementation of functions to deal with for TAD Receiver packet
 * representation.
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

#define TE_LGR_USER     "TAD Recv Pkt"

#include "te_config.h"

#include "logger_api.h"
#include "logger_ta_fast.h"
#include "asn_usr.h"

#include "tad_pkt.h"
#include "tad_recv_pkt.h"
#include "tad_csap_inst.h"
#include "tad_utils.h"


/* See the description in tad_recv_pkt.h */
void
tad_recv_pkt_free(csap_p csap, tad_recv_pkt *pkt)
{
    unsigned int    layer;

    if (pkt == NULL)
        return;

    assert(csap != NULL);

    F_ENTRY(CSAP_LOG_FMT "recv_pkt=%p", CSAP_LOG_ARGS(csap), pkt);

    tad_pkt_free(&pkt->payload);

    if (pkt->layers != NULL)
    {
        for (layer = 0; layer < csap->depth; ++layer)
        {
            csap_layer_release_opaque_cb_t  cb =
                csap_get_proto_support(csap, layer)->match_free_cb;

            if (pkt->nds == NULL)
                asn_free_value(pkt->layers[layer].nds);

            tad_free_pkts(&pkt->layers[layer].pkts);

            if (cb != NULL)
                cb(csap, layer, pkt->layers[layer].opaque);
        }
        free(pkt->layers);
    }

    tad_free_pkts(&pkt->raw);
    asn_free_value(pkt->nds);
    free(pkt);
}

/* See the description in tad_recv_pkt.h */
void
tad_recv_pkts_free(csap_p csap, tad_recv_pkts *pkts)
{
    tad_recv_pkt *pkt;

    assert(pkts != NULL);

    F_ENTRY(CSAP_LOG_FMT "recv_pkts=%p", CSAP_LOG_ARGS(csap), pkts);

    while ((pkt = pkts->tqh_first) != NULL)
    {
        TAILQ_REMOVE(pkts, pkt, links);
        tad_recv_pkt_free(csap, pkt);
    }
}

/* See the description in tad_recv_pkt.h */
tad_recv_pkt *
tad_recv_pkt_alloc(csap_p csap)
{
    tad_recv_pkt   *recv_pkt;
    unsigned int    layer;
    te_errno        rc = 0;
    tad_pkt        *pkt;

    recv_pkt = calloc(1, sizeof(*recv_pkt));
    if (recv_pkt == NULL)
        return NULL;

    tad_pkt_init(&recv_pkt->payload, NULL, NULL, NULL);

    tad_pkts_init(&recv_pkt->raw);
    pkt = tad_pkt_alloc(0, 0);
    if (pkt == NULL)
    {
        tad_recv_pkt_free(csap, recv_pkt);
        return NULL;
    }
    tad_pkts_add_one(&recv_pkt->raw, pkt);

    recv_pkt->layers = calloc(csap->depth, sizeof(*recv_pkt->layers));
    if (recv_pkt->layers == NULL)
    {
        tad_recv_pkt_free(csap, recv_pkt);
        return NULL;
    }

    for (layer = 0; layer < csap->depth; ++layer)
    {
        tad_pkts_init(&recv_pkt->layers[layer].pkts);
        if (rc == 0)
        {
            csap_layer_match_pre_cb_t   cb =
                csap_get_proto_support(csap, layer)->match_pre_cb;

            if (cb != NULL)
                rc = cb(csap, layer, recv_pkt->layers + layer);
                /* 'pkts' of all layers have to be initialized */

            if (rc == 0)
            {
                pkt = tad_pkt_alloc(0, 0);
                if (pkt == NULL)
                {
                    rc = TE_RC(TE_TAD_PKT, TE_ENOMEM);
                }
                else
                {
                    tad_pkts_add_one(&recv_pkt->layers[layer].pkts, pkt);
                }
            }
        }
    }

    if (rc != 0)
    {
        tad_recv_pkt_free(csap, recv_pkt);
        return NULL;
    }

    F_EXIT(CSAP_LOG_FMT "recv_pkt=%p", CSAP_LOG_ARGS(csap), recv_pkt);

    return recv_pkt;
}

/* See the description in tad_recv_pkt.h */
void
tad_recv_pkt_cleanup(csap_p csap, tad_recv_pkt *pkt)
{
    unsigned int    layer;

    assert(csap != NULL);
    assert(pkt != NULL);
    assert(pkt->layers != NULL);

    F_ENTRY(CSAP_LOG_FMT "recv_pkt=%p", CSAP_LOG_ARGS(csap), pkt);

    tad_pkt_cleanup(&pkt->payload);

    for (layer = 0; layer < csap->depth; ++layer)
    {
        if (pkt->nds == NULL)
        {
            asn_free_value(pkt->layers[layer].nds);
            pkt->layers[layer].nds = NULL;
        }

        tad_cleanup_pkts(&pkt->layers[layer].pkts);

        /* TODO: Free opaque */
    }

    tad_cleanup_pkts(&pkt->raw);

    asn_free_value(pkt->nds);
    pkt->nds = NULL;
}

