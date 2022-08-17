/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief TAD IP Stack
 *
 * Traffic Application Domain Command Handler.
 * UDP CSAP layer-related callbacks.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "TAD UDP"

#include "te_config.h"

#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif

#include "te_defs.h"
#include "te_alloc.h"
#include "logger_api.h"
#include "logger_ta_fast.h"

#include "tad_bps.h"
#include "tad_ipstack_impl.h"

/**
 * UDP layer specific data
 */
typedef struct tad_udp_proto_data {
    tad_bps_pkt_frag_def    hdr;
} tad_udp_proto_data;

/**
 * UDP layer specific data for PDU processing (both send and
 * receive).
 */
typedef struct tad_udp_proto_pdu_data {
    tad_bps_pkt_frag_data   hdr;
} tad_udp_proto_pdu_data;


/**
 * Definition of User Datagram Protocol (UDP) header.
 */
static const tad_bps_pkt_frag tad_udp_bps_hdr[] =
{
    { "src-port", 16, NDN_TAG_UDP_SRC_PORT,
      NDN_TAG_UDP_LOCAL_PORT, NDN_TAG_UDP_REMOTE_PORT, 0,
      TAD_DU_I32, FALSE },
    { "dst-port", 16, NDN_TAG_UDP_DST_PORT,
      NDN_TAG_UDP_REMOTE_PORT, NDN_TAG_UDP_LOCAL_PORT, 0,
      TAD_DU_I32, FALSE },
    { "length",   16, BPS_FLD_CONST_DEF(NDN_TAG_UDP_LENGTH, 0),
      TAD_DU_I32, TRUE },
    { "checksum", 16, BPS_FLD_CONST_DEF(NDN_TAG_UDP_CHECKSUM, 0),
      TAD_DU_I32, TRUE },
};


/* See description tad_ipstack_impl.h */
te_errno
tad_udp_init_cb(csap_p csap, unsigned int layer)
{
    te_errno            rc;
    tad_udp_proto_data *proto_data;
    const asn_value    *layer_nds;

    proto_data = TE_ALLOC(sizeof(*proto_data));
    if (proto_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    csap_set_proto_spec_data(csap, layer, proto_data);

    layer_nds = csap->layers[layer].nds;

    rc = tad_bps_pkt_frag_init(tad_udp_bps_hdr,
                               TE_ARRAY_LEN(tad_udp_bps_hdr),
                               layer_nds, &proto_data->hdr);

    assert(tad_bps_pkt_frag_data_bitlen(&proto_data->hdr, NULL) ==
               (TAD_UDP_HDR_LEN << 3));

    return rc;
}

/* See description tad_ipstack_impl.h */
te_errno
tad_udp_destroy_cb(csap_p csap, unsigned int layer)
{
    tad_udp_proto_data *proto_data;

    proto_data = csap_get_proto_spec_data(csap, layer);
    csap_set_proto_spec_data(csap, layer, NULL);

    tad_bps_pkt_frag_free(&proto_data->hdr);

    free(proto_data);

    return 0;
}

/**
 * Convert traffic template/pattern NDS to BPS internal data.
 *
 * @param csap          CSAP instance
 * @param proto_data    Protocol data prepared during CSAP creation
 * @param layer_pdu     Layer NDS
 * @param p_pdu_data    Location for PDU data pointer (updated in any
 *                      case and should be released by caller even in
 *                      the case of failure)
 *
 * @return Status code.
 */
static te_errno
tad_udp_nds_to_pdu_data(csap_p csap, tad_udp_proto_data *proto_data,
                        const asn_value *layer_pdu,
                        tad_udp_proto_pdu_data **p_pdu_data)
{
    te_errno                    rc;
    tad_udp_proto_pdu_data     *pdu_data;

    UNUSED(csap);

    assert(proto_data != NULL);
    assert(layer_pdu != NULL);
    assert(p_pdu_data != NULL);

    *p_pdu_data = pdu_data = TE_ALLOC(sizeof(*pdu_data));
    if (pdu_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    rc = tad_bps_nds_to_data_units(&proto_data->hdr, layer_pdu,
                                   &pdu_data->hdr);

    return rc;
}

/* See description in tad_ipstack_impl.h */
void
tad_udp_release_pdu_cb(csap_p csap, unsigned int layer, void *opaque)
{
    tad_udp_proto_data     *proto_data;
    tad_udp_proto_pdu_data *pdu_data = opaque;

    proto_data = csap_get_proto_spec_data(csap, layer);
    assert(proto_data != NULL);

    if (pdu_data != NULL)
    {
        tad_bps_free_pkt_frag_data(&proto_data->hdr, &pdu_data->hdr);
        free(pdu_data);
    }
}


/* See description in tad_ipstack_impl.h */
te_errno
tad_udp_confirm_tmpl_cb(csap_p csap, unsigned int layer,
                        asn_value *layer_pdu, void **p_opaque)
{
    te_errno                    rc;
    tad_udp_proto_data         *proto_data;
    tad_udp_proto_pdu_data     *tmpl_data;

    proto_data = csap_get_proto_spec_data(csap, layer);

    rc = tad_udp_nds_to_pdu_data(csap, proto_data, layer_pdu, &tmpl_data);
    *p_opaque = tmpl_data;
    if (rc != 0)
        return rc;

    tmpl_data = *p_opaque;

    rc = tad_bps_confirm_send(&proto_data->hdr, &tmpl_data->hdr);

    return rc;
}


/**
 * Callback to generate binary data per PDU.
 *
 * This function complies with tad_pkt_enum_cb prototype.
 */
static te_errno
tad_udp_gen_bin_cb_per_pdu(tad_pkt *pdu, void *hdr)
{
    size_t          len = tad_pkt_len(pdu);
    uint16_t        tmp;
    tad_pkt_seg    *seg = tad_pkt_first_seg(pdu);

    if (len > 0xffff)
    {
        ERROR("PDU is too big to be UDP PDU");
        return TE_RC(TE_TAD_CSAP, TE_E2BIG);
    }

    /* Set correct length in the header template */
    tmp = htons(len);
    memcpy(hdr + 4, &tmp, 2);

    /* Copy header template to packet */
    assert(seg->data_ptr != NULL);
    assert(seg->data_len == TAD_UDP_HDR_LEN);
    memcpy(seg->data_ptr, hdr, TAD_UDP_HDR_LEN);

    return 0;
}

/* See description in tad_ipstack_impl.h */
te_errno
tad_udp_gen_bin_cb(csap_p csap, unsigned int layer,
                   const asn_value *tmpl_pdu, void *opaque,
                   const tad_tmpl_arg_t *args, size_t arg_num,
                   tad_pkts *sdus, tad_pkts *pdus)
{
    tad_udp_proto_data     *proto_data;
    tad_udp_proto_pdu_data *tmpl_data = opaque;

    te_errno        rc;
    unsigned int    bitoff;
    uint8_t         hdr[TAD_UDP_HDR_LEN];


    assert(csap != NULL);
    F_ENTRY("(%d:%u) tmpl_pdu=%p args=%p arg_num=%u sdus=%p pdus=%p",
            csap->id, layer, (void *)tmpl_pdu, (void *)args,
            (unsigned)arg_num, sdus, pdus);

    proto_data = csap_get_proto_spec_data(csap, layer);

    /* Generate binary template of the header */
    bitoff = 0;
    rc = tad_bps_pkt_frag_gen_bin(&proto_data->hdr, &tmpl_data->hdr,
                                  args, arg_num, hdr,
                                  &bitoff, TAD_UDP_HDR_LEN << 3);
    if (rc != 0)
    {
        ERROR("%s(): tad_bps_pkt_frag_gen_bin failed for addresses: %r",
              __FUNCTION__, rc);
        return rc;
    }
    assert(bitoff == (TAD_UDP_HDR_LEN << 3));

    /* UDP layer does no fragmentation, just copy all SDUs to PDUs */
    tad_pkts_move(pdus, sdus);

    /* Allocate and add UDP header to all packets */
    rc = tad_pkts_add_new_seg(pdus, TRUE, NULL, TAD_UDP_HDR_LEN, NULL);
    if (rc != 0)
        return rc;

    /* Per-PDU processing - set correct length */
    rc = tad_pkt_enumerate(pdus, tad_udp_gen_bin_cb_per_pdu, hdr);
    if (rc != 0)
    {
        ERROR("Failed to process UDP PDUs: %r", rc);
        return rc;
    }

    return 0;
}



/* See description in tad_ipstack_impl.h */
te_errno
tad_udp_confirm_ptrn_cb(csap_p csap, unsigned int layer,
                        asn_value *layer_pdu, void **p_opaque)
{
    te_errno                rc;
    tad_udp_proto_data     *proto_data;
    tad_udp_proto_pdu_data *ptrn_data;

    F_ENTRY("(%d:%u) layer_pdu=%p", csap->id, layer,
            (void *)layer_pdu);

    proto_data = csap_get_proto_spec_data(csap, layer);

    rc = tad_udp_nds_to_pdu_data(csap, proto_data, layer_pdu, &ptrn_data);
    *p_opaque = ptrn_data;

    return rc;
}


/* See description in tad_ipstack_impl.h */
te_errno
tad_udp_match_pre_cb(csap_p              csap,
                     unsigned int        layer,
                     tad_recv_pkt_layer *meta_pkt_layer)
{
    tad_udp_proto_data     *proto_data;
    tad_udp_proto_pdu_data *pkt_data;
    te_errno                rc;

    proto_data = csap_get_proto_spec_data(csap, layer);

    pkt_data = malloc(sizeof(*pkt_data));
    if (pkt_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    meta_pkt_layer->opaque = pkt_data;

    rc = tad_bps_pkt_frag_match_pre(&proto_data->hdr, &pkt_data->hdr);

    return rc;
}

/* See description in tad_ipstack_impl.h */
te_errno
tad_udp_match_post_cb(csap_p              csap,
                      unsigned int        layer,
                      tad_recv_pkt_layer *meta_pkt_layer)
{
    tad_udp_proto_data     *proto_data;
    tad_udp_proto_pdu_data *pkt_data = meta_pkt_layer->opaque;
    tad_pkt                *pkt;
    te_errno                rc;
    unsigned int            bitoff = 0;

    if (~csap->state & CSAP_STATE_RESULTS)
        return 0;

    if ((meta_pkt_layer->nds = asn_init_value(ndn_udp_header)) == NULL)
    {
        ERROR_ASN_INIT_VALUE(ndn_udp_header);
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    }

    proto_data = csap_get_proto_spec_data(csap, layer);
    pkt = tad_pkts_first_pkt(&meta_pkt_layer->pkts);

    rc = tad_bps_pkt_frag_match_post(&proto_data->hdr, &pkt_data->hdr,
                                     pkt, &bitoff, meta_pkt_layer->nds);

    return rc;
}

/* See description in tad_ipstack_impl.h */
te_errno
tad_udp_match_do_cb(csap_p           csap,
                    unsigned int     layer,
                    const asn_value *ptrn_pdu,
                    void            *ptrn_opaque,
                    tad_recv_pkt    *meta_pkt,
                    tad_pkt         *pdu,
                    tad_pkt         *sdu)
{
    tad_udp_proto_data     *proto_data;
    tad_udp_proto_pdu_data *ptrn_data = ptrn_opaque;
    tad_udp_proto_pdu_data *pkt_data = meta_pkt->layers[layer].opaque;
    te_errno                rc;
    unsigned int            bitoff = 0;
    tad_data_unit_t        *udp_hdr_cksum_du;
    tad_cksum_str_code      cksum_str_code;

    UNUSED(ptrn_pdu);

    if (tad_pkt_len(pdu) < TAD_UDP_HDR_LEN)
    {
        F_VERB(CSAP_LOG_FMT "PDU is too small to be UDP datagram",
               CSAP_LOG_ARGS(csap));
        return TE_RC(TE_TAD_CSAP, TE_ETADNOTMATCH);
    }

    proto_data = csap_get_proto_spec_data(csap, layer);

    assert(proto_data != NULL);
    assert(ptrn_data != NULL);
    assert(pkt_data != NULL);

    /* Check whether an advanced checksum matching mode was requested */
    udp_hdr_cksum_du = &ptrn_data->hdr.dus[UDP_HDR_CKSUM_DU_INDEX];
    cksum_str_code = tad_du_get_cksum_str_code(udp_hdr_cksum_du);

    /*
     * Clear the DU, so that it will not be considered in
     * tad_bps_pkt_frag_match_do() matching path
     */
    if (cksum_str_code != TAD_CKSUM_STR_CODE_NONE)
        tad_data_unit_clear(udp_hdr_cksum_du);

    rc = tad_bps_pkt_frag_match_do(&proto_data->hdr, &ptrn_data->hdr,
                                   &pkt_data->hdr, pdu, &bitoff);
    if (rc != 0)
    {
        F_VERB(CSAP_LOG_FMT "Match PDU vs UDP header failed on bit "
               "offset %u: %r", CSAP_LOG_ARGS(csap), (unsigned)bitoff, rc);
        return rc;
    }

    if (cksum_str_code != TAD_CKSUM_STR_CODE_NONE)
    {
        rc = tad_l4_match_cksum_advanced(csap, pdu, meta_pkt, layer,
                                         IPPROTO_UDP, cksum_str_code);
        if (rc != 0)
            return rc;
    }

    rc = tad_pkt_get_frag(sdu, pdu, bitoff >> 3,
                          tad_pkt_len(pdu) - (bitoff >> 3),
                          TAD_PKT_GET_FRAG_ERROR);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Failed to prepare UDP SDU: %r",
              CSAP_LOG_ARGS(csap), rc);
        return rc;
    }

    EXIT(CSAP_LOG_FMT "OK", CSAP_LOG_ARGS(csap));

    return 0;
}
