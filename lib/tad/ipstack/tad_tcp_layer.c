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

#define TE_LGR_USER "TAD tcp" 
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
char* tcp_get_param_cb (csap_p csap_descr, int level, const char *param)
{
    tcp_csap_specific_data_t *   spec_data; 
    spec_data = (tcp_csap_specific_data_t *) csap_descr->layer_data[level]; 

    static char buf[20];

    if (strcmp (param, "local_port") == 0)
    { 
        sprintf(buf, "%d", (int)spec_data->local_port);
        return buf;
    } 
    if (strcmp (param, "remote_port") == 0)
    { 
        sprintf(buf, "%d", (int)spec_data->remote_port);
        return buf;
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
tcp_confirm_pdu_cb (int csap_id, int layer, asn_value_p tmpl_pdu)
{ 
    int rc = 0;
    csap_p csap_descr = csap_find(csap_id);

    tcp_csap_specific_data_t * spec_data = 
        (tcp_csap_specific_data_t *) csap_descr->layer_data[layer]; 

    tad_data_unit_convert(tmpl_pdu, "src-port", &spec_data->du_src_port);
    tad_data_unit_convert(tmpl_pdu, "dst-port", &spec_data->du_dst_port);
    tad_data_unit_convert(tmpl_pdu, "seqn",     &spec_data->du_seqn);
    tad_data_unit_convert(tmpl_pdu, "acqn",     &spec_data->du_acqn);
    tad_data_unit_convert(tmpl_pdu, "hlen",     &spec_data->du_hlen);
    tad_data_unit_convert(tmpl_pdu, "flags",    &spec_data->du_flags);
    tad_data_unit_convert(tmpl_pdu, "win-size", &spec_data->du_win_size);
    tad_data_unit_convert(tmpl_pdu, "checksum", &spec_data->du_checksum);
    tad_data_unit_convert(tmpl_pdu, "urg-p",    &spec_data->du_urg_p);
 
    return rc;
}


/**
 * Calculate amount of data necessary for all options in DHCP message.
 *
 * @param options       asn_value with sequence of DHCPv4-Option
 *
 * @return number of octets or -1 if error occured.
 */
int
tcp_calculate_options_data (asn_value_p options)
{
    asn_value_p sub_opts;
    int n_opts = asn_get_length(options, "");
    int i;
    int data_len = 0;
    char label_buf [10];

    for (i = 0; i < n_opts; i++)
    { 
        data_len += 2;  /* octets for type and len */
        snprintf (label_buf, sizeof(label_buf), "%d.options", i);
        if (asn_read_component_value(options, &sub_opts, label_buf) == 0)
        {
            data_len += tcp_calculate_options_data(sub_opts);
            asn_free_value(sub_opts);
        }
        else
        {
            snprintf (label_buf, sizeof(label_buf), "%d.value", i);
            data_len += asn_get_length(options, label_buf);
        }
    } 
    return data_len;
}

static int
fill_tcp_options(void *buf, asn_value_p options)
{
    asn_value_p opt;
    int         i;
    int         len;
    int         n_opts;
    int         rc = 0;
    uint8_t     opt_type;

    if (options == NULL)
        return 0;

    n_opts = asn_get_length(options, "");
    for (i = 0; i < n_opts && rc == 0; i++)
    { 
        opt = asn_read_indexed(options, i, "");
        len = 1;
        if ((rc = asn_read_value_field(opt, &opt_type, &len, "type.#plain")) != 0)
            return rc;
        memcpy(buf, &opt_type, len);
        buf += len;
        /* Options 255 and 0 don't have length and value parts */
        if (opt_type == 255 || opt_type == 0)
            continue;

        len = 1;
        if ((rc = asn_read_value_field(opt, buf, &len, "length.#plain")) != 0)
            return rc;
        buf += len;

        if (asn_get_length(opt, "options") > 0)
        {
            asn_value_p sub_opts;

            if ((rc = asn_read_component_value(opt, &sub_opts, "options")) != 0)
                return rc;
            rc = fill_tcp_options(buf, sub_opts);
        }
        else
        {
            len = asn_get_length(opt, "value.#plain");
            if ((rc = asn_read_value_field(opt, buf, &len, "value.#plain")) != 0)
                return rc;
            buf += len;
        }
    }
    return rc;
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
tcp_gen_bin_cb(int csap_id, int layer, const asn_value *tmpl_pdu,
               const tad_template_arg_t *args, size_t arg_num, 
               csap_pkts_p up_payload, csap_pkts_p pkts)
{
    int rc = 0;
    unsigned char *p; 

    UNUSED(up_payload); /* DHCP has no payload */ 
    UNUSED(csap_id); 
    UNUSED(layer); 
    UNUSED(args); 
    UNUSED(arg_num); 
    UNUSED(tmpl_pdu); 

    pkts->len = 236; /* total length of mandatory fields in DHCP message */ 

    pkts->data = malloc(pkts->len);
    pkts->next = NULL;
    
    p = pkts->data;
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
 * @param parsed_packet caller of mtcpod should pass here empty asn_value 
 *                      instance of ASN type 'Generic-PDU'. Callback 
 *                      have to fill this instance with values from 
 *                      parsed and matched packet
 *
 * @return zero on success or error code.
 */
int tcp_match_bin_cb (int csap_id, int layer, const asn_value *pattern_pdu,
                       const csap_pkts *  pkt, csap_pkts * payload, 
                       asn_value_p  parsed_packet )
{ 
    csap_p                    csap_descr;
    tcp_csap_specific_data_t *spec_data;

    uint8_t *data;
    uint8_t  tmp8;
    int      rc;

    UNUSED(pattern_pdu);
    UNUSED(parsed_packet);

    if ((csap_descr = csap_find(csap_id)) == NULL)
    {
        ERROR("null csap_descr for csap id %d", csap_id);
        return ETADCSAPNOTEX;
    } 
    spec_data = (tcp_csap_specific_data_t*)csap_descr->layer_data[layer]; 

    data = pkt->data; 

#define CHECK_FIELD(_du_field, _asn_label, _size) \
    do {                                                        \
        rc = tad_univ_match_field(&spec_data-> _du_field, NULL, \
                data, _size, _asn_label);                       \
        if (rc)                                                 \
            return rc;                                          \
        data += _size;                                          \
    } while(0)

    CHECK_FIELD(du_src_port, "src-port", 2);
    CHECK_FIELD(du_dst_port, "dst-port", 2);
    CHECK_FIELD(du_seqn,     "seqn", 4);
    CHECK_FIELD(du_acqn,     "acqn", 4);

    tmp8 = (*data) >> 4;
    rc = tad_univ_match_field(&spec_data->du_hlen, NULL, &tmp8, 1, "hlen");
    if (rc) return rc;
    data ++;

    tmp8 = (*data) & 0x3f;
    rc = tad_univ_match_field(&spec_data->du_flags, NULL, &tmp8, 1, "flags");
    if (rc) return rc;
    data++;

    CHECK_FIELD(du_win_size, "win-size", 2); 
    CHECK_FIELD(du_checksum, "checksum", 2); 
    CHECK_FIELD(du_urg_p,    "urg-p", 2);
 
#undef CHECK_FIELD

    /* TODO: Process TCP options */

    /* passing payload to upper layer */ 
    memset (payload, 0 , sizeof(*payload));
    payload->len = (pkt->len - (data - (uint8_t*)(pkt->data)));
    payload->data = malloc (payload->len);
    memcpy(payload->data, data, payload->len);
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
int tcp_gen_pattern_cb (int csap_id, int layer, const asn_value *tmpl_pdu, 
                                         asn_value_p   *pattern_pdu)
{
    int rc = 0;
    UNUSED(csap_id);
    UNUSED(layer);
    UNUSED(tmpl_pdu);
    UNUSED(pattern_pdu); 
    return rc;
}


