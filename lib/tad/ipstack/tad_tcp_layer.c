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

#include "logger_ta.h"

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
    spec_data = (tcp_csap_specific_data_t *) csap_descr->layers[level].specific_data; 

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
 * SEND: pass info from Template & CSAP NDS -> DU gen-bin fields.
 * RECV: pass info from CSAP NDS & csap_spec_data -> traffic PDU
 *
 * @param csap_id       identifier of CSAP
 * @param layer         numeric index of layer in CSAP type to be processed.
 * @param pdu           asn_value with traffic PDU (IN/OUT)
 *
 * @return zero on success or error code.
 */ 
int 
tcp_confirm_pdu_cb(int csap_id, int layer, asn_value_p pdu)
{ 
    int rc = 0;
    csap_p csap_descr = csap_find(csap_id);

    const asn_value *tcp_csap_pdu;
    const asn_value *du_field;
    asn_value       *tcp_pdu;

    tcp_csap_specific_data_t *spec_data = 
        (tcp_csap_specific_data_t *)csap_descr->layers[layer].specific_data;

    if (asn_get_syntax(pdu, "") == CHOICE)
    {
        if ((rc = asn_get_choice_value(pdu,
                                       (const asn_value **)&tcp_pdu,
                                       NULL, NULL))
             != 0)
            return rc;
    }
    else
        tcp_pdu = pdu; 

    tcp_csap_pdu = csap_descr->layers[layer].csap_layer_pdu; 

#define CONVERT_FIELD(tag_, du_field_)                                  \
    do {                                                                \
        rc = tad_data_unit_convert(tcp_pdu, tag_,                       \
                                   &(spec_data->du_field_));            \
        if (rc != 0)                                                    \
        {                                                               \
            ERROR("%s(csap %d),line %d, convert %s failed,rc 0x%X",     \
                  __FUNCTION__, csap_id, __LINE__, #du_field_, rc);     \
            return rc;                                                  \
        }                                                               \
    } while (0) 

    CONVERT_FIELD(NDN_TAG_TCP_SRC_PORT, du_src_port); 
    if (spec_data->du_src_port.du_type == TAD_DU_UNDEF)
    {
        if (csap_descr->state & TAD_STATE_SEND)
        {
            rc = tad_data_unit_convert(tcp_csap_pdu,
                                       NDN_TAG_TCP_LOCAL_PORT,
                                       &(spec_data->du_src_port));
            if (rc != 0)
            {          
                ERROR("%s(csap %d), local_port to src failed, rc 0x%X",
                      __FUNCTION__, csap_id, rc);
                return rc;
            }
        }
        else
        {
            if (asn_get_child_value(tcp_csap_pdu, &du_field, PRIVATE,
                                    NDN_TAG_TCP_REMOTE_PORT) == 0 &&
                (rc = asn_write_component_value(tcp_pdu, du_field,
                                               "src-port")) != 0)
            {
                ERROR("%s(): write src-port to tcp pdu failed 0x%X",
                      __FUNCTION__, rc);
                return TE_RC(TE_TAD_CSAP, rc);
            }
        }
    }

    CONVERT_FIELD(NDN_TAG_TCP_DST_PORT, du_dst_port);
    if (spec_data->du_dst_port.du_type == TAD_DU_UNDEF)
    {
        if (csap_descr->state & TAD_STATE_SEND)
        {
            rc = tad_data_unit_convert(tcp_csap_pdu,
                                       NDN_TAG_TCP_REMOTE_PORT,
                                       &(spec_data->du_dst_port));
            if (rc != 0)
            {          
                ERROR("%s(csap %d), remote port to dst failed, rc 0x%X",
                      __FUNCTION__, csap_id, rc);
                return rc;
            }
        }
        else
        {
            if (asn_get_child_value(tcp_csap_pdu, &du_field, PRIVATE,
                                    NDN_TAG_TCP_LOCAL_PORT) == 0 &&
                (rc = asn_write_component_value(tcp_pdu, du_field,
                                               "dst-port")) != 0)
            {
                ERROR("%s(): write dst-port to tcp pdu failed 0x%X",
                      __FUNCTION__, rc);
                return TE_RC(TE_TAD_CSAP, rc);
            }
        }
    }

    CONVERT_FIELD(NDN_TAG_TCP_SEQN, du_seqn);
    CONVERT_FIELD(NDN_TAG_TCP_ACKN, du_ackn);
    CONVERT_FIELD(NDN_TAG_TCP_HLEN, du_hlen);
    CONVERT_FIELD(NDN_TAG_TCP_FLAGS, du_flags);
    CONVERT_FIELD(NDN_TAG_TCP_WINDOW, du_win_size);
    CONVERT_FIELD(NDN_TAG_TCP_CHECKSUM, du_checksum);
    CONVERT_FIELD(NDN_TAG_TCP_URG, du_urg_p);
#undef CONVERT_FIELD 
    return 0;
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
    size_t      len;
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
 *                      fragmentation sometimes may occur. (OUT)
 *
 * @return zero on success or error code.
 */ 
int 
tcp_gen_bin_cb(csap_p csap_descr, int layer, const asn_value *tmpl_pdu,
               const tad_tmpl_arg_t *args, size_t arg_num, 
               csap_pkts_p up_payload, csap_pkts_p pkt_list)
{
    int rc = 0;
    unsigned char *p; 

    int hdr_len = 20;
    int pld_len = (up_payload == NULL) ? 0 : up_payload->len;

    tcp_csap_specific_data_t *spec_data = NULL;

    /* TODO: TCP options */

    if (csap_descr == NULL)
        return TE_RC(TE_TAD_CSAP, ETADCSAPNOTEX);

    if (pkt_list == NULL || tmpl_pdu == NULL)
        return TE_RC(TE_TAD_CSAP, ETEWRONGPTR);

    if (csap_descr->type == TAD_CSAP_DATA) /* TODO */
        return ETENOSUPP; 

    asn_save_to_file(tmpl_pdu, "/tmp/tcp­tmpl.asn");

    spec_data = (tcp_csap_specific_data_t *)
                csap_descr->layers[layer].specific_data; 


    pkt_list->len = hdr_len + pld_len; 
    pkt_list->data = malloc(pkt_list->len);
    pkt_list->next = NULL; 
    p = pkt_list->data;

#define CHECK_ERROR(fail_cond_, error_, msg_...) \
    do {                                \
        if (fail_cond_)                 \
        {                               \
            ERROR(msg_);                \
            rc = (error_);              \
            goto cleanup;               \
        }                               \
    } while (0)

#define PUT_BIN_DATA(c_du_field_, def_val_, length_) \
    do {                                                                \
        if (spec_data->c_du_field_.du_type != TAD_DU_UNDEF)             \
        {                                                               \
            rc = tad_data_unit_to_bin(&(spec_data->c_du_field_),        \
                                      args, arg_num, p, length_);       \
            CHECK_ERROR(rc != 0, rc,                                    \
                        "%s():%d: " #c_du_field_ " error: 0x%x",        \
                        __FUNCTION__,  __LINE__, rc);                   \
        }                                                               \
        else                                                            \
        {                                                               \
            switch (length_)                                            \
            {                                                           \
                case 1:                                                 \
                    *((uint8_t *)p) = (uint8_t)(def_val_);              \
                    break;                                              \
                                                                        \
                case 2:                                                 \
                    *((uint16_t *)p) = htons((uint16_t)(def_val_));     \
                    break;                                              \
                                                                        \
                case 4:                                                 \
                    *((uint32_t *)p) = htonl((uint32_t)(def_val_));     \
                    break;                                              \
            }                                                           \
        }                                                               \
        p += (length_);                                                 \
    } while (0) 


    CHECK_ERROR(spec_data->du_src_port.du_type == TAD_DU_UNDEF && 
                spec_data->local_port == 0,
                ETADLESSDATA, 
                "%s(): CSAP %d, no source port specified",
                __FUNCTION__, csap_descr->id);
    PUT_BIN_DATA(du_src_port, spec_data->local_port, 2);

    CHECK_ERROR(spec_data->du_dst_port.du_type == TAD_DU_UNDEF && 
                spec_data->remote_port == 0,
                ETADLESSDATA, 
                "%s(): CSAP %d, no destination port specified",
                __FUNCTION__, csap_descr->id);
    PUT_BIN_DATA(du_dst_port, spec_data->remote_port, 2);

    CHECK_ERROR(spec_data->du_seqn.du_type == TAD_DU_UNDEF,
                ETADLESSDATA, 
                "%s(): CSAP %d, no sequence number specified",
                __FUNCTION__, csap_descr->id); 
    rc = tad_data_unit_to_bin(&(spec_data->du_seqn),
                              args, arg_num, p, 4);
    CHECK_ERROR(rc != 0, rc, "%s():%d: seqn error: 0x%x",
                __FUNCTION__,  __LINE__, rc);
    p += 4;

    PUT_BIN_DATA(du_ackn, 0, 4);
    if (spec_data->du_hlen.du_type != TAD_DU_UNDEF)
    {
        WARN("%s(CSAP %d): hlen field is ignored required in NDS",
              __FUNCTION__, csap_descr->id);
    }
    *p = (hdr_len / 4) << 4;
    p++;
    PUT_BIN_DATA(du_flags, 0, 1);

    VERB("du flags type %d, value %d, put value %d",
         spec_data->du_flags.du_type, 
         spec_data->du_flags.val_i32,
         (int)(*(p-1)));

    PUT_BIN_DATA(du_win_size, 1400, 2); /* TODO: good default window */
    PUT_BIN_DATA(du_checksum, 0, 2);
    PUT_BIN_DATA(du_urg_p, 0, 2); 

    if (pld_len > 0 && up_payload != NULL)
    {
        memcpy(p, up_payload->data, pld_len);
    }

    return 0;
#undef PUT_BIN_DATA
cleanup:
    {
        csap_pkts_p pkt_fr;
        for (pkt_fr = pkt_list; pkt_fr != NULL; pkt_fr = pkt_fr->next)
        {
            free(pkt_fr->data);
            pkt_fr->data = NULL; pkt_fr->len = 0;
        }
    }
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
int tcp_match_bin_cb(int csap_id, int layer, const asn_value *pattern_pdu,
                     const csap_pkts *pkt, csap_pkts *payload, 
                     asn_value_p parsed_packet)
{ 
    csap_p                    csap_descr;
    tcp_csap_specific_data_t *spec_data;

    uint8_t *data;
    uint8_t  tmp8;
    int      rc;
    size_t   h_len = 0;

    asn_value *tcp_header_pdu = NULL;

    if ((csap_descr = csap_find(csap_id)) == NULL)
    {
        ERROR("null csap_descr for csap id %d", csap_id);
        return ETADCSAPNOTEX;
    } 

    if (parsed_packet)
        tcp_header_pdu = asn_init_value(ndn_tcp_header);

    spec_data = (tcp_csap_specific_data_t*)csap_descr->layers[layer].specific_data; 

    data = pkt->data; 

#define CHECK_FIELD(_asn_label, _size) \
    do {                                                        \
        rc = ndn_match_data_units(pattern_pdu, tcp_header_pdu,  \
                                  data, _size, _asn_label);     \
        if (rc)                                                 \
        {                                                       \
            F_VERB("%s: field %s not match, rc %X",             \
                    __FUNCTION__, _asn_label, rc);              \
            goto cleanup;                                       \
        }                                                       \
        data += _size;                                          \
    } while(0)

    CHECK_FIELD("src-port", 2);
    CHECK_FIELD("dst-port", 2);
    CHECK_FIELD("seqn", 4);
    CHECK_FIELD("ackn", 4);

    h_len = tmp8 = (*data) >> 4;
    rc = ndn_match_data_units(pattern_pdu, tcp_header_pdu, 
                              &tmp8, 1, "hlen");
    if (rc)
        goto cleanup;
    data ++;

    tmp8 = (*data) & 0x3f;
    rc = ndn_match_data_units(pattern_pdu, tcp_header_pdu, 
                              &tmp8, 1, "flags");
    if (rc)
        goto cleanup;
    data++;

    CHECK_FIELD("win-size", 2); 
    CHECK_FIELD("checksum", 2); 
    CHECK_FIELD("urg-p", 2);
 
#undef CHECK_FIELD

    /* TODO: Process TCP options */

    /* passing payload to upper layer */ 
    memset(payload, 0 , sizeof(*payload));
    payload->len = pkt->len - (h_len * 4);
    payload->data = malloc (payload->len);
    memcpy(payload->data, pkt->data + (h_len * 4), payload->len);

    if (parsed_packet)
    {
        rc = asn_write_component_value(parsed_packet, tcp_header_pdu,
                                       "#tcp"); 
        if (rc)
            ERROR("%s, write TCP header to packet fails %X\n", 
                  __FUNCTION__, rc);
    } 

cleanup:
    asn_free_value(tcp_header_pdu);

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


