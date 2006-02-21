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

#define _GNU_SOURCE
#include <stdio.h>
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

    iscsi_nds = csap->layers[layer].nds;

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
char *
tad_iscsi_get_param_cb(csap_p csap, unsigned int layer, const char *param)
{
    tad_iscsi_layer_data *spec_data;
    char                 *result = NULL;

    spec_data = csap_get_proto_spec_data(csap, layer); 

    if (strcmp(param, "total_received") == 0)
    {
        if (asprintf(&result, "%llu",
                     (unsigned long long)spec_data->total_received) < 0)
        {
            ERROR("%s(): asprintf() failed", __FUNCTION__);
            result = NULL;
        }
    }

    return result;
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
                        const asn_value *ptrn_pdu,
                        void            *ptrn_opaque,
                        tad_recv_pkt    *meta_pkt,
                        tad_pkt         *pdu,
                        tad_pkt         *sdu)
{ 
    tad_iscsi_layer_data   *spec_data; 
    tad_pkt_seg            *seg;

    uint8_t    *data_ptr; 
    size_t      data_len;
    asn_value  *iscsi_msg = NULL;
    te_errno    rc;
    int         defect;

    UNUSED(ptrn_opaque);

    assert(tad_pkt_seg_num(pdu) == 1);
    assert(tad_pkt_first_seg(pdu) != NULL);
    data_ptr = tad_pkt_first_seg(pdu)->data_ptr;
    data_len = tad_pkt_first_seg(pdu)->data_len;

    
    if ((csap->state & CSAP_STATE_RESULTS) &&
        (iscsi_msg = meta_pkt->layers[layer].nds =
             asn_init_value(ndn_iscsi_message)) == NULL)
    {
        ERROR_ASN_INIT_VALUE(ndn_iscsi_message);
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    }

    spec_data = csap_get_proto_spec_data(csap, layer); 

    INFO("%s(CSAP %d): got pkt %u bytes",
         __FUNCTION__, csap->id, (unsigned)data_len);

    /* Parse and reassembly iSCSI PDU */

    if (spec_data->wait_length == 0)
    {
        spec_data->wait_length = ISCSI_BHS_LENGTH + 
             iscsi_rest_data_len(data_ptr,
                                 spec_data->hdig, spec_data->ddig);
        INFO("%s(CSAP %d), calculated wait length %d",
                __FUNCTION__, csap->id, spec_data->wait_length);
    }
    else if (spec_data->wait_length == spec_data->stored_length)
        goto begin_match;

    if (spec_data->stored_buffer == NULL)
    {
        spec_data->stored_buffer = malloc(spec_data->wait_length);
        spec_data->stored_length = 0;
    }
    defect = spec_data->wait_length - 
            (spec_data->stored_length + data_len);

    if (defect < 0)
    {
        ERROR("%s(CSAP %d) get too many data: %u bytes, "
              "wait for %d, stored %d", 
              __FUNCTION__, csap->id, (unsigned)data_len,
              spec_data->wait_length, spec_data->stored_length); 
        rc = TE_ETADLOWER;
        goto cleanup;
    }

    memcpy(spec_data->stored_buffer + spec_data->stored_length, 
           data_ptr, data_len);
    spec_data->stored_length += data_len;

    if (defect > 0)
    {
        INFO("%s(CSAP %d) wait more %d bytes...", 
             __FUNCTION__, csap->id, defect);
        rc = TE_ETADLESSDATA;
        goto cleanup;
    }

    /* received message successfully processed and reassembled */

begin_match:
    do {
        uint8_t    tmp8;
        uint8_t   *p = spec_data->stored_buffer; 

        /* Here is a big ASS with matching: if packet 
         * does not match, but there are many pattern_units, 
         * what we have to do with stored reassembled buffer???
         */

        tmp8 = (*p >> 6) & 1;
        if ((rc = ndn_match_data_units(ptrn_pdu, NULL, 
                                       &tmp8, 1, "i-bit")) != 0)
            goto cleanup;

        tmp8 = *p & 0x3f;
        if ((rc = ndn_match_data_units(ptrn_pdu, NULL, 
                                       &tmp8, 1, "opcode")) != 0)
            goto cleanup;

        p++;
        tmp8 = *p >> 7;
        if ((rc = ndn_match_data_units(ptrn_pdu, NULL, 
                                       &tmp8, 1, "f-bit")) != 0)
            goto cleanup;

        if ((rc = ndn_match_data_units(ptrn_pdu, NULL, 
                                       p, 3, "op-specific")) != 0)
            goto cleanup;

    } while(0);

    seg = tad_pkt_alloc_seg(spec_data->stored_buffer,
                           spec_data->wait_length,
                           tad_pkt_seg_data_free);
    if (seg == NULL)
    {
        free(spec_data->stored_buffer);
        ERROR(CSAP_LOG_FMT "tad_pkt_alloc_seg() failed",
              CSAP_LOG_ARGS(csap));
        rc = TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    }
    else
    {
        tad_pkt_append_seg(sdu, seg);
    }

    spec_data->wait_length = 0;
    spec_data->stored_length = 0;
    spec_data->stored_buffer = NULL;

cleanup:
    return rc;
}


/* See description in tad_iscsi_impl.h */
te_errno
tad_iscsi_gen_pattern_cb(csap_p            csap,
                         unsigned int      layer,
                         const asn_value  *tmpl_pdu, 
                         asn_value       **ptrn_pdu)
{
    ENTRY("(%d:%u) tmpl_pdu=%p ptrn_pdu=%p",
          csap->id, layer, tmpl_pdu, ptrn_pdu);

    assert(ptrn_pdu != NULL);
    if ((*ptrn_pdu = asn_init_value(ndn_iscsi_message)) == NULL)
    {
        ERROR_ASN_INIT_VALUE(ndn_iscsi_message);
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    }

    return 0;
}


#if 0 

/* See description in tad_iscsi_impl.h */
te_errno
tad_iscsi_confirm_ptrn_cb(csap_p csap, unsigned int layer,
                          asn_value_p layer_pdu,
                          void **p_opaque)
{
    te_errno    rc = 0;

    const asn_value *iscsi_csap_pdu;

    tad_iscsi_layer_data *spec_data = csap_get_proto_spec_data(csap, layer);

    UNUSED(p_opaque);
    UNUSED(layer);

    iscsi_csap_pdu = csap->layers[layer].nds; 

#define CONVERT_FIELD(tag_, du_field_)                                  \
    do {                                                                \
        rc = tad_data_unit_convert(layer_pdu, tag_,                     \
                                   &(spec_data->du_field_));            \
        if (rc != 0)                                                    \
        {                                                               \
            ERROR("%s(csap %d),line %d, convert %s failed, rc %r",      \
                  __FUNCTION__, csap->id, __LINE__, #du_field_, rc);    \
            return rc;                                                  \
        }                                                               \
    } while (0) 

    CONVERT_FIELD(NDN_TAG_ISCSI_I_BIT, du_i_bit);
    CONVERT_FIELD(NDN_TAG_ISCSI_OPCODE, du_opcode);
    CONVERT_FIELD(NDN_TAG_ISCSI_F_BIT, du_f_bit);

    return rc;
}
#endif
