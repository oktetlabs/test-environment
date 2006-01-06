/** @file
 * @brief TAD IP Stack
 *
 * Traffic Application Domain Command Handler.
 * TCP CSAP layer-related callbacks.
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

#define TE_LGR_USER "TAD TCP" 

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include "logger_api.h"
#include "logger_ta_fast.h"

#include "tad_ipstack_impl.h"


/* See description tad_ipstack_impl.h */
te_errno
tad_tcp_init_cb(csap_p csap, unsigned int layer)
{
    tcp_csap_specific_data_t *spec_data; 
    const asn_value          *tcp_pdu;

    int32_t value_in_pdu;
    int     rc = 0;

    spec_data = calloc(1, sizeof(*spec_data));
    if (spec_data == NULL)
        return TE_ENOMEM;

    csap_set_proto_spec_data(csap, layer, spec_data);

    tcp_pdu = csap->layers[layer].csap_layer_pdu;

    /*
     * Set local port
     */
    rc = ndn_du_read_plain_int(tcp_pdu, NDN_TAG_TCP_LOCAL_PORT,
                               &value_in_pdu);
    if (rc == 0)
    {
        VERB("%s(): set TCP CSAP %d default local port to %d", 
             __FUNCTION__, csap->id, value_in_pdu);
        spec_data->local_port = value_in_pdu;
    }
    else if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
    {
        VERB("%s(): set TCP CSAP %d default local port to zero", 
             __FUNCTION__, csap->id);
        spec_data->local_port = 0;
    }
    else if (TE_RC_GET_ERROR(rc) == TE_EASNOTHERCHOICE)
    {
        ERROR("%s(): TCP CSAP %d, non-plain local port not supported",
              __FUNCTION__, csap->id);
        return TE_EOPNOTSUPP;
    }
    else
        return rc;

    /*
     * Set remote port
     */
    rc = ndn_du_read_plain_int(tcp_pdu, NDN_TAG_TCP_REMOTE_PORT,
                               &value_in_pdu);
    if (rc == 0)
    {
        VERB("%s(): set TCP CSAP %d default remote port to %d", 
             __FUNCTION__, csap->id, value_in_pdu);
        spec_data->remote_port = value_in_pdu;
    }
    else if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
    {
        VERB("%s(): set TCP CSAP %d default remote port to zero", 
             __FUNCTION__, csap->id);
        spec_data->remote_port = 0;
    }
    else if (TE_RC_GET_ERROR(rc) == TE_EASNOTHERCHOICE)
    {
        ERROR("%s(): TCP CSAP %d, non-plain remote port not supported",
              __FUNCTION__, csap->id);
        return TE_EOPNOTSUPP;
    }
    else
        return rc;

    return 0;
}

/* See description tad_ipstack_impl.h */
te_errno 
tad_tcp_destroy_cb(csap_p csap, unsigned int layer)
{
    UNUSED(csap);
    UNUSED(layer);
    return 0;
}


/* See description in tad_ipstack_impl.h */
char *
tad_tcp_get_param_cb(csap_p csap, unsigned int layer, const char *param)
{
    tcp_csap_specific_data_t *spec_data = 
        csap_get_proto_spec_data(csap, layer);

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


/* See description in tad_ipstack_impl.h */
te_errno
tad_tcp_confirm_pdu_cb(csap_p csap, unsigned int layer,
                       asn_value_p layer_pdu, void **p_opaque)
{ 
    te_errno    rc = 0;

    const asn_value *tcp_csap_pdu;
    const asn_value *du_field;
    asn_value       *tcp_pdu;

    tcp_csap_specific_data_t *spec_data = 
        csap_get_proto_spec_data(csap, layer);

    UNUSED(p_opaque);

    if (asn_get_syntax(layer_pdu, "") == CHOICE)
    {
        if ((rc = asn_get_choice_value(layer_pdu,
                                       (const asn_value **)&tcp_pdu,
                                       NULL, NULL))
             != 0)
            return rc;
    }
    else
        tcp_pdu = layer_pdu; 

    tcp_csap_pdu = csap->layers[layer].csap_layer_pdu; 

#define CONVERT_FIELD(tag_, du_field_)                                  \
    do {                                                                \
        rc = tad_data_unit_convert(tcp_pdu, tag_,                       \
                                   &(spec_data->du_field_));            \
        if (rc != 0)                                                    \
        {                                                               \
            ERROR("%s(csap %d),line %d, convert %s failed, rc %r",      \
                  __FUNCTION__, csap->id, __LINE__, #du_field_, rc);\
            return rc;                                                  \
        }                                                               \
    } while (0) 

    CONVERT_FIELD(NDN_TAG_TCP_SRC_PORT, du_src_port); 
    if (spec_data->du_src_port.du_type == TAD_DU_UNDEF)
    {
        if (csap->state & TAD_STATE_SEND)
        {
            rc = tad_data_unit_convert(tcp_csap_pdu,
                                       NDN_TAG_TCP_LOCAL_PORT,
                                       &(spec_data->du_src_port));
            if (rc != 0)
            {          
                ERROR("%s(csap %d), local_port to src failed, rc %r",
                      __FUNCTION__, csap->id, rc);
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
                ERROR("%s(): write src-port to tcp layer_pdu failed %r",
                      __FUNCTION__, rc);
                return TE_RC(TE_TAD_CSAP, rc);
            }
        }
    }

    CONVERT_FIELD(NDN_TAG_TCP_DST_PORT, du_dst_port);
    if (spec_data->du_dst_port.du_type == TAD_DU_UNDEF)
    {
        if (csap->state & TAD_STATE_SEND)
        {
            rc = tad_data_unit_convert(tcp_csap_pdu,
                                       NDN_TAG_TCP_REMOTE_PORT,
                                       &(spec_data->du_dst_port));
            if (rc != 0)
            {          
                ERROR("%s(csap %d), remote port to dst failed, rc %r",
                      __FUNCTION__, csap->id, rc);
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
                ERROR("%s(): write dst-port to tcp layer_pdu failed %r",
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
tcp_calculate_options_data(asn_value_p options)
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
        if ((rc = asn_read_value_field(opt, &opt_type, &len, 
                                       "type.#plain")) != 0)
            return rc;
        memcpy(buf, &opt_type, len);
        buf += len;
        /* Options 255 and 0 don't have length and value parts */
        if (opt_type == 255 || opt_type == 0)
            continue;

        len = 1;
        if ((rc = asn_read_value_field(opt, buf, &len, 
                                       "length.#plain")) != 0)
            return rc;
        buf += len;

        if (asn_get_length(opt, "options") > 0)
        {
            asn_value_p sub_opts;

            if ((rc = asn_read_component_value(opt, &sub_opts, 
                                               "options")) != 0)
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


/** Opaque data for tad_tcp_fill_in_hdr() callback */
typedef struct tad_tcp_fill_in_hdr_data {
    tcp_csap_specific_data_t   *spec_data;
    const tad_tmpl_arg_t       *args;
    unsigned int                arg_num;
} tad_tcp_fill_in_hdr_data;

/**
 * Callback function to fill in TCP header.
 *
 * This function complies with tad_pkt_seg_enum_cb prototype.
 */
static te_errno
tad_tcp_fill_in_hdr(const tad_pkt *pkt, tad_pkt_seg *seg,
                    unsigned int seg_num, void *opaque)
{
    tad_tcp_fill_in_hdr_data   *data = opaque;
    uint8_t                    *p = seg->data_ptr;
    te_errno                    rc;

    int hdr_len = 20;

    UNUSED(pkt);
    UNUSED(seg_num);
    assert(seg->data_len == 20);

#define CHECK_ERROR(fail_cond_, error_, msg_...) \
    do {                                     \
        if (fail_cond_)                      \
        {                                    \
            ERROR(msg_);                     \
            rc = TE_RC(TE_TAD_CSAP, error_); \
            goto cleanup;                    \
        }                                    \
    } while (0)

#define PUT_BIN_DATA(c_du_field_, def_val_, length_) \
    do {                                                                \
        if (data->spec_data->c_du_field_.du_type != TAD_DU_UNDEF)       \
        {                                                               \
            rc = tad_data_unit_to_bin(&(data->spec_data->c_du_field_),  \
                                      data->args, data->arg_num, p,     \
                                      length_);                         \
            CHECK_ERROR(rc != 0, rc,                                    \
                        "%s():%d: " #c_du_field_ " error: %r",          \
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
                default:                                                \
                    assert(FALSE);                                      \
            }                                                           \
        }                                                               \
        p += (length_);                                                 \
    } while (0) 


    CHECK_ERROR(data->spec_data->du_src_port.du_type == TAD_DU_UNDEF && 
                data->spec_data->local_port == 0,
                TE_ETADLESSDATA, 
                "%s(): no source port specified",
                __FUNCTION__);
    PUT_BIN_DATA(du_src_port, data->spec_data->local_port, 2);

    CHECK_ERROR(data->spec_data->du_dst_port.du_type == TAD_DU_UNDEF && 
                data->spec_data->remote_port == 0,
                TE_ETADLESSDATA, 
                "%s():  no destination port specified",
                __FUNCTION__);
    PUT_BIN_DATA(du_dst_port, data->spec_data->remote_port, 2);

    CHECK_ERROR(data->spec_data->du_seqn.du_type == TAD_DU_UNDEF,
                TE_ETADLESSDATA, 
                "%s():  no sequence number specified",
                __FUNCTION__); 
    rc = tad_data_unit_to_bin(&(data->spec_data->du_seqn),
                              data->args, data->arg_num, p, 4);
    CHECK_ERROR(rc != 0, rc, "%s():%d: seqn error: %r",
                __FUNCTION__,  __LINE__, rc);
    p += 4;

    PUT_BIN_DATA(du_ackn, 0, 4);
    if (data->spec_data->du_hlen.du_type != TAD_DU_UNDEF)
    {
        WARN("%s(): hlen field is ignored required in NDS",
              __FUNCTION__);
    }
    *p = (hdr_len / 4) << 4;
    p++;
    PUT_BIN_DATA(du_flags, 0, 1);

    VERB("du flags type %d, value %d, put value %d",
         data->spec_data->du_flags.du_type, 
         data->spec_data->du_flags.val_i32,
         (int)(*(p-1)));

    PUT_BIN_DATA(du_win_size, 1400, 2); /* TODO: good default window */
    PUT_BIN_DATA(du_checksum, 0, 2);
    PUT_BIN_DATA(du_urg_p, 0, 2); 

    return 0;
#undef PUT_BIN_DATA

cleanup:
    return rc;
}

/* See description in tad_ipstack_impl.h */
te_errno
tad_tcp_gen_bin_cb(csap_p csap, unsigned int layer,
                   const asn_value *tmpl_pdu, void *opaque,
                   const tad_tmpl_arg_t *args, size_t arg_num, 
                   tad_pkts *sdus, tad_pkts *pdus)
{
    te_errno                    rc;
    tad_tcp_fill_in_hdr_data    opaque_data;
 
    UNUSED(tmpl_pdu); 
    UNUSED(opaque);
 
    /* UDP layer does no fragmentation, just copy all SDUs to PDUs */
    tad_pkts_move(pdus, sdus);

    /*
     * Allocate and add TCP header to all packets.
     * FIXME sizeof(struct tcphdr) instead of 20.
     */
    rc = tad_pkts_add_new_seg(pdus, TRUE, NULL, 20, NULL);
    if (rc != 0)
        return rc;

    /* Fill in added segment as TCP header */
    opaque_data.spec_data = csap_get_proto_spec_data(csap, layer);
    opaque_data.args      = args;
    opaque_data.arg_num   = arg_num;
    rc = tad_pkts_enumerate_first_segs(pdus, tad_tcp_fill_in_hdr,
                                       &opaque_data);

    return rc;
}


/* See description in tad_ipstack_impl.h */
te_errno
tad_tcp_match_bin_cb(csap_p csap, unsigned int layer,
                     const asn_value *pattern_pdu,
                     const csap_pkts *pkt, csap_pkts *payload, 
                     asn_value_p parsed_packet)
{ 
    tcp_csap_specific_data_t *spec_data;

    uint8_t *data;
    uint8_t  tmp8;
    int      rc = 0;
    size_t   h_len = 0;

    uint8_t *pld_data = pkt->data;
    size_t   pld_len  = pkt->len;

    asn_value *tcp_header_pdu = NULL;


    if (parsed_packet)
        tcp_header_pdu = asn_init_value(ndn_tcp_header);

    spec_data = csap_get_proto_spec_data(csap, layer);

    data = pkt->data; 

    INFO("%s(CSAP %d)", __FUNCTION__, csap->id);

#define CHECK_FIELD(_asn_label, _size) \
    do {                                                        \
        rc = ndn_match_data_units(pattern_pdu, tcp_header_pdu,  \
                                  data, _size, _asn_label);     \
        if (rc)                                                 \
        {                                                       \
            F_VERB("%s: field %s not match, rc %r",             \
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

    pld_data += (h_len * 4);
    pld_len  -= (h_len * 4); 

    /* passing payload to upper layer */ 
    memset(payload, 0 , sizeof(*payload));

    if ((payload->len = pld_len) > 0)
    {
        payload->data = malloc(pld_len);
        memcpy(payload->data, pld_data, payload->len); 
    }

    if (parsed_packet)
    {
        rc = asn_write_component_value(parsed_packet, tcp_header_pdu,
                                       "#tcp"); 
        if (rc)
            ERROR("%s, write TCP header to packet fails %r", 
                  __FUNCTION__, rc);
    } 

    if (spec_data->wait_length > 0)
    { 
        free(spec_data->stored_buffer);
        spec_data->stored_buffer = NULL;
        spec_data->stored_length = 0;
        spec_data->wait_length = 0;
    }

cleanup:
    asn_free_value(tcp_header_pdu);

    return rc;
}
