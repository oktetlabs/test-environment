/** @file
 * @brief Test Environment: 
 *
 * Traffic Application Domain Command Handler
 * Dummy FILE protocol implementaion, layer-related callbacks.
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
 * Author: Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * @(#) $Id$
 */
#include "config.h"

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include "tad_file_impl.h"

#define TE_LGR_USER     "TAD File"
#include "logger_ta.h"


/**
 * Callback for read parameter value of "file" CSAP.
 *
 * @param csap_id       identifier of CSAP.
 * @param layer         numeric index of layer in CSAP type to be processed.
 * @param param         Protocol-specific name of parameter.
 *
 * @return 
 *     String with textual presentation of parameter value, or NULL 
 *     if error occured. User have to free memory at returned pointer.
 */ 
char *
file_get_param_cb(int csap_id, int layer, const char *param)
{
    UNUSED(csap_id);
    UNUSED(layer);
    UNUSED(param);

    return NULL;
}

/**
 * Callback for confirm PDU with 'file' CSAP parameters and possibilities.
 *
 * @param csap_id       identifier of CSAP
 * @param layer         numeric index of layer in CSAP type to be processed.
 * @param tmpl_pdu      asn_value with PDU (IN/OUT)
 *
 * @return zero on success or error code.
 */ 
int
file_confirm_pdu_cb(int csap_id, int layer, asn_value_p tmpl_pdu)
{
    UNUSED(csap_id);
    UNUSED(layer);
    UNUSED(tmpl_pdu);

    return 0;
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
 * @param pkts          Callback have to fill this structure with list of 
 *                      generated packets. Almost always this list will 
 *                      contain only one element, but need 
 *                      in fragmentation sometimes may occur. (OUT)
 *
 * @return zero on success or error code.
 */ 
int
file_gen_bin_cb(csap_p csap_descr, int layer, const asn_value *tmpl_pdu,
                const tad_tmpl_arg_t *args, size_t arg_num,
                csap_pkts_p up_payload, csap_pkts_p pkts)
{
    int rc;

    size_t line_len; 
    char  *line;

    UNUSED(csap_descr);
    UNUSED(layer);
    UNUSED(args);
    UNUSED(arg_num);
    UNUSED(up_payload); 

    line_len = asn_get_length(tmpl_pdu, "line");
    if (line_len <= 0)
    {
        return 1;
    }

    if ((line = malloc (line_len)) == NULL)
        return ENOMEM;
    rc = asn_read_value_field(tmpl_pdu, line, &line_len, "line");

    pkts->data = line;
    pkts->len  = line_len;
    pkts->next = NULL;

    pkts->free_data_cb = free;

    while (line_len)
    {
        if (!(*line)) *line = '\n';
        line++, line_len--;
    }

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
 * @param payload_len   before: length of allocated memory; 
 *                      after:  length of upper layer payload. (IN/OUT)
 * @param parsed_packet caller of method should pass here empty asn_value 
 *                      instance of ASN type 'Generic-PDU'. Callback 
 *                      have to fill this instance with values from 
 *                      parsed and matched packet
 *
 * @return zero on success or error code.
 */
int
file_match_bin_cb(int csap_id, int layer, const asn_value *pattern_pdu,
                  const csap_pkts *pkt, csap_pkts *payload, 
                  asn_value_p parsed_packet)
{
    char buf[10000];

    char *line = (char*) pkt->data;
    int line_len = pkt->len;
    int rc;

    UNUSED(csap_id);
    UNUSED(layer);
    UNUSED(pattern_pdu);
    UNUSED(payload);

    printf ("file_match. len: %d, line: %s\n", line_len, line);

    rc = asn_write_value_field(parsed_packet, line, line_len, 
                                "#file.line.#plain");

    if (rc) return rc;

    asn_sprint_value(parsed_packet, buf, sizeof(buf), 0);
    printf ("file_match. parsed packet:\n%s\n--\n", buf); 

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
file_gen_pattern_cb(int csap_id, int layer, const asn_value *tmpl_pdu, 
                    asn_value_p *pattern_pdu)
{
    UNUSED(csap_id);
    UNUSED(layer);
    UNUSED(tmpl_pdu);
    UNUSED(pattern_pdu);

    return ETENOSUPP;
}


