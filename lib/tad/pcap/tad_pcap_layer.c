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
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 *
 * $Id: $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#define TE_LGR_USER     "TAD Ethernet-PCAP"
#include "logger_ta.h"

#include "tad_eth_impl.h"
#include "te_defs.h"

/**
 * Callback for read parameter value of Ethernet-PCAP CSAP.
 *
 * @param csap_id       identifier of CSAP.
 * @param level         Index of level in CSAP stack, which param is wanted.
 * @param param         Protocol-specific name of parameter.
 *
 * @return 
 *     String with textual presentation of parameter value, or NULL 
 *     if error occured. User have to free memory at returned pointer.
 */ 
char* pcap_get_param_cb (csap_p csap_descr, int level, const char *param)
{
    char   *param_buf;            
    pcap_csap_specific_data_p spec_data;

    if (csap_descr == NULL)
    {
        VERB("error in pcap_get_param %s: wrong csap descr passed\n", param);
        return NULL;
    }

    spec_data =
        (pcap_csap_specific_data_p) csap_descr->layers[level].specific_data; 

    if (strcmp (param, "total_bytes") == 0)
    {
        param_buf  = malloc(20);
        sprintf(param_buf, "%u", spec_data->total_bytes);
        return param_buf;
    }

    VERB("error in pcap_get_param %s: not supported parameter\n", param);

    return NULL;
}


/**
 * Callback for parse received packet and match it with pattern. 
 *
 * @param csap_id       identifier of CSAP
 * @param layer         numeric index of layer in CSAP type to be processed.
 * @param pattern_pdu   pattern NDS 
 * @param pkt           recevied packet
 * @param payload       rest upper layer payload, if any exists. (OUT)
 * @param parsed_packet caller of method should pass here empty asn_value 
 *                      instance of ASN type 'Generic-PDU'. Callback 
 *                      have to fill this instance with values from 
 *                      parsed and matched packet
 *
 * @return zero on success or error code.
 */
int 
pcap_match_bin_cb(int csap_id, int layer, const asn_value *pattern_pdu,
                  const csap_pkts *pkt, csap_pkts *payload, 
                  asn_value_p parsed_packet)
{
    csap_p                      csap_descr;
    pcap_csap_specific_data_p   spec_data;
    int                         rc;
    uint8_t                    *data;
    asn_value                  *pcap_pdu = NULL;
  
    if ((csap_descr = csap_find(csap_id)) == NULL)
    {
        ERROR("null csap_descr for csap id %d", csap_id);
        return ETADCSAPNOTEX;
    }

    spec_data = (pcap_csap_specific_data_p)
        csap_descr->layers[layer].specific_data;
    data = pkt->data; 

#if 1
    {
        char buf[50], *p = buf;
        int i;
        for (i = 0; i < 14; i++)
            p += sprintf (p, "%02x ", data[i]);
            
        VERB("got data: %s", buf);
    } 
#endif

    if (pattern_pdu == NULL)
    {
        VERB("pattern pdu is NULL, packet matches");
    }

    if (parsed_packet)
        pcap_pdu = asn_init_value(ndn_pcap_packet);

    /* Perform matching */
    rc = tad_pcap_match(pattern_pdu, data);
    if (rc != 0)
    {
        asn_free_value(pcap_pdu);
        return rc;
    }

    rc = tad_pcap_packet_write_to_asn(parsed_packet, data);

    /* passing payload to upper layer */
    memset(payload, 0 , sizeof(*payload));
    payload->len = pkt->len;

    payload->data = malloc(payload->len);
    memcpy(payload->data, pkt->data, payload->len);

    F_VERB("Eth-PCAP csap N %d, packet matches, pkt len %d, pld len %d", 
           csap_id, pkt->len, payload->len);
    
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
int
pcap_gen_pattern_cb(int csap_id, int layer, const asn_value *tmpl_pdu, 
                    asn_value_p   *pattern_pdu)
{ 
    UNUSED(csap_id); 
    UNUSED(layer);
    UNUSED(tmpl_pdu);

    if (pattern_pdu)
        *pattern_pdu = NULL;

    return ETENOSUPP;
}


