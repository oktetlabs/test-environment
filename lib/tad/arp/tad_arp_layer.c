/** @file
 * @brief TAD ARP
 *
 * Traffic Application Domain Command Handler.
 * ARP CSAP layer-related callbacks.
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAD ARP"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "te_errno.h"
#include "logger_api.h"
#include "logger_ta_fast.h"
#include "asn_usr.h"
#include "tad_csap_support.h"
#include "tad_csap_inst.h"
#include "tad_arp_impl.h"
#include "tad_bps.h"
#include "ndn_arp.h"


/**
 * ARP layer specific data
 */
typedef struct tad_arp_proto_data {
    tad_bps_pkt_frag_def    hdr;
    tad_bps_pkt_frag_def    addrs;
} tad_arp_proto_data;

/**
 * ARP layer specific data for send processing
 */
typedef struct tad_arp_proto_tmpl_data {
    tad_bps_pkt_frag_data   hdr;
    tad_bps_pkt_frag_data   addrs;
} tad_arp_proto_tmpl_data;


/**
 * Definition of ARP header fixed length part.
 */
static const tad_bps_pkt_frag tad_arp_bps_hdr[] =
{
    { "hw-type",    16, BPS_FLD_SIMPLE(NDN_TAG_ARP_HW_TYPE)     },
    { "proto-type", 16, BPS_FLD_SIMPLE(NDN_TAG_ARP_PROTO)       },
    { "hw-size",    8,  BPS_FLD_SIMPLE(NDN_TAG_ARP_HW_SIZE),    },
    { "proto-size", 8,  BPS_FLD_SIMPLE(NDN_TAG_ARP_PROTO_SIZE), },
    { "opcode",     16, BPS_FLD_NO_DEF(NDN_TAG_ARP_OPCODE),     },
};

/**
 * Definition of ARP header volatile length part.
 */
static const tad_bps_pkt_frag tad_arp_bps_addrs[] =
{
    { "snd-hw-addr",    0, BPS_FLD_NO_DEF(NDN_TAG_ARP_SND_HW_ADDR)    },
    { "snd-proto-addr", 0, BPS_FLD_NO_DEF(NDN_TAG_ARP_SND_PROTO_ADDR) },
    { "tgt-hw-addr",    0, BPS_FLD_NO_DEF(NDN_TAG_ARP_TGT_HW_ADDR)    },
    { "tgt-proto-addr", 0, BPS_FLD_NO_DEF(NDN_TAG_ARP_TGT_PROTO_ADDR) },
};


/* See description in tad_arp_impl.h */
te_errno
tad_arp_init_cb(csap_p csap, unsigned int layer)
{
    te_errno            rc;
    tad_arp_proto_data *proto_data;
    const asn_value    *layer_nds;

    proto_data = calloc(1, sizeof(*proto_data));
    if (proto_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    csap_set_proto_spec_data(csap, layer, proto_data);

    layer_nds = csap->layers[layer].csap_layer_pdu;

    rc = tad_bps_pkt_frag_init(tad_arp_bps_hdr,
                               TE_ARRAY_LEN(tad_arp_bps_hdr),
                               layer_nds, &proto_data->hdr);
    if (rc != 0)
        return rc;

    rc = tad_bps_pkt_frag_init(tad_arp_bps_addrs,
                               TE_ARRAY_LEN(tad_arp_bps_addrs),
                               layer_nds, &proto_data->addrs);
    if (rc != 0)
        return rc;

    return rc;
}

/* See description in tad_arp_impl.h */
te_errno
tad_arp_destroy_cb(csap_p csap, unsigned int layer)
{
    F_ENTRY("(%d:%u)", csap->id, layer);

    return 0;
}

/* See description in tad_arp_impl.h */
te_errno
tad_arp_confirm_pdu_cb(csap_p csap, unsigned int layer, 
                       asn_value *layer_pdu, void **p_opaque)
{
    te_errno                    rc;
    tad_arp_proto_data         *proto_data;
    tad_arp_proto_tmpl_data    *tmpl_data;

    F_ENTRY("(%d:%u) layer_pdu=%p", csap->id, layer,
            (void *)layer_pdu);

    proto_data = csap_get_proto_spec_data(csap, layer);
    tmpl_data = malloc(sizeof(*tmpl_data));
    if (tmpl_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    *p_opaque = tmpl_data;

    rc = tad_bps_nds_to_data_units(&proto_data->hdr, layer_pdu,
                                   &tmpl_data->hdr);
    if (rc != 0)
        return rc;

    rc = tad_bps_nds_to_data_units(&proto_data->addrs, layer_pdu,
                                   &tmpl_data->addrs);
    if (rc != 0)
        return rc;

    if (csap->command == TAD_OP_SEND)
    {
        rc = tad_bps_confirm_send(&proto_data->hdr, &tmpl_data->hdr);
        if (rc != 0)
            return rc;
        rc = tad_bps_confirm_send(&proto_data->addrs, &tmpl_data->addrs);
        if (rc != 0)
            return rc;
    }

    return rc;
}

/* See description in tad_arp_impl.h */
te_errno
tad_arp_gen_bin_cb(csap_p                csap,
                   unsigned int          layer,
                   const asn_value      *tmpl_pdu,
                   void                 *opaque,
                   const tad_tmpl_arg_t *args,
                   size_t                arg_num,
                   tad_pkts             *sdus,
                   tad_pkts             *pdus)
{
    tad_arp_proto_data         *proto_data;
    tad_arp_proto_tmpl_data    *tmpl_data = opaque;

    te_errno    rc;
    size_t      hdr_bitlen;
    size_t      addrs_bitlen;
    size_t      bitlen;
    size_t      bitoff;
    uint8_t    *data;
    size_t      len;


    assert(csap != NULL);
    F_ENTRY("(%d:%u) tmpl_pdu=%p args=%p arg_num=%u sdus=%p pdus=%p",
            csap->id, layer, (void *)tmpl_pdu, (void *)args,
            (unsigned)arg_num, sdus, pdus);

    proto_data = csap_get_proto_spec_data(csap, layer);

    hdr_bitlen = tad_bps_pkt_frag_data_bitlen(&proto_data->hdr,
                                              &tmpl_data->hdr);
    addrs_bitlen = tad_bps_pkt_frag_data_bitlen(&proto_data->addrs,
                                                &tmpl_data->addrs);
    bitlen = hdr_bitlen + addrs_bitlen;
    if (hdr_bitlen == 0 || addrs_bitlen == 0 || (bitlen & 7) != 0)
    {
        ERROR("Unexpected lengths: header - %u bits, addresses - %u bits, "
              "total - %u bits", hdr_bitlen, addrs_bitlen, bitlen);
        return TE_RC(TE_TAD_CSAP, TE_EOPNOTSUPP);
    }

    len = (bitlen >> 3);
    data = malloc(len);
    if (data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    bitoff = 0;

    rc = tad_bps_pkt_frag_gen_bin(&proto_data->hdr, &tmpl_data->hdr,
                                  args, arg_num, data, &bitoff, bitlen);
    if (rc != 0)
    {
        ERROR("%s(): tad_bps_pkt_frag_gen_bin failed for header: %r",
              __FUNCTION__, rc);
        free(data);
        return rc;
    }

    rc = tad_bps_pkt_frag_gen_bin(&proto_data->addrs, &tmpl_data->addrs,
                                  args, arg_num, data, &bitoff, bitlen);
    if (rc != 0)
    {
        ERROR("%s(): tad_bps_pkt_frag_gen_bin failed for addresses: %r",
              __FUNCTION__, rc);
        free(data);
        return rc;
    }

#if 1
    if (bitoff != bitlen)
    {
        ERROR("Unexpected bit offset after processing");
        free(data);
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);
    }
#endif

    /* Move all SDUs to PDUs */
    tad_pkts_move(pdus, sdus);
    /* 
     * Add header segment to each PDU. All segments refer to the same
     * memory. Free function is set for segment of the first packet only.
     */
    rc = tad_pkts_add_new_seg(pdus, TRUE, data, len,
                              tad_pkt_seg_data_free);
    if (rc != 0)
    {
        free(data);
        return rc;
    }

    return 0;
}

/* See description in tad_arp_impl.h */
te_errno
tad_arp_match_bin_cb(csap_p           csap,
                     unsigned int     layer,
                     const asn_value *pattern_pdu,
                     const csap_pkts *pkt,
                     csap_pkts       *payload,
                     asn_value       *parsed_packet)
{
    F_ENTRY("(%d:%u) pattern_pdu=%p pkt=%p payload=%p parsed_packet=%p",
            csap->id, layer, (void *)pattern_pdu, pkt, payload,
            (void *)parsed_packet);

    return TE_RC(TE_TAD_CSAP, TE_EOPNOTSUPP);
}
