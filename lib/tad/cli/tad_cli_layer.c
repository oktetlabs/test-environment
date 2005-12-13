/** @file
 * @brief CLI TAD
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
 * $Id$
 */

#include "te_config.h"

#include "tad_cli_impl.h"

#include <stdio.h>


/* See description in tad_cli_impl.h */
char *
tad_cli_get_param_cb(csap_p csap_descr, unsigned int layer,
                     const char *param)
{
    UNUSED(csap_descr);
    UNUSED(layer);
    UNUSED(param);

    return NULL;
}

/* See description in tad_cli_impl.h */
te_errno
tad_cli_confirm_pdu_cb(csap_p csap_descr, unsigned int layer,
                       asn_value *layer_pdu)
{
    cli_csap_specific_data_p spec_data;

    UNUSED(layer_pdu);

    spec_data = (cli_csap_specific_data_p)
        csap_descr->layers[layer].specific_data; 
    
    return 0;
}


/* See description in tad_cli_impl.h */
te_errno
tad_cli_gen_bin_cb(csap_p csap_descr, unsigned int layer,
                   const asn_value *tmpl_pdu,
                   const tad_tmpl_arg_t *args, size_t arg_num,
                   const csap_pkts_p  up_payload, csap_pkts_p pkts)
{
    int rc;
    size_t msg_len;

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
        return TE_ENOMEM;
    rc = asn_read_value_field(tmpl_pdu, msg, &msg_len, "message");

    pkts->data = msg;
    pkts->len  = msg_len;
    pkts->next = NULL;

    pkts->free_data_cb = free;

    return 0;
}


/* See description in tad_cli_impl.h */
te_errno
tad_cli_match_bin_cb(csap_p           csap_descr,
                     unsigned int     layer,
                     const asn_value *pattern_pdu,
                     const csap_pkts *pkt,
                     csap_pkts       *payload, 
                     asn_value       *parsed_packet)
{
    char *msg = (char*) pkt->data;
    int msg_len = pkt->len;
#if 0
    int rc;
#endif

    UNUSED(csap_descr);
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

/* See description in tad_cli_impl.h */
te_errno
tad_cli_gen_pattern_cb(csap_p            csap_descr,
                       unsigned int      layer,
                       const asn_value  *tmpl_pdu, 
                       asn_value       **pattern_pdu)
{
    UNUSED(csap_descr);
    UNUSED(tmpl_pdu);

    *pattern_pdu = asn_init_value(ndn_cli_message);
    VERB("%s(): called, layer %d", __FUNCTION__, layer);
    return 0;
}
