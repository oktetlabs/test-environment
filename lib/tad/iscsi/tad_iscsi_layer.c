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

#define TE_LGR_USER     "TAD iSCSI layer"

#include "config.h"

#include <string.h>
#include <stdlib.h>

#include "tad_iscsi_impl.h"

#include "logger_api.h"


iscsi_target_params_t target_params = { 0 };

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
char* iscsi_get_param_cb (csap_p csap_descr, int level, const char *param)
{
    iscsi_csap_specific_data_t *spec_data; 

    spec_data = (iscsi_csap_specific_data_t *)
        csap_descr->layers[level].specific_data; 

    UNUSED(spec_data);
    UNUSED(param);
    return NULL;
}

/**
 * Callback for confirm PDU with iSCSI CSAP parameters and possibilities.
 *
 * @param csap_id       identifier of CSAP
 * @param layer         numeric index of layer in CSAP type to be processed.
 * @param nds_pdu      asn_value with PDU (IN/OUT)
 *
 * @return zero on success or error code.
 */ 
int 
iscsi_confirm_pdu_cb(int csap_id, int layer, asn_value *nds_pdu)
{ 
    UNUSED(csap_id);
    UNUSED(layer);
    UNUSED(nds_pdu);
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
 *                      contain only one element, but necessaty of 
 *                      fragmentation sometimes may occur (OUT)
 *
 * @return zero on success or error code.
 */ 
int 
iscsi_gen_bin_cb(csap_p csap_descr, int layer, const asn_value *tmpl_pdu,
                const tad_tmpl_arg_t *args, size_t arg_num,
                csap_pkts_p up_payload, csap_pkts_p pkt_list)
{
    int32_t val;
    UNUSED(csap_descr);
    UNUSED(layer);
    UNUSED(tmpl_pdu);
    UNUSED(args);
    UNUSED(arg_num);

    pkt_list->data = up_payload->data;
    pkt_list->len  = up_payload->len;
    pkt_list->next = up_payload->next;

    up_payload->data = NULL;
    up_payload->len  = 0;
    up_payload->next = NULL;

    if (asn_read_int32(tmpl_pdu, &val, "param") == 0)
        target_params.param = val;

    return 0;
}


/**
 * Callback for parse received packet and match it with pattern. 
 *
 * @param csap_id       identifier of CSAP
 * @param level         numeric index of layer in CSAP type to be processed.
 * @param pattern_pdu   pattern NDS 
 * @param pkt           recevied packet
 * @param payload       rest upper layer payload, if any exists (OUT)
 * @param parsed_packet caller of method should pass here empty asn_value 
 *                      instance of ASN type 'Generic-PDU'. Callback 
 *                      have to fill this instance with values from 
 *                      parsed and matched packet
 *
 * @return zero on success or error code.
 */
int 
iscsi_match_bin_cb(int csap_id, int level, const asn_value *pattern_pdu,
                   const csap_pkts *pkt, csap_pkts *payload, 
                   asn_value *parsed_packet )
{ 
    csap_p                      csap_descr;
    iscsi_csap_specific_data_t *spec_data; 

    asn_value *iscsi_msg = asn_init_value(ndn_iscsi_message);
    int        rc;
    int        defect;

    UNUSED(pattern_pdu);

    if ((csap_descr = csap_find(csap_id)) == NULL)
        return TE_ETADCSAPNOTEX;

    spec_data = (iscsi_csap_specific_data_t *)
                        csap_descr->layers[level].specific_data; 

    if (spec_data->wait_length == 0)
    {
        size_t  total_AHS_length,
                data_segment_length;

        int head_digest = 0, data_digest = 0; 

        const asn_value *sval;

        union { 
            uint32_t i;
            uint8_t b[4];
        } dsl_convert;

        if (pkt->len < ISCSI_BHS_LENGTH)
        {
            ERROR("%s(CSAP %d): very short first fragment, %d bytes", 
                  __FUNCTION__, csap_id, pkt->len);
            return TE_ETADLOWER;
        }

        total_AHS_length = ((uint8_t *)pkt->data)[4];

        dsl_convert.b[0] = 0;
        dsl_convert.b[1] = ((uint8_t *)pkt->data)[5];
        dsl_convert.b[2] = ((uint8_t *)pkt->data)[6];
        dsl_convert.b[3] = ((uint8_t *)pkt->data)[7];

        data_segment_length = ntohl(dsl_convert.i);

        /* DataSegment padding */
        if (data_segment_length % 4)
            data_segment_length += (4 - (data_segment_length % 4));

        rc = asn_get_child_value(pattern_pdu, &sval, PRIVATE, 
                                 NDN_TAG_ISCSI_HAVE_HDIG);
        if (rc == 0)
            head_digest = 1;

        rc = asn_get_child_value(pattern_pdu, &sval, PRIVATE, 
                                 NDN_TAG_ISCSI_HAVE_DDIG);
        if (rc == 0)
            data_digest = 1;

        RING("%s(CSAP %d): AHS len = %d; DataSegmLen = %d; "
             "HeadDig %d; DataDig %d", 
             __FUNCTION__, csap_id,
             (int)total_AHS_length,
             (int)data_segment_length,
             head_digest, data_digest);

        spec_data->wait_length = ISCSI_BHS_LENGTH + data_segment_length +
            (total_AHS_length + head_digest + data_digest) * 4;
        RING("%s(CSAP %d): calculated PDU len: %d", 
             __FUNCTION__, csap_id, spec_data->wait_length);
    }
    rc = 0;

    if (spec_data->stored_buffer == NULL)
    {
        spec_data->stored_buffer = malloc(spec_data->wait_length);
        spec_data->stored_length = 0;
    }

    defect = spec_data->wait_length - 
            (spec_data->stored_length + pkt->len);

    if (defect < 0)
    {
        ERROR("%s(CSAP %d) get too many data: %d bytes, "
              "wait for %d, stored %d", 
              __FUNCTION__, csap_id, pkt->len,
              spec_data->wait_length, spec_data->stored_length); 
        rc = TE_ETADLOWER;
        goto cleanup;
    }

    memcpy(spec_data->stored_buffer + spec_data->stored_length, 
           pkt->data, pkt->len);
    spec_data->stored_length += pkt->len;

    if (defect > 0)
    {
        RING("%s(CSAP %d) wait more %d bytes...", 
             __FUNCTION__, csap_id, defect);
        rc = TE_ETADLESSDATA;
        goto cleanup;
    }

    memset(payload, 0 , sizeof(*payload));
    payload->len = spec_data->wait_length;
    payload->data = spec_data->stored_buffer; 

    asn_write_int32(iscsi_msg, target_params.param, "param");

    asn_write_component_value(parsed_packet, iscsi_msg, "#iscsi");

cleanup:
    asn_free_value(iscsi_msg);

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
 * @param pattern_pdu   ASN value with pattern PDU, generated according 
 *                      to passed template PDU and CSAP parameters (OUT). 
 *
 * @return zero on success or error code.
 */
int 
iscsi_gen_pattern_cb(int csap_id, int layer, const asn_value *tmpl_pdu, 
                    asn_value **pattern_pdu)
{
    int rc = 0;
    *pattern_pdu = asn_init_value(ndn_iscsi_message); 

    /* TODO: DHCP options to be inserted into pattern */
    UNUSED(csap_id);
    UNUSED(layer);
    UNUSED(tmpl_pdu);


    return rc;
}


