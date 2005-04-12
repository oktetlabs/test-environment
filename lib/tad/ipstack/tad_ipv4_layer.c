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
ip4_confirm_pdu_cb (int csap_id, int layer, asn_value *tmpl_pdu)
{ 
    int rc;
    csap_p csap_descr = csap_find(csap_id);
    int len;

    ip4_csap_specific_data_t * spec_data = 
        (ip4_csap_specific_data_t *) csap_descr->layers[layer].specific_data; 


    tad_data_unit_convert(tmpl_pdu, "version",         &spec_data->du_version);
    tad_data_unit_convert(tmpl_pdu, "header-len",      &spec_data->du_header_len);
    tad_data_unit_convert(tmpl_pdu, "type-of-service", &spec_data->du_tos);
    tad_data_unit_convert(tmpl_pdu, "ip-len",          &spec_data->du_ip_len);
    tad_data_unit_convert(tmpl_pdu, "ip-ident",        &spec_data->du_ip_ident);
    tad_data_unit_convert(tmpl_pdu, "flags",           &spec_data->du_flags);
    tad_data_unit_convert(tmpl_pdu, "ip-offset",       &spec_data->du_ip_offset);
    tad_data_unit_convert(tmpl_pdu, "time-to-live",    &spec_data->du_ttl);
    tad_data_unit_convert(tmpl_pdu, "protocol",        &spec_data->du_protocol);
    tad_data_unit_convert(tmpl_pdu, "h-checksum",      &spec_data->du_h_checksum);

    /* Source address */

    rc = tad_data_unit_convert(tmpl_pdu, "src-addr", &spec_data->du_src_addr);

    len = sizeof(struct in_addr);
    rc = asn_read_value_field(tmpl_pdu, &spec_data->src_addr, 
                                &len, "src-addr");
    if (rc == EASNINCOMPLVAL)
    {
        spec_data->src_addr.s_addr = INADDR_ANY;
        rc = 0;
    }

    /* Destination address */
    rc = tad_data_unit_convert(tmpl_pdu, "dst-addr", &spec_data->du_dst_addr);

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
                WARN("%s(): cannot send without dst IP address, EINVAL");
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
 * @param csap_id       identifier of CSAP
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
 * @param pkts          Callback have to fill this structure with list of 
 *                      generated packets. Almost always this list will 
 *                      contain only one element, but necessaty of 
 *                      fragmentation sometimes may occur. (OUT)
 *
 * @return zero on success or error code.
 */ 
int 
ip4_gen_bin_cb(int csap_id, int layer, const asn_value *tmpl_pdu,
               const tad_tmpl_arg_t *args, size_t arg_num, 
               const csap_pkts_p  up_payload, csap_pkts_p pkts)
{
    csap_p  csap_descr;
    int     rc;

    const asn_value *ip4_csap_pdu;
    const asn_value *ip4_tmpl_pdu;


    if ((csap_descr = csap_find(csap_id)) == NULL)
        return TE_RC(TE_TAD_CSAP, EINVAL);

    if (csap_descr->type == TAD_CSAP_DATA)
        return 0;

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
    else
        ip4_tmpl_pdu = tmpl_pdu;

    
    UNUSED(up_payload);
    UNUSED(args); 
    UNUSED(arg_num); 
    UNUSED(pkts); 


    return 0;
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
            F_VERB("%s: field %s not match, rc %X",             \
                    __FUNCTION__, _asn_label, rc);              \
            return rc;                                          \
        }                                                       \
        data += _size;                                          \
    } while(0)


    tmp8 = (*data) >> 4;
    rc = ndn_match_data_units(pattern_pdu, ip4_header_pdu, 
                              &tmp8, 1, "version");
    if (rc) 
    {
        F_VERB("%s: field verxion not match, rc %X", __FUNCTION__, rc); 
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


