/** @file
 * @brief TAD iSCSI
 *
 * Traffic Application Domain Command Handler.
 * iSCSI CSAP layer-related callbacks.
 *
 * Copyright (C) 2005-2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
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

#define TE_LGR_USER     "TAD iSCSI"

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


/* See description tad_iscsi_impl.h */
te_errno
tad_iscsi_init_cb(csap_p csap, unsigned int layer)
{
    te_errno    rc;
    int32_t     int32_val;

    const asn_value        *iscsi_nds;
    tad_iscsi_layer_data   *spec_data; 


    spec_data = calloc(1, sizeof(*spec_data));
    if (spec_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    iscsi_nds = csap->layers[layer].csap_layer_pdu;

    if ((rc = asn_read_int32(iscsi_nds, &int32_val, "header-digest")) != 0)
    {
        ERROR("%s(): asn_read_bool() failed for 'header-digest': %r", 
              __FUNCTION__, rc);
        free(spec_data);
        return TE_RC(TE_TAD_CSAP, rc);
    }
    spec_data->hdig = int32_val;

    if ((rc = asn_read_int32(iscsi_nds, &int32_val, "data-digest")) != 0)
    {
        ERROR("%s(): asn_read_bool() failed for 'data-digest': %r", 
              __FUNCTION__, rc);
        free(spec_data);
        return TE_RC(TE_TAD_CSAP, rc);
    }
    spec_data->ddig = int32_val;

    csap_set_proto_spec_data(csap, layer, spec_data);

    return 0;
}

/* See description tad_iscsi_impl.h */
te_errno
tad_iscsi_destroy_cb(csap_p csap, unsigned int layer)
{
    tad_iscsi_layer_data *spec_data; 

    ENTRY("(%d:%u)", csap->id, layer);

    spec_data = csap_get_proto_spec_data(csap, layer); 
    if (spec_data == NULL)
        return 0;
    csap_set_proto_spec_data(csap, layer, NULL); 

    free(spec_data);

    return 0;
}


/* See description in tad_iscsi_impl.h */
te_errno
tad_iscsi_gen_bin_cb(csap_p csap, unsigned int layer,
                     const asn_value *tmpl_pdu, void *opaque,
                     const tad_tmpl_arg_t *args, size_t arg_num, 
                     tad_pkts *sdus, tad_pkts *pdus)
{
    te_errno    rc;

    tad_iscsi_layer_data *spec_data; 

    assert(csap != NULL);

    UNUSED(opaque);
    UNUSED(args);
    UNUSED(arg_num);
    ENTRY("(%d:%u)", csap->id, layer);

    spec_data = csap_get_proto_spec_data(csap, layer); 

    rc = asn_read_value_field(tmpl_pdu, NULL, NULL, "last-data");
    if (rc == 0 && spec_data->send_mode == ISCSI_SEND_USUAL) 
        spec_data->send_mode = ISCSI_SEND_LAST;

    INFO("%s(): read last-data rc: %r", __FUNCTION__, rc);

    tad_pkts_move(pdus, sdus);

    return 0;
}


/* See description in tad_iscsi_impl.h */
te_errno
tad_iscsi_match_bin_cb(csap_p           csap,
                       unsigned int     layer,
                       const asn_value *pattern_pdu,
                       const csap_pkts *pkt,
                       csap_pkts       *payload, 
                       asn_value       *parsed_packet)
{ 
    tad_iscsi_layer_data *spec_data; 

    asn_value *iscsi_msg = asn_init_value(ndn_iscsi_message);
    int        rc;
    int        defect;


    ENTRY("(%d:%u)", csap->id, layer);

    spec_data = csap_get_proto_spec_data(csap, layer); 

    INFO("%s(CSAP %d): got pkt %d bytes",
         __FUNCTION__, csap->id, pkt->len);

    if (spec_data->wait_length == 0)
    {
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
#else
        UNUSED(pattern_pdu);
#endif
        spec_data->wait_length = ISCSI_BHS_LENGTH + 
             iscsi_rest_data_len(pkt->data,
                                 spec_data->hdig, spec_data->ddig);
        INFO("%s(CSAP %d), calculated wait length %d",
                __FUNCTION__, csap->id, spec_data->wait_length);
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
              __FUNCTION__, csap->id, pkt->len,
              spec_data->wait_length, spec_data->stored_length); 
        rc = TE_ETADLOWER;
        goto cleanup;
    }

    memcpy(spec_data->stored_buffer + spec_data->stored_length, 
           pkt->data, pkt->len);
    spec_data->stored_length += pkt->len;

    if (defect > 0)
    {
        INFO("%s(CSAP %d) wait more %d bytes...", 
             __FUNCTION__, csap->id, defect);
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
tad_iscsi_gen_pattern_cb(csap_p            csap,
                         unsigned int      layer,
                         const asn_value  *tmpl_pdu, 
                         asn_value       **pattern_pdu)
{
    ENTRY("(%d:%u) tmpl_pdu=%p pattern_pdu=%p",
          csap->id, layer, tmpl_pdu, pattern_pdu);

    assert(pattern_pdu != NULL);
    *pattern_pdu = asn_init_value(ndn_iscsi_message); 

    return 0;
}
