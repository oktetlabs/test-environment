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
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAD IPv4"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <stdlib.h>

#include "tad_ipstack_impl.h"

#include "logger_api.h"
#include "logger_ta_fast.h"


/* See description tad_ipstack_impl.h */
te_errno
tad_ip4_init_cb(csap_p csap, unsigned int layer)
{ 
    ip4_csap_specific_data_t *spec_data; 
    size_t val_len;
    int    rc;

    VERB("%s called for csap %d, layer %d",
         __FUNCTION__, csap->id, layer); 

    spec_data = calloc(1, sizeof(ip4_csap_specific_data_t));
    if (spec_data == NULL)
        return TE_ENOMEM;

    csap_set_proto_spec_data(csap, layer, spec_data);

    val_len = sizeof(spec_data->remote_addr);
    rc = asn_read_value_field(csap->layers[layer].csap_layer_pdu,
                              &spec_data->remote_addr, &val_len,
                              "remote-addr.#plain");
    if (rc != 0)
    {
        INFO("%s(): read remote addr fails %X", __FUNCTION__, rc);
        spec_data->remote_addr.s_addr = INADDR_ANY;
    }

    val_len = sizeof(spec_data->local_addr);
    rc = asn_read_value_field(csap->layers[layer].csap_layer_pdu,
                              &spec_data->local_addr, &val_len,
                              "local-addr.#plain");
    if (rc != 0)
    {
        INFO("%s(): read local addr fails %X", __FUNCTION__, rc);
        spec_data->local_addr.s_addr = INADDR_ANY;
    }

    F_VERB("%s(): csap %d, layer %d",
            __FUNCTION__, csap->id, layer); 

    /* FIXME */
    if (layer > 0)
    {
        switch (csap->layers[layer - 1].proto_tag)
        {
            case TE_PROTO_IP4:
                spec_data->protocol = IPPROTO_IPIP;
                break;

            case TE_PROTO_UDP:
                spec_data->protocol = IPPROTO_UDP;
                break;

            case TE_PROTO_TCP:
                spec_data->protocol = IPPROTO_TCP;
                break;

            case TE_PROTO_ICMP4:
                spec_data->protocol = IPPROTO_ICMP;
                break;

            default:
                break;
        }
        VERB("%s(): try to guess default protocol = %u",
             __FUNCTION__, spec_data->protocol);
    }

    return 0;
}

/* See description tad_ipstack_impl.h */
te_errno
tad_ip4_destroy_cb(csap_p csap, unsigned int layer)
{
    ip4_csap_specific_data_t *spec_data = 
        csap_get_proto_spec_data(csap, layer); 
     
    tad_data_unit_clear(&spec_data->du_version);
    tad_data_unit_clear(&spec_data->du_header_len);
    tad_data_unit_clear(&spec_data->du_tos);
    tad_data_unit_clear(&spec_data->du_ip_len);
    tad_data_unit_clear(&spec_data->du_ip_ident);
    tad_data_unit_clear(&spec_data->du_flags);
    tad_data_unit_clear(&spec_data->du_ip_offset);
    tad_data_unit_clear(&spec_data->du_ttl);
    tad_data_unit_clear(&spec_data->du_protocol);
    tad_data_unit_clear(&spec_data->du_h_checksum);
    tad_data_unit_clear(&spec_data->du_src_addr);
    tad_data_unit_clear(&spec_data->du_dst_addr);

    return 0;
}


/* See description in tad_ipstack_impl.h */
te_errno
tad_ip4_confirm_pdu_cb(csap_p csap, unsigned int layer,
                       asn_value *layer_pdu, void **p_opaque)
{ 
    te_errno    rc;
    size_t      len;

    const asn_value *ip4_csap_pdu;
    const asn_value *du_field;
    asn_value       *ip4_pdu;

    ip4_csap_specific_data_t * spec_data = 
        (ip4_csap_specific_data_t *) csap_get_proto_spec_data(csap, layer); 

    UNUSED(p_opaque);

    if (asn_get_syntax(layer_pdu, "") == CHOICE)
    {
        if ((rc = asn_get_choice_value(layer_pdu,
                                       (const asn_value **)&ip4_pdu,
                                       NULL, NULL))
             != 0)
            return rc;
    }
    else
        ip4_pdu = layer_pdu; 



    ip4_csap_pdu = csap->layers[layer].csap_layer_pdu; 
    if (asn_get_syntax(ip4_csap_pdu, "") == CHOICE)
    {
        if ((rc = asn_get_choice_value(ip4_csap_pdu, &ip4_csap_pdu,
                                       NULL, NULL)) != 0)
        {
            ERROR("%s(CSAP %d) get choice value of csap layer_pdu fails %r",
                  __FUNCTION__, csap->id, rc);
            return TE_RC(TE_TAD_CSAP, rc);
        }
    }

    /**
     * Macro only set into gen-bin du fields specifications according
     * layer_pdu in traffic nds and csap specification, if respective field
     * in traffic layer_pdu is undefined. 
     * Clever choose of defaults and checks to layer_pdu fields 
     * should be done manually. 
     *
     * Be careful: ASN tag (passed in this macro) should be same in
     * CSAP specification layer_pdu and traffic PDU for the PDU field. 
     */
#define CONFIRM_FIELD(du_field_name_, tag_, label_) \
    do {                                                                \
        rc = tad_data_unit_convert(ip4_pdu, tag_,                       \
                                   &(spec_data-> du_field_name_ ));     \
        if (rc == 0 &&                                                  \
            spec_data-> du_field_name_ .du_type == TAD_DU_UNDEF &&      \
            asn_get_child_value(ip4_csap_pdu, &du_field,                \
                                PRIVATE, tag_) == 0)                    \
        {                                                               \
            asn_write_component_value(layer_pdu, du_field, label_);           \
            rc = tad_data_unit_convert(ip4_csap_pdu, tag_,              \
                                       &(spec_data-> du_field_name_ )); \
        }                                                               \
        if (rc != 0)                                                    \
        {                                                               \
            ERROR("%s(CSAP %d): du convert fails %r, tag %d, label %s", \
                  __FUNCTION__, csap->id, rc, tag_, label_);      \
            return TE_RC(TE_TAD_CSAP, rc);                              \
        }                                                               \
    } while (0)

    CONFIRM_FIELD(du_version, NDN_TAG_IP4_VERSION, "version");

    tad_data_unit_convert(layer_pdu, NDN_TAG_IP4_HLEN,
                          &spec_data->du_header_len);

    CONFIRM_FIELD(du_tos, NDN_TAG_IP4_TOS, "type-of-service");

    tad_data_unit_convert(layer_pdu, NDN_TAG_IP4_LEN,
                          &spec_data->du_ip_len);

    CONFIRM_FIELD(du_ip_ident, NDN_TAG_IP4_IDENT, "ip-ident");

    CONFIRM_FIELD(du_flags, NDN_TAG_IP4_FLAGS, "flags");
    CONFIRM_FIELD(du_ttl, NDN_TAG_IP4_TTL, "time-to-live");

    tad_data_unit_convert(layer_pdu, NDN_TAG_IP4_OFFSET,
                          &spec_data->du_ip_offset); 

    CONFIRM_FIELD(du_protocol, NDN_TAG_IP4_PROTOCOL, "protocol");

    if (spec_data->du_protocol.du_type == TAD_DU_UNDEF &&
        spec_data->protocol != 0)
    {
        rc = ndn_du_write_plain_int(ip4_pdu, NDN_TAG_IP4_PROTOCOL,
                                    spec_data->protocol);
        if (rc != 0)
        {
            ERROR("%s(): write protocol to ip4 layer_pdu failed %r",
                  __FUNCTION__, rc);
            return TE_RC(TE_TAD_CSAP, rc);
        }
    }

    tad_data_unit_convert(layer_pdu, NDN_TAG_IP4_H_CHECKSUM,
                          &spec_data->du_h_checksum);

    /* Source address */

    rc = tad_data_unit_convert(layer_pdu, NDN_TAG_IP4_SRC_ADDR,
                               &spec_data->du_src_addr);

    len = sizeof(struct in_addr);
    rc = asn_read_value_field(layer_pdu, &spec_data->src_addr, &len, "src-addr");
    if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
    {
        spec_data->src_addr.s_addr = INADDR_ANY;
        if (csap->state & CSAP_STATE_RECV && 
            (rc = asn_get_child_value(ip4_csap_pdu, &du_field, PRIVATE,
                                      NDN_TAG_IP4_REMOTE_ADDR)) == 0)
        { 
            rc = asn_write_component_value(ip4_pdu, du_field, "src-addr");
            if (rc != 0)
            {
                ERROR("%s(): write src-addr to ip4 layer_pdu failed %r",
                      __FUNCTION__, rc);
                return TE_RC(TE_TAD_CSAP, rc);
            }
        }
        rc = 0;
    }

    /* Destination address */
    rc = tad_data_unit_convert(layer_pdu, NDN_TAG_IP4_DST_ADDR,
                               &spec_data->du_dst_addr);

    if (rc == 0)
        rc = asn_read_value_field(layer_pdu, &spec_data->dst_addr, 
                                  &len, "dst-addr");
    if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
    {
        spec_data->dst_addr.s_addr = INADDR_ANY;

        if (csap->state & CSAP_STATE_SEND)
        { 
            if (spec_data->remote_addr.s_addr == INADDR_ANY)
            {
                WARN("%s(): cannot send without dst IP address, TE_EINVAL",
                     __FUNCTION__);
                rc = TE_EINVAL;
            }
            else
                rc = 0;
        }
        else if ((rc = asn_get_child_value(ip4_csap_pdu, &du_field, PRIVATE,
                                           NDN_TAG_IP4_LOCAL_ADDR)) == 0)
        {
            rc = asn_write_component_value(ip4_pdu, du_field, "dst-addr");
            if (rc != 0)
            {
                ERROR("%s(): write dst-addr to ip4 layer_pdu failed %r",
                      __FUNCTION__, rc);
                return TE_RC(TE_TAD_CSAP, rc);
            }
        }
        else
            rc = 0;
    } 

    return TE_RC(TE_TAD_CSAP, rc);
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

/**
 * Packet payload checksum calculation data.
 */
typedef struct tad_ip4_upper_checksum_pkt_cb_data {
    size_t  offset;             /**< Offset of the checksum itself
                                     in payload */
    uint8_t pseudo_header[12];  /**< Partially filled in pseudo header */
} tad_ip4_upper_checksum_pkt_cb_data;

/**
 * Calculate upper protocol (TCP, UDP) checksum.
 *
 * This function complies with tad_pkt_enum_cb prototype.
 */
static te_errno
tad_ip4_upper_checksum_pkt_cb(tad_pkt *pkt, void *opaque)
{
    tad_ip4_upper_checksum_pkt_cb_data *pkt_data = opaque;
    tad_ip4_upper_checksum_seg_cb_data  seg_data;
    uint8_t                            *ptr;

    assert(pkt->n_segs > 0);
    assert(tad_pkt_first_seg(pkt)->data_len >= pkt_data->offset + 2);

    /* FIXME: Not aligned memory access */
    *((uint16_t *)(pkt_data->pseudo_header + 10)) =
        htons(tad_pkt_len(pkt));
    seg_data.checksum = calculate_checksum(pkt_data->pseudo_header,
                                           sizeof(pkt_data->pseudo_header));

    ptr = tad_pkt_first_seg(pkt)->data_ptr;
    /* FIXME: Not aligned memory access */
    *((uint16_t *)(ptr + pkt_data->offset)) = (uint16_t)0;

    (void)tad_pkt_enumerate_seg(pkt, tad_ip4_upper_checksum_seg_cb,
                                &seg_data);

    F_VERB("%s(): calculated checksum %x", 
           __FUNCTION__, seg_data.checksum);

    /* FIXME: Not aligned memory access */
    *((uint16_t *)(ptr + pkt_data->offset)) =
        (uint16_t)(~((seg_data.checksum & 0xffff) +
                     (seg_data.checksum >> 16)));

    return 0;
}

/* See description in tad_ipstack_impl.h */
te_errno
tad_ip4_gen_bin_cb(csap_p csap, unsigned int layer,
                   const asn_value *tmpl_pdu, void *opaque,
                   const tad_tmpl_arg_t *args, size_t arg_num, 
                   tad_pkts *sdus, tad_pkts *pdus)
{
    static uint16_t ident = 1;

    int      rc = 0; 
    uint8_t *p;
    uint8_t *h_csum_place = NULL;
    size_t   pkt_len,
             h_len;
    int      fr_index,
             fr_number;

    int      pld_chksm_offset = -1;

    const asn_value    *fragments_seq; 
    const asn_value    *pld_checksum; 

    ip4_csap_specific_data_t *spec_data = NULL;

    uint8_t src_ip_addr[4];
    uint8_t dst_ip_addr[4];
    uint8_t protocol;

    tad_pkt *pkt;


    UNUSED(opaque);

    ident++;

#define CHECK(fail_cond_, msg_...) \
    do {                                \
        if (fail_cond_)                 \
        {                               \
            ERROR(msg_);                \
            return rc;                  \
        }                               \
    } while (0)


/* field should be integer and should have length 1, 2, or 4, 
 * place should be either pointer to big data array, or 
 *      pointer to variable of exactly same size.
 *      It is filled in network byte order. */
#define GEN_BIN_DATA(c_du_field_, def_val_, length_, place_) \
    do {                                                                \
        uint8_t *place_l = (uint8_t *)(place_);                         \
                                                                        \
        if (spec_data->c_du_field_.du_type != TAD_DU_UNDEF)             \
        {                                                               \
            rc = tad_data_unit_to_bin(&(spec_data->c_du_field_),        \
                                      args, arg_num, place_l, length_); \
            CHECK(rc != 0, "%s():%d: " #c_du_field_ " error: %r",       \
              __FUNCTION__,  __LINE__, rc);                             \
        }                                                               \
        else                                                            \
        {                                                               \
            switch (length_)                                            \
            {                                                           \
                case 1:                                                 \
                    *((uint8_t *)place_l) = def_val_;                   \
                    break;                                              \
                                                                        \
                case 2:                                                 \
                    *((uint16_t *)place_l) = htons((uint16_t)def_val_); \
                    break;                                              \
                                                                        \
                case 4:                                                 \
                    *((uint32_t *)place_l) = htonl((uint32_t)def_val_); \
                    break;                                              \
            }                                                           \
        }                                                               \
    } while (0) 

/* generate data and put in into place 'p', increment 'p' according */
#define PUT_BIN_DATA(c_du_field_, def_val_, length_) \
    do {                                                                \
        GEN_BIN_DATA(c_du_field_, def_val_, length_, p);                \
        p += (length_);                                                 \
    } while (0)

#define CUT_BITS(var_, n_bits_) \
    do {                                                                \
        if ( (((uint32_t)-1) << n_bits_) & var_)                        \
        {                                                               \
            INFO("%s():%d: var " #var_ " is : 0x%x, but should %d bits",\
                 __FUNCTION__, __LINE__, var_, n_bits_);                \
            var_ &= ~(((uint32_t)-1) << n_bits_);                       \
        }                                                               \
    } while (0);

    spec_data = csap_get_proto_spec_data(csap, layer); 

    /* TODO: IPv4 options generating */ 

    GEN_BIN_DATA(du_protocol, spec_data->protocol, 1, &protocol); 

    /* init payload checksum offset */
    if ((rc = asn_get_child_value(tmpl_pdu, &pld_checksum, 
                                  PRIVATE, NDN_TAG_IP4_PLD_CHECKSUM)) != 0)
    {
        pld_checksum = NULL;
        switch (protocol)
        {
            case IPPROTO_TCP:
                pld_chksm_offset = 16;
                break;
            case IPPROTO_UDP:
                pld_chksm_offset = 6;
                break;
            default:
                /* do nothing, keep defautl offset value = -1 */
                break;
        }
        F_VERB("%s(CSAP %d): proto %d, auto pld checksum offset %d", 
               __FUNCTION__, csap->id, protocol, pld_chksm_offset);
    }
    else
    {
        uint16_t tag = asn_get_tag(pld_checksum);

        if (tag == NDN_TAG_IP4_PLD_CH_OFFSET)
            asn_read_int32(pld_checksum, &pld_chksm_offset, "#offset");

        F_VERB("%s(CSAP %d): explicit payload checksum offset %d", 
               __FUNCTION__, csap->id, pld_chksm_offset);
    }

    GEN_BIN_DATA(du_src_addr, ntohl(spec_data->local_addr.s_addr),
                 4, src_ip_addr); 
    GEN_BIN_DATA(du_dst_addr, ntohl(spec_data->remote_addr.s_addr),
                 4, dst_ip_addr); 

    if (pld_chksm_offset > 0)
    {
        tad_ip4_upper_checksum_pkt_cb_data  csum_cb_data;

        csum_cb_data.offset = pld_chksm_offset;
        memcpy(csum_cb_data.pseudo_header, src_ip_addr, 4);
        memcpy(csum_cb_data.pseudo_header + 4, dst_ip_addr, 4);

        csum_cb_data.pseudo_header[8] = 0;
        csum_cb_data.pseudo_header[9] = protocol;
        /* Length to be filled in per packet */

        (void)tad_pkt_enumerate(sdus, tad_ip4_upper_checksum_pkt_cb,
                                &csum_cb_data);
    }

    /* Further processing assumes that there is only once SDU packet */
    assert(sdus->n_pkts == 1);

    if ((rc = asn_get_child_value(tmpl_pdu, &fragments_seq, 
                                  PRIVATE, NDN_TAG_IP4_FRAGMENTS)) != 0)
    {
        fragments_seq = NULL;
        fr_number = 1;
    }
    else
    {
        fr_number = asn_get_length(fragments_seq, "");
    }

    h_len = 20 / 4;
    /* 
     * Allocate PDU packets with one pre-allocated segment for IPv4
     * header
     */
    rc = tad_pkts_alloc(pdus, fr_number, 1, h_len << 2);
    if (rc != 0)
        return rc;

    for (fr_index = 0, pkt = pdus->pkts.cqh_first;
         fr_index < fr_number;
         fr_index++, pkt = pkt->links.cqe_next)
    {
        asn_value *frag_spec = NULL;
        int32_t          hdr_field;
        size_t           ip4_pld_real_len;
        int32_t          frag_offset;
        uint8_t         *hdr;

        h_csum_place = NULL;

        if (fragments_seq != NULL)
        {
            rc = asn_get_indexed(fragments_seq, &frag_spec, fr_index, NULL); 
            CHECK(rc != 0, "%s(): get frag fail %X", __FUNCTION__, rc);
            /* FIXME */
            asn_read_int32(frag_spec, &hdr_field, "real-length");
            ip4_pld_real_len = hdr_field;
        }
        else
        {
            ip4_pld_real_len = tad_pkt_len(sdus->pkts.cqh_first);
        }

        pkt_len = ip4_pld_real_len + (h_len * 4);
        p = hdr = tad_pkt_first_seg(pkt)->data_ptr;

        /* version, header len */
        {
            uint8_t version, hlen_field;

            rc = 0;
            if (spec_data->du_version.du_type != TAD_DU_UNDEF)
                rc = tad_data_unit_to_bin(&spec_data->du_version, 
                                          args, arg_num, &version, 1); 
            else
                version = 4;

            CHECK(rc != 0, "%s(): version error %X", __FUNCTION__, rc);
            CUT_BITS(version, 4);

            rc = 0;
            if (spec_data->du_header_len.du_type != TAD_DU_UNDEF)
                rc = tad_data_unit_to_bin(&spec_data->du_header_len, 
                                          args, arg_num, &hlen_field, 1);
            else 
                hlen_field = h_len;
            CHECK(rc != 0, "%s(): hlen error %X", __FUNCTION__, rc); 

            CUT_BITS(hlen_field, 4);

            *p = version << 4 | hlen_field;
            p++;
        }

        PUT_BIN_DATA(du_tos, 0, 1); 
        if (frag_spec == NULL)
            PUT_BIN_DATA(du_ip_len, pkt_len, 2);
        else
        {
            asn_read_int32(frag_spec, &hdr_field, "hdr-length");
            *((uint16_t *)p) = htons(hdr_field);
            p += 2;
        }

        PUT_BIN_DATA(du_ip_ident, ident, 2);

        {
            uint8_t  flags = 0;
            uint16_t offset = 0;

            if (frag_spec == NULL)
            {
                GEN_BIN_DATA(du_flags, 0, sizeof(flags), &flags);
                GEN_BIN_DATA(du_ip_offset, 0, sizeof(offset), &offset);
                CUT_BITS(flags, 3);
            }
            else
            {
                te_bool flag_value = 0;

                asn_read_int32(frag_spec, &hdr_field, "hdr-offset");
                offset = htons((uint16_t)(hdr_field >> 3));
                asn_read_bool(frag_spec, &flag_value, "more-frags");
                flags = !!(flag_value);
                asn_read_bool(frag_spec, &flag_value, "dont-frag");
                flags |= (!!(flag_value) << 1);
            }

            F_VERB("+ header offset (in network order) %x, flags %d",
                   offset, flags);

            *((uint16_t *)p) = offset; /* already in network byte order */
            *p &= 0x1f; /* clear 3 high bits for flags */
            *p |= (flags << 5); 
            F_VERB("put data: 0x%x, 0x%x", (int)p[0], (int)p[1]);
            p += 2;
        } 

        PUT_BIN_DATA(du_ttl, 64, 1); 

        *p = protocol;
        p++;

        if (spec_data->du_h_checksum.du_type == TAD_DU_UNDEF)
            h_csum_place = p; 
        PUT_BIN_DATA(du_h_checksum, 0, 2); 

        memcpy(p, src_ip_addr, 4);
        p += 4;
        memcpy(p, dst_ip_addr, 4);
        p += 4;

        if (h_csum_place != NULL) /* Have to calculate header checksum */
        {
            *((uint16_t *)h_csum_place) = 
                ~(calculate_checksum(hdr, h_len * 4));
        } 
        
        if (frag_spec == NULL)
        { 
            frag_offset = 0;
        }
        else
        {
            /* FIXME */
            asn_read_int32(frag_spec, &frag_offset, "real-offset");
        }

        rc = tad_pkt_get_frag(pkt, sdus->pkts.cqh_first,
                              frag_offset, ip4_pld_real_len,
                              TAD_PKT_GET_FRAG_RAND);
        if (rc != 0)
        {
            ERROR("%s(): Failed to get fragment %d:%u from payload: %r",
                  __FUNCTION__, (int)frag_offset,
                  (unsigned)ip4_pld_real_len, rc);
            return rc;
        }

    } /* for */

#undef CUT_BITS
#undef PUT_BIN_DATA
#undef GEN_BIN_DATA
#undef CHECK

    return 0;
}


/* See description in tad_ipstack_impl.h */
te_errno
tad_ip4_match_bin_cb(csap_p           csap, 
                     unsigned int     layer,
                     const asn_value *ptrn_pdu,
                     void            *ptrn_opaque,
                     tad_recv_pkt    *meta_pkt,
                     tad_pkt         *pdu,
                     tad_pkt         *sdu)
{ 
    ip4_csap_specific_data_t *spec_data;

    uint8_t    *data; 
    size_t      data_len;
    asn_value  *ip4_header_pdu = NULL;
    te_errno    rc;

    uint8_t  tmp8;
    size_t   h_len = 0;
    size_t   ip_len = 0;

    assert(tad_pkt_seg_num(pdu) == 1);
    assert(tad_pkt_first_seg(pdu) != NULL);
    data = tad_pkt_first_seg(pdu)->data_ptr;
    data_len = tad_pkt_first_seg(pdu)->data_len;

    if ((csap->state & CSAP_STATE_RESULTS) &&
        (ip4_header_pdu = meta_pkt->layers[layer].nds =
             asn_init_value(ndn_ip4_header)) == NULL)
    {
        ERROR_ASN_INIT_VALUE(ndn_ip4_header);
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    }

    spec_data = csap_get_proto_spec_data(csap, layer); 

#define CHECK_FIELD(_asn_label, _size) \
    do {                                                        \
        rc = ndn_match_data_units(ptrn_pdu, ip4_header_pdu,     \
                                  data, _size, _asn_label);     \
        if (rc != 0)                                            \
        {                                                       \
            F_VERB("%s: csap %d field %s not match, rc %r",     \
                   __FUNCTION__, csap->id, _asn_label, rc);     \
            goto cleanup;                                       \
        }                                                       \
        data += _size;                                          \
    } while(0)


    tmp8 = (*data) >> 4;
    rc = ndn_match_data_units(ptrn_pdu, ip4_header_pdu, 
                              &tmp8, 1, "version");
    if (rc) 
    {
        F_VERB("%s: field version not match, rc %r", __FUNCTION__, rc); 
        goto cleanup;
    }

    h_len = tmp8 = (*data) & 0x0f;
    rc = ndn_match_data_units(ptrn_pdu, ip4_header_pdu, 
                              &tmp8, 1, "header-len");
    if (rc) 
    {
        F_VERB("%s: field verxion not match, rc %r", __FUNCTION__, rc); 
        goto cleanup;
    }
    data++;

    CHECK_FIELD("type-of-service", 1); 

    ip_len = ntohs(*(uint16_t *)data);
    CHECK_FIELD("ip-len", 2);
    CHECK_FIELD("ip-ident", 2);

    tmp8 = (*data) >> 5;
    rc = ndn_match_data_units(ptrn_pdu, ip4_header_pdu, 
                              &tmp8, 1, "flags");
    if (rc != 0) 
        goto cleanup;

    *data &= 0x1f; 
    CHECK_FIELD("ip-offset", 2);
    CHECK_FIELD("time-to-live", 1);
    CHECK_FIELD("protocol", 1);
    CHECK_FIELD("h-checksum", 2);
    CHECK_FIELD("src-addr", 4);
    CHECK_FIELD("dst-addr", 4);
 
#undef CHECK_FIELD 

    /* TODO: Process IPv4 options */

    /* passing payload to upper layer */ 
    rc = tad_pkt_get_frag(sdu, pdu, h_len * 4, ip_len - (h_len * 4),
                          TAD_PKT_GET_FRAG_ERROR);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Failed to prepare IPv4 SDU: %r",
              CSAP_LOG_ARGS(csap), rc);
        return rc;
    }

    VERB("%s, return %r", __FUNCTION__, rc);
    
cleanup:
    return TE_RC(TE_TAD_CSAP, rc);
}
