/** @file
 * @brief TAD Ethernet
 *
 * Traffic Application Domain Command Handler.
 * Ethernet CSAP layer-related callbacks.
 *
 * See IEEE 802.1d, 802.1q.
 *
 * Copyright (C) 2003-2006 Test Environment authors (see file AUTHORS
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
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAD Ethernet"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif
#if HAVE_NET_IF_ETHER_H
#include <net/if_ether.h>
#endif
//#include <fcntl.h>

#include "te_defs.h"
#include "logger_api.h"
#include "logger_ta_fast.h"

#include "tad_bps.h"
#include "tad_eth_impl.h"


/**
 * Ethernet layer specific data
 */
typedef struct tad_eth_proto_data {
    tad_bps_pkt_frag_def        hdr_d;
    tad_bps_pkt_frag_def        hdr_q;
} tad_eth_proto_data;

/**
 * Ethernet layer specific data for PDU processing (both send and
 * receive).
 */
typedef struct tad_eth_proto_pdu_data {
    tad_bps_pkt_frag_data   hdr;
} tad_eth_proto_pdu_data;


/**
 * Definition of 802.1d Ethernet header.
 */
static const tad_bps_pkt_frag tad_802_1d_bps_hdr[] =
{
    { "dst-addr", 48, NDN_TAG_ETH_DST,
      NDN_TAG_ETH_REMOTE, NDN_TAG_ETH_LOCAL, 0, TAD_DU_OCTS },
    { "src-addr", 48, NDN_TAG_ETH_SRC,
      NDN_TAG_ETH_LOCAL, NDN_TAG_ETH_REMOTE, 0, TAD_DU_OCTS },
    { "eth-type", 16, BPS_FLD_SIMPLE(NDN_TAG_ETH_TYPE_LEN), TAD_DU_I32 },
};

/**
 * Definition of 802.1q Ethernet header.
 */
static const tad_bps_pkt_frag tad_802_1q_bps_hdr[] =
{
    { "dst-addr", 48, NDN_TAG_ETH_DST,
      NDN_TAG_ETH_REMOTE, NDN_TAG_ETH_LOCAL, 0, TAD_DU_OCTS },
    { "src-addr", 48, NDN_TAG_ETH_SRC,
      NDN_TAG_ETH_LOCAL, NDN_TAG_ETH_REMOTE, 0, TAD_DU_OCTS },
    { "tpid",     16, BPS_FLD_CONST(ETH_TAGGED_TYPE_LEN), TAD_DU_I32 },
    { "priority",  3, BPS_FLD_SIMPLE(NDN_TAG_ETH_PRIO), TAD_DU_I32 },
    { "cfi",       1, BPS_FLD_SIMPLE(NDN_TAG_ETH_CFI), TAD_DU_I32 },
    { "vlan-id",  12, BPS_FLD_SIMPLE(NDN_TAG_ETH_VLAN_ID), TAD_DU_I32 },
    { "eth-type", 16, BPS_FLD_SIMPLE(NDN_TAG_ETH_TYPE_LEN), TAD_DU_I32 },
};


/* See description in tad_eth_impl.h */
te_errno
tad_eth_init_cb(csap_p csap, unsigned int layer)
{
    te_errno            rc;
    tad_eth_proto_data *proto_data;
    const asn_value    *layer_nds;

    proto_data = calloc(1, sizeof(*proto_data));
    if (proto_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    csap_set_proto_spec_data(csap, layer, proto_data);

    layer_nds = csap->layers[layer].csap_layer_pdu;

    rc = tad_bps_pkt_frag_init(tad_802_1d_bps_hdr,
                               TE_ARRAY_LEN(tad_802_1d_bps_hdr),
                               layer_nds, &proto_data->hdr_d);
    if (rc != 0)
        return rc;

    rc = tad_bps_pkt_frag_init(tad_802_1q_bps_hdr,
                               TE_ARRAY_LEN(tad_802_1q_bps_hdr),
                               layer_nds, &proto_data->hdr_q);
    if (rc != 0)
        return rc;

    /* FIXME */
    if (layer > 0 &&
        proto_data->hdr_d.tx_def[2].du_type == TAD_DU_UNDEF &&
        proto_data->hdr_d.rx_def[2].du_type == TAD_DU_UNDEF)
    {
        uint16_t    eth_type;

        VERB("%s(): eth-type is not defined, try to guess", __FUNCTION__);
        switch (csap->layers[layer - 1].proto_tag)
        {
            case TE_PROTO_IP4:
                eth_type = ETHERTYPE_IP;
                break;

            case TE_PROTO_ARP:
                eth_type = ETHERTYPE_ARP;
                break;

            default:
                eth_type = 0;
                break;
        }
        if (eth_type != 0)
        {
            INFO("%s(): Guessed eth-type is 0x%x", __FUNCTION__, eth_type);
            proto_data->hdr_d.tx_def[2].du_type = TAD_DU_I32;
            proto_data->hdr_d.tx_def[2].val_i32 = eth_type;
            proto_data->hdr_d.rx_def[2].du_type = TAD_DU_I32;
            proto_data->hdr_d.rx_def[2].val_i32 = eth_type;
        }
    }

    return rc;
}

/* See description in tad_eth_impl.h */
te_errno
tad_eth_destroy_cb(csap_p csap, unsigned int layer)
{
    tad_eth_proto_data *proto_data;

    proto_data = csap_get_proto_spec_data(csap, layer);
    csap_set_proto_spec_data(csap, layer, NULL);

    tad_bps_pkt_frag_free(&proto_data->hdr_d);
    tad_bps_pkt_frag_free(&proto_data->hdr_q);

    free(proto_data);

    return 0;
}


/* See description in tad_eth_impl.h */
te_errno
tad_eth_confirm_tmpl_cb(csap_p csap, unsigned int layer,
                        asn_value *layer_pdu, void **p_opaque)
{
    te_errno                    rc;
    tad_eth_proto_data         *proto_data;
    tad_eth_proto_pdu_data     *tmpl_data;

    proto_data = csap_get_proto_spec_data(csap, layer);
    tmpl_data = malloc(sizeof(*tmpl_data));
    if (tmpl_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    *p_opaque = tmpl_data;

    rc = tad_bps_nds_to_data_units(&proto_data->hdr_d, layer_pdu,
                                   &tmpl_data->hdr);
    if (rc != 0)
        return rc;

    rc = tad_bps_confirm_send(&proto_data->hdr_d, &tmpl_data->hdr);
    if (rc != 0)
        return rc;

    return rc;
}

/* See description in tad_eth_impl.h */
void
tad_eth_release_tmpl_cb(csap_p csap, unsigned int layer, void *opaque)
{
    tad_eth_proto_data     *proto_data;
    tad_eth_proto_pdu_data *tmpl_data = opaque;

    proto_data = csap_get_proto_spec_data(csap, layer);
    assert(proto_data != NULL);

    if (tmpl_data != NULL)
    {
        tad_bps_free_pkt_frag_data(&proto_data->hdr_d, &tmpl_data->hdr);
        free(tmpl_data);
    }
}


/**
 * Check length of packet as Ethernet frame. If frame is too small,
 * add a new segment with data filled in by zeros.
 *
 * This function complies with tad_pkt_enum_cb prototype.
 */
static te_errno
tad_eth_check_frame_len(tad_pkt *pkt, void *opaque)
{
    ssize_t tailer_len = (ETHER_MIN_LEN - ETHER_CRC_LEN) -
                         tad_pkt_len(pkt);

    UNUSED(opaque);
    if (tailer_len > 0)
    {
        tad_pkt_seg *seg = tad_pkt_alloc_seg(NULL, tailer_len, NULL);

        if (seg == NULL)
        {
            ERROR("%s(): Failed to allocate a new segment",
                  __FUNCTION__);
            return TE_RC(TE_TAD_PKT, TE_ENOMEM);
        }
        memset(seg->data_ptr, 0, seg->data_len);
        tad_pkt_append_seg(pkt, seg);
    }
    return 0;
}

/* See description in tad_eth_impl.h */
te_errno
tad_eth_gen_bin_cb(csap_p csap, unsigned int layer,
                   const asn_value *tmpl_pdu, void *opaque,
                   const tad_tmpl_arg_t *args, size_t arg_num, 
                   tad_pkts *sdus, tad_pkts *pdus)
{
    tad_eth_proto_data     *proto_data;
    tad_eth_proto_pdu_data *tmpl_data = opaque;

    te_errno    rc;
    size_t      bitlen;
    size_t      bitoff;
    uint8_t    *data;
    size_t      len;


    assert(csap != NULL);
    F_ENTRY("(%d:%u) tmpl_pdu=%p args=%p arg_num=%u sdus=%p pdus=%p",
            csap->id, layer, (void *)tmpl_pdu, (void *)args,
            (unsigned)arg_num, sdus, pdus);

    proto_data = csap_get_proto_spec_data(csap, layer);

    bitlen = tad_bps_pkt_frag_data_bitlen(&proto_data->hdr_d,
                                          &tmpl_data->hdr);
    if ((bitlen & 7) != 0)
    {
        ERROR("Unexpected lengths: total - %u bits", bitlen);
        return TE_RC(TE_TAD_CSAP, TE_EOPNOTSUPP);
    }

    len = (bitlen >> 3);
    data = malloc(len);
    if (data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    bitoff = 0;

    rc = tad_bps_pkt_frag_gen_bin(&proto_data->hdr_d, &tmpl_data->hdr,
                                  args, arg_num, data, &bitoff, bitlen);
    if (rc != 0)
    {
        ERROR("%s(): tad_bps_pkt_frag_gen_bin failed for header: %r",
              __FUNCTION__, rc);
        free(data);
        return rc;
    }
    assert(bitoff == bitlen);

    /* Move all SDUs to PDUs */
    tad_pkts_move(pdus, sdus);
    /* 
     * Add header segment to each PDU. All segments refer to the same
     * memory. Free function is set for segment of the first packet only.
     */
    rc = tad_pkts_add_new_seg(pdus, TRUE, data, len,
                              tad_pkt_seg_data_free);
    if (rc != 0)
    {
        free(data);
        return rc;
    }

    rc = tad_pkt_enumerate(pdus, tad_eth_check_frame_len, NULL);
    if (rc != 0)
    {
        ERROR("Failed to check length of Ethernet frames to send: %r", rc);
        return rc;
    }

    return 0;
}


/* See description in tad_eth_impl.h */
te_errno
tad_eth_confirm_ptrn_cb(csap_p csap, unsigned int layer,
                        asn_value *layer_pdu, void **p_opaque)
{
    te_errno                rc;
    tad_eth_proto_data     *proto_data;
    tad_eth_proto_pdu_data *ptrn_data;

    F_ENTRY("(%d:%u) layer_pdu=%p", csap->id, layer,
            (void *)layer_pdu);

    proto_data = csap_get_proto_spec_data(csap, layer);
    ptrn_data = malloc(sizeof(*ptrn_data));
    if (ptrn_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    *p_opaque = ptrn_data;

    rc = tad_bps_nds_to_data_units(&proto_data->hdr_d, layer_pdu,
                                   &ptrn_data->hdr);
    if (rc != 0)
        return rc;

    return rc;
}

/* See description in tad_eth_impl.h */
void
tad_eth_release_ptrn_cb(csap_p csap, unsigned int layer, void *opaque)
{
    tad_eth_proto_data     *proto_data;
    tad_eth_proto_pdu_data *ptrn_data = opaque;

    proto_data = csap_get_proto_spec_data(csap, layer);
    assert(proto_data != NULL);

    if (ptrn_data != NULL)
    {
        tad_bps_free_pkt_frag_data(&proto_data->hdr_d, &ptrn_data->hdr);
        free(ptrn_data);
    }
}


/* See description in tad_eth_impl.h */
te_errno
tad_eth_match_pre_cb(csap_p              csap,
                     unsigned int        layer,
                     tad_recv_pkt_layer *meta_pkt_layer)
{
    tad_eth_proto_data     *proto_data;
    tad_eth_proto_pdu_data *pkt_data;
    te_errno                rc;

    proto_data = csap_get_proto_spec_data(csap, layer);

    pkt_data = malloc(sizeof(*pkt_data));
    if (pkt_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    meta_pkt_layer->opaque = pkt_data;

    rc = tad_bps_pkt_frag_match_pre(&proto_data->hdr_d, &pkt_data->hdr);

    return rc;
}

/* See description in tad_eth_impl.h */
te_errno
tad_eth_match_post_cb(csap_p              csap,
                      unsigned int        layer,
                      tad_recv_pkt_layer *meta_pkt_layer)
{
    tad_eth_proto_data     *proto_data;
    tad_eth_proto_pdu_data *pkt_data = meta_pkt_layer->opaque;
    te_errno                rc;
    size_t                  bitoff = 0;

    proto_data = csap_get_proto_spec_data(csap, layer);

    if (csap->state & CSAP_STATE_RESULTS)
        meta_pkt_layer->nds = asn_init_value(ndn_eth_header);

    rc = tad_bps_pkt_frag_match_post(&proto_data->hdr_d, &pkt_data->hdr,
             tad_pkts_first_pkt(&meta_pkt_layer->pkts),
             &bitoff, meta_pkt_layer->nds);

    return rc;
}

/* See description in tad_eth_impl.h */
te_errno
tad_eth_match_do_cb(csap_p           csap,
                    unsigned int     layer,
                    const asn_value *ptrn_pdu,
                    void            *ptrn_opaque,
                    tad_recv_pkt    *meta_pkt,
                    tad_pkt         *pdu,
                    tad_pkt         *sdu)
{
    tad_eth_proto_data     *proto_data;
    tad_eth_proto_pdu_data *ptrn_data = ptrn_opaque;
    tad_eth_proto_pdu_data *pkt_data = meta_pkt->layers[layer].opaque;
    te_errno                rc;
    size_t                  bitoff = 0;

    UNUSED(ptrn_pdu);

    if (tad_pkt_len(pdu) < ETHER_HDR_LEN)
    {
        VERB(CSAP_LOG_FMT "PDU is too small to be Ethernet frame",
             CSAP_LOG_ARGS(csap));
        return TE_RC(TE_TAD_CSAP, TE_ETADNOTMATCH);
    }
  
    proto_data = csap_get_proto_spec_data(csap, layer);

    assert(proto_data != NULL);
    assert(ptrn_data != NULL);
    assert(pkt_data != NULL);
    rc = tad_bps_pkt_frag_match_do(&proto_data->hdr_d, &ptrn_data->hdr,
                                   &pkt_data->hdr, pdu, &bitoff);
    if (rc != 0)
    {
        VERB(CSAP_LOG_FMT "Match PDU vs Ethernet header failed on bit "
             "offset %u: %r", CSAP_LOG_ARGS(csap), (unsigned)bitoff, rc);
        return rc;
    }

    assert(bitoff == (ETHER_HDR_LEN << 3));

    rc = tad_pkt_get_frag(sdu, pdu, bitoff >> 3,
                          tad_pkt_len(pdu) - (bitoff >> 3),
                          TAD_PKT_GET_FRAG_ERROR);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Failed to prepare Ethernet SDU: %r",
              CSAP_LOG_ARGS(csap), rc);
        return rc;
    }

    EXIT(CSAP_LOG_FMT "OK", CSAP_LOG_ARGS(csap));

    return 0;
}
