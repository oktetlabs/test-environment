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

/*
 * The following constants could be found in linux/icmpv6.h
 * but in order to minimize dependencies we define there constants here.
 */
#define ICMPV6_TYPE_ECHO_REQUEST             128
#define ICMPV6_TYPE_ECHO_REPLY               129
#define ICMPV6_TYPE_MLD_QUERY                130
#define ICMPV6_TYPE_MLD_REPORT               131
#define ICMPV6_TYPE_MLD_DONE                 132

/**
 * ICMPv6 layer specific data
 */
typedef struct tad_icmp6_proto_data {
    tad_bps_pkt_frag_def    hdr;
    tad_bps_pkt_frag_def    echo_body;
    tad_bps_pkt_frag_def    mld_body;
} tad_icmp6_proto_data;

/**
 * IPv6 layer specific data for PDU processing (both send and
 * receive).
 */
typedef struct tad_icmp6_proto_pdu_data {
    tad_bps_pkt_frag_data   hdr;
    tad_bps_pkt_frag_data   body;
    tad_bps_pkt_frag_def   *body_def;
} tad_icmp6_proto_pdu_data;


/**
 * Definition of Internet Control Message Protocol for Internet
 * Protocol version 6 (ICMPv6) header.
 */
static const tad_bps_pkt_frag tad_icmp6_bps_hdr[] =
{
    { "type",      8, BPS_FLD_NO_DEF(NDN_TAG_ICMP6_TYPE),
      TAD_DU_I32, TRUE },
    { "code",      8, BPS_FLD_CONST_DEF(NDN_TAG_ICMP6_CODE, 0),
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

/**
 * Definition of MLDv1 Message subheader.
 */
static const tad_bps_pkt_frag tad_icmp6_mld_pbs_hdr[] =
{
    { "max-response-delay", 16,
      BPS_FLD_CONST_DEF(NDN_TAG_ICMP6_MLD_MAX_RESPONSE_DELAY, 0),
      TAD_DU_I32, FALSE },
    { "reserved",           16, BPS_FLD_CONST(0), TAD_DU_I32, FALSE },
    { "group-addr",         128, BPS_FLD_CONST_DEF(NDN_TAG_IP6_GROUP_ADDR, 0),
      TAD_DU_OCTS, FALSE },
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
                               NULL, &proto_data->echo_body);
    if (rc != 0)
        return rc;

    rc = tad_bps_pkt_frag_init(tad_icmp6_mld_pbs_hdr,
                               TE_ARRAY_LEN(tad_icmp6_mld_pbs_hdr),
                               NULL, &proto_data->mld_body);
    return rc;
}

/* See description tad_ipstack_impl.h */
te_errno
tad_icmp6_destroy_cb(csap_p csap, unsigned int layer)
{
    tad_icmp6_proto_data *proto_data;

    proto_data = csap_get_proto_spec_data(csap, layer);
    csap_set_proto_spec_data(csap, layer, NULL);

    tad_bps_pkt_frag_free(&proto_data->hdr);
    tad_bps_pkt_frag_free(&proto_data->echo_body);
    tad_bps_pkt_frag_free(&proto_data->mld_body);

    free(proto_data);

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
        if (pdu_data->body_def != NULL)
            tad_bps_free_pkt_frag_data(pdu_data->body_def,
                                       &pdu_data->body);
        free(pdu_data);
    }
}

/**
 * Convert traffic template NDS to BPS internal data and
 * check the result for completeness.
 *
 * @param def   default values for packet fragment
 * @param nds   ASN value for this packet fragment
 * @param data  BPS internal data to fill and check
 *
 * @return Status of the operation
 */
static te_errno
tad_ip6_nds_to_data_and_confirm(tad_bps_pkt_frag_def *def, asn_value *nds,
                                tad_bps_pkt_frag_data *data)
{
    te_errno rc;

    rc = tad_bps_nds_to_data_units(def, nds, data);
    if (rc != 0)
        return rc;
    return tad_bps_confirm_send(def, data);
}

/* See description in tad_ipstack_impl.h */
te_errno
tad_icmp6_confirm_tmpl_cb(csap_p csap, unsigned int layer,
                          asn_value *layer_pdu, void **p_opaque)
{
    te_errno                    rc;
    tad_icmp6_proto_data       *proto_data;
    tad_icmp6_proto_pdu_data   *tmpl_data;
    int                         type;

    proto_data = csap_get_proto_spec_data(csap, layer);

    tmpl_data = TE_ALLOC(sizeof(*tmpl_data));
    if (tmpl_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    *p_opaque = tmpl_data;

    if ((rc = asn_read_int32(layer_pdu, &type, "type")) != 0)
    {
        ERROR("Failed to get ICMPv6 message type value, %r", rc);
        return rc;
    }

    rc = tad_ip6_nds_to_data_and_confirm(&proto_data->hdr,
                                         layer_pdu, &tmpl_data->hdr);
    if (rc != 0)
        return rc;

    switch (type)
    {
        case ICMPV6_TYPE_ECHO_REQUEST:
        case ICMPV6_TYPE_ECHO_REPLY:
            tmpl_data->body_def = &proto_data->echo_body;
            break;

        case ICMPV6_TYPE_MLD_QUERY:
        case ICMPV6_TYPE_MLD_REPORT:
        case ICMPV6_TYPE_MLD_DONE:
            tmpl_data->body_def = &proto_data->mld_body;
            break;

        default:
            ERROR("Unsupported ICMPv6 message type %d specified", type);
            return TE_RC(TE_TAD_CSAP, TE_EOPNOTSUPP);
    }

    rc = tad_ip6_nds_to_data_and_confirm(tmpl_data->body_def,
                                         layer_pdu, &tmpl_data->body);
    return rc;
}


static te_errno
tad_get_ip6_addr(uint8_t *addr, csap_p csap, unsigned int layer,
                 const char *label)
{
    te_errno  rc = TE_RC(TE_TAD_CSAP, TE_ENOENT);
    uint8_t  *ptr;

    if ((layer + 1) < csap->depth &&
        csap->layers[layer + 1].proto_tag == TE_PROTO_IP6 &&
        csap->layers[layer + 1].pdu != NULL &&
        (rc = asn_get_field_data(csap->layers[layer + 1].pdu,
                                 (void *)&ptr, label)) == 0)
    {
        memcpy(addr, ptr, IP6_ADDR_LEN);
    }
    return rc;
}

#define tad_get_ip6_src(addr_, csap_, layer_) \
            tad_get_ip6_addr(addr_, csap_, layer_, "src-addr.#plain")

#define tad_get_ip6_dst(addr_, csap_, layer_) \
            tad_get_ip6_addr(addr_, csap_, layer_, "dst-addr.#plain")

/** Structure to pass per-PDU context in PDU enumeration function */
typedef struct per_pdu_ctx {
    uint8_t  ip6_src[IP6_ADDR_LEN]; /**< IPv6 source address */
    uint8_t  ip6_dst[IP6_ADDR_LEN]; /**< IPv6 destination address */
    size_t   msg_len; /**< Size of ICMPv6 message in bytes */
    uint8_t *msg; /**< ICMPv6 message data */
} per_pdu_ctx;

/**
 * Calculate checksum of the segment data.
 *
 * This function complies with tad_pkt_seg_enum_cb prototype.
 */
static te_errno
csum_seg_cb(const tad_pkt *pkt, tad_pkt_seg *seg,
            unsigned int seg_num, void *opaque)
{
    uint32_t *csum = (uint32_t *)opaque;

    /* Data length is even or it is the last segument */
    assert(((seg->data_len & 1) == 0) ||
           (seg_num == tad_pkt_seg_num(pkt) - 1));
    *csum += calculate_checksum(seg->data_ptr, seg->data_len);

    return 0;
}

/**
 * Callback to generate binary data per PDU.
 *
 * This function complies with tad_pkt_enum_cb prototype.
 */
static te_errno
tad_icmp6_gen_bin_cb_per_pdu(tad_pkt *pdu, void *opaque)
{
    tad_pkt_seg *seg = tad_pkt_first_seg(pdu);
    per_pdu_ctx *ctx = (per_pdu_ctx *)opaque;
    uint8_t      pseudo_hdr[IP6_PSEUDO_HDR_LEN];
    uint32_t     csum;
    uint16_t     csum_val;

    /* Copy header template to packet */
    assert(seg->data_ptr != NULL);
    assert(seg->data_len == ctx->msg_len);
    memcpy(seg->data_ptr, ctx->msg, seg->data_len);

    /*
     * Calculate checksum
     * TODO: If checksum existed in PDU template this should not be done
     */
    tad_ip6_fill_pseudo_hdr(pseudo_hdr, ctx->ip6_src, ctx->ip6_dst,
                            tad_pkt_len(pdu), IPPROTO_ICMPV6);

    csum = calculate_checksum(pseudo_hdr, sizeof(pseudo_hdr));
    /* ICMPv6 message checksum */
    tad_pkt_enumerate_seg(pdu, csum_seg_cb, &csum);
    /* Finalize checksum calculation */
    csum_val = ~((csum & 0xffff) + (csum >> 16));

    memcpy(seg->data_ptr + 2, &csum_val, sizeof(csum_val));

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
    te_errno                  rc;
    unsigned int              bitoff;
    size_t                    bitlen;
    per_pdu_ctx               ctx;
    uint8_t                  *msg;

    assert(csap != NULL);
    F_ENTRY("(%d:%u) tmpl_pdu=%p args=%p arg_num=%u sdus=%p pdus=%p",
            csap->id, layer, (void *)tmpl_pdu, (void *)args,
            (unsigned)arg_num, sdus, pdus);

    proto_data = csap_get_proto_spec_data(csap, layer);

    bitlen = tad_bps_pkt_frag_data_bitlen(&proto_data->hdr,
                                          &tmpl_data->hdr);
    bitlen += tad_bps_pkt_frag_data_bitlen(tmpl_data->body_def,
                                           &tmpl_data->body);
    /* Allocate memory for binary template of the header */
    if ((msg = TE_ALLOC(bitlen)) == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    /* Generate binary template of the header */
    bitoff = 0;
    rc = tad_bps_pkt_frag_gen_bin(&proto_data->hdr, &tmpl_data->hdr,
                                  args, arg_num, msg,
                                  &bitoff, bitlen);
    if (rc != 0)
    {
        ERROR("%s(): tad_bps_pkt_frag_gen_bin failed for header: %r",
              __FUNCTION__, rc);
        goto cleanup;
    }
    rc = tad_bps_pkt_frag_gen_bin(tmpl_data->body_def, &tmpl_data->body,
                                  args, arg_num, msg,
                                  &bitoff, bitlen);
    if (rc != 0)
    {
        ERROR("%s(): tad_bps_pkt_frag_gen_bin failed for body: %r",
              __FUNCTION__, rc);
        goto cleanup;
    }
    assert((bitoff & 7) == 0);

    /* ICMPv6 layer does no fragmentation, just copy all SDUs to PDUs */
    tad_pkts_move(pdus, sdus);

    /* Allocate a segment for ICMPv6 header and body for all pkts */
    rc = tad_pkts_add_new_seg(pdus, TRUE, NULL, bitlen >> 3, NULL);
    if (rc != 0)
        return rc;

    rc = tad_get_ip6_src(ctx.ip6_src, csap, layer);
    if (rc != 0)
        return rc;
    rc = tad_get_ip6_dst(ctx.ip6_dst, csap, layer);
    if (rc != 0)
        return rc;

    ctx.msg_len = bitlen >> 3;
    ctx.msg = msg;

    /* Per-PDU processing - set correct length */
    rc = tad_pkt_enumerate(pdus, tad_icmp6_gen_bin_cb_per_pdu, &ctx);

cleanup:
    free(msg);

    return rc;
}


/* See description in tad_ipstack_impl.h */
te_errno
tad_icmp6_confirm_ptrn_cb(csap_p csap, unsigned int layer,
                          asn_value *layer_pdu, void **p_opaque)
{
#if 1
    UNUSED(csap);
    UNUSED(layer);
    UNUSED(layer_pdu);
    UNUSED(p_opaque);

    return 0;
#else
    te_errno                    rc;
    tad_icmp6_proto_data       *proto_data;
    tad_icmp6_proto_pdu_data   *ptrn_data;

    F_ENTRY("(%d:%u) layer_pdu=%p", csap->id, layer,
            (void *)layer_pdu);

    proto_data = csap_get_proto_spec_data(csap, layer);

    rc = tad_icmp6_nds_to_pdu_data(csap, proto_data, layer_pdu, &ptrn_data);
    *p_opaque = ptrn_data;

    return rc;
#endif
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
