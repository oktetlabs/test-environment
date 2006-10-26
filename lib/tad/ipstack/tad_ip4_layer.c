/** @file
 * @brief TAD IP Stack
 *
 * Traffic Application Domain Command Handler.
 * IPv4 CSAP layer-related callbacks.
 *
 * Copyright (C) 2004-2006 Test Environment authors (see file AUTHORS
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAD IPv4"

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
 * IPv4 layer specific data
 */
typedef struct tad_ip4_proto_data {
    tad_bps_pkt_frag_def    hdr;
} tad_ip4_proto_data;

/**
 * IPv4 layer specific data for PDU processing (both send and
 * receive).
 */
typedef struct tad_ip4_proto_pdu_data {
    tad_bps_pkt_frag_data   hdr;
} tad_ip4_proto_pdu_data;


/**
 * Definition of Internet Protocol version 4 (IPv4) header.
 */
static const tad_bps_pkt_frag tad_ip4_bps_hdr[] =
{
    { "version",          4, BPS_FLD_CONST(4),
      TAD_DU_I32, FALSE },
    { "h-length",         4, BPS_FLD_CONST_DEF(NDN_TAG_IP4_HLEN, 5),
      TAD_DU_I32, TRUE },
    { "type-of-service",  8, BPS_FLD_CONST_DEF(NDN_TAG_IP4_TOS, 0),
      TAD_DU_I32, FALSE },
    { "total-length",    16, BPS_FLD_CONST_DEF(NDN_TAG_IP4_LEN, 0),
      TAD_DU_I32, TRUE },
    { "ip-ident",        16, BPS_FLD_CONST_DEF(NDN_TAG_IP4_IDENT, 0),
      TAD_DU_I32, FALSE },
    { "flag-reserved",    1, BPS_FLD_CONST(0),
      TAD_DU_I32, FALSE },
    { "dont-frag",        1, BPS_FLD_CONST_DEF(NDN_TAG_IP4_DONT_FRAG, 0),
      TAD_DU_I32, FALSE },
    { "more-frags",       1, BPS_FLD_CONST_DEF(NDN_TAG_IP4_MORE_FRAGS, 0),
      TAD_DU_I32, FALSE },
    { "frag-offset",     13, BPS_FLD_CONST_DEF(NDN_TAG_IP4_FRAG_OFFSET, 0),
      TAD_DU_I32, FALSE },
    { "time-to-live",     8, BPS_FLD_CONST_DEF(NDN_TAG_IP4_TTL, 64),
      TAD_DU_I32, FALSE },
    { "protocol",         8, BPS_FLD_SIMPLE(NDN_TAG_IP4_PROTOCOL),
      TAD_DU_I32, FALSE },
    { "h-checksum",      16, BPS_FLD_CONST_DEF(NDN_TAG_IP4_H_CHECKSUM, 0),
      TAD_DU_I32, TRUE },
    { "src-addr",        32, NDN_TAG_IP4_SRC_ADDR,
      NDN_TAG_IP4_LOCAL_ADDR, NDN_TAG_IP4_REMOTE_ADDR, 0,
      TAD_DU_OCTS, FALSE },
    { "dst-addr",        32, NDN_TAG_IP4_DST_ADDR,
      NDN_TAG_IP4_REMOTE_ADDR, NDN_TAG_IP4_LOCAL_ADDR, 0,
      TAD_DU_OCTS, FALSE },
};


/* See description tad_ipstack_impl.h */
te_errno
tad_ip4_init_cb(csap_p csap, unsigned int layer)
{ 
    te_errno            rc;
    tad_ip4_proto_data *proto_data;
    const asn_value    *layer_nds;

    proto_data = TE_ALLOC(sizeof(*proto_data));
    if (proto_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    csap_set_proto_spec_data(csap, layer, proto_data);

    layer_nds = csap->layers[layer].nds;

    rc = tad_bps_pkt_frag_init(tad_ip4_bps_hdr,
                               TE_ARRAY_LEN(tad_ip4_bps_hdr),
                               layer_nds, &proto_data->hdr);
    if (rc != 0)
        return rc;

    /* FIXME */
    if (layer > 0 &&
        proto_data->hdr.tx_def[10].du_type == TAD_DU_UNDEF &&
        proto_data->hdr.rx_def[10].du_type == TAD_DU_UNDEF)
    {
        uint8_t protocol;

        VERB("%s(): ip4er-type is not defined, try to guess", __FUNCTION__);
        switch (csap->layers[layer - 1].proto_tag)
        {
#ifdef IPPROTO_IPIP
            case TE_PROTO_IP4:
                protocol = IPPROTO_IPIP;
                break;
#endif

            case TE_PROTO_UDP:
                protocol = IPPROTO_UDP;
                break;

            case TE_PROTO_TCP:
                protocol = IPPROTO_TCP;
                break;

            case TE_PROTO_ICMP4:
                protocol = IPPROTO_ICMP;
                break;

            default:
                protocol = 0;
                break;
        }
        if (protocol != 0)
        {
            INFO("%s(): Guessed protocol is %u", __FUNCTION__, protocol);
            proto_data->hdr.tx_def[10].du_type = TAD_DU_I32;
            proto_data->hdr.tx_def[10].val_i32 = protocol;
            proto_data->hdr.rx_def[10].du_type = TAD_DU_I32;
            proto_data->hdr.rx_def[10].val_i32 = protocol;
        }
    }

    return 0; 
}

/* See description tad_ipstack_impl.h */
te_errno
tad_ip4_destroy_cb(csap_p csap, unsigned int layer)
{
    tad_ip4_proto_data *proto_data;

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
tad_ip4_nds_to_pdu_data(csap_p csap, tad_ip4_proto_data *proto_data,
                        const asn_value *layer_pdu,
                        tad_ip4_proto_pdu_data **p_pdu_data)
{
    te_errno                    rc;
    tad_ip4_proto_pdu_data     *pdu_data;

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
tad_ip4_release_pdu_cb(csap_p csap, unsigned int layer, void *opaque)
{
    tad_ip4_proto_data     *proto_data;
    tad_ip4_proto_pdu_data *pdu_data = opaque;

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
tad_ip4_confirm_tmpl_cb(csap_p csap, unsigned int layer,
                        asn_value *layer_pdu, void **p_opaque)
{
    te_errno                    rc;
    tad_ip4_proto_data         *proto_data;
    tad_ip4_proto_pdu_data     *tmpl_data;

    proto_data = csap_get_proto_spec_data(csap, layer);

    rc = tad_ip4_nds_to_pdu_data(csap, proto_data, layer_pdu, &tmpl_data);
    *p_opaque = tmpl_data;
    if (rc != 0)
        return rc;

    tmpl_data = *p_opaque;

    rc = tad_bps_confirm_send(&proto_data->hdr, &tmpl_data->hdr);

    return rc;
}


/**
 * Segment payload checksum calculation data.
 */
typedef struct tad_ip4_upper_checksum_seg_cb_data {
    uint32_t    checksum;   /**< Accumulated checksum */
} tad_ip4_upper_checksum_seg_cb_data;

/**
 * Calculate checksum of the segment data.
 *
 * This function complies with tad_pkt_seg_enum_cb prototype.
 */
static te_errno
tad_ip4_upper_checksum_seg_cb(const tad_pkt *pkt, tad_pkt_seg *seg,
                              unsigned int seg_num, void *opaque)
{
    tad_ip4_upper_checksum_seg_cb_data *data = opaque;

    /* Data length is even or it is the last segument */
    assert(((seg->data_len & 1) == 0) ||
           (seg_num == tad_pkt_seg_num(pkt) - 1));
    data->checksum += calculate_checksum(seg->data_ptr, seg->data_len);

    return 0;
}

/** Data to be passed as opaque to tad_ip4_gen_bin_cb_per_sdu() callback. */
typedef struct tad_ip4_gen_bin_cb_per_sdu_data {

    const asn_value    *tmpl_pdu;   /**< ASN.1 template of IPv4 PDU */

    tad_pkts   *pdus;   /**< List to put generated IPv4 PDUs */
    uint8_t    *hdr;    /**< Binary template of the IPv4 header */
    size_t      hlen;   /**< Length of the IPv4 header */

    int         upper_chksm_offset; /**< Offset of the upper layer
                                         checksum in the IPv4 SDU */
    uint32_t    phdr_chksm;         /**< Precalculated checksum of the
                                         pseudo-header with 0 length */

} tad_ip4_gen_bin_cb_per_sdu_data;

/**
 * Callback to generate binary data per sdu.
 *
 * This function complies with tad_pkt_enum_cb prototype.
 */
static te_errno
tad_ip4_gen_bin_cb_per_sdu(tad_pkt *sdu, void *opaque)
{
    tad_ip4_gen_bin_cb_per_sdu_data    *data = opaque;

    te_errno            rc;
    size_t              sdu_len = tad_pkt_len(sdu);
    const asn_value    *frags_seq;
    unsigned int        frags_num;
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
                 "the first segment of IPv4 SDU is too short");
        }
        else
        {
            tad_ip4_upper_checksum_seg_cb_data  seg_data;
            uint16_t                            tmp;

            if (sdu_len > 0xffff)
            {
                ERROR("SDU is too big to put its length in IPv4 "
                      "pseudo-header to calculate checksum");
                return TE_RC(TE_TAD_CSAP, TE_E2BIG);
            }
            tmp = htons(sdu_len);

            /* Pseudo-header checksum */
            seg_data.checksum = data->phdr_chksm +
                calculate_checksum(&tmp, sizeof(tmp));

            /* Preset checksum field by zeros */
            memset((uint8_t *)seg->data_ptr + data->upper_chksm_offset,
                   0, 2);

            /* Upper layer data checksum */
            (void)tad_pkt_enumerate_seg(sdu, tad_ip4_upper_checksum_seg_cb,
                                        &seg_data);

            /* Finalize checksum calculation */
            tmp = ~((seg_data.checksum & 0xffff) +
                    (seg_data.checksum >> 16));

            /* Write calculcated checksum to packet */
            memcpy((uint8_t *)seg->data_ptr + data->upper_chksm_offset,
                   &tmp, sizeof(tmp));
        }
    }

    /* Get fragments sequence specification from ASN.1 template */
    if ((rc = asn_get_child_value(data->tmpl_pdu, &frags_seq, 
                                  PRIVATE, NDN_TAG_IP4_FRAGMENTS)) != 0)
    {
        frags_seq = NULL;
        frags_num = 1;
    }
    else
    {
        frags_num = asn_get_length(frags_seq, "");
    }

    /* 
     * Allocate PDU packets with one pre-allocated segment for IPv4
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
        te_bool     bool_tmp;
        size_t      ip4_pld_real_len;
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

        /* Real length of the IPv4 packet payload */
        if (frags_seq != NULL)
        {
            rc = asn_get_indexed(frags_seq, &frag_spec, frags_i, NULL); 
            if (rc != 0)
            {
                ERROR("%s(): Failed to get %u fragment specification "
                      "from IPv4 PDU template: %r", __FUNCTION__,
                      frags_i, rc);
                return TE_RC(TE_TAD_CSAP, rc);
            }
            assert(frag_spec != NULL);

            ASN_READ_FRAG_SPEC(int32, "real-length", &i32_tmp);
            ip4_pld_real_len = i32_tmp;
        }
        else
        {
            ip4_pld_real_len = sdu_len;
        }

        /* Version, header length and TOS are kept unchanged */

        /* Total Length */
        if (frag_spec == NULL)
        {
            if (data->hlen + ip4_pld_real_len > 0xffff)
            {
                ERROR("SDU is too big to be IPv4 packet SDU");
                return TE_RC(TE_TAD_CSAP, TE_E2BIG);
            }
            i16_tmp = htons(data->hlen + ip4_pld_real_len);
        }
        else
        {
            ASN_READ_FRAG_SPEC(int32, "hdr-length", &i32_tmp);
            assert((uint32_t)i32_tmp <= 0xffff);
            i16_tmp = htons(i32_tmp);
        }
        memcpy(hdr + 2, &i16_tmp, 2);

        /* Identification is kept unchanged */

        /* Flags + Fragment Offset */
        if (frag_spec != NULL)
        {
            ASN_READ_FRAG_SPEC(int32, "hdr-offset", &i32_tmp);
            if ((i32_tmp & 7) != 0)
            {
                ERROR("'hdr-offset' in fragment specification has to "
                      "be multiple of 8");
                return TE_RC(TE_TAD_CSAP, TE_EINVAL);
            }
            else if (i32_tmp > 0xffff)
            {
                ERROR("'hdr-offset' in fragment specification is too "
                      "big");
                return TE_RC(TE_TAD_CSAP, TE_E2BIG);
            }
            i16_tmp = (i32_tmp >> 3);
            ASN_READ_FRAG_SPEC(bool, "more-frags", &bool_tmp);
            i16_tmp |= !!(bool_tmp) << 13;
            ASN_READ_FRAG_SPEC(bool, "dont-frag", &bool_tmp);
            i16_tmp |= !!(bool_tmp) << 14;

            i16_tmp = htons(i16_tmp);
            memcpy(hdr + 6, &i16_tmp, 2);
        } 

        /* TTL and protocol are kept unchanged */

        /* Calculate header checksum */
        assert(hdr[10] == 0 && hdr[11] == 0);
        i16_tmp = ~(calculate_checksum(hdr, data->hlen));
        memcpy(hdr + 10, &i16_tmp, 2);

        /* Addresses are kept unchanged */

        /* Real offset of the fragment */
        if (frag_spec == NULL)
            frag_offset = 0;
        else
            ASN_READ_FRAG_SPEC(int32, "real-offset", &frag_offset);

        /* Prepare fragment payload */
        rc = tad_pkt_get_frag(frag, sdu,
                              frag_offset, ip4_pld_real_len,
                              TAD_PKT_GET_FRAG_RAND);
        if (rc != 0)
        {
            ERROR("%s(): Failed to get fragment %d:%u from payload: %r",
                  __FUNCTION__, (int)frag_offset,
                  (unsigned)ip4_pld_real_len, rc);
            return rc;
        }

#undef ASN_READ_FRAG_SPEC

    } /* for */

    /* Move all fragments to IPv4 PDUs */
    tad_pkts_move(data->pdus, &frags);

    return 0;
}

/* See description in tad_ipstack_impl.h */
te_errno
tad_ip4_gen_bin_cb(csap_p csap, unsigned int layer,
                   const asn_value *tmpl_pdu, void *opaque,
                   const tad_tmpl_arg_t *args, size_t arg_num, 
                   tad_pkts *sdus, tad_pkts *pdus)
{
    tad_ip4_proto_data                 *proto_data;
    tad_ip4_proto_pdu_data             *tmpl_data = opaque;
    tad_ip4_gen_bin_cb_per_sdu_data     cb_data;

    const asn_value    *pld_checksum;

    te_errno        rc;
    size_t          bitlen;
    unsigned int    bitoff;


    assert(csap != NULL);
    F_ENTRY("(%d:%u) tmpl_pdu=%p args=%p arg_num=%u sdus=%p pdus=%p",
            csap->id, layer, (void *)tmpl_pdu, (void *)args,
            (unsigned)arg_num, sdus, pdus);

    proto_data = csap_get_proto_spec_data(csap, layer);

    /* Calculate length of the header */
    bitlen = tad_bps_pkt_frag_data_bitlen(&proto_data->hdr,
                                          &tmpl_data->hdr);
    assert((bitlen & 7) == 0);

    /* Allocate memory for binary template of the header */
    cb_data.hdr = malloc(cb_data.hlen = (bitlen >> 3));
    if (cb_data.hdr == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    /* Generate binary template of the header */
    bitoff = 0;
    rc = tad_bps_pkt_frag_gen_bin(&proto_data->hdr, &tmpl_data->hdr,
                                  args, arg_num, cb_data.hdr,
                                  &bitoff, bitlen);
    if (rc != 0)
    {
        ERROR("%s(): tad_bps_pkt_frag_gen_bin failed for addresses: %r",
              __FUNCTION__, rc);
        free(cb_data.hdr);
        return rc;
    }
    assert(bitoff == bitlen);

    /* 
     * Location of the upper protocol checksum which uses IP
     * pseudo-header.
     */
    rc = asn_get_child_value(tmpl_pdu, &pld_checksum,
                             PRIVATE, NDN_TAG_IP4_PLD_CHECKSUM);
    if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
    {
        /* Not specified exactly, determine using protocol */
        switch (cb_data.hdr[9])
        {
            case IPPROTO_TCP:
                cb_data.upper_chksm_offset = 16;
                break;

            case IPPROTO_UDP:
                cb_data.upper_chksm_offset = 6;
                break;

            case IPPROTO_ICMP:
                cb_data.upper_chksm_offset = 2;
                break;

            default:
                cb_data.upper_chksm_offset = -1; /* Do nothing */
                break;
        }
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
            case NDN_TAG_IP4_PLD_CH_OFFSET:
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

            case NDN_TAG_IP4_PLD_CH_DISABLE:
                cb_data.upper_chksm_offset = -1;
                break;

            default:
                ERROR("%s(): Unexpected choice tag value for "
                      "'pld-checksum'", __FUNCTION__);
                return TE_RC(TE_TAD_CSAP, TE_EASNOTHERCHOICE);
        }
    }

    /* Precalculate checksum of the pseudo-header */
    if (cb_data.upper_chksm_offset != -1)
    {
        uint8_t proto[2] = { 0, cb_data.hdr[9] };

        cb_data.phdr_chksm = calculate_checksum(cb_data.hdr + 12, 8) +
                             calculate_checksum(proto, sizeof(proto));
    }

    /* Per-SDU processing */
    cb_data.tmpl_pdu = tmpl_pdu;
    cb_data.pdus = pdus;
    rc = tad_pkt_enumerate(sdus, tad_ip4_gen_bin_cb_per_sdu, &cb_data);
    if (rc != 0)
    {
        ERROR("Failed to process IPv4 SDUs: %r", rc);
        return rc;
    }

    return 0;
}



/* See description in tad_ipstack_impl.h */
te_errno
tad_ip4_confirm_ptrn_cb(csap_p csap, unsigned int layer,
                        asn_value *layer_pdu, void **p_opaque)
{
    te_errno                rc;
    tad_ip4_proto_data     *proto_data;
    tad_ip4_proto_pdu_data *ptrn_data;

    F_ENTRY("(%d:%u) layer_pdu=%p", csap->id, layer,
            (void *)layer_pdu);

    proto_data = csap_get_proto_spec_data(csap, layer);

    rc = tad_ip4_nds_to_pdu_data(csap, proto_data, layer_pdu, &ptrn_data);
    *p_opaque = ptrn_data;

    return rc;
}


/* See description in tad_ipstack_impl.h */
te_errno
tad_ip4_match_pre_cb(csap_p              csap,
                     unsigned int        layer,
                     tad_recv_pkt_layer *meta_pkt_layer)
{
    tad_ip4_proto_data     *proto_data;
    tad_ip4_proto_pdu_data *pkt_data;
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
tad_ip4_match_post_cb(csap_p              csap,
                      unsigned int        layer,
                      tad_recv_pkt_layer *meta_pkt_layer)
{
    tad_ip4_proto_data     *proto_data;
    tad_ip4_proto_pdu_data *pkt_data = meta_pkt_layer->opaque;
    tad_pkt                *pkt;
    te_errno                rc;
    unsigned int            bitoff = 0;

    if (~csap->state & CSAP_STATE_RESULTS)
        return 0;

    if ((meta_pkt_layer->nds = asn_init_value(ndn_ip4_header)) == NULL)
    {
        ERROR_ASN_INIT_VALUE(ndn_ip4_header);
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
tad_ip4_match_do_cb(csap_p           csap,
                    unsigned int     layer,
                    const asn_value *ptrn_pdu,
                    void            *ptrn_opaque,
                    tad_recv_pkt    *meta_pkt,
                    tad_pkt         *pdu,
                    tad_pkt         *sdu)
{
    tad_ip4_proto_data     *proto_data;
    tad_ip4_proto_pdu_data *ptrn_data = ptrn_opaque;
    tad_ip4_proto_pdu_data *pkt_data = meta_pkt->layers[layer].opaque;
    te_errno                rc;
    unsigned int            bitoff = 0;

    UNUSED(ptrn_pdu);

    if (tad_pkt_len(pdu) < 20)
    {
        F_VERB(CSAP_LOG_FMT "PDU is too small to be IPv4 packet",
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

    /* TODO: IPv4 options processing */

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
