/** @file
 * @brief TAD VxLAN
 *
 * Traffic Application Domain Command Handler
 * VxLAN CSAP layer-related callbacks
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Ivan Malov <Ivan.Malov@oktetlabs.ru>
 */

#define TE_LGR_USER "TAD VxLAN"

#include "te_config.h"


#include "te_alloc.h"

#include "ndn_vxlan.h"
#include "tad_bps.h"
#include "tad_vxlan_impl.h"

#define TAD_VXLAN_HEADER_LEN 8

/** VxLAN layer specific data */
typedef struct tad_vxlan_proto_data {
    tad_bps_pkt_frag_def header;
} tad_vxlan_proto_data_t;

/**
 * VxLAN layer specific data for PDU processing
 * (both send and receive)
 */
typedef struct tad_vxlan_proto_pdu_data {
    tad_bps_pkt_frag_data header;
} tad_vxlan_proto_pdu_data_t;

/** VxLAN header BPS representation (RFC 7348) */
static const tad_bps_pkt_frag tad_vxlan_bps_header[] =
{
    { "flags-reserved-1",  4,  BPS_FLD_CONST(0),
      TAD_DU_I32, FALSE },
    { "vni-valid",         1,  BPS_FLD_CONST(1),
      TAD_DU_I32, FALSE },
    { "flags-reserved-2",  3,  BPS_FLD_CONST(0),
      TAD_DU_I32, FALSE },
    { "reserved-1",        24, BPS_FLD_CONST(0),
      TAD_DU_I32, FALSE },
    { "vni",               24,
      BPS_FLD_CONST_DEF(NDN_TAG_VXLAN_VNI, 0),
      TAD_DU_I32, FALSE },
    { "reserved-2",        8,  BPS_FLD_CONST(0),
      TAD_DU_I32, FALSE },
};

/* See description in 'tad_vxlan_impl.h' */
te_errno
tad_vxlan_init_cb(csap_p       csap,
                  unsigned int layer_idx)
{
    tad_vxlan_proto_data_t *proto_data = NULL;
    te_errno                rc;

    if ((csap == NULL) || (csap->layers == NULL) || (layer_idx >= csap->depth))
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);

    proto_data = TE_ALLOC(sizeof(*proto_data));
    if (proto_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    rc = tad_bps_pkt_frag_init(tad_vxlan_bps_header,
                               TE_ARRAY_LEN(tad_vxlan_bps_header),
                               csap->layers[layer_idx].nds,
                               &proto_data->header);
    if (rc != 0)
    {
        /*
         * FIXME: It should be responsibility of tad_bps_pkt_frag_init()
         *        to perform cleanup for a fragment in case of failure
         */
        tad_bps_pkt_frag_free(&proto_data->header);

        free(proto_data);
        return TE_RC(TE_TAD_CSAP, rc);
    }

    csap_set_proto_spec_data(csap, layer_idx, proto_data);

    return 0;
}

/* See description in 'tad_vxlan_impl.h' */
te_errno
tad_vxlan_destroy_cb(csap_p       csap,
                     unsigned int layer_idx)
{
    tad_vxlan_proto_data_t *proto_data;

    if (csap == NULL)
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);

    proto_data = csap_get_proto_spec_data(csap, layer_idx);
    if (proto_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);

    csap_set_proto_spec_data(csap, layer_idx, NULL);
    tad_bps_pkt_frag_free(&proto_data->header);
    free(proto_data);

    return 0;
}

/* See description in 'tad_vxlan_impl.h' */
void
tad_vxlan_release_pdu_cb(csap_p        csap,
                         unsigned int  layer_idx,
                         void         *opaque)
{
    tad_vxlan_proto_data_t     *proto_data;
    tad_vxlan_proto_pdu_data_t *pdu_data = opaque;

    if ((csap == NULL) || (pdu_data == NULL))
        return;

    proto_data = csap_get_proto_spec_data(csap, layer_idx);
    if (proto_data == NULL)
        return;

    tad_bps_free_pkt_frag_data(&proto_data->header, &pdu_data->header);
    free(pdu_data);
}

/* See description in 'tad_vxlan_impl.h' */
te_errno
tad_vxlan_confirm_tmpl_cb(csap_p         csap,
                          unsigned int   layer_idx,
                          asn_value     *layer_pdu,
                          void         **p_opaque)
{
    tad_vxlan_proto_data_t     *proto_data;
    tad_vxlan_proto_pdu_data_t *tmpl_data = NULL;
    te_errno                    rc = 0;

    if ((csap == NULL) || (layer_pdu == NULL) || (p_opaque == NULL))
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);

    proto_data = csap_get_proto_spec_data(csap, layer_idx);
    if (proto_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);

    tmpl_data = TE_ALLOC(sizeof(*tmpl_data));
    if (tmpl_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    rc = tad_bps_nds_to_data_units(&proto_data->header, layer_pdu,
                                   &tmpl_data->header);
    if (rc != 0)
        goto fail;

    rc = tad_bps_confirm_send(&proto_data->header, &tmpl_data->header);
    if (rc != 0)
        goto fail;

    *p_opaque = tmpl_data;

    return 0;

fail:
    tad_bps_free_pkt_frag_data(&proto_data->header, &tmpl_data->header);
    free(tmpl_data);

    return TE_RC(TE_TAD_CSAP, rc);
}

/* See description in 'tad_vxlan_impl.h' */
te_errno
tad_vxlan_gen_bin_cb(csap_p                csap,
                     unsigned int          layer_idx,
                     const asn_value      *tmpl_pdu,
                     void                 *opaque,
                     const tad_tmpl_arg_t *args,
                     size_t                arg_num,
                     tad_pkts             *sdus,
                     tad_pkts             *pdus)
{
    tad_vxlan_proto_data_t     *proto_data;
    tad_vxlan_proto_pdu_data_t *tmpl_data = opaque;
    unsigned int                bitoff;
    uint8_t                    *binary = NULL;
    te_errno                    rc = 0;

    UNUSED(tmpl_pdu);

    if ((csap == NULL) || (tmpl_data == NULL))
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);

    proto_data = csap_get_proto_spec_data(csap, layer_idx);
    if (proto_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);

    binary = TE_ALLOC(TAD_VXLAN_HEADER_LEN);
    if (binary == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    bitoff = 0;
    rc = tad_bps_pkt_frag_gen_bin(&proto_data->header, &tmpl_data->header,
                                  args, arg_num, binary, &bitoff,
                                  TAD_VXLAN_HEADER_LEN << 3);
    if (rc != 0)
        goto fail;

    if (bitoff != (TAD_VXLAN_HEADER_LEN << 3))
    {
        rc = TE_EINVAL;
        goto fail;
    }

    tad_pkts_move(pdus, sdus);
    rc = tad_pkts_add_new_seg(pdus, TRUE, binary, TAD_VXLAN_HEADER_LEN,
                              tad_pkt_seg_data_free);
    if (rc != 0)
        goto fail;

    return 0;

fail:
    free(binary);

    return TE_RC(TE_TAD_CSAP, rc);
}

/* See description in 'tad_vxlan_impl.h' */
te_errno
tad_vxlan_confirm_ptrn_cb(csap_p         csap,
                          unsigned int   layer_idx,
                          asn_value     *layer_pdu,
                          void         **p_opaque)
{
    tad_vxlan_proto_data_t     *proto_data;
    tad_vxlan_proto_pdu_data_t *ptrn_data;
    te_errno                    rc = 0;

    if ((csap == NULL) || (layer_pdu == NULL) || (p_opaque == NULL))
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);

    proto_data = csap_get_proto_spec_data(csap, layer_idx);
    if (proto_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);

    ptrn_data = TE_ALLOC(sizeof(*ptrn_data));
    if (ptrn_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    rc = tad_bps_nds_to_data_units(&proto_data->header, layer_pdu,
                                   &ptrn_data->header);
    if (rc == 0)
        *p_opaque = ptrn_data;
    else
        free(ptrn_data);

    return TE_RC(TE_TAD_CSAP, rc);
}

/* See description in 'tad_vxlan_impl.h' */
te_errno
tad_vxlan_match_post_cb(csap_p              csap,
                        unsigned int        layer_idx,
                        tad_recv_pkt_layer *meta_pkt_layer)
{
    asn_value                  *meta_pkt_layer_nds = NULL;
    tad_vxlan_proto_data_t     *proto_data;
    tad_vxlan_proto_pdu_data_t *pkt_data;
    tad_pkt                    *pkt;
    unsigned int                bitoff = 0;
    te_errno                    rc = 0;

    if (csap == NULL)
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);

    if (~csap->state & CSAP_STATE_RESULTS)
        return 0;

    if (meta_pkt_layer == NULL)
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);

    proto_data = csap_get_proto_spec_data(csap, layer_idx);
    pkt = tad_pkts_first_pkt(&meta_pkt_layer->pkts);
    pkt_data = meta_pkt_layer->opaque;
    if ((proto_data == NULL) || (pkt == NULL) || (pkt_data == NULL))
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);

    meta_pkt_layer_nds = asn_init_value(ndn_vxlan_header);
    if (meta_pkt_layer_nds == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    rc = tad_bps_pkt_frag_match_post(&proto_data->header, &pkt_data->header,
                                     pkt, &bitoff, meta_pkt_layer_nds);
    if (rc != 0)
    {
        asn_free_value(meta_pkt_layer_nds);
        return TE_RC(TE_TAD_CSAP, rc);
    }

    meta_pkt_layer->nds = meta_pkt_layer_nds;

    return 0;
}

/* See description in 'tad_vxlan_impl.h' */
te_errno
tad_vxlan_match_do_cb(csap_p           csap,
                      unsigned int     layer_idx,
                      const asn_value *ptrn_pdu,
                      void            *ptrn_opaque,
                      tad_recv_pkt    *meta_pkt,
                      tad_pkt         *pdu,
                      tad_pkt         *sdu)
{
    tad_vxlan_proto_data_t     *proto_data;
    tad_vxlan_proto_pdu_data_t *ptrn_data = ptrn_opaque;
    tad_vxlan_proto_pdu_data_t *pkt_data = NULL;
    unsigned int                bitoff = 0;
    te_errno                    rc = 0;

    UNUSED(ptrn_pdu);

    if ((csap == NULL) || (ptrn_data == NULL) || (meta_pkt == NULL) ||
        (meta_pkt->layers == NULL))
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);

    if (tad_pkt_len(pdu) < TAD_VXLAN_HEADER_LEN)
        return TE_RC(TE_TAD_CSAP, TE_ETADNOTMATCH);

    proto_data = csap_get_proto_spec_data(csap, layer_idx);
    if (proto_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);

    pkt_data = TE_ALLOC(sizeof(*pkt_data));
    if (pkt_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    rc = tad_bps_pkt_frag_match_pre(&proto_data->header, &pkt_data->header);
    if (rc != 0)
        goto fail;

    rc = tad_bps_pkt_frag_match_do(&proto_data->header, &ptrn_data->header,
                                   &pkt_data->header, pdu, &bitoff);
    if (rc != 0)
        goto fail;

    rc = tad_pkt_get_frag(sdu, pdu, bitoff >> 3,
                          tad_pkt_len(pdu) - (bitoff >> 3),
                          TAD_PKT_GET_FRAG_ERROR);
    if (rc != 0)
        goto fail;

    meta_pkt->layers[layer_idx].opaque = pkt_data;

    return 0;

fail:
    tad_bps_free_pkt_frag_data(&proto_data->header, &pkt_data->header);
    free(pkt_data);

    return TE_RC(TE_TAD_CSAP, rc);
}
