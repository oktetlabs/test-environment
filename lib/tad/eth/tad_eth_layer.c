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
 * Author: Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * @(#) $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <fcntl.h>

#define TE_LGR_USER     "TAD Ethernet"
#include "logger_ta.h"

#include "tad_eth_impl.h"
#include "te_defs.h"

/**
 * Callback for read parameter value of ethernet CSAP.
 *
 * @param csap_id       identifier of CSAP.
 * @param level         Index of level in CSAP stack, which param is wanted.
 * @param param         Protocol-specific name of parameter.
 *
 * @return 
 *     String with textual presentation of parameter value, or NULL 
 *     if error occured. User have to free memory at returned pointer.
 */ 
char* eth_get_param_cb (csap_p csap_descr, int level, const char *param)
{
    char   *par_buffer;            
    eth_csap_specific_data_p spec_data;

    if (csap_descr == NULL)
    {
        VERB(
                   "error in eth_get_param %s: wrong csap descr passed\n", param);
        return NULL;
    }

    spec_data = (eth_csap_specific_data_p)
            csap_descr->layers[level].specific_data; 

    if (strcmp (param, "total_bytes") == 0)
    {
        par_buffer  = malloc(20);
        sprintf(par_buffer, "%u", (uint32_t)spec_data->total_bytes);
        return par_buffer;
    }

    VERB(
            "error in eth_get_param %s: not supported parameter\n", param);

    return NULL;
}

/**
 * Callback for confirm PDU with ethernet CSAP parameters and possibilities.
 *
 * @param csap_id       identifier of CSAP
 * @param layer         numeric index of layer in CSAP type to be processed.
 * @param tmpl_pdu      asn_value with PDU (IN/OUT)
 *
 * @return zero on success or error code.
 */ 
int eth_confirm_pdu_cb (int csap_id, int layer, asn_value_p tmpl_pdu)
{
    csap_p                   csap_descr;
    eth_csap_specific_data_p spec_data; 

    size_t  val_len;
    int     rc; 
    
    
    if ((csap_descr = csap_find(csap_id)) == NULL)
    {
        ERROR("null csap_descr for csap id %d", csap_id);
        return ETADCSAPNOTEX;
    }
    
    spec_data = (eth_csap_specific_data_p)
        csap_descr->layers[layer].specific_data; 
    
    /* =========== Destination MAC address ============ */

    rc = tad_data_unit_convert(tmpl_pdu, NDN_TAG_ETH_DST,
                               &spec_data->du_dst_addr);

    VERB("rc from DU convert dst-addr %x, du-type: %d", 
            rc, spec_data->du_dst_addr.du_type);
    if (rc)
    {
        ERROR("convert of dst addr rc %x", rc);
        return rc; 
    }
    
    if (spec_data->du_dst_addr.du_type == TAD_DU_UNDEF)
    {
        if (csap_descr->command == TAD_OP_RECV)
        {
            if (spec_data->local_addr != NULL)
            {
                VERB("receive, dst = local");
                rc = tad_data_unit_from_bin(spec_data->local_addr, ETH_ALEN,
                                            &spec_data->du_dst_addr);

                if (rc == 0)
                    rc = asn_write_value_field(tmpl_pdu, 
                                               spec_data->local_addr, 
                                               ETH_ALEN, "dst-addr.#plain");
                if (rc)
                {
                    ERROR("construct dst addr rc %x", rc);
                    return rc;
                }
            }
        }
        else
        { 
            if (spec_data->remote_addr != NULL)
            {
                VERB("sending, dst = remote");
                rc = tad_data_unit_from_bin(spec_data->remote_addr, ETH_ALEN, 
                                           &spec_data->du_dst_addr);
                if (rc == 0)
                    rc = asn_write_value_field(tmpl_pdu, 
                                               spec_data->remote_addr, 
                                               ETH_ALEN, "dst-addr.#plain");
            }
            else
            {
                ERROR("sending csap, no remote address found, ret EINVAL.");
                return EINVAL; /* NO DESTINATION ADDRESS IS SPECIFIED */
            }                  
        }
    }
    VERB("dst DU type %d", spec_data->du_dst_addr.du_type);

    
    /* =========== Source MAC address ============ */

    rc = tad_data_unit_convert(tmpl_pdu, NDN_TAG_ETH_SRC,
                               &spec_data->du_src_addr);
    VERB("rc from DU convert src-addr %x, du-type: %d", 
            rc, spec_data->du_src_addr.du_type);

    if (rc)
    {
        ERROR("convert of src addr rc %x", rc);
        return rc; 
    }

    if (spec_data->du_src_addr.du_type == TAD_DU_UNDEF)
    {
        if (csap_descr->command == TAD_OP_RECV)
        {
            if (spec_data->remote_addr != NULL)
            {
                VERB("receive, src = remote");
                rc = tad_data_unit_from_bin(spec_data->remote_addr, 
                                            ETH_ALEN,
                                            &spec_data->du_src_addr);
                if (rc == 0)
                    rc = asn_write_value_field(tmpl_pdu, 
                                               spec_data->remote_addr, 
                                               ETH_ALEN, "src-addr.#plain");
            }
        }
        else
        {
            spec_data->du_src_addr.du_type = TAD_DU_EXPR;
            uint8_t *local_addr;
            if (spec_data->local_addr != NULL)
            {
                VERB("sending, src = local/csap");
                local_addr = spec_data->local_addr;
            }
            else
            {
                VERB("sending, src = local/iface");
                local_addr = spec_data->interface->local_addr;
            }                  
            rc = tad_data_unit_from_bin(local_addr, ETH_ALEN, 
                                       &spec_data->du_src_addr);
            if (rc == 0)
                rc = asn_write_value_field(tmpl_pdu, local_addr, 
                                           ETH_ALEN, "src-addr.#plain");
        }
        if (rc)
        {
            ERROR("construct src addr rc %x", rc);
            return rc;
        }
    }
    VERB("src DU type %d", spec_data->du_src_addr.du_type);

    

    /* =========== Ethernet type/length field ============ */
    rc = tad_data_unit_convert_by_label(tmpl_pdu, "eth-type",
                                        &spec_data->du_eth_type);
    VERB("%s(CSAP %d): rc from DU convert eth-type %x, du-type: %d", 
         __FUNCTION__, csap_id, rc, spec_data->du_eth_type.du_type); 

    if (rc)
    {
        ERROR("convert of eth type rc %x", rc);
        return rc; 
    }

    if (spec_data->du_eth_type.du_type == TAD_DU_UNDEF && 
        spec_data->eth_type > 0)
    {
        spec_data->du_eth_type.du_type = TAD_DU_I32;     
        spec_data->du_eth_type.val_i32 = spec_data->eth_type;         
        asn_write_value_field(tmpl_pdu, &(spec_data->eth_type), 
                              sizeof(spec_data->eth_type), 
                              "eth-type.#plain");
        VERB("%s(CSAP %d): chosen eth-type %d", 
             __FUNCTION__, csap_id, spec_data->eth_type); 
    }

    {
        int int_val;
        int is_cfi = 0;
        int is_prio = 0;
        int is_vlan_id = 0; 

        val_len = sizeof(int_val);

#define CHECK_VLAN_FIELD(c_du_field, label, flag) \
    do { \
        rc = tad_data_unit_convert_by_label(tmpl_pdu, label,    \
                                   &spec_data-> c_du_field );   \
        if (rc)                                                 \
        {                                                       \
            ERROR(\
                    "convert of VLAN " label " rc %x", rc);     \
            return rc;                                          \
        }                                                       \
        F_VERB(\
                    "success " label " convert; du type: %d",   \
                    (int) spec_data-> c_du_field .du_type);     \
        if (spec_data-> c_du_field .du_type != TAD_DU_UNDEF)   \
            flag = 1;                                           \
    } while (0)

        /* cfi is not data unit! */
        rc = asn_read_value_field(tmpl_pdu, &int_val, &val_len, "cfi");
        if (rc == 0)
        {
            spec_data->du_cfi.du_type = TAD_DU_I32;
            spec_data->du_cfi.val_i32 = int_val;
            is_cfi = 1;
        }
        CHECK_VLAN_FIELD (du_priority,  "priority", is_prio);
        CHECK_VLAN_FIELD (du_vlan_id,   "vlan-id",  is_vlan_id);

        if ((csap_descr->state & TAD_STATE_SEND) && 
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
int eth_gen_bin_cb(csap_p csap_descr, int layer, const asn_value *tmpl_pdu,
                   const tad_tmpl_arg_t *args, size_t arg_num, 
                   const csap_pkts_p up_payload, csap_pkts_p pkts)
{
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

    if (csap_descr == NULL)
    {
        ERROR("%s(): null csap_descr passed %d", __FUNCTION__);
        return ETADCSAPNOTEX;
    }

    if (up_payload == NULL)
        return ETADWRONGNDS;

    pld_fragment = up_payload;
    do {

        frame_size = pld_fragment->len + ETH_HLEN;

        F_VERB("%s(): pld_fragment len %d, frame size %d", 
               __FUNCTION__, pld_fragment->len, frame_size);

        spec_data = (eth_csap_specific_data_p)
            csap_descr->layers[layer].specific_data; 

        is_tagged = (spec_data->du_cfi     .du_type != TAD_DU_UNDEF && 
                     spec_data->du_priority.du_type != TAD_DU_UNDEF &&
                     spec_data->du_vlan_id .du_type != TAD_DU_UNDEF); 

        if (is_tagged)
            frame_size += ETH_TAG_EXC_LEN + ETH_TYPE_LEN;
        
        if (frame_size < ETH_ZLEN)
        {
            frame_size = ETH_ZLEN;
        }
#if 0 /* for attacks tests we decided theat this check is not necessary */
        else if (frame_size > ETH_FRAME_LEN) 
        { /* TODO: this check seems to be not correct, compare with interface MTU
                  should be here. */
            ERROR("too greate frame size %d", frame_size);
            return EMSGSIZE; 
        }
#endif

        F_VERB("%s(): corrected frame size %d", 
               __FUNCTION__, frame_size);

        if ((data = malloc(frame_size)) == NULL)
        {
            return ENOMEM; /* can't allocate memory for frame data */
        } 
        memset(data, 0, frame_size); 
        p = data;

#define PUT_BIN_DATA(c_du_field, length) \
        do {                                                                \
            rc = tad_data_unit_to_bin(&(spec_data->c_du_field),             \
                                      args, arg_num, p, length);            \
            if (rc != 0)                                                    \
            {                                                               \
                ERROR("%s(): generate " #c_du_field ", error: 0x%x",        \
                      __FUNCTION__,  rc);                                   \
                return rc;                                                  \
            }                                                               \
            p += length;                                                    \
        } while (0) 

        PUT_BIN_DATA(du_dst_addr, ETH_ALEN);
        PUT_BIN_DATA(du_src_addr, ETH_ALEN); 

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
                ERROR("%s(): generate " #c_du_field ", error: 0x%x",        \
                      __FUNCTION__,  rc);                                   \
                return rc;                                                  \
            }                                                               \
        } while (0)

            CALC_VLAN_PARAM(du_cfi, cfi);
            CALC_VLAN_PARAM(du_priority, priority);
            CALC_VLAN_PARAM(du_vlan_id, vlan_id);

            /* put "tagged special" eth type/length */
            *((uint16_t *) p) = htons (ETH_TAGGED_TYPE_LEN); 
            p += ETH_TYPE_LEN; 

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
        PUT_BIN_DATA(du_eth_type, ETH_TYPE_LEN); 

        memcpy (p, pld_fragment->data, pld_fragment->len);

        if (prev_frame != NULL)
        {
            curr_frame = prev_frame->next = malloc(sizeof(*curr_frame));
        }

        curr_frame->data = data;
        curr_frame->len  = frame_size;
        curr_frame->next = NULL; 
        curr_frame->free_data_cb = NULL; 

        pld_fragment = pld_fragment->next;
        prev_frame = curr_frame; 
    } while (pld_fragment != NULL);

#if 0
    if (up_payload->free_data_cb)
        up_payload->free_data_cb(up_payload->data);
    else
        free(up_payload->data);

    up_payload->data = NULL;
    up_payload->len = 0;
#endif

#undef PUT_BIN_DATA

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
 * @param parsed_packet caller of method should pass here empty asn_value 
 *                      instance of ASN type 'Generic-PDU'. Callback 
 *                      have to fill this instance with values from 
 *                      parsed and matched packet
 *
 * @return zero on success or error code.
 */
int 
eth_match_bin_cb(int csap_id, int layer, const asn_value *pattern_pdu,
                 const csap_pkts *pkt, csap_pkts *payload, 
                 asn_value_p parsed_packet)
{
    struct timeval moment;
    csap_p                   csap_descr;
    eth_csap_specific_data_p spec_data;
    int      rc;
    uint8_t *data;
    asn_value *eth_hdr_pdu = NULL;
  
    if ((csap_descr = csap_find(csap_id)) == NULL)
    {
        ERROR("null csap_descr for csap id %d", csap_id);
        return ETADCSAPNOTEX;
    }

    spec_data = (eth_csap_specific_data_p)
        csap_descr->layers[layer].specific_data;
    data = pkt->data; 

#if 0
    {
        char buf[50], *p = buf;
        int i;
        gettimeofday(&moment, NULL);

        for (i = 0; i < 14; i++)
            p += sprintf (p, "%02x ", data[i]);

            
        VERB("%s(): CSAP %d got data: %s, mcs %d",
             __FUNCTION__, csap_id, buf, moment.tv_usec);
    } 
#endif

    if (pattern_pdu == NULL)
    {
        VERB("pattern pdu is NULL, packet matches");
    }

    if (parsed_packet)
        eth_hdr_pdu = asn_init_value(ndn_eth_header);

#if 0
    VERB("come data: %tm6.1\n", data,  ETH_ALEN);
#endif

    rc = ndn_match_data_units(pattern_pdu, eth_hdr_pdu, 
                              data, ETH_ALEN, "dst-addr");
    data += ETH_ALEN; 

    VERB("%s(CSAP %d): univ match for dst rc %x\n",
         __FUNCTION__, csap_id, rc);

    if (rc != 0)
        goto cleanup;

    /* source  */ 
    rc = ndn_match_data_units(pattern_pdu, eth_hdr_pdu, 
                              data, ETH_ALEN, "src-addr");
    data += ETH_ALEN;
    VERB("%s(CSAP %d): univ match for src rc %x\n",
         __FUNCTION__, csap_id, rc);

    if (rc == 0 && (ntohs(*((uint16_t *)data)) == ETH_TAGGED_TYPE_LEN))
    {
        uint8_t prio;
        uint8_t cfi;
        int32_t cfi_pattern;

        VERB("VLan info found in Ethernet frame");

        data += ETH_TYPE_LEN; 

        prio = *data >> 5; 

        cfi = (*data >> 4) & 1;

        *data &= 0x0f; 

        rc = asn_read_int32(pattern_pdu, &cfi_pattern, "cfi"); 
        if (rc == 0)
        {
            if (cfi_pattern != cfi)
            {
                rc = ETADNOTMATCH;
                goto cleanup;
            }
        }
        else if (rc != EASNINCOMPLVAL)
        {
            WARN("read cfi from pattern failed %X", rc);
            goto cleanup;
        }

        rc = ndn_match_data_units(pattern_pdu, eth_hdr_pdu, 
                                  &prio, 1, "priority");
        if (rc != 0)
        {
            WARN("match of priority failed %X", rc);
            goto cleanup;
        }
        
        rc = ndn_match_data_units(pattern_pdu, eth_hdr_pdu, 
                                  data, 2, "vlan-id");
        if (rc != 0)
        {
            WARN("match of vlan-id failed %X", rc);
            goto cleanup;
        }

        data += ETH_TAG_EXC_LEN; 
    }

    if (rc == 0)
    { 
        rc = ndn_match_data_units(pattern_pdu, eth_hdr_pdu, 
                                  data, ETH_TYPE_LEN, "eth-type");
        VERB("%s(CSAP %d): univ match for eth-type rc %x\n",
             __FUNCTION__, csap_id, rc);
    }

    if (rc == 0 && eth_hdr_pdu)
    {
        rc = asn_write_component_value(parsed_packet, eth_hdr_pdu, "#eth"); 
        if (rc)
            ERROR("write eth header to packet rc %x\n", rc);
    }

    if (rc != 0)
        goto cleanup;

    /* passing payload to upper layer */
    memset(payload, 0 , sizeof(*payload));
#if 0
    /* Correction for number of read bytes was insered to synchronize 
     * with OS interface statistics, but it cause many side effects, 
     * therefore it is disabled now. */ 
    payload->len = (pkt->len - ETH_HLEN - ETH_TAILING_CHECKSUM);
#else
    payload->len = (pkt->len - ETH_HLEN);
#endif
    payload->data = malloc(payload->len);
    memcpy(payload->data, pkt->data + ETH_HLEN, payload->len); 

    gettimeofday(&moment, NULL);
    VERB("%s(CSAP %d), packet matches, pkt len %d, pld len %d, mcs %d", 
         __FUNCTION__, csap_id, pkt->len, payload->len, moment.tv_usec);

cleanup:
    asn_free_value(eth_hdr_pdu);
    
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
int eth_gen_pattern_cb (int csap_id, int layer, const asn_value *tmpl_pdu, 
                                         asn_value_p   *pattern_pdu)
{ 
    UNUSED(csap_id); 
    UNUSED(layer);
    UNUSED(tmpl_pdu);

    if (pattern_pdu)
        *pattern_pdu = NULL;

    return ETENOSUPP;
}


