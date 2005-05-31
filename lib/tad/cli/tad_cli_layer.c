/** @file
 * @brief Test Environment: 
 *
 * Traffic Application Domain Command Handler
 * CLI CSAP layer-related callbacks.
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
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 *
 * @(#) $Id$
 */

#include "te_config.h"

#include "tad_cli_impl.h"

#include <stdio.h>

/**
 * Callback for read parameter value of CLI CSAP.
 *
 * @param csap_id       identifier of CSAP.
 * @param level         Index of level in CSAP stack, which param is wanted.
 * @param param         Protocol-specific name of parameter.
 *
 * @return 
 *     String with textual presentation of parameter value, or NULL 
 *     if error occured. User have to free memory at returned pointer.
 */ 
char *
cli_get_param_cb(int csap_id, int level, const char *param)
{
    UNUSED(csap_id);
    UNUSED(level);
    UNUSED(param);

    return NULL;
}

/**
 * Callback for confirm PDU with CLI CSAP parameters and possibilities.
 *
 * @param csap_id       identifier of CSAP
 * @param layer         numeric index of layer in CSAP type to be processed.
 * @param tmpl_pdu      asn_value with PDU (IN/OUT)
 *
 * @return zero on success or error code.
 */ 
int
cli_confirm_pdu_cb(int csap_id, int layer, asn_value * tmpl_pdu)
{
    csap_p                   csap_descr;
    cli_csap_specific_data_p spec_data;

    UNUSED(tmpl_pdu);

    if ((csap_descr = csap_find(csap_id)) == NULL)
    {
        return ETADCSAPNOTEX;
    }
    
    spec_data = (cli_csap_specific_data_p)
        csap_descr->layers[layer].specific_data; 
    
    return 0;
}

/**
 * Callback for generate binary data to be sent to media.
 *
 * @param csap_descr    CSAP instance
 * @param layer         numeric index of layer in CSAP type to be processed.
 * @param tmpl_pdu      asn_value with PDU. 
 * @param args          Template iteration parameters array, may be used to 
 *                      prepare binary data.
 * @param arg_num       Length of array above. 
 * @param up_payload    pointer to data which is already generated for upper 
 *                      layers and is payload for this protocol level. 
 *                      May be zero.  Presented as list of packets. 
 *                      Almost always this list will contain only one element, 
 *                      but need in fragmentation sometimes may occur. 
 *                      Of cause, on up level only one PDU is passed, 
 *                      but upper layer (if any present) may perform 
 *                      fragmentation, and current layer may have possibility 
 *                      to de-fragment payload.
 *                      Callback is responsible for freeing of memory, used in 
 *                      up_payload list. 
 * @param pkts          Callback have to fill this structure with list of 
 *                      generated packets. Almost always this list will 
 *                      contain only one element, but need 
 *                      in fragmentation sometimes may occur. (OUT)
 *
 * @return zero on success or error code.
 */
int
cli_gen_bin_cb(csap_p csap_descr, int layer, const asn_value * tmpl_pdu,
               const tad_tmpl_arg_t *args, size_t arg_num,
               const csap_pkts_p  up_payload, csap_pkts_p pkts)
{
    int rc;
    int msg_len;

    char *msg;

    /* XXX */
    UNUSED(csap_descr);
    UNUSED(layer);
    UNUSED(args);
    UNUSED(arg_num);
    UNUSED(up_payload);

    msg_len = asn_get_length(tmpl_pdu, "message");
    if (msg_len <= 0)
    {
        return 1;
    }

    if ((msg = malloc(msg_len)) == NULL)
        return ENOMEM;
    rc = asn_read_value_field(tmpl_pdu, msg, &msg_len, "message");

    pkts->data = msg;
    pkts->len  = msg_len;
    pkts->next = NULL;

    pkts->free_data_cb = free;

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
 * @param parsed_packet caller of method should pass here empty asn_value 
 *                      instance of ASN type 'Generic-PDU'. Callback 
 *                      have to fill this instance with values from 
 *                      parsed and matched packet
 *
 * @return zero on success or error code.
 */
int
cli_match_bin_cb(int                csap_id,
                 int                layer,
                 const asn_value   *pattern_pdu,
                 const csap_pkts   *pkt,
                 csap_pkts         *payload, 
                 asn_value         *parsed_packet)
{
    char *msg = (char*) pkt->data;
    int msg_len = pkt->len;
    int rc;

    UNUSED(csap_id);
    UNUSED(layer);
    UNUSED(pattern_pdu);
    UNUSED(payload);
#if 1
    UNUSED(parsed_packet);
#endif

    VERB("cli_match. len: %d, message: %s\n", msg_len, msg);

#if 0
    rc = asn_write_value_field(parsed_packet, msg, msg_len, 
                                "#cli.message.#plain");

    if (rc)
    {
        VERB("cli_match. asn_write_value_field() failed");
        free(buf);
        return rc;
    }

#else
    memset(payload, 0 , sizeof(*payload));
    payload->len = pkt->len;
    payload->data = malloc(payload->len);
    memcpy(payload->data, pkt->data, payload->len);
#endif

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
cli_gen_pattern_cb(int csap_id, int layer, const asn_value * tmpl_pdu, 
                   asn_value **pattern_pdu)
{
    UNUSED(csap_id);
    UNUSED(tmpl_pdu);

    *pattern_pdu = asn_init_value(ndn_cli_message);
    VERB("%s(): called, layer %d", __FUNCTION__, layer);
    return 0;
}
