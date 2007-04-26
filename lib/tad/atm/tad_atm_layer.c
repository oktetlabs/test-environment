/** @file
 * @brief TAD ATM
 *
 * Traffic Application Domain Command Handler.
 * ATM CSAP layer-related callbacks.
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAD ATM"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

/* To get ntohl(),... */
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "te_errno.h"
#include "logger_api.h"
#include "logger_ta_fast.h"
#include "asn_usr.h"

#include "tad_csap_support.h"
#include "tad_csap_inst.h"
#include "tad_bps.h"

#include "ndn_atm.h"
#include "tad_atm_impl.h"


/** Control cell bit in payload type */
#define TAD_ATM_CONTROL_CELL    (1 << 2)

/** ATM-user to ATM-user indication bit in payload type */
#define TAD_ATM_U2U_IND         (1 << 0)


/**
 * ATM layer specific data
 */
typedef struct tad_atm_proto_data {
    int32_t                 type;
    tad_bps_pkt_frag_def    hdr;
    tad_data_unit_t         congestion;
} tad_atm_proto_data;

/**
 * ATM layer specific data for send processing
 */
typedef struct tad_atm_proto_pdu_data {
    tad_bps_pkt_frag_data   hdr;
    tad_data_unit_t         congestion;
} tad_atm_proto_pdu_data;


/**
 * Definition of ATM cell UNI header.
 */
static const tad_bps_pkt_frag tad_atm_uni_bps_hdr[] =
{
    { "gfc",            4,  BPS_FLD_CONST_DEF(NDN_TAG_ATM_GFC, 0),
      TAD_DU_I32, FALSE },
    { "vpi",            8,  BPS_FLD_SIMPLE(NDN_TAG_ATM_VPI),
      TAD_DU_I32, TRUE },
    { "vci",            16, BPS_FLD_SIMPLE(NDN_TAG_ATM_VCI),
      TAD_DU_I32, TRUE },
    { "payload-type",   3,  NDN_TAG_ATM_PAYLOAD_TYPE,
                            ASN_TAG_CONST, ASN_TAG_INVALID, 0,
      TAD_DU_I32, TRUE },
    { "clp",            1,  BPS_FLD_CONST_DEF(NDN_TAG_ATM_CLP, 0),
      TAD_DU_I32, FALSE },
    { "hec",            8,  NDN_TAG_ATM_HEC,
                            ASN_TAG_CONST, ASN_TAG_INVALID, 0,
      TAD_DU_I32, TRUE },
};

/**
 * Definition of ATM cell NNI header.
 */
static const tad_bps_pkt_frag tad_atm_nni_bps_hdr[] =
{
    { "vpi",            12, BPS_FLD_SIMPLE(NDN_TAG_ATM_VPI),
      TAD_DU_I32, TRUE },
    { "vci",            16, BPS_FLD_SIMPLE(NDN_TAG_ATM_VCI),
      TAD_DU_I32, TRUE },
    { "payload-type",   3,  NDN_TAG_ATM_PAYLOAD_TYPE,
                            ASN_TAG_CONST, ASN_TAG_INVALID, 0,
      TAD_DU_I32, TRUE },
    { "clp",            1,  BPS_FLD_CONST_DEF(NDN_TAG_ATM_CLP, 0),
      TAD_DU_I32, FALSE },
    { "hec",            8,  NDN_TAG_ATM_HEC,
                            ASN_TAG_CONST, ASN_TAG_INVALID, 0,
      TAD_DU_I32, TRUE },
};


/* See description in tad_atm_impl.h */
te_errno
tad_atm_init_cb(csap_p csap, unsigned int layer)
{
    te_errno                rc;
    tad_atm_proto_data     *proto_data;
    const asn_value        *layer_nds;
    const tad_bps_pkt_frag *hdr_descr;
    unsigned int            hdr_descr_len;

    proto_data = calloc(1, sizeof(*proto_data));
    if (proto_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    csap_set_proto_spec_data(csap, layer, proto_data);

    layer_nds = csap->layers[layer].nds;

    rc = asn_read_int32(layer_nds, &proto_data->type, "type");
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "%s() failed to get ATM type",
              CSAP_LOG_ARGS(csap), __FUNCTION__);
        return rc;
    }
    switch (proto_data->type)
    {
        case NDN_ATM_NNI:
            hdr_descr = tad_atm_nni_bps_hdr;
            hdr_descr_len = TE_ARRAY_LEN(tad_atm_nni_bps_hdr);
            break;

        case NDN_ATM_UNI:
            hdr_descr = tad_atm_uni_bps_hdr;
            hdr_descr_len = TE_ARRAY_LEN(tad_atm_uni_bps_hdr);
            break;

        default:
            ERROR(CSAP_LOG_FMT "Unexpected ATM cell header format type %d",
                  CSAP_LOG_ARGS(csap), (int)proto_data->type);
            return TE_RC(TE_TAD_CH, TE_EINVAL);
    }

    /* Get default for congestion state */
    rc = tad_data_unit_convert(layer_nds,
                               NDN_TAG_ATM_CONGESTION,
                               &proto_data->congestion);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Failed to get congestion default from "
              "layer parameters: %r", CSAP_LOG_ARGS(csap), rc);
        return rc;
    }

    /* Initialize ATM cell header binary support */
    rc = tad_bps_pkt_frag_init(hdr_descr, hdr_descr_len,
                               layer_nds, &proto_data->hdr);
    if (rc != 0)
        return rc;

    if (tad_bps_pkt_frag_data_bitlen(&proto_data->hdr, NULL) !=
            (ATM_HEADER_LEN << 3))
    {
        ERROR(CSAP_LOG_FMT "Unexpected ATM cell header length",
              CSAP_LOG_ARGS(csap));
        return TE_RC(TE_TAD_CH, TE_EINVAL);
    }

    return rc;
}

/* See description in tad_atm_impl.h */
te_errno
tad_atm_destroy_cb(csap_p csap, unsigned int layer)
{
    tad_atm_proto_data *proto_data;

    proto_data = csap_get_proto_spec_data(csap, layer);
    csap_set_proto_spec_data(csap, layer, NULL);

    tad_bps_pkt_frag_free(&proto_data->hdr);

    free(proto_data);

    return 0;
}


/* See description in tad_atm_impl.h */
te_errno
tad_atm_confirm_tmpl_cb(csap_p csap, unsigned int  layer, 
                        asn_value *layer_pdu, void **p_opaque)
{
    te_errno                rc;
    tad_atm_proto_data     *proto_data;
    tad_atm_proto_pdu_data *tmpl_data;

    proto_data = csap_get_proto_spec_data(csap, layer);

    /* FIXME: Use malloc() and initialize data units at first */
    tmpl_data = calloc(1, sizeof(*tmpl_data));
    if (tmpl_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    *p_opaque = tmpl_data;

    /* Get template value for congestion state */
    rc = tad_data_unit_convert(layer_pdu,
                               NDN_TAG_ATM_CONGESTION,
                               &tmpl_data->congestion);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Failed to get congestion default from "
              "layer parameters: %r", CSAP_LOG_ARGS(csap), rc);
        return rc;
    }

    /* Get template values for ATM cell header fields */
    rc = tad_bps_nds_to_data_units(&proto_data->hdr, layer_pdu,
                                   &tmpl_data->hdr);
    if (rc != 0)
        return rc;

    rc = tad_bps_confirm_send(&proto_data->hdr, &tmpl_data->hdr);
    if (rc != 0)
        return rc;

    return rc;
}

/* See description in tad_atm_impl.h */
te_errno
tad_atm_confirm_ptrn_cb(csap_p csap, unsigned int  layer, 
                        asn_value *layer_pdu, void **p_opaque)
{
    te_errno                rc;
    tad_atm_proto_data     *proto_data;
    tad_atm_proto_pdu_data *tmpl_data;

    proto_data = csap_get_proto_spec_data(csap, layer);

    /* FIXME: Use malloc() and initialize data units at first */
    tmpl_data = calloc(1, sizeof(*tmpl_data));
    if (tmpl_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    *p_opaque = tmpl_data;

    /* Get template value for congestion state */
    rc = tad_data_unit_convert(layer_pdu,
                               NDN_TAG_ATM_CONGESTION,
                               &tmpl_data->congestion);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Failed to get congestion default from "
              "layer parameters: %r", CSAP_LOG_ARGS(csap), rc);
        return rc;
    }

    /* Get template values for ATM cell header fields */
    rc = tad_bps_nds_to_data_units(&proto_data->hdr, layer_pdu,
                                   &tmpl_data->hdr);
    if (rc != 0)
        return rc;

    return rc;
}

/* See description in tad_atm_impl.h */
void
tad_atm_release_pdu_cb(csap_p csap, unsigned int layer, void *opaque)
{
    tad_atm_proto_data     *proto_data;
    tad_atm_proto_pdu_data *tmpl_data = opaque;

    if (tmpl_data == NULL)
        return;

    proto_data = csap_get_proto_spec_data(csap, layer);

    tad_bps_free_pkt_frag_data(&proto_data->hdr, &tmpl_data->hdr);
    free(tmpl_data);
}


/**
 * Check length of packet is equal to 53 bytes (ATM cell).
 * Fill in the first segment as ATM header.
 *
 * @a opaque parameter refers to ATM cell header template.
 *
 * This function complies with tad_pkt_enum_cb prototype.
 */
static te_errno
tad_atm_prepare_cell(tad_pkt *pkt, void *opaque)
{
    tad_pkt_seg            *seg;
    tad_atm_cell_ctrl_data *cell_ctrl;

    if (tad_pkt_len(pkt) != ATM_CELL_LEN)
    {
        ERROR("Invalid length (%u) of the packet as ATM cell",
              (unsigned)tad_pkt_len(pkt));
        return TE_RC(TE_TAD_CSAP, TE_ETADWRONGNDS);
    }

    seg = tad_pkt_first_seg(pkt);
    assert(seg->data_len >= ATM_HEADER_LEN);

    /* Copy template of the header */
    memcpy(seg->data_ptr, opaque, ATM_HEADER_LEN);

    /* Process packet specific data provided by the upper layer */
    cell_ctrl = tad_pkt_opaque(pkt);
    if (cell_ctrl != NULL)
    {
        uint32_t    ind = !!cell_ctrl->indication;
        uint32_t    hdr;

        memcpy(&hdr, seg->data_ptr, sizeof(hdr));
        hdr = ntohl(hdr);
        if (((hdr >> 1) & 1) != ind)
        {
            hdr &= ~(1 << 1);
            hdr |= (ind << 1);
            hdr = htonl(hdr);
            memcpy(seg->data_ptr, &hdr, sizeof(hdr));
            VERB("ATM cell indication is set to %u in pkt=%p",
                 (unsigned)ind, pkt);
        }
    }

    /* Calculate HEC */
    /* FIXME: Implement HEC calculation */

    return 0;
}

/* See description in tad_atm_impl.h */
te_errno
tad_atm_gen_bin_cb(csap_p                csap,
                   unsigned int          layer,
                   const asn_value      *tmpl_pdu,
                   void                 *opaque,
                   const tad_tmpl_arg_t *args,
                   size_t                arg_num,
                   tad_pkts             *sdus,
                   tad_pkts             *pdus)
{
    tad_atm_proto_data     *proto_data;
    tad_atm_proto_pdu_data *tmpl_data = opaque;

    te_errno        rc;
    uint8_t         header[ATM_HEADER_LEN];
    unsigned int    bitoff = 0;

    UNUSED(tmpl_pdu);

    proto_data = csap_get_proto_spec_data(csap, layer);

    /* Prepare ATM cell header template */
    rc = tad_bps_pkt_frag_gen_bin(&proto_data->hdr, &tmpl_data->hdr,
                                  args, arg_num, header, &bitoff,
                                  ATM_HEADER_LEN << 3);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Failed to prepare ATM cell header: %r",
              CSAP_LOG_ARGS(csap), rc);
        return rc;
    }
    assert(bitoff == (sizeof(header) << 3));

    /* Move all SDUs to PDUs */
    tad_pkts_move(pdus, sdus);

    /* Add space for ATM cell header segment to each PDU */
    rc = tad_pkts_add_new_seg(pdus, TRUE, NULL, ATM_HEADER_LEN, NULL);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Failed to add ATM cell header segment: %r",
              CSAP_LOG_ARGS(csap), rc);
        return rc;
    }

    /* Check each packet and fill in its header */
    rc = tad_pkt_enumerate(pdus, tad_atm_prepare_cell, header);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Failed to prepare ATM cells: %r",
              CSAP_LOG_ARGS(csap), rc);
        return rc;
    }

    return 0;
}


/* See description in tad_atm_impl.h */
te_errno
tad_atm_match_pre_cb(csap_p              csap,
                     unsigned int        layer,
                     tad_recv_pkt_layer *meta_pkt_layer)
{
    tad_atm_proto_data     *proto_data;
    tad_atm_proto_pdu_data *pkt_data;
    te_errno                rc;

    proto_data = csap_get_proto_spec_data(csap, layer);

    pkt_data = malloc(sizeof(*pkt_data));
    if (pkt_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    meta_pkt_layer->opaque = pkt_data;

    pkt_data->congestion.du_type = TAD_DU_UNDEF;

    rc = tad_bps_pkt_frag_match_pre(&proto_data->hdr, &pkt_data->hdr);

    return rc;
}

/* See description in tad_atm_impl.h */
te_errno
tad_atm_match_post_cb(csap_p              csap,
                      unsigned int        layer,
                      tad_recv_pkt_layer *meta_pkt_layer)
{
    tad_atm_proto_data     *proto_data;
    tad_atm_proto_pdu_data *pkt_data = meta_pkt_layer->opaque;
    tad_pkt                *pkt;
    te_errno                rc;
    unsigned int            bitoff = 0;

    if (~csap->state & CSAP_STATE_RESULTS)
        return 0;

    if ((meta_pkt_layer->nds = asn_init_value(ndn_atm_header)) == NULL)
    {
        ERROR_ASN_INIT_VALUE(ndn_atm_header);
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    }

    proto_data = csap_get_proto_spec_data(csap, layer);
    pkt =  tad_pkts_first_pkt(&meta_pkt_layer->pkts);

    rc = tad_bps_pkt_frag_match_post(&proto_data->hdr, &pkt_data->hdr,
                                     pkt, &bitoff, meta_pkt_layer->nds);

    return rc;
}

/* See description in tad_atm_impl.h */
te_errno
tad_atm_match_do_cb(csap_p           csap,
                    unsigned int     layer,
                    const asn_value *ptrn_pdu,
                    void            *ptrn_opaque,
                    tad_recv_pkt    *meta_pkt,
                    tad_pkt         *pdu,
                    tad_pkt         *sdu)
{
    tad_atm_proto_data     *proto_data;
    tad_atm_proto_pdu_data *ptrn_data = ptrn_opaque;
    tad_atm_proto_pdu_data *pkt_data = meta_pkt->layers[layer].opaque;
    te_errno                rc;
    unsigned int            bitoff = 0;

    UNUSED(ptrn_pdu);

    if (tad_pkt_len(pdu) != ATM_CELL_LEN)
    {
        F_VERB(CSAP_LOG_FMT "PDU is too small/big (%u) to be ATM cell",
               CSAP_LOG_ARGS(csap), (unsigned)tad_pkt_len(pdu));
        return TE_RC(TE_TAD_CSAP, TE_ETADNOTMATCH);
    }
  
    proto_data = csap_get_proto_spec_data(csap, layer);

    assert(proto_data != NULL);
    assert(ptrn_data != NULL);
    assert(pkt_data != NULL);

    rc = tad_bps_pkt_frag_match_do(&proto_data->hdr, &ptrn_data->hdr,
                                   &pkt_data->hdr, pdu, &bitoff);
    if (rc != 0)
    {
        F_VERB(CSAP_LOG_FMT "Match PDU vs ATM header failed on bit "
               "offset %u: %r", CSAP_LOG_ARGS(csap), (unsigned)bitoff, rc);
        return rc;
    }

    rc = tad_pkt_get_frag(sdu, pdu, bitoff >> 3,
                          tad_pkt_len(pdu) - (bitoff >> 3),
                          TAD_PKT_GET_FRAG_ERROR);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Failed to prepare ATM SDU: %r",
              CSAP_LOG_ARGS(csap), rc);
        return rc;
    }

    /*
     * If ATM is not the top layer, allocate and fill in ATM cell data
     * required for the next layer (AAL).
     */
    if (layer > 0)
    {
        tad_atm_cell_ctrl_data *cell_ctrl;
        unsigned int            gfc_shift;

        cell_ctrl = malloc(sizeof(*cell_ctrl));
        if (cell_ctrl == NULL)
            return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
        tad_pkt_set_opaque(sdu, cell_ctrl, free);

        gfc_shift = (proto_data->type == NDN_ATM_UNI) ? 1 : 0;

        assert(pkt_data->hdr.dus[2 + gfc_shift].du_type = TAD_DU_I32);
        cell_ctrl->user_data = !(pkt_data->hdr.dus[2 + gfc_shift].val_i32 &
                                 TAD_ATM_CONTROL_CELL);

        cell_ctrl->indication = (cell_ctrl->user_data) ?
            !!(pkt_data->hdr.dus[2 + gfc_shift].val_i32 & TAD_ATM_U2U_IND) :
            0;

        assert(pkt_data->hdr.dus[0 + gfc_shift].du_type = TAD_DU_I32);
        cell_ctrl->vpi = pkt_data->hdr.dus[0 + gfc_shift].val_i32;
        assert(pkt_data->hdr.dus[1 + gfc_shift].du_type = TAD_DU_I32);
        cell_ctrl->vci = pkt_data->hdr.dus[1 + gfc_shift].val_i32;
    }

    EXIT(CSAP_LOG_FMT "OK", CSAP_LOG_ARGS(csap));

    return 0;
}
