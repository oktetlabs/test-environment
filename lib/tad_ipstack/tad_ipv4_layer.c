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
char* ip4_get_param_cb (csap_p csap_descr, int level, const char *param)
{
    UNUSED(csap_descr);
    UNUSED(level);
    UNUSED(param);
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
ip4_confirm_pdu_cb (int csap_id, int layer, asn_value_p tmpl_pdu)
{ 
    int rc;
    csap_p csap_descr = csap_find(csap_id);
    int len;

    ip4_csap_specific_data_t * ip4_spec_data = 
        (ip4_csap_specific_data_t *) csap_descr->layer_data[layer]; 
    len = sizeof(struct in_addr);
    rc = asn_read_value_field(tmpl_pdu, &ip4_spec_data->src_addr, 
                                &len, "src-addr");
    if (rc == EASNINCOMPLVAL)
    {
        ip4_spec_data->src_addr.s_addr = INADDR_ANY;
        rc = 0;
    }


    if (rc == 0)
        rc = asn_read_value_field(tmpl_pdu, &ip4_spec_data->dst_addr, 
                                &len, "dst-addr");
    if (rc == EASNINCOMPLVAL)
    {
        ip4_spec_data->dst_addr.s_addr = INADDR_ANY;

        /* cannot sent packet without IP address! */
        if (ip4_spec_data->remote_addr.s_addr == INADDR_ANY)
            rc = EINVAL;
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
ip4_gen_bin_cb (int csap_id, int layer, const asn_value_p tmpl_pdu,
                 const csap_pkts_p  up_payload, csap_pkts_p pkts)
{
    csap_p csap_descr;

    if ((csap_descr = csap_find(csap_id)) == NULL)
        return TE_RC(TE_TAD_CSAP, EINVAL);

    UNUSED (up_payload); /* DHCP has no payload */ 
    UNUSED (layer); 
    UNUSED (tmpl_pdu); 
    UNUSED (pkts); 

    if (csap_descr->type == TAD_DATA_CSAP)
        return 0;

    return TE_RC(TE_TAD_CSAP, ETENOSUPP);
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
int ip4_match_bin_cb (int csap_id, int layer, const asn_value_p pattern_pdu,
                       const csap_pkts *  pkt, csap_pkts * payload, 
                       asn_value_p  parsed_packet )
{ 
    UNUSED(csap_id);
    UNUSED(layer);
    UNUSED(pattern_pdu);
    UNUSED(pkt);
    UNUSED(payload);
    UNUSED(parsed_packet);

    return 0;
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
int ip4_gen_pattern_cb (int csap_id, int layer, const asn_value_p tmpl_pdu, 
                                         asn_value_p   *pattern_pdu)
{ 
    UNUSED(pattern_pdu);
    UNUSED(csap_id);
    UNUSED(layer);
    UNUSED(tmpl_pdu);

    return ETENOSUPP;
}


