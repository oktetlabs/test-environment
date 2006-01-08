/** @file
 * @brief TAD Ethernet
 *
 * Traffic Application Domain Command Handler.
 * Ethernet CSAP layer-related callbacks.
 *
 * See IEEE 802.1d, 802.1q.
 *
 * Copyright (C) 2003-2006 Test Environment authors (see file AUTHORS
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

#define TE_LGR_USER     "TAD Ethernet"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif
#if HAVE_NET_IF_ETHER_H
#include <net/if_ether.h>
#endif
//#include <fcntl.h>

#include "te_defs.h"
#include "logger_api.h"
#include "logger_ta_fast.h"

#include "tad_bps.h"
#include "tad_eth_impl.h"


/* 
 * Ethernet CSAP specific data
 */
typedef struct eth_csap_specific_data {
    uint16_t eth_type;  /**< Ethernet protocol type                     */

    char *remote_addr;  /**< Default remote address (NULL if undefined) */
    char *local_addr;   /**< Default local address (NULL if undefined)  */

    int   cfi;
    int   vlan_id;
    int   priority;

    tad_data_unit_t   du_dst_addr;
    tad_data_unit_t   du_src_addr;
    tad_data_unit_t   du_eth_type;
    tad_data_unit_t   du_cfi;
    tad_data_unit_t   du_vlan_id;
    tad_data_unit_t   du_priority;

} eth_csap_specific_data_t, *eth_csap_specific_data_p;

/**
 * Ethernet layer specific data
 */
typedef struct tad_eth_proto_data {
    tad_bps_pkt_frag_def        hdr_d;
    tad_bps_pkt_frag_def        hdr_q;
    eth_csap_specific_data_t    old;
} tad_eth_proto_data;

/**
 * Ethernet layer specific data for send processing
 */
typedef struct tad_eth_proto_tmpl_data {
    tad_bps_pkt_frag_data   hdr;
} tad_eth_proto_tmpl_data;


/**
 * Definition of 802.1d Ethernet header.
 */
static const tad_bps_pkt_frag tad_802_1d_bps_hdr[] =
{
    { "dst-addr", 48, NDN_TAG_ETH_DST,
      NDN_TAG_ETH_REMOTE, NDN_TAG_ETH_LOCAL, 0 },
    { "src-addr", 48, NDN_TAG_ETH_SRC,
      NDN_TAG_ETH_LOCAL, NDN_TAG_ETH_REMOTE, 0 },
    { "eth-type", 16, BPS_FLD_SIMPLE(NDN_TAG_ETH_TYPE_LEN) },
};

/**
 * Definition of 802.1q Ethernet header.
 */
static const tad_bps_pkt_frag tad_802_1q_bps_hdr[] =
{
    { "dst-addr", 48, NDN_TAG_ETH_DST,
      NDN_TAG_ETH_REMOTE, NDN_TAG_ETH_LOCAL, 0 },
    { "src-addr", 48, NDN_TAG_ETH_SRC,
      NDN_TAG_ETH_LOCAL, NDN_TAG_ETH_REMOTE, 0 },
    { "tpid",     16, BPS_FLD_CONST(ETH_TAGGED_TYPE_LEN) },
    { "priority",  3, BPS_FLD_SIMPLE(NDN_TAG_ETH_PRIO) },
    { "cfi",       1, BPS_FLD_SIMPLE(NDN_TAG_ETH_CFI) },
    { "vlan-id",  12, BPS_FLD_SIMPLE(NDN_TAG_ETH_VLAN_ID) },
    { "eth-type", 16, BPS_FLD_SIMPLE(NDN_TAG_ETH_TYPE_LEN) },
};


/* See description in tad_eth_impl.h */
te_errno
tad_eth_init_cb(csap_p csap, unsigned int layer)
{
    te_errno            rc;
    tad_eth_proto_data *proto_data;
    const asn_value    *layer_nds;

    proto_data = calloc(1, sizeof(*proto_data));
    if (proto_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    csap_set_proto_spec_data(csap, layer, proto_data);

    layer_nds = csap->layers[layer].csap_layer_pdu;

    rc = tad_bps_pkt_frag_init(tad_802_1d_bps_hdr,
                               TE_ARRAY_LEN(tad_802_1d_bps_hdr),
                               layer_nds, &proto_data->hdr_d);
    if (rc != 0)
        return rc;

    rc = tad_bps_pkt_frag_init(tad_802_1q_bps_hdr,
                               TE_ARRAY_LEN(tad_802_1q_bps_hdr),
                               layer_nds, &proto_data->hdr_q);
    if (rc != 0)
        return rc;

    /* FIXME */
    if (layer > 0 &&
        proto_data->hdr_d.tx_def[2].du_type == TAD_DU_UNDEF &&
        proto_data->hdr_d.rx_def[2].du_type == TAD_DU_UNDEF)
    {
        uint16_t    eth_type;

        VERB("%s(): eth-type is not defined, try to guess", __FUNCTION__);
        switch (csap->layers[layer - 1].proto_tag)
        {
            case TE_PROTO_IP4:
                eth_type = ETHERTYPE_IP;
                break;

            case TE_PROTO_ARP:
                eth_type = ETHERTYPE_ARP;
                break;

            default:
                eth_type = 0;
                break;
        }
        if (eth_type != 0)
        {
            INFO("%s(): Guessed eth-type is 0x%x", __FUNCTION__, eth_type);
            proto_data->hdr_d.tx_def[2].du_type = TAD_DU_I32;
            proto_data->hdr_d.tx_def[2].val_i32 = eth_type;
            proto_data->hdr_d.rx_def[2].du_type = TAD_DU_I32;
            proto_data->hdr_d.rx_def[2].val_i32 = eth_type;
        }
    }

#if 1
{
    uint8_t remote_addr[ETHER_ADDR_LEN];
    uint8_t local_addr[ETHER_ADDR_LEN];
    uint16_t eth_type;
    size_t              val_len;
    eth_csap_specific_data_p spec_data = &proto_data->old;

    /* setting remote address */
    val_len = sizeof(remote_addr);
    rc = asn_read_value_field(layer_nds, remote_addr, &val_len, "remote-addr");
    
    if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL) 
    {
        spec_data->remote_addr = NULL;
    }
    else if (rc)
    {
        free_eth_csap_data(spec_data, ETH_COMPLETE_FREE);
        ERROR("Init, asn read of rem_addr");
        return TE_RC(TE_TAD_CSAP, rc);
    }
    else
    {
        spec_data->remote_addr = malloc(ETHER_ADDR_LEN);
    
        if (spec_data->remote_addr == NULL)
        {
            free_eth_csap_data(spec_data, ETH_COMPLETE_FREE);
            ERROR("Init, no mem");
            return TE_RC(TE_TAD_CSAP, TE_ENOMEM); 
        }
        memcpy (spec_data->remote_addr, remote_addr, ETHER_ADDR_LEN);
    }

    /* setting local address */
    
    val_len = sizeof(local_addr); 
    rc = asn_read_value_field(layer_nds, local_addr, &val_len, "local-addr");
    
    if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL) 
        spec_data->local_addr = NULL;
    else if (rc)
    {
        free_eth_csap_data(spec_data, ETH_COMPLETE_FREE);
        ERROR("ASN processing error %r", rc);
        return TE_RC(TE_TAD_CSAP, rc); 
    }
    else
    {
        spec_data->local_addr = malloc(ETHER_ADDR_LEN);
        if (spec_data->local_addr == NULL)
        {
            free_eth_csap_data(spec_data,ETH_COMPLETE_FREE);
            ERROR("Init, no mem");
            return TE_RC(TE_TAD_CSAP, TE_ENOMEM); 
        }
        memcpy (spec_data->local_addr, local_addr, ETHER_ADDR_LEN);
    }
    
    /* setting ethernet type */
    val_len = sizeof(eth_type); 
    
    rc = asn_read_value_field(layer_nds, &eth_type, &val_len, "eth-type");
    
    if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL) 
    {
        spec_data->eth_type = DEFAULT_ETH_TYPE;
        rc = 0;
    }
    else if (rc != 0)
    {
        ERROR("ASN processing error %r", rc);
        return TE_RC(TE_TAD_CSAP, rc);
    }
    else 
    {
        spec_data->eth_type = eth_type;
    }    

    /* Read CFI */
    val_len = sizeof(spec_data->cfi);
    rc = asn_read_value_field(layer_nds, &spec_data->cfi, 
            &val_len, "cfi");
    if (rc)
        spec_data->cfi = -1;

    /* Read VLAN ID */
    val_len = sizeof(spec_data->vlan_id);
    rc = asn_read_value_field(layer_nds, &spec_data->vlan_id, 
            &val_len, "vlan-id");
    if (rc)
        spec_data->vlan_id = -1;

    /* Read priority */
    val_len = sizeof(spec_data->priority);
    rc = asn_read_value_field(layer_nds, &spec_data->priority, 
            &val_len, "priority");
    if (rc)
        spec_data->priority = -1;

    rc = 0;
}
#endif

    return rc;
}

/* See description in tad_eth_impl.h */
te_errno
tad_eth_destroy_cb(csap_p csap, unsigned int layer)
{
#if 1
    tad_eth_proto_data         *proto_data;
    eth_csap_specific_data_p    spec_data;

    proto_data = csap_get_proto_spec_data(csap, layer);
    spec_data = &proto_data->old;

    tad_data_unit_clear(&spec_data->du_dst_addr);
    tad_data_unit_clear(&spec_data->du_src_addr);
    tad_data_unit_clear(&spec_data->du_eth_type);
    tad_data_unit_clear(&spec_data->du_cfi);
    tad_data_unit_clear(&spec_data->du_priority);
    tad_data_unit_clear(&spec_data->du_vlan_id);
#endif

    return 0;
}


/* See description in tad_eth_impl.h */
te_errno
tad_eth_confirm_tmpl_cb(csap_p csap, unsigned int layer,
                        asn_value *layer_pdu, void **p_opaque)
{
    te_errno                    rc;
    tad_eth_proto_data         *proto_data;
    tad_eth_proto_tmpl_data    *tmpl_data;

    F_ENTRY("(%d:%u) layer_pdu=%p", csap->id, layer,
            (void *)layer_pdu);

    proto_data = csap_get_proto_spec_data(csap, layer);
    tmpl_data = malloc(sizeof(*tmpl_data));
    if (tmpl_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    *p_opaque = tmpl_data;

    rc = tad_bps_nds_to_data_units(&proto_data->hdr_d, layer_pdu,
                                   &tmpl_data->hdr);
    if (rc != 0)
        return rc;

    rc = tad_bps_confirm_send(&proto_data->hdr_d, &tmpl_data->hdr);
    if (rc != 0)
        return rc;

    return rc;
}


/**
 * Check length of packet as Ethernet frame. If frame is too small,
 * add a new segment with data filled in by zeros.
 *
 * This function complies with tad_pkt_enum_cb prototype.
 */
static te_errno
tad_eth_check_frame_len(tad_pkt *pkt, void *opaque)
{
    ssize_t tailer_len = (ETHER_MIN_LEN - ETHER_CRC_LEN) -
                         tad_pkt_len(pkt);

    UNUSED(opaque);
    if (tailer_len > 0)
    {
        tad_pkt_seg *seg = tad_pkt_alloc_seg(NULL, tailer_len, NULL);

        if (seg == NULL)
        {
            ERROR("%s(): Failed to allocate a new segment",
                  __FUNCTION__);
            return TE_RC(TE_TAD_PKT, TE_ENOMEM);
        }
        memset(seg->data_ptr, 0, seg->data_len);
        tad_pkt_append_seg(pkt, seg);
    }
    return 0;
}

/* See description in tad_eth_impl.h */
te_errno
tad_eth_gen_bin_cb(csap_p csap, unsigned int layer,
                   const asn_value *tmpl_pdu, void *opaque,
                   const tad_tmpl_arg_t *args, size_t arg_num, 
                   tad_pkts *sdus, tad_pkts *pdus)
{
    tad_eth_proto_data         *proto_data;
    tad_eth_proto_tmpl_data    *tmpl_data = opaque;

    te_errno    rc;
    size_t      bitlen;
    size_t      bitoff;
    uint8_t    *data;
    size_t      len;


    assert(csap != NULL);
    F_ENTRY("(%d:%u) tmpl_pdu=%p args=%p arg_num=%u sdus=%p pdus=%p",
            csap->id, layer, (void *)tmpl_pdu, (void *)args,
            (unsigned)arg_num, sdus, pdus);

    proto_data = csap_get_proto_spec_data(csap, layer);

    bitlen = tad_bps_pkt_frag_data_bitlen(&proto_data->hdr_d,
                                          &tmpl_data->hdr);
    if ((bitlen & 7) != 0)
    {
        ERROR("Unexpected lengths: total - %u bits", bitlen);
        return TE_RC(TE_TAD_CSAP, TE_EOPNOTSUPP);
    }

    len = (bitlen >> 3);
    data = malloc(len);
    if (data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    bitoff = 0;

    rc = tad_bps_pkt_frag_gen_bin(&proto_data->hdr_d, &tmpl_data->hdr,
                                  args, arg_num, data, &bitoff, bitlen);
    if (rc != 0)
    {
        ERROR("%s(): tad_bps_pkt_frag_gen_bin failed for header: %r",
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

    rc = tad_pkt_enumerate(pdus, tad_eth_check_frame_len, NULL);
    if (rc != 0)
    {
        ERROR("Failed to check length of Ethernet frames to send: %r", rc);
        return rc;
    }

    return 0;
#if 0
    eth_csap_specific_data_p spec_data; 
    csap_pkts_p pld_fragment;
    csap_pkts_p prev_frame = NULL,
                curr_frame = pkts;

    int rc = 0;
    int frame_size;
    int is_tagged;

    uint8_t *data;
    uint8_t *p;

    UNUSED(arg_num);
    UNUSED(tmpl_pdu);/* All data from tmpl_pdu analyzed in confirm */

    VERB("%s(): entered", __FUNCTION__);

    pld_fragment = up_payload;
    do {

        frame_size = pld_fragment->len + ETHER_HDR_LEN;

        F_VERB("%s(): pld_fragment len %d, frame size %d", 
               __FUNCTION__, pld_fragment->len, frame_size);

        spec_data = (eth_csap_specific_data_p)
            csap_get_proto_spec_data(csap, layer); 

        is_tagged = (spec_data->du_cfi     .du_type != TAD_DU_UNDEF && 
                     spec_data->du_priority.du_type != TAD_DU_UNDEF &&
                     spec_data->du_vlan_id .du_type != TAD_DU_UNDEF); 

        if (is_tagged)
            frame_size += ETH_TAG_EXC_LEN + ETHER_TYPE_LEN;
        
#if 0 /* for attacks tests we decided theat this check is not necessary */
        else if (frame_size > (ETHER_MAX_LEN - ETHER_CRC_LEN)) 
        { /* TODO: this check seems to be not correct, compare with interface MTU
                  should be here. */
            ERROR("too greate frame size %d", frame_size);
            return TE_RC(TE_TAD_CSAP, TE_EMSGSIZE); 
        }
#endif

        F_VERB("%s(): corrected frame size %d", 
               __FUNCTION__, frame_size);

        if ((data = malloc(frame_size)) == NULL)
        {
            return TE_RC(TE_TAD_CSAP, TE_ENOMEM); 
                   /* can't allocate memory for frame data */
        } 
        memset(data, 0, frame_size); 
        p = data;

#define PUT_BIN_DATA(c_du_field, length) \
        do {                                                                \
            rc = tad_data_unit_to_bin(&(spec_data->c_du_field),             \
                                      args, arg_num, p, length);            \
            if (rc != 0)                                                    \
            {                                                               \
                ERROR("%s(): generate " #c_du_field ", error: %r",        \
                      __FUNCTION__,  rc);                                   \
                return rc;                                                  \
            }                                                               \
            p += length;                                                    \
        } while (0) 

        PUT_BIN_DATA(du_dst_addr, ETHER_ADDR_LEN);
        PUT_BIN_DATA(du_src_addr, ETHER_ADDR_LEN); 

        if (is_tagged)
        { 
            uint8_t cfi;
            uint8_t priority;
            uint16_t vlan_id;

            F_VERB("tagged frame will be formed");

#define CALC_VLAN_PARAM(c_du_field, var) \
        do {\
            rc = tad_data_unit_to_bin(&(spec_data->c_du_field),             \
                                      args, arg_num,                        \
                                      (uint8_t *)&var, sizeof(var));        \
            if (rc != 0)                                                    \
            {                                                               \
                ERROR("%s(): generate " #c_du_field ", error: %r",          \
                      __FUNCTION__,  rc);                                   \
                return rc;                                                  \
            }                                                               \
        } while (0)

            CALC_VLAN_PARAM(du_cfi, cfi);
            CALC_VLAN_PARAM(du_priority, priority);
            CALC_VLAN_PARAM(du_vlan_id, vlan_id);

            /* put "tagged special" eth type/length */
            *((uint16_t *) p) = htons (ETH_TAGGED_TYPE_LEN); 
            p += ETHER_TYPE_LEN; 

            /* vlan_id prepared already in network byte order */
            *((uint16_t *)p) = vlan_id;

            *p |= (cfi ? 1 : 0) << 4;

            *p |= priority << 5; 

            p += ETH_TAG_EXC_LEN; 
#undef CALC_VLAN_PARAM
        }

        VERB("put eth-type");  
        if (spec_data->du_eth_type.du_type == TAD_DU_UNDEF)
        { /* ethernet type-len field is not specified neither in csap, 
             nor in template put length of payload to the frame. */
            VERB("not specified, put payload length");  
            spec_data->du_eth_type.du_type = TAD_DU_I32;
            spec_data->du_eth_type.val_i32 = pld_fragment->len;

        }
        PUT_BIN_DATA(du_eth_type, ETHER_TYPE_LEN); 
    } while (pld_fragment != NULL);

#undef PUT_BIN_DATA
#endif
    return 0;
}


/* See description in tad_eth_impl.h */
te_errno
tad_eth_confirm_ptrn_cb(csap_p csap, unsigned int layer,
                        asn_value *layer_pdu, void **p_opaque)
{
    eth_csap_specific_data_p spec_data; 

    size_t  val_len;
    int     rc;
    
    UNUSED(p_opaque);
    
    spec_data =
        &((tad_eth_proto_data *)csap_get_proto_spec_data(csap, layer))->old;

#if 1
    tad_data_unit_clear(&spec_data->du_dst_addr);
    tad_data_unit_clear(&spec_data->du_src_addr);
    tad_data_unit_clear(&spec_data->du_eth_type);
    tad_data_unit_clear(&spec_data->du_cfi);
    tad_data_unit_clear(&spec_data->du_priority);
    tad_data_unit_clear(&spec_data->du_vlan_id);
#endif
    
    /* =========== Destination MAC address ============ */

    rc = tad_data_unit_convert(layer_pdu, NDN_TAG_ETH_DST,
                               &spec_data->du_dst_addr);

    VERB("rc from DU convert dst-addr %r, du-type: %d", 
          rc, spec_data->du_dst_addr.du_type);
    if (rc)
    {
        ERROR("convert of dst addr rc %r", rc);
        return rc; 
    }
    
    if (spec_data->du_dst_addr.du_type == TAD_DU_UNDEF &&
        spec_data->local_addr != NULL)
    {
        VERB("receive, dst = local");
        rc = tad_data_unit_from_bin(spec_data->local_addr, ETHER_ADDR_LEN,
                                    &spec_data->du_dst_addr);

        if (rc == 0)
            rc = asn_write_value_field(layer_pdu, 
                                       spec_data->local_addr, 
                                       ETHER_ADDR_LEN, "dst-addr.#plain");
        if (rc)
        {
            ERROR("construct dst addr rc %r", rc);
            return rc;
        }
    }
    VERB("dst DU type %d", spec_data->du_dst_addr.du_type);

    
    /* =========== Source MAC address ============ */

    rc = tad_data_unit_convert(layer_pdu, NDN_TAG_ETH_SRC,
                               &spec_data->du_src_addr);
    VERB("rc from DU convert src-addr %x, du-type: %d", 
            rc, spec_data->du_src_addr.du_type);

    if (rc)
    {
        ERROR("convert of src addr rc %r", rc);
        return rc; 
    }

    if (spec_data->du_src_addr.du_type == TAD_DU_UNDEF &&
        spec_data->remote_addr != NULL)
    {
        VERB("receive, src = remote");
        rc = tad_data_unit_from_bin(spec_data->remote_addr, 
                                    ETHER_ADDR_LEN,
                                    &spec_data->du_src_addr);
        if (rc == 0)
            rc = asn_write_value_field(layer_pdu, 
                                       spec_data->remote_addr, 
                                       ETHER_ADDR_LEN, "src-addr.#plain");
        if (rc)
        {
            ERROR("construct src addr rc %r", rc);
            return rc;
        }
    }
    VERB("src DU type %d", spec_data->du_src_addr.du_type);

    

    /* =========== Ethernet type/length field ============ */
    rc = tad_data_unit_convert_by_label(layer_pdu, "eth-type",
                                        &spec_data->du_eth_type);
    VERB("%s(CSAP %d): rc from DU convert eth-type %x, du-type: %d", 
         __FUNCTION__, csap->id, rc, spec_data->du_eth_type.du_type); 

    if (rc)
    {
        ERROR("convert of eth type rc %r", rc);
        return rc; 
    }

    if (spec_data->du_eth_type.du_type == TAD_DU_UNDEF && 
        spec_data->eth_type > 0)
    {
        spec_data->du_eth_type.du_type = TAD_DU_I32;     
        spec_data->du_eth_type.val_i32 = spec_data->eth_type;         
        asn_write_int32(layer_pdu, spec_data->eth_type, 
                        "eth-type.#plain");
        VERB("%s(CSAP %d): chosen eth-type %d", 
             __FUNCTION__, csap->id, spec_data->eth_type); 
    }

    {
        int int_val;
        int is_cfi = 0;
        int is_prio = 0;
        int is_vlan_id = 0; 

        val_len = sizeof(int_val);

#define CHECK_VLAN_FIELD(c_du_field, label, flag) \
    do { \
        rc = tad_data_unit_convert_by_label(layer_pdu, label,   \
                                   &spec_data-> c_du_field );   \
        if (rc)                                                 \
        {                                                       \
            ERROR("convert of VLAN " label " rc %r", rc);       \
            return rc;                                          \
        }                                                       \
        F_VERB("success " label " convert; du type: %d",        \
               (int) spec_data-> c_du_field .du_type);          \
        if (spec_data-> c_du_field .du_type != TAD_DU_UNDEF)    \
            flag = 1;                                           \
    } while (0)

        /* cfi is not data unit! */
        rc = asn_read_value_field(layer_pdu, &int_val, &val_len, "cfi");
        if (rc == 0)
        {
            spec_data->du_cfi.du_type = TAD_DU_I32;
            spec_data->du_cfi.val_i32 = int_val;
            is_cfi = 1;
        }
        CHECK_VLAN_FIELD(du_priority, "priority", is_prio);
        CHECK_VLAN_FIELD(du_vlan_id,  "vlan-id",  is_vlan_id);

        if ((csap->state & TAD_STATE_SEND) && 
            (is_cfi || is_prio || is_vlan_id ||
             spec_data->cfi >= 0 || spec_data->vlan_id >= 0 || 
             spec_data->priority >= 0)
           )
        {
            F_VERB("send command, fill all fields.");
            if (!is_cfi)
            { 
                F_VERB("was not cfi, set zero");
                spec_data->du_cfi.du_type = TAD_DU_I32;
                spec_data->du_cfi.val_i32 = 
                            spec_data->cfi >= 0 ? spec_data->cfi : 0;
            }
            if (!is_prio)
            {
                F_VERB("was not priority, set zero");
                spec_data->du_priority.du_type = TAD_DU_I32;
                spec_data->du_priority.val_i32 =
                            spec_data->priority >= 0 ? spec_data->priority : 0;
            }

            if (!is_vlan_id)
            {
                F_VERB("was not vlan id, set zero");
                spec_data->du_vlan_id.du_type = TAD_DU_I32;
                spec_data->du_vlan_id.val_i32 = 
                            spec_data->vlan_id >= 0 ? spec_data->vlan_id : 0;
            }
        }
    }
#undef CHECK_VLAN_FIELD

    VERB("exit, return 0");
    
    return 0;
}


/* See description in tad_eth_impl.h */
te_errno
tad_eth_match_bin_cb(csap_p csap, unsigned int layer,
                     const asn_value *pattern_pdu,
                     const csap_pkts *pkt, csap_pkts *payload, 
                     asn_value_p parsed_packet)
{
    struct timeval moment;
    eth_csap_specific_data_p spec_data;
    int      rc;
    uint8_t *data;
    asn_value *eth_hdr_pdu = NULL;
  
    spec_data =
        &((tad_eth_proto_data *)csap_get_proto_spec_data(csap, layer))->old;
    data = pkt->data; 

    if (pattern_pdu == NULL)
    {
        VERB("pattern pdu is NULL, packet matches");
    }

    if (parsed_packet)
        eth_hdr_pdu = asn_init_value(ndn_eth_header);

    rc = ndn_match_data_units(pattern_pdu, eth_hdr_pdu, 
                              data, ETHER_ADDR_LEN, "dst-addr");
    data += ETHER_ADDR_LEN; 

    VERB("%s(CSAP %d): univ match for dst rc %x\n",
         __FUNCTION__, csap->id, rc);

    if (rc != 0)
        goto cleanup;

    /* source  */ 
    rc = ndn_match_data_units(pattern_pdu, eth_hdr_pdu, 
                              data, ETHER_ADDR_LEN, "src-addr");
    data += ETHER_ADDR_LEN;
    VERB("%s(CSAP %d): univ match for src rc %x\n",
         __FUNCTION__, csap->id, rc);

    if (rc == 0 && (ntohs(*((uint16_t *)data)) == ETH_TAGGED_TYPE_LEN))
    {
        uint8_t prio;
        uint8_t cfi;
        int32_t cfi_pattern;

        VERB("VLan info found in Ethernet frame");

        data += ETHER_TYPE_LEN; 

        prio = *data >> 5; 

        cfi = (*data >> 4) & 1;

        *data &= 0x0f; 

        rc = asn_read_int32(pattern_pdu, &cfi_pattern, "cfi"); 
        if (rc == 0)
        {
            if (cfi_pattern != cfi)
            {
                rc = TE_RC(TE_TAD_CSAP, TE_ETADNOTMATCH);
                goto cleanup;
            }
        }
        else if (TE_RC_GET_ERROR(rc) != TE_EASNINCOMPLVAL)
        {
            WARN("read cfi from pattern failed %r", rc);
            goto cleanup;
        }

        rc = ndn_match_data_units(pattern_pdu, eth_hdr_pdu, 
                                  &prio, 1, "priority");
        if (rc != 0)
        {
            WARN("match of priority failed %r", rc);
            goto cleanup;
        }
        
        rc = ndn_match_data_units(pattern_pdu, eth_hdr_pdu, 
                                  data, 2, "vlan-id");
        if (rc != 0)
        {
            WARN("match of vlan-id failed %r", rc);
            goto cleanup;
        }

        data += ETH_TAG_EXC_LEN; 
    }

    if (rc == 0)
    { 
        rc = ndn_match_data_units(pattern_pdu, eth_hdr_pdu, 
                                  data, ETHER_TYPE_LEN, "eth-type");
        VERB("%s(CSAP %d): univ match for eth-type rc %x\n",
             __FUNCTION__, csap->id, rc);
    }

    if (rc == 0 && eth_hdr_pdu)
    {
        rc = asn_write_component_value(parsed_packet, eth_hdr_pdu, "#eth"); 
        if (rc)
            ERROR("write eth header to packet rc %r", rc);
    }

    if (rc != 0)
        goto cleanup;

    /* passing payload to upper layer */
    memset(payload, 0 , sizeof(*payload));
#if 0
    /* Correction for number of read bytes was insered to synchronize 
     * with OS interface statistics, but it cause many side effects, 
     * therefore it is disabled now. */ 
    payload->len = (pkt->len - ETHER_HDR_LEN - ETH_TAILING_CHECKSUM);
#else
    payload->len = (pkt->len - ETHER_HDR_LEN);
#endif
    payload->data = malloc(payload->len);
    memcpy(payload->data, pkt->data + ETHER_HDR_LEN, payload->len); 

    gettimeofday(&moment, NULL);
    VERB("%s(CSAP %d), packet matches, pkt len %d, pld len %d, mcs %d", 
         __FUNCTION__, csap->id, pkt->len, payload->len, moment.tv_usec);

cleanup:
    asn_free_value(eth_hdr_pdu);
    
    return rc;
}
