/** @file
 * @brief TAD IP Stack
 *
 * Traffic Application Domain Command Handler.
 * UDP CSAP layer-related callbacks.
 *
 * Copyright (C) 2004-2005 Test Environment authors (see file AUTHORS
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

#define TE_LGR_USER "TAD UDP" 

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <stdlib.h>

#include "logger_api.h"
#include "logger_ta_fast.h"

#include "tad_ipstack_impl.h"



/* See description tad_ipstack_impl.h */
te_errno 
tad_udp_init_cb(csap_p csap, unsigned int layer)
{
    udp_csap_specific_data_t *udp_spec_data; 
    const asn_value          *proto_nds;

    size_t len; 
    int    rc; 

    proto_nds = csap->layers[layer].csap_layer_pdu;

    udp_spec_data = calloc (1, sizeof(udp_csap_specific_data_t));
    if (udp_spec_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    /* Init local port */
    len = sizeof(udp_spec_data->local_port);
    rc = asn_read_value_field(proto_nds, &udp_spec_data->local_port,
                              &len, "local-port");
    if (rc != 0)
    {
        WARN("%s: %d.local-port is not found in CSAP pattern, set to 0",
             __FUNCTION__, layer);
        udp_spec_data->local_port = 0;
    }

    /* Init remote port */
    len = sizeof(udp_spec_data->remote_port);
    rc = asn_read_value_field(proto_nds, &udp_spec_data->remote_port, 
                              &len, "remote-port");
    if (rc != 0)
    {
        WARN("%s: %d.remote-port is not found in CSAP pattern, set to 0",
             __FUNCTION__, layer);
        udp_spec_data->remote_port = 0;
    }

    csap_set_proto_spec_data(csap, layer, udp_spec_data);

    return 0;
}

/* See description tad_ipstack_impl.h */
te_errno 
tad_udp_destroy_cb(csap_p csap, unsigned int layer)
{
    UNUSED(csap);
    UNUSED(layer);
    return 0;
}

/* See description in tad_ipstack_impl.h */
te_errno
tad_udp_confirm_tmpl_cb(csap_p csap, unsigned int layer,
                        asn_value_p layer_pdu, void **p_opaque)
{
    te_errno                  rc;
    asn_value                *udp_layer_pdu;
    udp_csap_specific_data_t *udp_spec_data;

    UNUSED(p_opaque);

    udp_spec_data = csap_get_proto_spec_data(csap, layer);
    if (udp_spec_data == NULL)
    {
        ERROR("%s: CSAP-specific data is NULL", __FUNCTION__);
        return TE_EWRONGPTR;
    } 

    if(asn_get_syntax(layer_pdu, "") != CHOICE)
        asn_get_choice_value(layer_pdu, (const asn_value **)&udp_layer_pdu,
                             NULL, NULL);
    else
        udp_layer_pdu = layer_pdu;

    rc = tad_data_unit_convert(layer_pdu, NDN_TAG_UDP_SRC_PORT,
                              &(udp_spec_data->du_src_port)); 
    if (rc != 0)
    {
        ERROR("%s: failed to convert src-port from pattern: %r",
              __FUNCTION__, rc);
        return TE_RC(TE_TAD_CSAP, rc);
    }
    if (udp_spec_data->du_src_port.du_type == TAD_DU_I32)
        udp_spec_data->src_port = udp_spec_data->du_src_port.val_i32;


    rc = tad_data_unit_convert(layer_pdu, NDN_TAG_UDP_DST_PORT,
                              &(udp_spec_data->du_dst_port)); 
    if (rc != 0)
    {
        ERROR("%s: failed to convert dst-port from pattern: %r",
              __FUNCTION__, rc);
        return TE_RC(TE_TAD_CSAP, rc);
    }
    if (udp_spec_data->du_dst_port.du_type == TAD_DU_I32)
        udp_spec_data->dst_port = udp_spec_data->du_dst_port.val_i32;


    /* Update pattern with CSAP defaults */
    /* send, local is source */
    if (udp_spec_data->du_src_port.du_type == TAD_DU_UNDEF &&
        udp_spec_data->local_port != 0)
    {
        VERB("%s: set src-port to %u",
              __FUNCTION__, udp_spec_data->local_port);
        rc = asn_write_int32(layer_pdu, udp_spec_data->local_port,
                             "src-port.#plain");
        if (rc != 0)
        {
            ERROR("%s: failed to update src-port in pattern: %r",
                  __FUNCTION__, rc);
            return TE_RC(TE_TAD_CSAP, rc);
        }
        udp_spec_data->src_port = udp_spec_data->local_port;
        udp_spec_data->du_src_port.val_i32 = udp_spec_data->local_port;
        udp_spec_data->du_src_port.du_type = TAD_DU_I32;
    }
    if (udp_spec_data->du_dst_port.du_type == TAD_DU_UNDEF)
    {
        if (udp_spec_data->remote_port != 0)
        {
            VERB("%s: set dst-port to %u",
                  __FUNCTION__, udp_spec_data->remote_port);
            rc = asn_write_int32(layer_pdu, udp_spec_data->remote_port,
                                 "dst-port.#plain");
            if (rc != 0)
            {
                ERROR("%s: failed to update dst-port in pattern: %r",
                      __FUNCTION__, rc);
                return TE_RC(TE_TAD_CSAP, rc);
            }
            udp_spec_data->dst_port = udp_spec_data->remote_port;
            udp_spec_data->du_dst_port.val_i32 =
                                      udp_spec_data->remote_port;
            udp_spec_data->du_dst_port.du_type = TAD_DU_I32;
        }
        else
        {
            ERROR("%s: sending csap #%d, "
                  "has no dst-port in template and has no remote port",
                  __FUNCTION__, csap->id);
            return TE_RC(TE_TAD_CSAP, TE_EINVAL);
        }
    }

    return 0;
}

/* See description in tad_ipstack_impl.h */
te_errno
tad_udp_confirm_ptrn_cb(csap_p csap, unsigned int layer,
                        asn_value_p layer_pdu, void **p_opaque)
{
    te_errno                  rc;
    asn_value                *udp_layer_pdu;
    udp_csap_specific_data_t *udp_spec_data;

    UNUSED(p_opaque);

    udp_spec_data = csap_get_proto_spec_data(csap, layer);
    if (udp_spec_data == NULL)
    {
        ERROR("%s: CSAP-specific data is NULL", __FUNCTION__);
        return TE_EWRONGPTR;
    } 

    if(asn_get_syntax(layer_pdu, "") != CHOICE)
        asn_get_choice_value(layer_pdu, (const asn_value **)&udp_layer_pdu,
                             NULL, NULL);
    else
        udp_layer_pdu = layer_pdu;

    rc = tad_data_unit_convert(layer_pdu, NDN_TAG_UDP_SRC_PORT,
                              &(udp_spec_data->du_src_port)); 
    if (rc != 0)
    {
        ERROR("%s: failed to convert src-port from pattern: %r",
              __FUNCTION__, rc);
        return TE_RC(TE_TAD_CSAP, rc);
    }
    if (udp_spec_data->du_src_port.du_type == TAD_DU_I32)
        udp_spec_data->src_port = udp_spec_data->du_src_port.val_i32;


    rc = tad_data_unit_convert(layer_pdu, NDN_TAG_UDP_DST_PORT,
                              &(udp_spec_data->du_dst_port)); 
    if (rc != 0)
    {
        ERROR("%s: failed to convert dst-port from pattern: %r",
              __FUNCTION__, rc);
        return TE_RC(TE_TAD_CSAP, rc);
    }
    if (udp_spec_data->du_dst_port.du_type == TAD_DU_I32)
        udp_spec_data->dst_port = udp_spec_data->du_dst_port.val_i32;


    /* Update pattern with CSAP defaults */
    /* receive, local is destination */
    if (udp_spec_data->dst_port == 0 && 
        udp_spec_data->local_port != 0)
    {
        VERB("%s: set dst-port to %u",
              __FUNCTION__, udp_spec_data->local_port);
        rc = asn_write_int32(layer_pdu, udp_spec_data->local_port,
                             "dst-port.#plain");
        if (rc != 0)
        {
            ERROR("%s: failed to update dst-port in pattern: %r",
                  __FUNCTION__, rc);
            return TE_RC(TE_TAD_CSAP, rc);
        }
        udp_spec_data->dst_port = udp_spec_data->local_port;
    }
    if (udp_spec_data->src_port == 0 &&
        udp_spec_data->remote_port != 0)
    {
        VERB("%s: set src-port to %u",
              __FUNCTION__, udp_spec_data->remote_port);
        rc = asn_write_int32(layer_pdu, udp_spec_data->remote_port,
                             "src-port.#plain");
        if (rc != 0)
        {
            ERROR("%s: failed to update src-port in pattern: %r",
                  __FUNCTION__, rc);
            return TE_RC(TE_TAD_CSAP, rc);
        }
        udp_spec_data->src_port = udp_spec_data->remote_port;
    }

    return 0;
}


/** Opaque data for tad_udp_fill_in_hdr() callback */
typedef struct tad_udp_fill_in_hdr_data {
    udp_csap_specific_data_t   *spec_data;
    const tad_tmpl_arg_t       *args;
    unsigned int                arg_num;
} tad_udp_fill_in_hdr_data;

/**
 * Callback function to fill in UDP header.
 *
 * This function complies with tad_pkt_seg_enum_cb prototype.
 */
static te_errno
tad_udp_fill_in_hdr(const tad_pkt *pkt, tad_pkt_seg *seg,
                    unsigned int seg_num, void *opaque)
{
    tad_udp_fill_in_hdr_data   *data = opaque;
    uint8_t                    *p = seg->data_ptr;
    te_errno                    rc;

    UNUSED(seg_num);
    assert(seg->data_len == 8);

#define PUT_UINT16(val_) \
    do {                                            \
        *((uint16_t *)p) = htons((uint16_t)val_);   \
        p += sizeof(uint16_t);                      \
    } while (0) 

#define PUT_DU16(c_du_field_, def_val_) \
    do {                                                                \
        if (data->spec_data->c_du_field_.du_type != TAD_DU_UNDEF)       \
        {                                                               \
            rc = tad_data_unit_to_bin(&(data->spec_data->c_du_field_),  \
                                      data->args, data->arg_num, p, 2); \
            if (rc != 0)                                                \
            {                                                           \
                ERROR("%s():%d: " #c_du_field_ " error: %r",            \
                      __FUNCTION__,  __LINE__, rc);                     \
                return rc;                                              \
            }                                                           \
            p += sizeof(uint16_t);                                      \
        }                                                               \
        else                                                            \
            PUT_UINT16(def_val_);                                       \
    } while (0) 

    PUT_DU16(du_src_port, data->spec_data->local_port);
    PUT_DU16(du_dst_port, data->spec_data->remote_port);

    /* Length in the UDP header includes length of the header itself */
    PUT_UINT16(tad_pkt_len(pkt)); 
    /* FIXME Checksum will be filled in later by IPv4 layer */
    PUT_UINT16(0);

#undef PUT_UINT16
#undef PUT_DU16

    return 0;
}

/* See description in tad_ipstack_impl.h */
te_errno
tad_udp_gen_bin_cb(csap_p csap, unsigned int layer,
                   const asn_value *tmpl_pdu, void *opaque,
                   const tad_tmpl_arg_t *args, size_t arg_num, 
                   tad_pkts *sdus, tad_pkts *pdus)
{
    te_errno                  rc;
    tad_udp_fill_in_hdr_data  opaque_data;
 
    UNUSED(tmpl_pdu); 
    UNUSED(opaque);
 
    /* UDP layer does no fragmentation, just copy all SDUs to PDUs */
    tad_pkts_move(pdus, sdus);

    /*
     * Allocate and add UDP header to all packets.
     * FIXME sizeof(struct udphdr) instead of 8.
     */
    rc = tad_pkts_add_new_seg(pdus, TRUE, NULL, 8, NULL);
    if (rc != 0)
        return rc;

    /* Fill in added segment as UDP header */
    opaque_data.spec_data = csap_get_proto_spec_data(csap, layer);
    opaque_data.args      = args;
    opaque_data.arg_num   = arg_num;
    rc = tad_pkts_enumerate_first_segs(pdus, tad_udp_fill_in_hdr,
                                       &opaque_data);

    return rc;
}


/* See description in tad_ipstack_impl.h */
te_errno
tad_udp_match_bin_cb(csap_p csap,
                        unsigned int     layer,
                        const asn_value *ptrn_pdu,
                        void            *ptrn_opaque,
                        tad_recv_pkt    *meta_pkt,
                        tad_pkt         *pdu,
                        tad_pkt         *sdu)
{
    uint8_t    *data_ptr; 
    uint8_t    *data; 
    size_t      data_len;
    asn_value  *udp_header_pdu = NULL;
    te_errno    rc;

    assert(tad_pkt_seg_num(pdu) == 1);
    assert(tad_pkt_first_seg(pdu) != NULL);
    data_ptr = data = tad_pkt_first_seg(pdu)->data_ptr;
    data_len = tad_pkt_first_seg(pdu)->data_len;

    if ((csap->state & CSAP_STATE_RESULTS) &&
        (udp_header_pdu = meta_pkt->layers[layer].nds =
             asn_init_value(ndn_udp_header)) == NULL)
    {
        ERROR_ASN_INIT_VALUE(ndn_udp_header);
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    }

    UNUSED(csap);
    UNUSED(layer);

#define CHECK_FIELD(_asn_label, _size) \
    do {                                                        \
        rc = ndn_match_data_units(ptrn_pdu, udp_header_pdu,  \
                                  data, _size, _asn_label);     \
        if (rc != 0)                                            \
        {                                                       \
            VERB("%s: field '%s' not match: %r",                \
                 __FUNCTION__, _asn_label, rc);                 \
            return rc;                                          \
        }                                                       \
        data += _size;                                          \
    } while(0)

    CHECK_FIELD("src-port", 2);
    CHECK_FIELD("dst-port", 2);
    CHECK_FIELD("length", 2);
    CHECK_FIELD("checksum", 2);
#undef CHECK_FIELD

    /* Passing payload to upper layer */
    rc = tad_pkt_get_frag(sdu, pdu, data - data_ptr,
                          data_len - (data - data_ptr),
                          TAD_PKT_GET_FRAG_ERROR);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Failed to prepare UDP SDU: %r",
              CSAP_LOG_ARGS(csap), rc);
        return rc;
    }

    return rc;
}
