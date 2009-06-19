/** @file
 * @brief TAD IGMP version 2 layer
 *
 * Traffic Application Domain Command Handler.
 * IGMPv2 CSAP layer-related callbacks.
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS
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
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 *
 * $Id: $
 */

#define TE_LGR_USER     "TAD IGMPv2"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include <linux/igmp.h>

#include "te_defs.h"
#include "te_alloc.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "logger_ta_fast.h"
#include "tad_bps.h"
#include "tad_igmp_impl.h"


static unsigned char mac_mcast[] = { 0x01, 0x00, 0x5E, 0x00, 0x00, 0x00 };

#define TE_TAD_IGMP_MAXLEN 1500

/**
 * IGMPv2 layer specific data
 */
typedef struct tad_igmp_proto_data {
    tad_bps_pkt_frag_def    hdr;
    tad_bps_pkt_frag_def    group_address;
    tad_bps_pkt_frag_def    v3_query;
    tad_bps_pkt_frag_def    v3_report;
} tad_igmp_proto_data;

/**
 * IGMPv2 layer specific data for PDU processing
 * (both send and receive).
 */
typedef struct tad_igmp_proto_pdu_data {
    tad_bps_pkt_frag_data   hdr;
    tad_bps_pkt_frag_data   group_address;
    tad_bps_pkt_frag_data   v3_query;
    tad_bps_pkt_frag_data   v3_report;
} tad_igmp_proto_pdu_data;


/**
 * Definition of IGMP header.
 */
static const tad_bps_pkt_frag tad_igmp_bps_hdr[] =
{
    { "type", 8, BPS_FLD_NO_DEF(NDN_TAG_IGMP_TYPE),
      TAD_DU_I32, FALSE },
    { "max-resp-time", 8,
      BPS_FLD_CONST_DEF(NDN_TAG_IGMP_MAX_RESPONSE_TIME, 0),
      TAD_DU_I32, FALSE },
    { "checksum", 16, BPS_FLD_CONST_DEF(NDN_TAG_IGMP_CHECKSUM, 0),
      TAD_DU_I32, TRUE },
};

/**
 * Definition of IGMP header group address field.
 */
static const tad_bps_pkt_frag tad_igmp_bps_group_address[] =
{
    { "group-address", 32,
      BPS_FLD_CONST_DEF(NDN_TAG_IGMP_GROUP_ADDRESS, 0),
      TAD_DU_OCTS, FALSE },
};

/**
 * Definition of IGMPv3 Query specific data.
 */
static const tad_bps_pkt_frag tad_igmp_bps_v3_query[] =
{
    { "reserved", 4, BPS_FLD_CONST(0),
      TAD_DU_I32, FALSE },
    { "s-flag", 1, BPS_FLD_CONST_DEF(NDN_TAG_IGMP3_S_FLAG, 0),
      TAD_DU_I32, FALSE },
    { "qrv", 3, BPS_FLD_CONST_DEF(NDN_TAG_IGMP3_QRV, 0),
      TAD_DU_I32, FALSE },
    { "qqic", 8, BPS_FLD_CONST_DEF(NDN_TAG_IGMP3_QQIC, 0),
      TAD_DU_I32, FALSE },
    { "number-of-sources", 16,
      BPS_FLD_NO_DEF(NDN_TAG_IGMP3_NUMBER_OF_SOURCES),
      TAD_DU_I32, FALSE },
    { "source-address-list", 0,
      BPS_FLD_CONST_DEF(NDN_TAG_IGMP3_SOURCE_ADDRESS_LIST, 0),
      TAD_DU_OCTS, FALSE },
};

/**
 * Definition of IGMPv3 Report specific data.
 */
static const tad_bps_pkt_frag tad_igmp_bps_v3_report[] =
{
    { "reserved", 16, BPS_FLD_CONST(0),
      TAD_DU_I32, FALSE },
    { "number-of-groups", 16, BPS_FLD_NO_DEF(NDN_TAG_IGMP3_NUMBER_OF_GROUPS),
      TAD_DU_I32, FALSE },
    { "group-record-list", 0,
      BPS_FLD_CONST_DEF(NDN_TAG_IGMP3_GROUP_RECORD_LIST, 0),
      TAD_DU_OCTS, FALSE },
};


/* See description tad_igmp_impl.h */
te_errno
tad_igmp_init_cb(csap_p csap, unsigned int layer)
{
    te_errno                rc;
    tad_igmp_proto_data    *proto_data;
    const asn_value        *layer_nds;

    proto_data = TE_ALLOC(sizeof(*proto_data));
    if (proto_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    csap_set_proto_spec_data(csap, layer, proto_data);

    layer_nds = csap->layers[layer].nds;

    rc = tad_bps_pkt_frag_init(tad_igmp_bps_hdr,
                               TE_ARRAY_LEN(tad_igmp_bps_hdr),
                               layer_nds, &proto_data->hdr);
    if (rc != 0)
        return rc;

    rc = tad_bps_pkt_frag_init(tad_igmp_bps_group_address,
                               TE_ARRAY_LEN(tad_igmp_bps_group_address),
                               layer_nds, &proto_data->group_address);
    if (rc != 0)
        return rc;

    rc = tad_bps_pkt_frag_init(tad_igmp_bps_v3_query,
                               TE_ARRAY_LEN(tad_igmp_bps_v3_query),
                               layer_nds, &proto_data->v3_query);
    if (rc != 0)
        return rc;

    rc = tad_bps_pkt_frag_init(tad_igmp_bps_v3_report,
                               TE_ARRAY_LEN(tad_igmp_bps_v3_report),
                               layer_nds, &proto_data->v3_report);
    if (rc != 0)
        return rc;

    return 0; 
}

/* See description tad_igmp_impl.h */
te_errno
tad_igmp_destroy_cb(csap_p csap, unsigned int layer)
{
    tad_igmp_proto_data *proto_data;

    proto_data = csap_get_proto_spec_data(csap, layer);
    csap_set_proto_spec_data(csap, layer, NULL);

    tad_bps_pkt_frag_free(&proto_data->hdr);
    tad_bps_pkt_frag_free(&proto_data->group_address);
    tad_bps_pkt_frag_free(&proto_data->v3_query);
    tad_bps_pkt_frag_free(&proto_data->v3_report);

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
tad_igmp_nds_to_pdu_data(csap_p csap, tad_igmp_proto_data *proto_data,
                         const asn_value *layer_pdu,
                         tad_igmp_proto_pdu_data **p_pdu_data)
{
    te_errno                    rc;
    tad_igmp_proto_pdu_data    *pdu_data;

    UNUSED(csap);

    assert(proto_data != NULL);
    assert(layer_pdu != NULL);
    assert(p_pdu_data != NULL);

    *p_pdu_data = pdu_data = TE_ALLOC(sizeof(*pdu_data));
    if (pdu_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    rc = tad_bps_nds_to_data_units(&proto_data->hdr, layer_pdu,
                                   &pdu_data->hdr);
    if (rc != 0)
        return rc;

    rc = tad_bps_nds_to_data_units(&proto_data->group_address, layer_pdu,
                                   &pdu_data->group_address);
    if (rc != 0)
        return rc;

    rc = tad_bps_nds_to_data_units(&proto_data->v3_query, layer_pdu,
                                   &pdu_data->v3_query);
    if (rc != 0)
        return rc;

    rc = tad_bps_nds_to_data_units(&proto_data->v3_report, layer_pdu,
                                   &pdu_data->v3_report);
    if (rc != 0)
        return rc;

    return 0;
}

/* See description in tad_igmp_impl.h */
void
tad_igmp_release_pdu_cb(csap_p csap, unsigned int layer, void *opaque)
{
    tad_igmp_proto_data     *proto_data;
    tad_igmp_proto_pdu_data *pdu_data = opaque;

    proto_data = csap_get_proto_spec_data(csap, layer);
    assert(proto_data != NULL);

    if (pdu_data != NULL)
    {
        tad_bps_free_pkt_frag_data(&proto_data->hdr,
                                   &pdu_data->hdr);
        tad_bps_free_pkt_frag_data(&proto_data->group_address,
                                   &pdu_data->group_address);
        tad_bps_free_pkt_frag_data(&proto_data->v3_query,
                                   &pdu_data->v3_query);
        tad_bps_free_pkt_frag_data(&proto_data->v3_report,
                                   &pdu_data->v3_report);
        free(pdu_data);
    }
}


/* See description in tad_igmp_impl.h */
te_errno
tad_igmp_confirm_tmpl_cb(csap_p csap, unsigned int layer,
                          asn_value *layer_pdu, void **p_opaque)
{
    te_errno                 rc;
    tad_igmp_proto_data     *proto_data;
    tad_igmp_proto_pdu_data *tmpl_data;
    uint8_t                  type;

    proto_data = csap_get_proto_spec_data(csap, layer);

    rc = tad_igmp_nds_to_pdu_data(csap, proto_data, layer_pdu, &tmpl_data);
    *p_opaque = tmpl_data;
    if (rc != 0)
        return rc;

    tmpl_data = *p_opaque;

    rc = tad_bps_confirm_send(&proto_data->hdr, &tmpl_data->hdr);
    if (rc != 0)
        return rc;

    if (tmpl_data->hdr.dus[0].du_type != TAD_DU_I32)
    {
        ERROR("Sending IGMP messages with not plain specification of "
              "the type is not supported yet");
        return TE_RC(TE_TAD_CSAP, TE_ENOSYS);
    }
    type = tmpl_data->hdr.dus[0].val_i32;

    switch (type)
    {
        case IGMPV3_HOST_MEMBERSHIP_REPORT:
            rc = tad_bps_confirm_send(&proto_data->v3_report,
                                      &tmpl_data->v3_report);
            if (rc != 0)
                return rc;
            break;

        case IGMP_HOST_MEMBERSHIP_QUERY:
        case IGMP_HOST_MEMBERSHIP_REPORT:
        case IGMPV2_HOST_MEMBERSHIP_REPORT:
        case IGMP_HOST_LEAVE_MESSAGE:
            rc = tad_bps_confirm_send(&proto_data->group_address,
                                      &tmpl_data->group_address);
            if (rc != 0)
                return rc;

            /* Check if it is V3 Query */
            if (type != IGMP_HOST_MEMBERSHIP_QUERY)
                break;

            /* Check if there is 'number-of-source' field */
            if (asn_get_length(layer_pdu, "number-of-sources") == -1)
                break;

            rc = tad_bps_confirm_send(&proto_data->v3_query,
                                      &tmpl_data->v3_query);
            if (rc != 0)
                return rc;

            break;

        default:
            return TE_RC(TE_TAD_CSAP, TE_EINVAL);
    }

    return rc;
}

/**
 * Callback to generate binary data per PDU.
 *
 * This function complies with tad_pkt_enum_cb prototype.
 */
static te_errno
tad_igmp_gen_bin_cb_per_pdu(tad_pkt *pdu, void *hdr)
{
    tad_pkt_seg    *seg = tad_pkt_first_seg(pdu);

    /* Copy header template to packet */
    assert(seg->data_ptr != NULL);
    memcpy(seg->data_ptr, hdr, seg->data_len);

    return 0;
}

/* See description in tad_igmp_impl.h */
te_errno
tad_igmp_gen_bin_cb(csap_p csap, unsigned int layer,
                    const asn_value *tmpl_pdu, void *opaque,
                    const tad_tmpl_arg_t *args, size_t arg_num,
                    tad_pkts *sdus, tad_pkts *pdus)
{
    tad_igmp_proto_data     *proto_data;
    tad_igmp_proto_pdu_data *tmpl_data = opaque;

    te_errno     rc;
    unsigned int bitoff;
    uint8_t      hdr[TE_TAD_IGMP_MAXLEN];
    uint8_t      type;


    assert(csap != NULL);
    F_ENTRY("(%d:%u) tmpl_pdu=%p args=%p arg_num=%u sdus=%p pdus=%p",
            csap->id, layer, (void *)tmpl_pdu, (void *)args,
            (unsigned)arg_num, sdus, pdus);

    proto_data = csap_get_proto_spec_data(csap, layer);

    /* Generate binary template of the header */
    bitoff = 0;
    rc = tad_bps_pkt_frag_gen_bin(&proto_data->hdr, &tmpl_data->hdr,
                                  args, arg_num, hdr,
                                  &bitoff, sizeof(hdr) << 3);
    if (rc != 0)
    {
        ERROR("%s(): tad_bps_pkt_frag_gen_bin failed for addresses: %r",
              __FUNCTION__, rc);
        return rc;
    }
    assert((bitoff & 7) == 0);

    type = hdr[0];
    switch (type)
    {
        case IGMPV3_HOST_MEMBERSHIP_REPORT:
            rc = tad_bps_pkt_frag_gen_bin(&proto_data->v3_report,
                                          &tmpl_data->v3_report,
                                          args, arg_num, hdr,
                                          &bitoff, sizeof(hdr) << 3);
            if (rc != 0)
            {
                ERROR("%s(): tad_bps_pkt_frag_gen_bin failed for addresses: %r",
                      __FUNCTION__, rc);
                return rc;
            }
            break;

        case IGMP_HOST_MEMBERSHIP_QUERY:
        case IGMP_HOST_MEMBERSHIP_REPORT:
        case IGMPV2_HOST_MEMBERSHIP_REPORT:
        case IGMP_HOST_LEAVE_MESSAGE:
            rc = tad_bps_pkt_frag_gen_bin(&proto_data->group_address,
                                          &tmpl_data->group_address,
                                          args, arg_num, hdr,
                                          &bitoff, sizeof(hdr) << 3);
            if (rc != 0)
            {
                ERROR("%s(): tad_bps_pkt_frag_gen_bin failed for addresses: %r",
                      __FUNCTION__, rc);
                return rc;
            }

            /* Check if it is V3 Query */
            if (type != IGMP_HOST_MEMBERSHIP_QUERY)
                break;

            /* Check if there is 'number-of-source' field */
            if (asn_get_length(tmpl_pdu, "number-of-sources") == -1)
                break;

            rc = tad_bps_pkt_frag_gen_bin(&proto_data->v3_query,
                                          &tmpl_data->v3_query,
                                          args, arg_num, hdr,
                                          &bitoff, sizeof(hdr) << 3);
            if (rc != 0)
            {
                ERROR("%s(): tad_bps_pkt_frag_gen_bin failed for addresses: %r",
                      __FUNCTION__, rc);
                return rc;
            }
            break;

        default:
            return TE_RC(TE_TAD_CSAP, TE_EINVAL);
    }

    assert((bitoff & 7) == 0);

    /* IGMPv2 layer does no fragmentation, just copy all SDUs to PDUs */
    tad_pkts_move(pdus, sdus);

    /* Allocate and add IGMPv2 header to all packets */
    rc = tad_pkts_add_new_seg(pdus, TRUE, NULL, bitoff >> 3, NULL);
    if (rc != 0)
        return rc;

    /* Per-PDU processing */
    rc = tad_pkt_enumerate(pdus, tad_igmp_gen_bin_cb_per_pdu, hdr);
    if (rc != 0)
    {
        ERROR("Failed to process IGMP PDUs: %r", rc);
        return rc;
    }

    return 0;
}


/* See description in tad_igmp_impl.h */
te_errno
tad_igmp_confirm_ptrn_cb(csap_p csap, unsigned int layer,
                         asn_value *layer_pdu, void **p_opaque)
{
    te_errno                    rc;
    tad_igmp_proto_data        *proto_data;
    tad_igmp_proto_pdu_data    *ptrn_data;

    F_ENTRY("(%d:%u) layer_pdu=%p", csap->id, layer,
            (void *)layer_pdu);

    proto_data = csap_get_proto_spec_data(csap, layer);

    rc = tad_igmp_nds_to_pdu_data(csap, proto_data, layer_pdu, &ptrn_data);
    *p_opaque = ptrn_data;

    return rc;
}


/* See description in tad_igmp_impl.h */
te_errno
tad_igmp_match_pre_cb(csap_p              csap,
                      unsigned int        layer,
                      tad_recv_pkt_layer *meta_pkt_layer)
{
    tad_igmp_proto_data        *proto_data;
    tad_igmp_proto_pdu_data    *pkt_data;
    te_errno                    rc;

    proto_data = csap_get_proto_spec_data(csap, layer);

    pkt_data = malloc(sizeof(*pkt_data));
    if (pkt_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    meta_pkt_layer->opaque = pkt_data;

    rc = tad_bps_pkt_frag_match_pre(&proto_data->hdr, &pkt_data->hdr);
    if (rc != 0)
        return rc;

    rc = tad_bps_pkt_frag_match_pre(&proto_data->group_address, &pkt_data->group_address);
    if (rc != 0)
        return rc;

    rc = tad_bps_pkt_frag_match_pre(&proto_data->v3_query, &pkt_data->v3_query);
    if (rc != 0)
        return rc;

    rc = tad_bps_pkt_frag_match_pre(&proto_data->v3_report, &pkt_data->v3_report);
    if (rc != 0)
        return rc;

    return 0;
}


/* See description in tad_igmp_impl.h */
te_errno
tad_igmp_match_post_cb(csap_p              csap,
                       unsigned int        layer,
                       tad_recv_pkt_layer *meta_pkt_layer)
{
    tad_igmp_proto_data        *proto_data;
    tad_igmp_proto_pdu_data    *pkt_data = meta_pkt_layer->opaque;
    tad_pkt                    *pkt;
    te_errno                    rc;
    unsigned int                bitoff = 0;

    if (~csap->state & CSAP_STATE_RESULTS)
        return 0;

    if ((meta_pkt_layer->nds = asn_init_value(ndn_igmp_message)) == NULL)
    {
        ERROR_ASN_INIT_VALUE(ndn_igmp_message);
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    }

    proto_data = csap_get_proto_spec_data(csap, layer);
    pkt = tad_pkts_first_pkt(&meta_pkt_layer->pkts);

    rc = tad_bps_pkt_frag_match_post(&proto_data->hdr, &pkt_data->hdr,
                                     pkt, &bitoff, meta_pkt_layer->nds);
    if (rc != 0)
        return rc;

    /* TODO: not sure post-check should be so simple */

    rc = tad_bps_pkt_frag_match_post(&proto_data->group_address,
                                     &pkt_data->group_address,
                                     pkt, &bitoff, meta_pkt_layer->nds);
    if (rc != 0)
        return rc;

    rc = tad_bps_pkt_frag_match_post(&proto_data->v3_query,
                                     &pkt_data->v3_query,
                                     pkt, &bitoff, meta_pkt_layer->nds);
    if (rc != 0)
        return rc;

    rc = tad_bps_pkt_frag_match_post(&proto_data->v3_report,
                                     &pkt_data->v3_report,
                                     pkt, &bitoff, meta_pkt_layer->nds);
    if (rc != 0)
        return rc;

    return rc;
}


/* See description in tad_igmp_impl.h */
te_errno
tad_igmp_match_do_cb(csap_p           csap,
                     unsigned int     layer,
                     const asn_value *ptrn_pdu,
                     void            *ptrn_opaque,
                     tad_recv_pkt    *meta_pkt,
                     tad_pkt         *pdu,
                     tad_pkt         *sdu)
{
    tad_igmp_proto_data     *proto_data;
    tad_igmp_proto_pdu_data *ptrn_data = ptrn_opaque;
    tad_igmp_proto_pdu_data *pkt_data  = meta_pkt->layers[layer].opaque;
    te_errno                 rc;
    unsigned int             bitoff    = 0;
    unsigned int             len;
    int                      type;

    UNUSED(ptrn_pdu);

    if (tad_pkt_len(pdu) < 8) /* FIXME */
    {
        F_VERB(CSAP_LOG_FMT "PDU is too small to be ICMPv4 datagram",
               CSAP_LOG_ARGS(csap));
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
        F_VERB(CSAP_LOG_FMT "Match PDU vs IGMP header failed on bit "
               "offset %u: %r", CSAP_LOG_ARGS(csap),
               (unsigned)bitoff, rc);
        return rc;
    }

    type = pkt_data->hdr.dus[0].val_i32;
    switch (type)
    {
        case IGMPV3_HOST_MEMBERSHIP_REPORT:
            len = tad_pkt_len(pdu) - (bitoff >> 3) - 4;

            /* Reallocate Source Addresses List binary data */
            rc = tad_du_realloc(&pkt_data->v3_report.dus[2], len);
            if (rc != 0)
                return rc;

            rc = tad_bps_pkt_frag_match_do(&proto_data->v3_report,
                                           &ptrn_data->v3_report,
                                           &pkt_data->v3_report,
                                           pdu, &bitoff);
            if (rc != 0)
            {
                F_VERB(CSAP_LOG_FMT "Match PDU vs IGMP header failed on bit "
                       "offset %u: %r", CSAP_LOG_ARGS(csap),
                       (unsigned)bitoff, rc);
                return rc;
            }
            break;

        case IGMP_HOST_MEMBERSHIP_QUERY:
        case IGMP_HOST_MEMBERSHIP_REPORT:
        case IGMPV2_HOST_MEMBERSHIP_REPORT:
        case IGMP_HOST_LEAVE_MESSAGE:
            rc = tad_bps_pkt_frag_match_do(&proto_data->group_address,
                                           &ptrn_data->group_address,
                                           &pkt_data->group_address,
                                           pdu, &bitoff);
            if (rc != 0)
            {
                F_VERB(CSAP_LOG_FMT "Match PDU vs IGMP header failed on bit "
                       "offset %u: %r", CSAP_LOG_ARGS(csap),
                       (unsigned)bitoff, rc);
                return rc;
            }

            /* Check if it is V3 Query */
            if (type != IGMP_HOST_MEMBERSHIP_QUERY)
                break;

            len = tad_pkt_len(pdu) - (bitoff >> 3) - 4;
            if (len == 0)
                break;

            /* Reallocate Group Records List binary data */
            rc = tad_du_realloc(&pkt_data->v3_query.dus[5], len);
            if (rc != 0)
            {
                ERROR("DU realloc failed to set length to %d", len);
                return rc;
            }

            rc = tad_bps_pkt_frag_match_do(&proto_data->v3_query,
                                           &ptrn_data->v3_query,
                                           &pkt_data->v3_query,
                                           pdu, &bitoff);
            if (rc != 0)
            {
                F_VERB(CSAP_LOG_FMT "Match PDU vs IGMP header failed on bit "
                       "offset %u: %r", CSAP_LOG_ARGS(csap),
                       (unsigned)bitoff, rc);
                return rc;
            }

            break;

        default:
            F_VERB(CSAP_LOG_FMT "Match PDU vs IGMP header failed on bit "
                   "offset %u: %r", CSAP_LOG_ARGS(csap),
                   (unsigned)bitoff, rc);
            return TE_RC(TE_TAD_CSAP, TE_EINVAL);
    }

    /* There should be nothing remaining */
    rc = tad_pkt_get_frag(sdu, pdu, bitoff >> 3,
                          tad_pkt_len(pdu) - (bitoff >> 3),
                          TAD_PKT_GET_FRAG_ERROR);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Failed to prepare IPv4 SDU: %r",
              CSAP_LOG_ARGS(csap), rc);
        return rc;
    }

    EXIT(CSAP_LOG_FMT "OK", CSAP_LOG_ARGS(csap));

    return 0;
}

