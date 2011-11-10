/** @file
 * @brief TAD IP Stack
 *
 * Traffic Application Domain Command Handler.
 * ICMPv4 CSAP layer-related callbacks.
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

#define TE_LGR_USER     "TAD ICMPv4"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_NETINET_IN_SYSTM_H
#include <netinet/in_systm.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif
#if HAVE_NETINET_IP_ICMP_H
#include <netinet/ip_icmp.h>
#endif

#include "te_defs.h"
#include "te_alloc.h"
#include "te_ethernet.h"
#include "logger_api.h"
#include "logger_ta_fast.h"

#include "tad_bps.h"
#include "tad_ipstack_impl.h"


/** Maximum of ICMP header length (Timestamp message) */
#define TE_TAD_ICMP4_MAXLEN     (20)

/**
 * ICMPv4 layer specific data
 */
typedef struct tad_icmp4_proto_data {
    tad_bps_pkt_frag_def    hdr;
    tad_bps_pkt_frag_def    unused;
    tad_bps_pkt_frag_def    pp;
    tad_bps_pkt_frag_def    redirect;
    tad_bps_pkt_frag_def    echo_info;
    tad_bps_pkt_frag_def    ts;
} tad_icmp4_proto_data;

/**
 * IPv4 layer specific data for PDU processing (both send and
 * receive).
 */
typedef struct tad_icmp4_proto_pdu_data {
    tad_bps_pkt_frag_data   hdr;
    tad_bps_pkt_frag_data   unused;
    tad_bps_pkt_frag_data   pp;
    tad_bps_pkt_frag_data   redirect;
    tad_bps_pkt_frag_data   echo_info;
    tad_bps_pkt_frag_data   ts;
} tad_icmp4_proto_pdu_data;


/**
 * Definition of Internet Control Message Protocol for Internet
 * Protocol version 4 (ICMPv4) header.
 */
static const tad_bps_pkt_frag tad_icmp4_bps_hdr[] =
{
    { "type",      8, BPS_FLD_NO_DEF(NDN_TAG_ICMP4_TYPE),
      TAD_DU_I32, TRUE },
    { "code",      8, BPS_FLD_NO_DEF(NDN_TAG_ICMP4_CODE),
      TAD_DU_I32, FALSE },
    { "checksum", 16, BPS_FLD_CONST_DEF(NDN_TAG_ICMP4_CHECKSUM, 0),
      TAD_DU_I32, TRUE },
};

/**
 * Definition of ICMPv4 unused field in the header.
 */
static const tad_bps_pkt_frag tad_icmp4_unused_bps_hdr[] =
{
    { "unused",   32, BPS_FLD_CONST_DEF(NDN_TAG_ICMP4_UNUSED, 0),
      TAD_DU_I32, FALSE },
};

/**
 * Definition of ICMPv4 Parameter Problem message subheader.
 */
static const tad_bps_pkt_frag tad_icmp4_pp_bps_hdr[] =
{
    { "ptr",       8, BPS_FLD_NO_DEF(NDN_TAG_ICMP4_PP_PTR),
      TAD_DU_I32, FALSE },
    { "unused",   24, BPS_FLD_CONST_DEF(NDN_TAG_ICMP4_UNUSED, 0),
      TAD_DU_I32, FALSE },
};

/**
 * Definition of ICMPv4 Redirect Message subheader.
 */
static const tad_bps_pkt_frag tad_icmp4_redirect_bps_hdr[] =
{
    { "gw",       32, BPS_FLD_NO_DEF(NDN_TAG_ICMP4_REDIRECT_GW),
      TAD_DU_I32, FALSE },
};

/**
 * Definition of ICMPv4 Echo or Echo Reply Message subheader.
 */
static const tad_bps_pkt_frag tad_icmp4_echo_bps_hdr[] =
{
    { "id",       16, BPS_FLD_NO_DEF(NDN_TAG_ICMP4_ID),
      TAD_DU_I32, FALSE },
    { "seq",      16, BPS_FLD_NO_DEF(NDN_TAG_ICMP4_SEQ),
      TAD_DU_I32, FALSE },
};

/**
 * Definition of ICMPv4 Timestamp or Timestamp Reply Message subheader.
 */
static const tad_bps_pkt_frag tad_icmp4_ts_bps_hdr[] =
{
    { "id",       16, BPS_FLD_NO_DEF(NDN_TAG_ICMP4_ID),
      TAD_DU_I32, FALSE },
    { "seq",      16, BPS_FLD_NO_DEF(NDN_TAG_ICMP4_SEQ),
      TAD_DU_I32, FALSE },
    { "orig-ts",  32, BPS_FLD_NO_DEF(NDN_TAG_ICMP4_ORIG_TS),
      TAD_DU_I32, FALSE },
    { "rx-ts",    32, BPS_FLD_NO_DEF(NDN_TAG_ICMP4_RX_TS),
      TAD_DU_I32, FALSE },
    { "tx-ts",    32, BPS_FLD_NO_DEF(NDN_TAG_ICMP4_TX_TS),
      TAD_DU_I32, FALSE },
};


/* See description tad_ipstack_impl.h */
te_errno
tad_icmp4_init_cb(csap_p csap, unsigned int layer)
{
    te_errno                rc;
    tad_icmp4_proto_data   *proto_data;
    const asn_value        *layer_nds;

    proto_data = TE_ALLOC(sizeof(*proto_data));
    if (proto_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    csap_set_proto_spec_data(csap, layer, proto_data);

    layer_nds = csap->layers[layer].nds;

    rc = tad_bps_pkt_frag_init(tad_icmp4_bps_hdr,
                               TE_ARRAY_LEN(tad_icmp4_bps_hdr),
                               layer_nds, &proto_data->hdr);
    if (rc != 0)
        return rc;

    rc = tad_bps_pkt_frag_init(tad_icmp4_unused_bps_hdr,
                               TE_ARRAY_LEN(tad_icmp4_unused_bps_hdr),
                               NULL, &proto_data->unused);
    if (rc != 0)
        return rc;

    rc = tad_bps_pkt_frag_init(tad_icmp4_pp_bps_hdr,
                               TE_ARRAY_LEN(tad_icmp4_pp_bps_hdr),
                               NULL, &proto_data->pp);
    if (rc != 0)
        return rc;

    rc = tad_bps_pkt_frag_init(tad_icmp4_redirect_bps_hdr,
                               TE_ARRAY_LEN(tad_icmp4_redirect_bps_hdr),
                               NULL, &proto_data->redirect);
    if (rc != 0)
        return rc;

    rc = tad_bps_pkt_frag_init(tad_icmp4_echo_bps_hdr,
                               TE_ARRAY_LEN(tad_icmp4_echo_bps_hdr),
                               NULL, &proto_data->echo_info);
    if (rc != 0)
        return rc;

    rc = tad_bps_pkt_frag_init(tad_icmp4_ts_bps_hdr,
                               TE_ARRAY_LEN(tad_icmp4_ts_bps_hdr),
                               NULL, &proto_data->ts);
    if (rc != 0)
        return rc;

    return 0;
}

/* See description tad_ipstack_impl.h */
te_errno
tad_icmp4_destroy_cb(csap_p csap, unsigned int layer)
{
    tad_icmp4_proto_data *proto_data;

    proto_data = csap_get_proto_spec_data(csap, layer);
    csap_set_proto_spec_data(csap, layer, NULL);

    tad_bps_pkt_frag_free(&proto_data->hdr);
    tad_bps_pkt_frag_free(&proto_data->unused);
    tad_bps_pkt_frag_free(&proto_data->pp);
    tad_bps_pkt_frag_free(&proto_data->redirect);
    tad_bps_pkt_frag_free(&proto_data->echo_info);
    tad_bps_pkt_frag_free(&proto_data->ts);

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
tad_icmp4_nds_to_pdu_data(csap_p csap, tad_icmp4_proto_data *proto_data,
                          const asn_value *layer_pdu,
                          tad_icmp4_proto_pdu_data **p_pdu_data)
{
    te_errno                    rc;
    tad_icmp4_proto_pdu_data   *pdu_data;

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

    rc = tad_bps_nds_to_data_units(&proto_data->unused, layer_pdu,
                                   &pdu_data->unused);
    if (rc != 0)
        return rc;

    rc = tad_bps_nds_to_data_units(&proto_data->pp, layer_pdu,
                                   &pdu_data->pp);
    if (rc != 0)
        return rc;

    rc = tad_bps_nds_to_data_units(&proto_data->echo_info, layer_pdu,
                                   &pdu_data->echo_info);
    if (rc != 0)
        return rc;

    rc = tad_bps_nds_to_data_units(&proto_data->ts, layer_pdu,
                                   &pdu_data->ts);
    if (rc != 0)
        return rc;

    rc = tad_bps_nds_to_data_units(&proto_data->redirect, layer_pdu,
                                   &pdu_data->redirect);
    if (rc != 0)
        return rc;

    return 0;
}

/* See description in tad_ipstack_impl.h */
void
tad_icmp4_release_pdu_cb(csap_p csap, unsigned int layer, void *opaque)
{
    tad_icmp4_proto_data     *proto_data;
    tad_icmp4_proto_pdu_data *pdu_data = opaque;

    proto_data = csap_get_proto_spec_data(csap, layer);
    assert(proto_data != NULL);

    if (pdu_data != NULL)
    {
        tad_bps_free_pkt_frag_data(&proto_data->hdr,
                                   &pdu_data->hdr);
        tad_bps_free_pkt_frag_data(&proto_data->unused,
                                   &pdu_data->unused);
        tad_bps_free_pkt_frag_data(&proto_data->pp,
                                   &pdu_data->pp);
        tad_bps_free_pkt_frag_data(&proto_data->redirect,
                                   &pdu_data->redirect);
        tad_bps_free_pkt_frag_data(&proto_data->echo_info,
                                   &pdu_data->echo_info);
        tad_bps_free_pkt_frag_data(&proto_data->ts,
                                   &pdu_data->ts);
        free(pdu_data);
    }
}

/**
 * ICMP message fragment control structures by ICMP message type.
 */
static void
tad_icmp4_frag_structs_by_type(const unsigned int         type,
                               tad_icmp4_proto_data      *proto_data,
                               tad_icmp4_proto_pdu_data  *tmpl_data,
                               tad_bps_pkt_frag_def     **def,
                               tad_bps_pkt_frag_data    **data)
{
    switch (type)
    {
        default:
#ifdef ICMP_DEST_UNREACH
        case ICMP_DEST_UNREACH:
#elif defined(ICMP_UNREACH)
        case ICMP_UNREACH:
#else
#error Dont know type for ICMP destination unreachable message
#endif
#ifdef ICMP_TIME_EXCEEDED
        case ICMP_TIME_EXCEEDED:
#elif defined(ICMP_TIMXCEED)
        case ICMP_TIMXCEED:
#else
#error Dont know type for ICMP time exceeded message
#endif
#ifdef ICMP_SOURCE_QUENCH
        case ICMP_SOURCE_QUENCH:
#elif defined(ICMP_SOURCEQUENCH)
        case ICMP_SOURCEQUENCH:
#else
#error Dont know type for ICMP source quench message
#endif
            *def = &proto_data->unused;
            *data = &tmpl_data->unused;
            break;

        case ICMP_REDIRECT:
            *def = &proto_data->redirect;
            *data = &tmpl_data->redirect;
            break;

        case ICMP_ECHO:
        case ICMP_ECHOREPLY:
#ifdef ICMP_INFO_REQUEST
        case ICMP_INFO_REQUEST:
#elif defined(ICMP_IREQ)
        case ICMP_IREQ:
#else
#error Dont know type for ICMP information request
#endif
#ifdef ICMP_INFO_REPLY
        case ICMP_INFO_REPLY:
#elif defined(ICMP_IREQREPLY)
        case ICMP_IREQREPLY:
#else
#error Dont know type for ICMP information reply
#endif
            *def = &proto_data->echo_info;
            *data = &tmpl_data->echo_info;
            break;

#ifdef ICMP_PARAMETERPROB
        case ICMP_PARAMETERPROB:
#elif defined(ICMP_PARAMPROB)
        case ICMP_PARAMPROB:
#else
#error Dont know type for ICMP parameter problem message
#endif
            *def = &proto_data->pp;
            *data = &tmpl_data->pp;
            break;

#ifdef ICMP_TIMESTAMP
        case ICMP_TIMESTAMP:
#elif defined(ICMP_TSTAMP)
        case ICMP_TSTAMP:
#else
#error Dont know type for ICMP timestamp request
#endif
#ifdef ICMP_TIMESTAMPREPLY
        case ICMP_TIMESTAMPREPLY:
#elif defined(ICMP_TSTAMPREPLY)
        case ICMP_TSTAMPREPLY:
#else
#error Dont know type for ICMP timestamp reply
#endif
            *def = &proto_data->ts;
            *data = &tmpl_data->ts;
            break;
    }
}

/* See description in tad_ipstack_impl.h */
te_errno
tad_icmp4_confirm_tmpl_cb(csap_p csap, unsigned int layer,
                          asn_value *layer_pdu, void **p_opaque)
{
    te_errno                    rc;
    tad_icmp4_proto_data       *proto_data;
    tad_icmp4_proto_pdu_data   *tmpl_data;
    tad_bps_pkt_frag_def       *add_def;
    tad_bps_pkt_frag_data      *add_data;

    proto_data = csap_get_proto_spec_data(csap, layer);

    rc = tad_icmp4_nds_to_pdu_data(csap, proto_data, layer_pdu, &tmpl_data);
    *p_opaque = tmpl_data;
    if (rc != 0)
        return rc;

    tmpl_data = *p_opaque;

    rc = tad_bps_confirm_send(&proto_data->hdr, &tmpl_data->hdr);
    if (rc != 0)
        return rc;

    if (tmpl_data->hdr.dus[0].du_type != TAD_DU_I32)
    {
        ERROR("Sending ICMP messages with not plain specification of "
              "the type is not supported yet");
        return TE_RC(TE_TAD_CSAP, TE_ENOSYS);
    }
    tad_icmp4_frag_structs_by_type(tmpl_data->hdr.dus[0].val_i32,
                                   proto_data, tmpl_data,
                                   &add_def, &add_data);
    rc = tad_bps_confirm_send(add_def, add_data);

    return rc;
}

/**
 * Callback to generate binary data per PDU.
 *
 * This function complies with tad_pkt_enum_cb prototype.
 */
static te_errno
tad_icmp4_gen_bin_cb_per_pdu(tad_pkt *pdu, void *hdr)
{
    tad_pkt_seg    *seg = tad_pkt_first_seg(pdu);

    /* Copy header template to packet */
    assert(seg->data_ptr != NULL);
    memcpy(seg->data_ptr, hdr, seg->data_len);

    return 0;
}

/* See description in tad_ipstack_impl.h */
te_errno
tad_icmp4_gen_bin_cb(csap_p csap, unsigned int layer,
                     const asn_value *tmpl_pdu, void *opaque,
                     const tad_tmpl_arg_t *args, size_t arg_num,
                     tad_pkts *sdus, tad_pkts *pdus)
{
    tad_icmp4_proto_data     *proto_data;
    tad_icmp4_proto_pdu_data *tmpl_data = opaque;
    tad_bps_pkt_frag_def     *add_def;
    tad_bps_pkt_frag_data    *add_data;

    te_errno        rc;
    unsigned int    bitoff;
    uint8_t         hdr[TE_TAD_ICMP4_MAXLEN];


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
    tad_icmp4_frag_structs_by_type(hdr[0], proto_data, tmpl_data,
                                   &add_def, &add_data);
    rc = tad_bps_pkt_frag_gen_bin(add_def, add_data,
                                  args, arg_num, hdr,
                                  &bitoff, sizeof(hdr) << 3);
    if (rc != 0)
    {
        ERROR("%s(): tad_bps_pkt_frag_gen_bin failed for addition: %r",
              __FUNCTION__, rc);
        return rc;
    }
    assert((bitoff & 7) == 0);

    /* ICMPv4 layer does no fragmentation, just copy all SDUs to PDUs */
    tad_pkts_move(pdus, sdus);

    /* Allocate and add ICMPv4 header to all packets */
    rc = tad_pkts_add_new_seg(pdus, TRUE, NULL, bitoff >> 3, NULL);
    if (rc != 0)
        return rc;

    /* Per-PDU processing - set correct length */
    rc = tad_pkt_enumerate(pdus, tad_icmp4_gen_bin_cb_per_pdu, hdr);
    if (rc != 0)
    {
        ERROR("Failed to process ICMPv4 PDUs: %r", rc);
        return rc;
    }

    return 0;
}



/* See description in tad_ipstack_impl.h */
te_errno
tad_icmp4_confirm_ptrn_cb(csap_p csap, unsigned int layer,
                          asn_value *layer_pdu, void **p_opaque)
{
    te_errno                    rc;
    tad_icmp4_proto_data       *proto_data;
    tad_icmp4_proto_pdu_data   *ptrn_data;

    F_ENTRY("(%d:%u) layer_pdu=%p", csap->id, layer,
            (void *)layer_pdu);

    proto_data = csap_get_proto_spec_data(csap, layer);

    rc = tad_icmp4_nds_to_pdu_data(csap, proto_data, layer_pdu, &ptrn_data);
    *p_opaque = ptrn_data;

    return rc;
}


/* See description in tad_ipstack_impl.h */
te_errno
tad_icmp4_match_pre_cb(csap_p              csap,
                       unsigned int        layer,
                       tad_recv_pkt_layer *meta_pkt_layer)
{
    tad_icmp4_proto_data       *proto_data;
    tad_icmp4_proto_pdu_data   *pkt_data;
    te_errno                    rc;

    proto_data = csap_get_proto_spec_data(csap, layer);

    pkt_data = malloc(sizeof(*pkt_data));
    if (pkt_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    meta_pkt_layer->opaque = pkt_data;

    rc = tad_bps_pkt_frag_match_pre(&proto_data->hdr, &pkt_data->hdr);
    if (rc != 0)
        return rc;

    rc = tad_bps_pkt_frag_match_pre(&proto_data->unused, &pkt_data->unused);
    if (rc != 0)
        return rc;

    rc = tad_bps_pkt_frag_match_pre(&proto_data->pp, &pkt_data->pp);
    if (rc != 0)
        return rc;

    rc = tad_bps_pkt_frag_match_pre(&proto_data->echo_info,
                                    &pkt_data->echo_info);
    if (rc != 0)
        return rc;

    rc = tad_bps_pkt_frag_match_pre(&proto_data->ts, &pkt_data->ts);
    if (rc != 0)
        return rc;

    rc = tad_bps_pkt_frag_match_pre(&proto_data->redirect,
                                    &pkt_data->redirect);
    if (rc != 0)
        return rc;

    return 0;
}

/* See description in tad_ipstack_impl.h */
te_errno
tad_icmp4_match_post_cb(csap_p              csap,
                        unsigned int        layer,
                        tad_recv_pkt_layer *meta_pkt_layer)
{
    tad_icmp4_proto_data       *proto_data;
    tad_icmp4_proto_pdu_data   *pkt_data = meta_pkt_layer->opaque;
    tad_pkt                    *pkt;
    te_errno                    rc;
    unsigned int                bitoff = 0;

    if (~csap->state & CSAP_STATE_RESULTS)
        return 0;

    if ((meta_pkt_layer->nds = asn_init_value(ndn_icmp4_message)) == NULL)
    {
        ERROR_ASN_INIT_VALUE(ndn_icmp4_message);
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
tad_icmp4_match_do_cb(csap_p           csap,
                      unsigned int     layer,
                      const asn_value *ptrn_pdu,
                      void            *ptrn_opaque,
                      tad_recv_pkt    *meta_pkt,
                      tad_pkt         *pdu,
                      tad_pkt         *sdu)
{
    tad_icmp4_proto_data       *proto_data;
    tad_icmp4_proto_pdu_data   *ptrn_data = ptrn_opaque;
    tad_icmp4_proto_pdu_data   *pkt_data = meta_pkt->layers[layer].opaque;
    te_errno                    rc;
    unsigned int                bitoff = 0;

    UNUSED(ptrn_pdu);

    if (tad_pkt_len(pdu) < 4) /* FIXME */
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
        F_VERB(CSAP_LOG_FMT "Match PDU vs IPv4 header failed on bit "
               "offset %u: %r", CSAP_LOG_ARGS(csap), (unsigned)bitoff, rc);
        return rc;
    }

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
