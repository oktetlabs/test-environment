/** @file
 * @brief TAD IP Stack
 *
 * Traffic Application Domain Command Handler.
 * ICMPv6 CSAP layer-related callbacks.
 *
 * Copyright (C) 2009 Test Environment authors (see file AUTHORS
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

#define TE_LGR_USER     "TAD ICMPv6"

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

/* TODO: chenge this value */
#define TE_TAD_ICMP6_MAXLEN     (20)

/**
 * ICMPv6 layer specific data
 */
typedef struct tad_icmp6_proto_data {
    tad_bps_pkt_frag_def    hdr;
    tad_bps_pkt_frag_def    echo_data;
} tad_icmp6_proto_data;

/**
 * IPv6 layer specific data for PDU processing (both send and
 * receive).
 */
typedef struct tad_icmp6_proto_pdu_data {
    tad_bps_pkt_frag_data   hdr;
    tad_bps_pkt_frag_data   echo_data;
} tad_icmp6_proto_pdu_data;


/**
 * Definition of Internet Control Message Protocol for Internet
 * Protocol version 6 (ICMPv6) header.
 */
static const tad_bps_pkt_frag tad_icmp6_bps_hdr[] =
{
    { "type",      8, BPS_FLD_NO_DEF(NDN_TAG_ICMP6_TYPE),
      TAD_DU_I32, TRUE },
    { "code",      8, BPS_FLD_NO_DEF(NDN_TAG_ICMP6_CODE),
      TAD_DU_I32, FALSE },
    { "checksum", 16, BPS_FLD_CONST_DEF(NDN_TAG_ICMP6_CHECKSUM, 0),
      TAD_DU_I32, TRUE },
};

/**
 * Definition of ICMPv6 Echo or Echo Reply Message subheader.
 */
static const tad_bps_pkt_frag tad_icmp6_echo_bps_hdr[] =
{
    { "id",       16, BPS_FLD_NO_DEF(NDN_TAG_ICMP6_ID),
      TAD_DU_I32, FALSE },
    { "seq",      16, BPS_FLD_NO_DEF(NDN_TAG_ICMP6_SEQ),
      TAD_DU_I32, FALSE },
};

/* See description tad_ipstack_impl.h */
te_errno
tad_icmp6_init_cb(csap_p csap, unsigned int layer)
{ 
    te_errno                rc;
    tad_icmp6_proto_data   *proto_data;
    const asn_value        *layer_nds;

    proto_data = TE_ALLOC(sizeof(*proto_data));
    if (proto_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    csap_set_proto_spec_data(csap, layer, proto_data);

    layer_nds = csap->layers[layer].nds;

    rc = tad_bps_pkt_frag_init(tad_icmp6_bps_hdr,
                               TE_ARRAY_LEN(tad_icmp6_bps_hdr),
                               layer_nds, &proto_data->hdr);

    if (rc != 0)
        return rc;

    rc = tad_bps_pkt_frag_init(tad_icmp6_echo_bps_hdr,
                               TE_ARRAY_LEN(tad_icmp6_echo_bps_hdr),
                               NULL, &proto_data->echo_data);
    return 0; 
}

/* See description tad_ipstack_impl.h */
te_errno
tad_icmp6_destroy_cb(csap_p csap, unsigned int layer)
{
    tad_icmp6_proto_data *proto_data;

    proto_data = csap_get_proto_spec_data(csap, layer);
    csap_set_proto_spec_data(csap, layer, NULL);

    tad_bps_pkt_frag_free(&proto_data->hdr);
    tad_bps_pkt_frag_free(&proto_data->echo_data);

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
tad_icmp6_nds_to_pdu_data(csap_p csap, tad_icmp6_proto_data *proto_data,
                          const asn_value *layer_pdu,
                          tad_icmp6_proto_pdu_data **p_pdu_data)
{
    te_errno                    rc;
    tad_icmp6_proto_pdu_data   *pdu_data;

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

    rc = tad_bps_nds_to_data_units(&proto_data->echo_data, layer_pdu,
                                   &pdu_data->echo_data);
    return 0;
}

/* See description in tad_ipstack_impl.h */
void
tad_icmp6_release_pdu_cb(csap_p csap, unsigned int layer, void *opaque)
{
    tad_icmp6_proto_data     *proto_data;
    tad_icmp6_proto_pdu_data *pdu_data = opaque;

    proto_data = csap_get_proto_spec_data(csap, layer);
    assert(proto_data != NULL);

    if (pdu_data != NULL)
    {
        tad_bps_free_pkt_frag_data(&proto_data->hdr,
                                   &pdu_data->hdr);
        tad_bps_free_pkt_frag_data(&proto_data->echo_data,
                                   &pdu_data->echo_data);
        free(pdu_data);
    }
}

/**
 * ICMP message fragment control structures by ICMP message type.
 */
static void
tad_icmp6_frag_structs_by_type(const unsigned int         type,
                               tad_icmp6_proto_data      *proto_data,
                               tad_icmp6_proto_pdu_data  *tmpl_data,
                               tad_bps_pkt_frag_def     **def,
                               tad_bps_pkt_frag_data    **data)
{
    /* TODO: write this function */
    UNUSED(type);

    *def = &proto_data->echo_data;
    *data = &tmpl_data->echo_data;
}

/* See description in tad_ipstack_impl.h */
te_errno
tad_icmp6_confirm_tmpl_cb(csap_p csap, unsigned int layer,
                          asn_value *layer_pdu, void **p_opaque)
{
    te_errno                    rc;
    tad_icmp6_proto_data       *proto_data;
    tad_icmp6_proto_pdu_data   *tmpl_data;
    tad_bps_pkt_frag_def       *add_def;
    tad_bps_pkt_frag_data      *add_data;

    proto_data = csap_get_proto_spec_data(csap, layer);

    rc = tad_icmp6_nds_to_pdu_data(csap, proto_data, layer_pdu, &tmpl_data);
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
    tad_icmp6_frag_structs_by_type(tmpl_data->hdr.dus[0].val_i32,
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
tad_icmp6_gen_bin_cb_per_pdu(tad_pkt *pdu, void *hdr)
{
    tad_pkt_seg    *seg = tad_pkt_first_seg(pdu);

    /* Copy header template to packet */
    assert(seg->data_ptr != NULL);
    memcpy(seg->data_ptr, hdr, seg->data_len);

    return 0;
}

/* See description in tad_ipstack_impl.h */
te_errno
tad_icmp6_gen_bin_cb(csap_p csap, unsigned int layer,
                     const asn_value *tmpl_pdu, void *opaque,
                     const tad_tmpl_arg_t *args, size_t arg_num, 
                     tad_pkts *sdus, tad_pkts *pdus)
{
    tad_icmp6_proto_data     *proto_data;
    tad_icmp6_proto_pdu_data *tmpl_data = opaque;
    tad_bps_pkt_frag_def     *add_def;
    tad_bps_pkt_frag_data    *add_data;

    te_errno        rc;
    unsigned int    bitoff;
    uint8_t         hdr[TE_TAD_ICMP6_MAXLEN];


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
    tad_icmp6_frag_structs_by_type(hdr[0], proto_data, tmpl_data,
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

    /* ICMPv6 layer does no fragmentation, just copy all SDUs to PDUs */
    tad_pkts_move(pdus, sdus);

    /* Allocate and add ICMPv6 header to all packets */
    rc = tad_pkts_add_new_seg(pdus, TRUE, NULL, bitoff >> 3, NULL);
    if (rc != 0)
        return rc;

    /* Per-PDU processing - set correct length */
    rc = tad_pkt_enumerate(pdus, tad_icmp6_gen_bin_cb_per_pdu, hdr);
    if (rc != 0)
    {
        ERROR("Failed to process ICMPv6 PDUs: %r", rc);
        return rc;
    }

    return 0;
}



/* See description in tad_ipstack_impl.h */
te_errno
tad_icmp6_confirm_ptrn_cb(csap_p csap, unsigned int layer,
                          asn_value *layer_pdu, void **p_opaque)
{
    te_errno                    rc;
    tad_icmp6_proto_data       *proto_data;
    tad_icmp6_proto_pdu_data   *ptrn_data;

    F_ENTRY("(%d:%u) layer_pdu=%p", csap->id, layer,
            (void *)layer_pdu);

    proto_data = csap_get_proto_spec_data(csap, layer);

    rc = tad_icmp6_nds_to_pdu_data(csap, proto_data, layer_pdu, &ptrn_data);
    *p_opaque = ptrn_data;

    return rc;
}


/* See description in tad_ipstack_impl.h */
te_errno
tad_icmp6_match_pre_cb(csap_p              csap,
                       unsigned int        layer,
                       tad_recv_pkt_layer *meta_pkt_layer)
{
    /* TODO: write this function */
    UNUSED(csap);
    UNUSED(layer);
    UNUSED(meta_pkt_layer);

    return 0;
}

/* See description in tad_ipstack_impl.h */
te_errno
tad_icmp6_match_post_cb(csap_p              csap,
                        unsigned int        layer,
                        tad_recv_pkt_layer *meta_pkt_layer)
{
    /* TODO: write this function */
    UNUSED(csap);
    UNUSED(layer);
    UNUSED(meta_pkt_layer);
    
    return 0;
}

/* See description in tad_ipstack_impl.h */
te_errno
tad_icmp6_match_do_cb(csap_p           csap,
                      unsigned int     layer,
                      const asn_value *ptrn_pdu,
                      void            *ptrn_opaque,
                      tad_recv_pkt    *meta_pkt,
                      tad_pkt         *pdu,
                      tad_pkt         *sdu)
{
    /* TODO: write this function */
    UNUSED(csap);
    UNUSED(layer);
    UNUSED(ptrn_pdu);
    UNUSED(ptrn_opaque);
    UNUSED(meta_pkt);
    UNUSED(pdu);
    UNUSED(sdu);

    return 0;
}
