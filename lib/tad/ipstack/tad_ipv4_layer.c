/** @file
 * @brief Test Environment: 
 *
 * Traffic Application Domain Command Handler
 * Ethernet CSAP layer-related callbacks.
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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
 * Author: Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * @(#) $Id$
 */

#include <string.h>
#include <stdlib.h>

#define TE_LGR_USER     "TAD IPv4"

#if 0
#define TE_LOG_LEVEL    0xff
#endif

#include "tad_ipstack_impl.h"

#include "logger_ta.h"


/**
 * Callback for read parameter value of ethernet CSAP.
 *
 * @param csap_descr    CSAP descriptor structure.
 * @param level         Index of level in CSAP stack, which param is wanted.
 * @param param         Protocol-specific name of parameter.
 *
 * @return 
 *     String with textual presentation of parameter value, or NULL 
 *     if error occured. User have to free memory at returned pointer.
 */ 
char* ip4_get_param_cb (csap_p csap_descr, int level, const char *param)
{
    UNUSED(csap_descr);
    UNUSED(level);
    UNUSED(param);
    return NULL;
}



/**
 * Callback for confirm PDU with IPv4 CSAP parameters and possibilities.
 *
 * @param csap_id       identifier of CSAP
 * @param layer         numeric index of layer in CSAP type to be processed.
 * @param tmpl_pdu      asn_value with PDU (IN/OUT)
 *
 * @return zero on success or error code.
 */ 
int 
ip4_confirm_pdu_cb(int csap_id, int layer, asn_value *tmpl_pdu)
{ 
    int rc;
    int len;
    csap_p csap_descr = csap_find(csap_id);

    const asn_value *ip4_csap_pdu;
    const asn_value *ip4_tmpl_pdu;

    ip4_csap_specific_data_t * spec_data = 
        (ip4_csap_specific_data_t *) csap_descr->layers[layer].specific_data; 


    if (asn_get_syntax(tmpl_pdu, "") == CHOICE)
    {
        if ((rc = asn_get_choice_value(tmpl_pdu, &ip4_tmpl_pdu, NULL, NULL))
             != 0)
            return rc;
    }
    else
        ip4_tmpl_pdu = tmpl_pdu; 



    ip4_csap_pdu = csap_descr->layers[layer].csap_layer_pdu; 
    if (asn_get_syntax(ip4_csap_pdu, "") == CHOICE)
    {
        if ((rc = asn_get_choice_value(ip4_csap_pdu, &ip4_csap_pdu,
                                       NULL, NULL)) != 0)
            return rc;
    }

    tad_data_unit_convert(tmpl_pdu, NDN_TAG_IP4_VERSION,
                          &spec_data->du_version);
    if (spec_data->du_version.du_type == TAD_DU_UNDEF)
        tad_data_unit_convert(ip4_csap_pdu, NDN_TAG_IP4_VERSION,
                              &spec_data->du_version);

    tad_data_unit_convert(tmpl_pdu, NDN_TAG_IP4_HLEN,
                          &spec_data->du_header_len);
    tad_data_unit_convert(tmpl_pdu, NDN_TAG_IP4_TOS,
                          &spec_data->du_tos);
    if (spec_data->du_tos.du_type == TAD_DU_UNDEF)
        tad_data_unit_convert(ip4_csap_pdu, NDN_TAG_IP4_TOS,
                              &spec_data->du_tos);

    tad_data_unit_convert(tmpl_pdu, NDN_TAG_IP4_LEN,
                          &spec_data->du_ip_len);
    tad_data_unit_convert(tmpl_pdu, NDN_TAG_IP4_IDENT,
                          &spec_data->du_ip_ident);
    tad_data_unit_convert(tmpl_pdu, NDN_TAG_IP4_FLAGS,
                          &spec_data->du_flags);
    tad_data_unit_convert(tmpl_pdu, NDN_TAG_IP4_OFFSET,
                          &spec_data->du_ip_offset);
    tad_data_unit_convert(tmpl_pdu, NDN_TAG_IP4_TTL,
                          &spec_data->du_ttl);

    tad_data_unit_convert(tmpl_pdu, NDN_TAG_IP4_PROTOCOL,
                          &spec_data->du_protocol);
    if (spec_data->du_protocol.du_type == TAD_DU_UNDEF)
    {
        if (layer > 0) /* There is in CSAP upper protocol */
        {
            const char *up_proto = csap_descr->layers[layer - 1].proto;

            if (strcmp(up_proto, "tcp") == 0) 
                spec_data->protocol = IPPROTO_TCP;
            else if (strcmp(up_proto, "udp") == 0) 
                spec_data->protocol = IPPROTO_UDP;
            else if (strcmp(up_proto, "icmp4") == 0) 
                spec_data->protocol = IPPROTO_ICMP;
            else if (strcmp(up_proto, "ip4") == 0) 
                spec_data->protocol = IPPROTO_IPIP;
            else
                spec_data->protocol = 0;
        }
        else
            spec_data->protocol = 0;
    }

    tad_data_unit_convert(tmpl_pdu, NDN_TAG_IP4_H_CHECKSUM,
                          &spec_data->du_h_checksum);

    /* Source address */

    rc = tad_data_unit_convert(tmpl_pdu, NDN_TAG_IP4_SRC_ADDR, &spec_data->du_src_addr);

    len = sizeof(struct in_addr);
    rc = asn_read_value_field(tmpl_pdu, &spec_data->src_addr, 
                                &len, "src-addr");
    if (rc == EASNINCOMPLVAL)
    {
        spec_data->src_addr.s_addr = INADDR_ANY;
        rc = 0;
    }

    /* Destination address */
    rc = tad_data_unit_convert(tmpl_pdu, NDN_TAG_IP4_DST_ADDR,
                               &spec_data->du_dst_addr);

    if (rc == 0)
        rc = asn_read_value_field(tmpl_pdu, &spec_data->dst_addr, 
                                &len, "dst-addr");
    if (rc == EASNINCOMPLVAL)
    {
        spec_data->dst_addr.s_addr = INADDR_ANY;

        if (csap_descr->state & TAD_OP_SEND)
        { 
            if (spec_data->remote_addr.s_addr == INADDR_ANY)
            {
                WARN("%s(): cannot send without dst IP address, EINVAL",
                     __FUNCTION__);
                rc = EINVAL;
            }
            else
                rc = 0;
        }
        else
            rc = 0;
    } 

    return TE_RC(TE_TAD_CSAP, rc);
}

/**
 * Callback for generate binary data to be sent to media.
 *
 * @param csap_descr    CSAP instance
 * @param layer         numeric index of layer in CSAP type to be processed.
 * @param tmpl_pdu      asn_value with PDU. 
 * @param up_payload    pointer to data which is already generated for upper 
 *                      layers and is payload for this protocol level. 
 *                      May be zero.  Presented as list of packets. 
 *                      Almost always this list will contain only one element, 
 *                      but need in fragmentation sometimes may occur. 
 *                      Of cause, on up level only one PDU is passed, 
 *                      but upper layer (if any present) may perform 
 *                      fragmentation, and current layer may have possibility 
 *                      to de-fragment payload.
 * @param pkt_list      Location for head of list with generated packets.
 *                      Almost always this list will contain only
 *                      one element, but necessaty of fragmentation
 *                      sometimes may occur. (OUT)
 *
 * @return zero on success or error code.
 */ 
int 
ip4_gen_bin_cb(csap_p csap_descr, int layer, const asn_value *tmpl_pdu,
               const tad_tmpl_arg_t *args, size_t arg_num, 
               const csap_pkts_p up_payload, csap_pkts_p pkt_list)
{
    int      rc = 0; 
    uint8_t *p;
    uint8_t *checksum_place = NULL;
    size_t   pkt_len,
             h_len;
    int      fr_index = 0,
             fr_number = 0;
    uint16_t ident = random();

    int      pld_chksm_offset = -1;

    const asn_value    *fragments_seq; 
    const asn_value    *pld_checksum; 
    csap_pkts_p         pkt_curr,
                        pkt_prev;

    ip4_csap_specific_data_t *spec_data;

    uint8_t src_ip_addr[4];
    uint8_t dst_ip_addr[4];
    uint8_t protocol;

#define CHECK(fail_cond_, msg_...) \
    do {                                \
        if (fail_cond_)                 \
        {                               \
            ERROR(msg_);                \
            goto cleanup;               \
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
            CHECK(rc != 0, "%s():%d: " #c_du_field_ " error: 0x%x",     \
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
            RING("%s():%d: var " #var_ " is : 0x%x, but should %d bits",\
                 __FUNCTION__, __LINE__, var_, n_bits_);                \
            var_ &= ~(((uint32_t)-1) << n_bits_);                       \
        }                                                               \
    } while (0);

    if (csap_descr == NULL)
        return TE_RC(TE_TAD_CSAP, ETADCSAPNOTEX);

    if (pkt_list == NULL)
        return TE_RC(TE_TAD_CSAP, ETEWRONGPTR);

    if (up_payload == NULL)
    {
        ERROR("%s(CSAP %d): Cannot generate IP packet without payload",
              __FUNCTION__, csap_descr->id);
        return TE_RC(TE_TAD_CSAP, ETADLESSDATA);
    }

    spec_data = (ip4_csap_specific_data_t *)
                csap_descr->layers[layer].specific_data; 

    if (csap_descr->type == TAD_CSAP_DATA) /* TODO */
        return ETENOSUPP;

    /* TODO: IPv4 options generating */ 

    if ((rc = asn_get_child_value(tmpl_pdu, &fragments_seq, 
                                  PRIVATE, NDN_TAG_IP4_FRAGMENTS)) != 0)
        fragments_seq = NULL;
    else
        fr_number = asn_get_length(fragments_seq, "");

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
               __FUNCTION__, csap_descr->id, protocol, pld_chksm_offset);
    }
    else
    {
        uint16_t tag = asn_get_tag(pld_checksum);

        if (tag == NDN_TAG_IP4_PLD_CH_OFFSET)
            asn_read_int32(pld_checksum, &pld_chksm_offset, "#offset");

        F_VERB("%s(CSAP %d): explicit payload checksum offset %d", 
               __FUNCTION__, csap_descr->id, pld_chksm_offset);
    }
        
    pkt_prev = NULL;
    pkt_curr = pkt_list; 

    GEN_BIN_DATA(du_src_addr, ntohl(spec_data->local_addr.s_addr),
                 4, src_ip_addr); 
    GEN_BIN_DATA(du_dst_addr, ntohl(spec_data->remote_addr.s_addr),
                 4, dst_ip_addr); 

    if (pld_chksm_offset > 0)
    {
        uint32_t full_checksum;
        uint8_t pseudo_header[12];

        memcpy(pseudo_header, src_ip_addr, 4);
        memcpy(pseudo_header + 4, dst_ip_addr, 4);

        pseudo_header[8] = 0;
        pseudo_header[9] = protocol;
        *((uint16_t *)(&pseudo_header[10])) = htons(up_payload->len);

        *((uint16_t *)(up_payload->data + pld_chksm_offset)) = (uint16_t)0;

        full_checksum = calculate_checksum(pseudo_header,
                                           sizeof(pseudo_header));
        full_checksum += calculate_checksum(up_payload->data, 
                                            up_payload->len);

        F_VERB("%s(CSAP %d): calculated checksum %x", 
               __FUNCTION__, csap_descr->id, full_checksum);

        *((uint16_t *)(up_payload->data + pld_chksm_offset)) =
            (uint16_t)(~((full_checksum & 0xffff) + (full_checksum >> 16)));
    }

    do {
        const asn_value *frag_spec = NULL;
        int32_t          hdr_field;
        size_t           ip4_pld_real_len;

        checksum_place = NULL;
        h_len = 20 / 4;


        if (fragments_seq != NULL)
        {
            rc = asn_get_indexed(fragments_seq, &frag_spec, fr_index); 
            CHECK(rc != 0, "%s(): get frag fail %X", __FUNCTION__, rc);
            asn_read_int32(frag_spec, &ip4_pld_real_len, "real-length");

            if (pkt_prev != NULL) 
                pkt_curr = pkt_prev->next = calloc(1, sizeof(*pkt_curr));
        }
        else
        {
            ip4_pld_real_len = up_payload->len;
        }

        pkt_len = ip4_pld_real_len + (h_len * 4);
        p = pkt_curr->data = malloc(pkt_len);
        pkt_curr->next = NULL;
        pkt_curr->len = pkt_len;

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
            PUT_BIN_DATA(du_ip_len, ip4_pld_real_len, 2);
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
                offset = htons((uint16_t)hdr_field);
                asn_read_bool(frag_spec, &flag_value, "more-frags");
                flags = !!(flag_value);
                asn_read_bool(frag_spec, &flag_value, "dont-frag");
                flags = (!!(flag_value) << 1);
            }

            *((uint16_t *)p) = offset; /* already in network byte order */
            *p &= 0x1f; /* clear 3 high bits for flags */
            *p |= (flags << 5); 
            p += 2;
        } 

        PUT_BIN_DATA(du_ttl, 64, 1); 

        *p = protocol;
        p++;

        if (spec_data->du_h_checksum.du_type == TAD_DU_UNDEF)
            checksum_place = p; 
        PUT_BIN_DATA(du_h_checksum, 0, 2); 

        memcpy(p, src_ip_addr, 4);
        p += 4;
        memcpy(p, dst_ip_addr, 4);
        p += 4;

        if (checksum_place != NULL) /* Have to calculate header checksum */
        {
            *((uint16_t *)checksum_place) = 
                ~(calculate_checksum(pkt_curr->data, h_len * 4));
        } 
        

        if (frag_spec == NULL)
        { 
            F_VERB("%s(CSAP %d): simple copy up payload %d bytes",
                   __FUNCTION__, csap_descr->id, up_payload->len);
            memcpy(p, up_payload->data, up_payload->len); 
        }
        else
        {
            int32_t pkt_offset,
                    frag_data_len;

            asn_read_int32(frag_spec, &pkt_offset, "real-offset");
            frag_data_len = up_payload->len - pkt_offset; 

            if (frag_data_len > (int32_t)ip4_pld_real_len)
                frag_data_len = ip4_pld_real_len;

            if (frag_data_len < 0)
                frag_data_len = 0;

            if (frag_data_len > 0)
            {
                memcpy(p, up_payload->data + pkt_offset, frag_data_len);
                p += frag_data_len;
            }

            while (frag_data_len < (int32_t)ip4_pld_real_len)
            {
                *p = rand();
                p++, frag_data_len++;
            }
            F_VERB("%s(CSAP %d): fill fragment, real offset %d, %d bytes",
                   __FUNCTION__, csap_descr->id, pkt_offset,
                   ip4_pld_real_len);
        }
        /* fragment iteration procedures */
        fr_index++;
        if (pkt_prev == NULL) 
            pkt_prev = pkt_curr;
    } while (fragments_seq != NULL && fr_index < fr_number);

#if 0
    if (up_payload->free_data_cb)
        up_payload->free_data_cb(up_payload->data);
    else
        free(up_payload->data);

    up_payload->data = NULL;
    up_payload->len = 0;
#endif

#undef PUT_BIN_DATA
#undef GEN_BIN_DATA
#undef CHECK

    return 0;
cleanup:
    {
        csap_pkts_p pkt_fr;
        for (pkt_fr = pkt_list; pkt_fr != NULL; pkt_fr = pkt_fr->next)
        {
            free(pkt_fr->data);
            pkt_fr->data = NULL; pkt_fr->len = 0;
        }
    }
    return rc;
}

/**
 * Callback for parse received packet and match it with pattern. 
 *
 * @param csap_id       identifier of CSAP
 * @param layer         numeric index of layer in CSAP type to be processed.
 * @param pattern_pdu   pattern NDS 
 * @param pkt           recevied packet
 * @param payload       rest upper layer payload, if any exists. (OUT)
 * @param parsed_packet caller of mipod should pass here empty asn_value 
 *                      instance of ASN type 'Generic-PDU'. Callback 
 *                      have to fill this instance with values from 
 *                      parsed and matched packet
 *
 * @return zero on success or error code.
 */
int
ip4_match_bin_cb(int csap_id, int layer, const asn_value *pattern_pdu,
                 const csap_pkts *  pkt, csap_pkts * payload, 
                 asn_value_p  parsed_packet )
{ 
    csap_p                    csap_descr;
    ip4_csap_specific_data_t *spec_data;

    uint8_t *data;
    uint8_t  tmp8;
    int      rc = 0;

    asn_value *ip4_header_pdu = NULL;

    if (parsed_packet)
        ip4_header_pdu = asn_init_value(ndn_ip4_header);

    if ((csap_descr = csap_find(csap_id)) == NULL)
    {
        ERROR("null csap_descr for csap id %d", csap_id);
        return ETADCSAPNOTEX;
    } 
    spec_data = (ip4_csap_specific_data_t*)csap_descr->layers[layer].specific_data; 

    data = pkt->data; 

#define CHECK_FIELD(_asn_label, _size) \
    do {                                                        \
        rc = ndn_match_data_units(pattern_pdu, ip4_header_pdu,  \
                                  data, _size, _asn_label);     \
        if (rc)                                                 \
        {                                                       \
            F_VERB("%s: csap %d field %s not match, rc %X",     \
                    csap_id, __FUNCTION__, _asn_label, rc);     \
            return rc;                                          \
        }                                                       \
        data += _size;                                          \
    } while(0)


    tmp8 = (*data) >> 4;
    rc = ndn_match_data_units(pattern_pdu, ip4_header_pdu, 
                              &tmp8, 1, "version");
    if (rc) 
    {
        F_VERB("%s: field version not match, rc %X", __FUNCTION__, rc); 
        return rc;
    }

    tmp8 = (*data) & 0x0f;
    rc = ndn_match_data_units(pattern_pdu, ip4_header_pdu, 
                              &tmp8, 1, "header-len");
    if (rc) 
    {
        F_VERB("%s: field verxion not match, rc %X", __FUNCTION__, rc); 
        return rc;
    }
    data++;

    CHECK_FIELD("type-of-service", 1); 
    CHECK_FIELD("ip-len", 2);
    CHECK_FIELD("ip-ident", 2);

    tmp8 = (*data) >> 5;
    rc = ndn_match_data_units(pattern_pdu, ip4_header_pdu, 
                              &tmp8, 1, "flags");
    if (rc) return rc;

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
    {
        int parsed_len = (data - (uint8_t*)(pkt->data));

        memset (payload, 0 , sizeof(*payload));
        payload->len = (pkt->len - parsed_len);
        payload->data = malloc (payload->len);
        memcpy(payload->data, data, payload->len);
    }

    if (parsed_packet)
    {
        rc = asn_write_component_value(parsed_packet, ip4_header_pdu, "#ip4"); 
        if (rc)
            ERROR("%s, write IPv4 header to packet fails %X\n", 
                  __FUNCTION__, rc);
    } 

    asn_free_value(ip4_header_pdu);

    VERB("%s, return %X", __FUNCTION__, rc);
    

    return rc;
}

/* Now we don't support traffic generating for this CSAP */
/**
 * Callback for generating pattern to filter 
 * just one response to the packet which will be sent by this CSAP 
 * according to this template. 
 *
 * @param csap_id       identifier of CSAP
 * @param layer         numeric index of layer in CSAP type to be processed.
 * @param tmpl_pdu      ASN value with template PDU.
 * @param pattern_pdu   OUT: ASN value with pattern PDU, generated according 
 *                      to passed template PDU and CSAP parameters. 
 *
 * @return zero on success or error code.
 */
#if 1
int ip4_gen_pattern_cb (int csap_id, int layer, const asn_value *tmpl_pdu, 
                                         asn_value_p   *pattern_pdu)
{ 
    UNUSED(pattern_pdu);
    UNUSED(csap_id);
    UNUSED(layer);
    UNUSED(tmpl_pdu);

    return ETENOSUPP;
}
#endif


