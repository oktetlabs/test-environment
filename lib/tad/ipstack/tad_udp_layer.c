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
#define TE_LGR_USER "TAD UDP" 

#include "tad_ipstack_impl.h"

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
char* udp_get_param_cb (csap_p csap_descr, int level, const char *param)
{
    udp_csap_specific_data_t *   spec_data; 
    spec_data = (udp_csap_specific_data_t *) csap_descr->layers[level].specific_data; 

    if (strcmp (param, "ipaddr") == 0)
    { 
        return NULL; /* some other parameters will be soon.. */
    }
    return NULL;
}

/**
 * Callback for confirm PDU with DHCP CSAP parameters and possibilities.
 *
 * @param csap_id       identifier of CSAP
 * @param layer         numeric index of layer in CSAP type to be processed.
 * @param tmpl_pdu      asn_value with PDU (IN/OUT)
 *
 * @return zero on success or error code.
 */ 
int 
udp_confirm_pdu_cb (int csap_id, int layer, asn_value_p tmpl_pdu)
{
    int                       rc;
    csap_p                    csap_descr;
    asn_value *               udp_tmpl_pdu;
    udp_csap_specific_data_t *udp_spec_data;

    if ((csap_descr = csap_find(csap_id)) == NULL)
    {
        ERROR("%s: failed to find csap_descr by csap_id %d",
              __FUNCTION__, csap_id);
        return TE_ETADCSAPNOTEX;
    }
    udp_spec_data = (udp_csap_specific_data_t *)
                    csap_descr->layers[layer].specific_data;

    if (udp_spec_data == NULL)
    {
        ERROR("%s: CSAP-specific data is NULL", __FUNCTION__);
        return TE_EWRONGPTR;
    } 

    if(asn_get_syntax(tmpl_pdu, "") != CHOICE)
        asn_get_choice_value(tmpl_pdu, (const asn_value **)&udp_tmpl_pdu,
                             NULL, NULL);
    else
        udp_tmpl_pdu = tmpl_pdu;

    rc = tad_data_unit_convert(tmpl_pdu, NDN_TAG_UDP_SRC_PORT,
                              &(udp_spec_data->du_src_port)); 
    if (rc != 0)
    {
        ERROR("%s: failed to convert src-port from pattern: %r",
              __FUNCTION__, rc);
        return TE_RC(TE_TAD_CSAP, rc);
    }
    if (udp_spec_data->du_src_port.du_type == TAD_DU_I32)
        udp_spec_data->src_port = udp_spec_data->du_src_port.val_i32;


    rc = tad_data_unit_convert(tmpl_pdu, NDN_TAG_UDP_DST_PORT,
                              &(udp_spec_data->du_dst_port)); 
    if (rc != 0)
    {
        ERROR("%s: failed to convert dst-port from pattern: %r",
              __FUNCTION__, rc);
        return TE_RC(TE_TAD_CSAP, rc);
    }
    if (udp_spec_data->du_dst_port.du_type == TAD_DU_I32)
        udp_spec_data->dst_port = udp_spec_data->du_dst_port.val_i32;


    /* Update pattern with CSAP defaults */
    if (csap_descr->command == TAD_OP_RECV)
    {
        /* receive, local is destination */
        if (udp_spec_data->dst_port == 0 && 
            udp_spec_data->local_port != 0)
        {
            VERB("%s: set dst-port to %u",
                  __FUNCTION__, udp_spec_data->local_port);
            rc = asn_write_value_field(tmpl_pdu, &udp_spec_data->local_port,
                                       sizeof(udp_spec_data->local_port),
                                       "dst-port.#plain");
            if (rc != 0)
            {
                ERROR("%s: failed to update dst-port in pattern: %r",
                      __FUNCTION__, rc);
                return TE_RC(TE_TAD_CSAP, rc);
            }
            udp_spec_data->dst_port = udp_spec_data->local_port;
        }
        if (udp_spec_data->src_port == 0 &&
            udp_spec_data->remote_port != 0)
        {
            VERB("%s: set src-port to %u",
                  __FUNCTION__, udp_spec_data->remote_port);
            rc = asn_write_value_field(tmpl_pdu, &udp_spec_data->remote_port,
                                       sizeof(udp_spec_data->remote_port),
                                       "src-port.#plain");
            if (rc != 0)
            {
                ERROR("%s: failed to update src-port in pattern: %r",
                      __FUNCTION__, rc);
                return TE_RC(TE_TAD_CSAP, rc);
            }
            udp_spec_data->src_port = udp_spec_data->remote_port;
        }
    }
    else
    {
        /* send, local is source */
        if (udp_spec_data->du_src_port.du_type == TAD_DU_UNDEF &&
            udp_spec_data->local_port != 0)
        {
            VERB("%s: set src-port to %u",
                  __FUNCTION__, udp_spec_data->local_port);
            rc = asn_write_value_field(tmpl_pdu, &udp_spec_data->local_port,
                                       sizeof(udp_spec_data->local_port),
                                       "src-port.#plain");
            if (rc != 0)
            {
                ERROR("%s: failed to update src-port in pattern: %r",
                      __FUNCTION__, rc);
                return TE_RC(TE_TAD_CSAP, rc);
            }
            udp_spec_data->src_port = udp_spec_data->local_port;
            udp_spec_data->du_src_port.val_i32 = udp_spec_data->local_port;
            udp_spec_data->du_src_port.du_type = TAD_DU_I32;
        }
        if (udp_spec_data->du_dst_port.du_type == TAD_DU_UNDEF)
        {
            if (udp_spec_data->remote_port != 0)
            {
                VERB("%s: set dst-port to %u",
                      __FUNCTION__, udp_spec_data->remote_port);
                rc = asn_write_value_field(tmpl_pdu,
                                           &udp_spec_data->remote_port,
                                           sizeof(udp_spec_data->remote_port),
                                           "dst-port.#plain");
                if (rc != 0)
                {
                    ERROR("%s: failed to update dst-port in pattern: %r",
                          __FUNCTION__, rc);
                    return TE_RC(TE_TAD_CSAP, rc);
                }
                udp_spec_data->dst_port = udp_spec_data->remote_port;
                udp_spec_data->du_dst_port.val_i32 =
                                          udp_spec_data->remote_port;
                udp_spec_data->du_dst_port.du_type = TAD_DU_I32;
            }
            else
            {
                ERROR("%s: sending csap #%d, "
                      "has no dst-port in template and has no remote port",
                      __FUNCTION__, csap_id);
                return TE_RC(TE_TAD_CSAP, TE_EINVAL);
            }
        }
    }

    return 0;
}

/**
 * Callback for generate binary data to be sent to media.
 *
 * @param csap_descr    CSAP instance
 * @param layer       numeric index of layer in CSAP type to be processed.
 * @param tmpl_pdu    asn_value with PDU. 
 * @param up_payload  pointer to data which is already generated for upper 
 *                    layers and is payload for this protocol level. 
 *                    May be zero.  Presented as list of packets. 
 *                    Almost always this list will contain only one element,
 *                    but need in fragmentation sometimes may occur. 
 *                    Of cause, on up level only one PDU is passed, 
 *                    but upper layer (if any present) may perform 
 *                    fragmentation, and current layer may have possibility 
 *                    to de-fragment payload.
 * @param pkts        Callback have to fill this structure with list of 
 *                    generated packets. Almost always this list will 
 *                    contain only one element, but necessaty of 
 *                    fragmentation sometimes may occur. (OUT)
 *
 * @return zero on success or error code.
 */ 
int 
udp_gen_bin_cb(csap_p csap_descr, int layer, const asn_value *tmpl_pdu,
               const tad_tmpl_arg_t *args, size_t arg_num, 
               const csap_pkts_p  up_payload, csap_pkts_p pkts)
{
    int rc;
    udp_csap_specific_data_t *spec_data;
 
    if (csap_descr == NULL)
    {
        ERROR("%s(): null csap descr passed", __FUNCTION__);
        return TE_RC(TE_TAD_CSAP, TE_ETADCSAPNOTEX);
    }
 
    UNUSED(args); 
    UNUSED(arg_num); 
    UNUSED(tmpl_pdu); 

    spec_data = (udp_csap_specific_data_t *)
                    csap_descr->layers[layer].specific_data;

    if (csap_descr->type == TAD_CSAP_DATA)
    {
        if (up_payload != NULL)
        {
            pkts->data = malloc(up_payload->len);
            if (pkts->data == NULL)
                return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

            pkts->len  = up_payload->len;
            memcpy(pkts->data, up_payload->data, up_payload->len);
        }
        else
        {
            pkts->data = NULL;
            pkts->len = 0;
        }

        pkts->free_data_cb = NULL;
        pkts->next = NULL; 

        return 0;
    }
    else
    {
        int       header_len = 8; /* sizeof(struct udphdr) */
        int       payload_len = (up_payload == NULL) ? 0 : up_payload->len;
        uint8_t  *p;

        pkts->len = header_len + payload_len;
        pkts->data = malloc(pkts->len);
        if (pkts->data == NULL)
            return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

        pkts->next = NULL;
        p = (uint8_t *)pkts->data;

#define PUT_BIN_DATA(c_du_field_, def_val_) \
    do {                                                                \
        if (spec_data->c_du_field_.du_type != TAD_DU_UNDEF)             \
        {                                                               \
            rc = tad_data_unit_to_bin(&(spec_data->c_du_field_),        \
                                      args, arg_num, p, 2);             \
            if (rc != 0)                                                \
            {                                                           \
                ERROR("%s():%d: " #c_du_field_ " error: %r",            \
                      __FUNCTION__,  __LINE__, rc);                     \
                goto cleanup;                                           \
            }                                                           \
        }                                                               \
        else                                                            \
            *((uint16_t *)p) = htons((uint16_t)def_val_);               \
        p += 2;                                                         \
    } while (0) 


#define PUT_UINT16(val_)                                \
    do {                                                \
        *((uint16_t *)p) = htons((uint16_t)val_);       \
        p +=  2;                                        \
    } while (0) 

        PUT_BIN_DATA(du_src_port, spec_data->local_port);
        PUT_BIN_DATA(du_dst_port, spec_data->remote_port);

        PUT_UINT16(pkts->len); 
        /* Checksum */
        PUT_UINT16(0);
#undef PUT_UINT16
#undef PUT_BIN_DATA

        if (payload_len > 0)
            memcpy(p, up_payload->data, payload_len);
    }

    return 0;
cleanup:
    free(pkts->data);
    pkts->data = NULL; pkts->len = 0;

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
 * @param parsed_packet caller of mudpod should pass here empty asn_value 
 *                      instance of ASN type 'Generic-PDU'. Callback 
 *                      have to fill this instance with values from 
 *                      parsed and matched packet
 *
 * @return zero on success or error code.
 */
int udp_match_bin_cb(int csap_id, int layer, const asn_value *pattern_pdu,
                     const csap_pkts *pkt, csap_pkts *payload,
                     asn_value_p parsed_packet)
{
    asn_value  *udp_header_pdu = NULL;
    csap_p      csap_descr;
    uint8_t    *data;
    int         rc = 0;

    UNUSED(layer);

    if (pkt == NULL || payload == NULL)
    {
        ERROR("%s: pkt or payload is NULL", __FUNCTION__);
        return TE_EWRONGPTR;
    }
    data = (uint8_t *)(pkt->data);

    if ((csap_descr = csap_find(csap_id)) == NULL)
    {
        ERROR("%s: csap_descr is NULL for csap id %d", __FUNCTION__, csap_id);
        return TE_ETADCSAPNOTEX;
    }

    /* Match UDP header fields */
    if (parsed_packet != 0)
        udp_header_pdu = asn_init_value(ndn_udp_header);

#define CHECK_FIELD(_asn_label, _size) \
    do {                                                        \
        rc = ndn_match_data_units(pattern_pdu, udp_header_pdu,  \
                                  data, _size, _asn_label);     \
        if (rc != 0)                                            \
        {                                                       \
            VERB("%s: field '%s' not match: %r",                \
                 __FUNCTION__, _asn_label, rc);                 \
            return rc;                                          \
        }                                                       \
        data += _size;                                          \
    } while(0)

    CHECK_FIELD("src-port", 2);
    CHECK_FIELD("dst-port", 2);
    CHECK_FIELD("length", 2);
    CHECK_FIELD("checksum", 2);
#undef CHECK_FIELD

    /* Passing payload to upper layer */
    memset(payload, 0, sizeof(*payload));
    payload->len = (pkt->len - (data - (uint8_t *)(pkt->data)));
    payload->data = malloc(payload->len);
    if (payload->data == NULL)
    {
        ERROR("%s: failed to allocate memory for payload", __FUNCTION__);
        asn_free_value(udp_header_pdu);
        return TE_ENOMEM;
    }
    memcpy(payload->data, data, payload->len);

    /* Insert UDP header data */
    if (parsed_packet != NULL)
    {
        rc = asn_write_component_value(parsed_packet, udp_header_pdu, "#udp");
        if (rc != 0)
            ERROR("%s: failed to insert UDP header to packet: %r",
                  __FUNCTION__, rc);
    }
    asn_free_value(udp_header_pdu);

    return rc;
}


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
int udp_gen_pattern_cb (int csap_id, int layer, const asn_value *tmpl_pdu, 
                                         asn_value_p   *pattern_pdu)
{ 
    UNUSED(pattern_pdu);
    UNUSED(csap_id);
    UNUSED(layer);
    UNUSED(tmpl_pdu);

    return TE_EOPNOTSUPP;
}

