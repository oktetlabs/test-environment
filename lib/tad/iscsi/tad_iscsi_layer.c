/** @file
 * @brief iSCSI TAD
 *
 * Traffic Application Domain Command Handler
 * iSCSI CSAP layer-related callbacks.
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in
 * the root directory of the distribution).
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
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAD iSCSI layer"
#define TE_LOG_LEVEL    0xff

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "logger_api.h"
#include "logger_ta_fast.h"
#include "ndn_iscsi.h"
#include "asn_usr.h"
#include "tad_csap_inst.h"
#include "tad_csap_support.h"

#include "tad_iscsi_impl.h"



/* See description in tad_iscsi_impl.h */
char *
tad_iscsi_get_param_cb(csap_p csap_descr, unsigned int layer,
                       const char *param)
{
    assert(csap_descr != NULL);
    ENTRY("(%d:%u) param=%s", csap_descr->id, layer, param);

    return NULL;
}


/* See description in tad_iscsi_impl.h */
int 
tad_iscsi_confirm_pdu_cb(csap_p csap_descr, unsigned int layer,
                         asn_value *nds_pdu)
{ 
    F_ENTRY("(%d:%u) nds=%p", csap_descr->id, layer, (void *)nds_pdu);
    return 0;
}


/* See description in tad_iscsi_impl.h */
te_errno
tad_iscsi_gen_bin_cb(csap_p csap_descr, unsigned int layer,
                     const asn_value *tmpl_pdu,
                     const tad_tmpl_arg_t *args, size_t arg_num,
                     csap_pkts_p up_payload, csap_pkts_p pkt_list)
{
    UNUSED(tmpl_pdu);
    UNUSED(args);
    UNUSED(arg_num);

    assert(csap_descr != NULL);
    ENTRY("(%d:%u)", csap_descr->id, layer);

    pkt_list->data = up_payload->data;
    pkt_list->len  = up_payload->len;
    pkt_list->next = up_payload->next;

    up_payload->data = NULL;
    up_payload->len  = 0;
    up_payload->next = NULL;

    return 0;
}


/* See description in tad_iscsi_impl.h */
te_errno
tad_iscsi_match_bin_cb(csap_p           csap_descr,
                       unsigned int     layer,
                       const asn_value *pattern_pdu,
                       const csap_pkts *pkt,
                       csap_pkts       *payload, 
                       asn_value       *parsed_packet)
{ 
    iscsi_csap_specific_data_t *spec_data; 

    asn_value *iscsi_msg = asn_init_value(ndn_iscsi_message);
    int        rc;
    int        defect;


    ENTRY("(%d:%u)", csap_descr->id, layer);

    spec_data = (iscsi_csap_specific_data_t *)
                        csap_descr->layers[layer].specific_data; 

    INFO("%s(CSAP %d): got pkt %d bytes",
         __FUNCTION__, csap_descr->id, pkt->len);

    if (spec_data->wait_length == 0)
    {
#if 0
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
                  __FUNCTION__, csap_descr->id, pkt->len);
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
             __FUNCTION__, csap_descr->id,
             (int)total_AHS_length,
             (int)data_segment_length,
             head_digest, data_digest);

        spec_data->wait_length = ISCSI_BHS_LENGTH + data_segment_length +
            (total_AHS_length + head_digest + data_digest) * 4;
        RING("%s(CSAP %d): calculated PDU len: %d", 
             __FUNCTION__, csap_descr->id, spec_data->wait_length);
#else
#if 0
        iscsi_digest_type digest = ISCSI_DIGEST_NONE;
        const             asn_value *sval;

        rc = asn_get_child_value(pattern_pdu, &sval, PRIVATE, 
                                 NDN_TAG_ISCSI_HAVE_HDIG);
        if (rc == 0)
            digest |= ISCSI_DIGEST_HEADER;

        rc = asn_get_child_value(pattern_pdu, &sval, PRIVATE, 
                                 NDN_TAG_ISCSI_HAVE_DDIG);
        if (rc == 0)
            digest |= ISCSI_DIGEST_DATA;
#endif
        spec_data->wait_length = ISCSI_BHS_LENGTH + 
             iscsi_rest_data_len(pkt->data,
                                 spec_data->hdig, spec_data->ddig);
        INFO("%s(CSAP %d), calculated wait length %d",
                __FUNCTION__, csap_descr->id, spec_data->wait_length);
#endif
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
              __FUNCTION__, csap_descr->id, pkt->len,
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
             __FUNCTION__, csap_descr->id, defect);
        rc = TE_ETADLESSDATA;
        goto cleanup;
    }

    memset(payload, 0 , sizeof(*payload));
    payload->len = spec_data->wait_length;
    payload->data = spec_data->stored_buffer; 

    spec_data->wait_length = 0;
    spec_data->stored_length = 0;
    spec_data->stored_buffer = NULL;

    asn_write_component_value(parsed_packet, iscsi_msg, "#iscsi");

cleanup:
    asn_free_value(iscsi_msg);

    return rc;
}


/* See description in tad_iscsi_impl.h */
te_errno
tad_iscsi_gen_pattern_cb(csap_p            csap_descr,
                         unsigned int      layer,
                         const asn_value  *tmpl_pdu, 
                         asn_value       **pattern_pdu)
{
    ENTRY("(%d:%u) tmpl_pdu=%p pattern_pdu=%p",
          csap_descr->id, layer, tmpl_pdu, pattern_pdu);

    assert(pattern_pdu != NULL);
    *pattern_pdu = asn_init_value(ndn_iscsi_message); 

    return 0;
}
