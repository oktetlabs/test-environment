/** @file
 * @brief TAD IP Stack
 *
 * Traffic Application Domain Command Handler.
 * IPv6 CSAP layer-related callbacks.
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
 * @author Yurij Plotnikov <Yurij.Plotnikov@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAD IPv6"

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

#include "te_defs.h"
#include "te_alloc.h"
#include "logger_api.h"
#include "logger_ta_fast.h"

#include "tad_bps.h"
#include "tad_ipstack_impl.h"

/**
 * IPv6 layer specific data
 */

/* RFC 2460: 4.1 
 * Each extension header should occur at most once, except for the
 * Destination Options header which should occur at most twice (once
 * before a Routing header and once before the upper-layer header).
 */
/* TODO: May be this structure should be changed */
typedef struct tad_ip6_proto_data {
    tad_bps_pkt_frag_def    hdr; /* TODO: I think here should be extentions
                                  * headers. See icmp4 to understand how to
                                  * do this
                                  */
} tad_ip6_proto_data;

/**
 * IPv6 layer specific data for PDU processing (both send and
 * receive).
 */
typedef struct tad_ip6_proto_pdu_data {
    tad_bps_pkt_frag_data   hdr; /* TODO: I think here should be extentions
                                  * headers
                                  */
} tad_ip6_proto_pdu_data;

/**
 * Definition of Internet Protocol version 6 (IPv6) header.
 */
static const tad_bps_pkt_frag tad_ip6_bps_hdr[] =
{
    { "version",          4, BPS_FLD_CONST(6),
      TAD_DU_I32, FALSE },
    { "traffic-class",    8, BPS_FLD_CONST_DEF(NDN_TAG_IP6_TCL, 0),
      TAD_DU_I32, TRUE },
    { "flow-label",      20, BPS_FLD_CONST_DEF(NDN_TAG_IP6_FLAB, 0),
      TAD_DU_I32, FALSE },
    { "payload-length",  16, BPS_FLD_CONST_DEF(NDN_TAG_IP6_LEN, 0),
      TAD_DU_I32, TRUE },
    { "next-header",      8, BPS_FLD_SIMPLE(NDN_TAG_IP6_NHDR),
      TAD_DU_I32, FALSE },
    { "hop-limit",        8, BPS_FLD_CONST_DEF(NDN_TAG_IP6_HLIM, 77),
      TAD_DU_I32, FALSE },
    { "src-addr",       128, NDN_TAG_IP6_SRC_ADDR,
      NDN_TAG_IP6_LOCAL_ADDR, NDN_TAG_IP6_REMOTE_ADDR, 0,
      TAD_DU_OCTS, FALSE },
    { "dst-addr",       128, NDN_TAG_IP6_DST_ADDR,
      NDN_TAG_IP6_REMOTE_ADDR, NDN_TAG_IP6_LOCAL_ADDR, 0,
      TAD_DU_OCTS, FALSE },
};

/* See description tad_ipstack_impl.h */
te_errno
tad_ip6_init_cb(csap_p csap, unsigned int layer)
{
    te_errno            rc;
    tad_ip6_proto_data *proto_data;
    const asn_value    *layer_nds;

    proto_data = TE_ALLOC(sizeof(*proto_data));
    if (proto_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    csap_set_proto_spec_data(csap, layer, proto_data);

    layer_nds = csap->layers[layer].nds;

    rc = tad_bps_pkt_frag_init(tad_ip6_bps_hdr,
                               TE_ARRAY_LEN(tad_ip6_bps_hdr),
                               layer_nds, &proto_data->hdr);

    if (rc != 0)
        return rc;

    /* TODO: Perhaps some checks should be added here */

    return 0;
}


/* See description tad_ipstack_impl.h */
te_errno
tad_ip6_destroy_cb(csap_p csap, unsigned int layer)
{
    tad_ip6_proto_data *proto_data;

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
tad_ip6_nds_to_pdu_data(csap_p csap, tad_ip6_proto_data *proto_data,
                        const asn_value *layer_pdu,
                        tad_ip6_proto_pdu_data **p_pdu_data)
{

    te_errno                    rc;
    tad_ip6_proto_pdu_data     *pdu_data;

    UNUSED(csap);

    assert(proto_data != NULL);
    assert(layer_pdu != NULL);
    assert(p_pdu_data != NULL);

    *p_pdu_data = pdu_data = TE_ALLOC(sizeof(*pdu_data));
    if (pdu_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    rc = tad_bps_nds_to_data_units(&proto_data->hdr, layer_pdu,
                                   &pdu_data->hdr);

    /* TODO: Add extentions headers */
    return rc;
}

/* See description in tad_ipstack_impl.h */
te_errno
tad_ip6_confirm_tmpl_cb(csap_p csap, unsigned int layer,
                        asn_value *layer_pdu, void **p_opaque)
{
    fprintf(stderr, "[yuran]tad_ip6_confirm_tmpl_cb()\n");

    te_errno                    rc;
    tad_ip6_proto_data         *proto_data;
    tad_ip6_proto_pdu_data     *tmpl_data;

    proto_data = csap_get_proto_spec_data(csap, layer);

    rc = tad_ip6_nds_to_pdu_data(csap, proto_data, layer_pdu, &tmpl_data);
    *p_opaque = tmpl_data;
    if (rc != 0)
        return rc;

    tmpl_data = *p_opaque;

    rc = tad_bps_confirm_send(&proto_data->hdr, &tmpl_data->hdr);

    /* TODO: Add extentions headers */

    return rc;
}

/* TODO: See this structure, may be there are some extra fields */
/** Data to be passed as opaque to tad_ip6_gen_bin_cb_per_sdu() callback. */
typedef struct tad_ip6_gen_bin_cb_per_sdu_data {

    const asn_value    *tmpl_pdu;   /**< ASN.1 template of IPv6 PDU */

    tad_pkts   *pdus;   /**< List to put generated IPv6 PDUs */
    uint8_t    *hdr;    /**< Binary template of the IPv6 header */
    size_t      hlen;   /**< Length of the IPv6 header */
    int         upper_chksm_offset; /**< Offset of the upper layer
                                         checksum in the IPv6 SDU */
    te_bool     use_phdr;           /**< Should pseudo-header be included
                                         in checksum calculation */
    uint32_t    init_chksm;

} tad_ip6_gen_bin_cb_per_sdu_data;

/**
 * Segment payload checksum calculation data.
 */
typedef struct tad_ip6_upper_checksum_seg_cb_data {
    uint32_t    checksum;   /**< Accumulated checksum */
} tad_ip6_upper_checksum_seg_cb_data;

/**
 * Calculate checksum of the segment data.
 *
 * This function complies with tad_pkt_seg_enum_cb prototype.
 */
static te_errno
tad_ip6_upper_checksum_seg_cb(const tad_pkt *pkt, tad_pkt_seg *seg,
                              unsigned int seg_num, void *opaque)
{
    tad_ip6_upper_checksum_seg_cb_data *data = opaque;

    /* Data length is even or it is the last segument */
    assert(((seg->data_len & 1) == 0) ||
           (seg_num == tad_pkt_seg_num(pkt) - 1));
    data->checksum += calculate_checksum(seg->data_ptr, seg->data_len);

    return 0;
}

/* See description in tad_ipstack_impl.h */
static te_errno
tad_ip6_gen_bin_cb_per_sdu(tad_pkt *sdu, void *opaque)
{
    tad_ip6_gen_bin_cb_per_sdu_data    *data = opaque;

    te_errno            rc;
    size_t              sdu_len = tad_pkt_len(sdu);
    const asn_value    *frags_seq = NULL;
    unsigned int        frags_num = 1;
    unsigned int        frags_i;
    tad_pkts            frags;
    tad_pkt            *frag;

    if (data->upper_chksm_offset != -1)
    {
        /* Generate and insert upper layer checksum */
        tad_pkt_seg    *seg = tad_pkt_first_seg(sdu);

        if (seg == NULL ||
            seg->data_len < (size_t)data->upper_chksm_offset + 2)
        {
            WARN("Skip calculation of upper layer checksum, since "
                 "the first segment of IPv6 SDU is too short");
        }
        else
        {
            tad_ip6_upper_checksum_seg_cb_data  seg_data;
            uint16_t                            tmp;

            if (sdu_len > 0xffff)
            {
                ERROR("SDU is too big to put its length in IPv6 "
                      "pseudo-header to calculate checksum");
                return TE_RC(TE_TAD_CSAP, TE_E2BIG);
            }
            tmp = htons(sdu_len);

            seg_data.checksum = data->init_chksm;
            if (data->use_phdr)
            {
                /* Pseudo-header checksum */
                seg_data.checksum += calculate_checksum(&tmp, sizeof(tmp));
            }

            /* Preset checksum field by zeros */
            memset((uint8_t *)seg->data_ptr + data->upper_chksm_offset,
                   0, 2);

            /* Upper layer data checksum */
            (void)tad_pkt_enumerate_seg(sdu, tad_ip6_upper_checksum_seg_cb,
                                        &seg_data);

            /* Finalize checksum calculation */
            tmp = ~((seg_data.checksum & 0xffff) +
                    (seg_data.checksum >> 16));

            /* Write calculcated checksum to packet */
            memcpy((uint8_t *)seg->data_ptr + data->upper_chksm_offset,
                   &tmp, sizeof(tmp));
        }
    }

    /* 
     * Allocate PDU packets with one pre-allocated segment for IPv6
     * header
     */
    tad_pkts_init(&frags);
    rc = tad_pkts_alloc(&frags, frags_num, 1, data->hlen);
    if (rc != 0)
        return rc;

    for (frags_i = 0, frag = tad_pkts_first_pkt(&frags);
         frags_i < frags_num;
         frags_i++, frag = frag->links.cqe_next)
    {
        uint8_t    *hdr = tad_pkt_first_seg(frag)->data_ptr;
        asn_value  *frag_spec = NULL;
        int32_t     i32_tmp;
        int16_t     i16_tmp;

        size_t      ip6_pld_real_len;
        int32_t     frag_offset;

        /* Copy template of the header */
        memcpy(hdr, data->hdr, data->hlen);

#define ASN_READ_FRAG_SPEC(_type, _fld, _var) \
    do {                                                            \
        rc = asn_read_##_type(frag_spec, _var, _fld);               \
        if (rc != 0)                                                \
        {                                                           \
            ERROR("%s(): asn_read_%s(%s) failed: %r", __FUNCTION__, \
                  #_type, _fld, rc);                                \
            return TE_RC(TE_TAD_CSAP, rc);                          \
        }                                                           \
    } while (0)

        /* Real length of the IPv6 packet payload */
        if (frags_seq != NULL)
        {
            rc = asn_get_indexed(frags_seq, &frag_spec, frags_i, NULL); 
            if (rc != 0)
            {
                ERROR("%s(): Failed to get %u fragment specification "
                      "from IPv6 PDU template: %r", __FUNCTION__,
                      frags_i, rc);
                return TE_RC(TE_TAD_CSAP, rc);
            }
            assert(frag_spec != NULL);

            ASN_READ_FRAG_SPEC(int32, "real-length", &i32_tmp);
            ip6_pld_real_len = i32_tmp;
        }
        else
        {
            ip6_pld_real_len = sdu_len;
        }

        /* Version, header length and TOS are kept unchanged */

        /* Total Length */
        if (frag_spec == NULL)
        {
            if (data->hlen + ip6_pld_real_len > 0xffff)
            {
                ERROR("SDU is too big to be IPv6 packet SDU");
                return TE_RC(TE_TAD_CSAP, TE_E2BIG);
            }
            i16_tmp = htons(ip6_pld_real_len);
        }
        else
        {
            ASN_READ_FRAG_SPEC(int32, "hdr-length", &i32_tmp);
            assert((uint32_t)i32_tmp <= 0xffff);
            i16_tmp = 0;
        }
        memcpy(hdr + 4, &i16_tmp, 2);

        /* Real offset of the fragment */
        if (frag_spec == NULL)
            frag_offset = 0;
        else
            ASN_READ_FRAG_SPEC(int32, "real-offset", &frag_offset);

        /* Prepare fragment payload */
        rc = tad_pkt_get_frag(frag, sdu,
                              frag_offset, ip6_pld_real_len,
                              TAD_PKT_GET_FRAG_RAND);
        if (rc != 0)
        {
            ERROR("%s(): Failed to get fragment %d:%u from payload: %r",
                  __FUNCTION__, (int)frag_offset,
                  (unsigned)ip6_pld_real_len, rc);
            return rc;
        }

#undef ASN_READ_FRAG_SPEC

    } /* for */

    /* Move all fragments to IPv6 PDUs */
    tad_pkts_move(data->pdus, &frags);

    return 0;
}

/* See description in tad_ipstack_impl.h */
te_errno
tad_ip6_gen_bin_cb(csap_p csap, unsigned int layer,
                   const asn_value *tmpl_pdu, void *opaque,
                   const tad_tmpl_arg_t *args, size_t arg_num, 
                   tad_pkts *sdus, tad_pkts *pdus)
{
    tad_ip6_proto_data                 *proto_data;
    tad_ip6_proto_pdu_data             *tmpl_data = opaque;
    tad_ip6_gen_bin_cb_per_sdu_data     cb_data;

    const asn_value    *pld_checksum;

    te_errno        rc;
    size_t          bitlen;
    unsigned int    bitoff;


    assert(csap != NULL);
    F_ENTRY("(%d:%u) tmpl_pdu=%p args=%p arg_num=%u sdus=%p pdus=%p",
            csap->id, layer, (void *)tmpl_pdu, (void *)args,
            (unsigned)arg_num, sdus, pdus);

    proto_data = csap_get_proto_spec_data(csap, layer);

    /* IPv6 header length */
    cb_data.hlen = 40;
    bitlen = 320;

    /* FIXME: Override 'h-length' */
    tmpl_data->hdr.dus[1].du_type = TAD_DU_I32;
    tmpl_data->hdr.dus[1].val_i32 = cb_data.hlen >> 2;

    /* Allocate memory for binary template of the header */
    cb_data.hdr = malloc(cb_data.hlen);
    if (cb_data.hdr == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    /* Generate binary template of the header */
    bitoff = 0;
    rc = tad_bps_pkt_frag_gen_bin(&proto_data->hdr, &tmpl_data->hdr,
                                  args, arg_num, cb_data.hdr,
                                  &bitoff, bitlen);
    if (rc != 0)
    {
        ERROR("%s(): tad_bps_pkt_frag_gen_bin failed for header: %r",
              __FUNCTION__, rc);
        free(cb_data.hdr);
        return rc;
    }

    /* TODO: Here should be added extantions headers */

    assert(bitoff == bitlen);

    cb_data.init_chksm = 0;
    /* Checksum field offset */
    switch (cb_data.hdr[6])
    {
        case IPPROTO_ICMPV6:
            cb_data.upper_chksm_offset = 2;
            cb_data.use_phdr = TRUE;
            break;

        default:
            cb_data.upper_chksm_offset = -1; /* Do nothing */
            break;
    }

    /* 
     * Location of the upper protocol checksum which uses IP
     * pseudo-header.
     */
    rc = asn_get_child_value(tmpl_pdu, &pld_checksum,
                             PRIVATE, NDN_TAG_IP6_PLD_CHECKSUM);
    if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
    {
        /* Nothing special */
    }
    else if (rc != 0)
    {
        ERROR("%s(): asn_get_child_value() failed for 'pld-checksum': "
              "%r", __FUNCTION__, rc);
        return TE_RC(TE_TAD_CSAP, rc);
    }
    else
    {
        asn_tag_value   tv;

        rc = asn_get_choice_value(pld_checksum,
                                  (asn_value **)&pld_checksum,
                                  NULL, &tv);
        if (rc != 0)
        {
            ERROR("%s(): asn_get_choice_value() failed for "
                  "'pld-checksum': %r", __FUNCTION__, rc);
            return rc;
        }
        switch (tv)
        {
            case NDN_TAG_IP6_PLD_CH_DISABLE:
                cb_data.upper_chksm_offset = -1;
                break;

            case NDN_TAG_IP6_PLD_CH_OFFSET:
                rc = asn_read_int32(pld_checksum,
                                    &cb_data.upper_chksm_offset,
                                    NULL);
                if (rc != 0)
                {
                    ERROR("%s(): asn_read_int32() failed for "
                          "'pld-checksum.#offset': %r", __FUNCTION__, rc);
                    return rc;
                }
                break;

            case NDN_TAG_IP6_PLD_CH_DIFF:
            {
                int32_t tmp;

                rc = asn_read_int32(pld_checksum, &tmp, NULL);
                if (rc != 0)
                {
                    ERROR("%s(): asn_read_int32() failed for "
                          "'pld-checksum.#diff': %r", __FUNCTION__, rc);
                    return rc;
                }
                cb_data.init_chksm += tmp;
                break;
            }

            default:
                ERROR("%s(): Unexpected choice tag value for "
                      "'pld-checksum'", __FUNCTION__);
                return TE_RC(TE_TAD_CSAP, TE_EASNOTHERCHOICE);
        }
    }

    /* Precalculate checksum of the pseudo-header */
    if (cb_data.upper_chksm_offset != -1 && cb_data.use_phdr)
    {
        uint8_t proto[2] = { 0, cb_data.hdr[6] };

        cb_data.init_chksm += calculate_checksum(cb_data.hdr + 8, 32) +
                              calculate_checksum(proto, sizeof(proto));
    }

    /* Per-SDU processing */
    cb_data.tmpl_pdu = tmpl_pdu;
    cb_data.pdus = pdus;
    rc = tad_pkt_enumerate(sdus, tad_ip6_gen_bin_cb_per_sdu, &cb_data);
    if (rc != 0)
    {
        ERROR("Failed to process IPv6 SDUs: %r", rc);
        return rc;
    }

    return 0;
}

void
tad_ip6_release_pdu_cb(csap_p csap, unsigned int layer, void *opaque)
{
    fprintf(stderr, "[yuran]tad_ip6_release_pdu_cb()\n");

    tad_ip6_proto_data     *proto_data;
    tad_ip6_proto_pdu_data *pdu_data = opaque;

    proto_data = csap_get_proto_spec_data(csap, layer);
    assert(proto_data != NULL);

    if (pdu_data != NULL)
    {
        tad_bps_free_pkt_frag_data(&proto_data->hdr, &pdu_data->hdr);
        free(pdu_data);
    }
}

te_errno
tad_ip6_confirm_ptrn_cb(csap_p csap, unsigned int layer,
                        asn_value *layer_pdu, void **p_opaque)
{
    /* TODO: write this function */
    UNUSED(csap);
    UNUSED(layer);
    UNUSED(layer_pdu);
    UNUSED(p_opaque);

    return 0;
}

/* See description in tad_ipstack_impl.h */
te_errno
tad_ip6_match_pre_cb(csap_p              csap,
                     unsigned int        layer,
                     tad_recv_pkt_layer *meta_pkt_layer)
{
    /* TODO: write this function */
    UNUSED(csap);
    UNUSED(layer);
    UNUSED(meta_pkt_layer);

    return 0;
}

te_errno
tad_ip6_match_do_cb(csap_p           csap,
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

te_errno
tad_ip6_match_post_cb(csap_p              csap,
                      unsigned int        layer,
                      tad_recv_pkt_layer *meta_pkt_layer)
{
    /* TODO: write this function */
    UNUSED(csap);
    UNUSED(layer);
    UNUSED(meta_pkt_layer);

    return 0;
}

te_errno
tad_ip6_rw_init_cb(csap_p csap)
{
    /* TODO: write this function */
    UNUSED(csap);

    return 0;
}

te_errno
tad_ip6_rw_destroy_cb(csap_p csap)
{
    /* TODO: write this function */
    UNUSED(csap);

    return 0;
}

te_errno
tad_ip6_write_cb(csap_p csap, const tad_pkt *pkt)
{
    /* TODO: write this function */
    UNUSED(csap);
    UNUSED(pkt);

    return 0;
}

te_errno
tad_ip6_read_cb(csap_p csap, unsigned int timeout,
                tad_pkt *pkt, size_t *pkt_len)
{
    /* TODO: write this function */
    UNUSED(csap);
    UNUSED(timeout);
    UNUSED(pkt);
    UNUSED(pkt_len);

    return 0;
}
