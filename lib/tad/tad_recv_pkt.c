/** @file
 * @brief TAD Receiver
 *
 * Traffic Application Domain Command Handler.
 * Implementation of functions to deal with for TAD Receiver packet
 * representation.
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
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

    while ((pkt = TAILQ_FIRST(pkts)) != NULL)
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

    recv_pkt->match_unit = -1;

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


static void
tad_recv_pkt_cleanup_layer(csap_p csap, tad_recv_pkt *pkt,
                           unsigned int layer)
{
    assert(csap != NULL);
    assert(pkt != NULL);
    assert(pkt->layers != NULL);
    assert(layer < csap->depth);

    F_ENTRY(CSAP_LOG_FMT "recv_pkt=%p layer=%u", CSAP_LOG_ARGS(csap),
            pkt, layer);

    if (pkt->nds == NULL)
    {
        asn_free_value(pkt->layers[layer].nds);
        pkt->layers[layer].nds = NULL;
    }

    tad_cleanup_pkts(&pkt->layers[layer].pkts);

    /* TODO: Free opaque */
}

/* See the description in tad_recv_pkt.h */
void
tad_recv_pkt_cleanup_upper(csap_p csap, tad_recv_pkt *pkt)
{
    unsigned int    layer;

    assert(csap != NULL);
    assert(pkt != NULL);

    F_ENTRY(CSAP_LOG_FMT "recv_pkt=%p", CSAP_LOG_ARGS(csap), pkt);

    tad_pkt_cleanup(&pkt->payload);

    assert(csap->depth > 0);
    for (layer = 0; layer < csap->depth - 1; ++layer)
    {
        tad_recv_pkt_cleanup_layer(csap, pkt, layer);
    }
}

/* See the description in tad_recv_pkt.h */
void
tad_recv_pkt_cleanup(csap_p csap, tad_recv_pkt *pkt)
{
    assert(csap != NULL);
    assert(pkt != NULL);

    F_ENTRY(CSAP_LOG_FMT "recv_pkt=%p", CSAP_LOG_ARGS(csap), pkt);

    tad_recv_pkt_cleanup_upper(csap, pkt);

    tad_recv_pkt_cleanup_layer(csap, pkt, csap->depth - 1);

    tad_cleanup_pkts(&pkt->raw);

    asn_free_value(pkt->nds);
    pkt->nds = NULL;
}

