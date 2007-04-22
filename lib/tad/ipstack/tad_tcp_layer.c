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
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    csap_set_proto_spec_data(csap, layer, spec_data);

    tcp_pdu = csap->layers[layer].nds;

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


static size_t
tad_tcp_option_len(const asn_value *opt_tmpl)
{
    asn_tag_class t_cl;
    asn_tag_value t_val;
    te_errno rc;
    asn_value *opt;
    int length = 0;

    rc = asn_get_choice_value(opt_tmpl, &opt, &t_cl, &t_val);
    if (rc != 0)
    {
        WARN("%s() get particular TCP option failed %r", 
             __FUNCTION__, rc);
        return 0;
    }
    switch (t_val)
    {
        case NDN_TAG_TCP_OPT_EOL:
        case NDN_TAG_TCP_OPT_NOP:
            return 1;
            break;
        case NDN_TAG_TCP_OPT_MSS:
        case NDN_TAG_TCP_OPT_WIN_SCALE:
        case NDN_TAG_TCP_OPT_TIMESTAMP:
            rc = asn_read_int32(opt, &length, "length");
            break;
        case NDN_TAG_TCP_OPT_SACK_PERM:
        case NDN_TAG_TCP_OPT_SACK_DATA:
            WARN("%s() SACK TCP option not supported.", 
                 __FUNCTION__, rc);
            return 0;
    }
    if (rc == 0)
        return length;
    if (rc == TE_EASNINCOMPLVAL)
        switch (t_val)
        {
            case NDN_TAG_TCP_OPT_MSS:
                return 4;
            case NDN_TAG_TCP_OPT_WIN_SCALE:
                return 3;
            case NDN_TAG_TCP_OPT_TIMESTAMP:
                return 10;
        }
    return 0;
}

/* See description in tad_ipstack_impl.h */
te_errno
tad_tcp_confirm_tmpl_cb(csap_p csap, unsigned int layer,
                       asn_value *layer_pdu, void **p_opaque)
{ 
    te_errno    rc = 0; 
    const asn_value *tcp_csap_pdu;
    asn_value *options;
    asn_value *option;

    tcp_csap_specific_data_t *spec_data = 
        csap_get_proto_spec_data(csap, layer);

    UNUSED(p_opaque);

    if (!(csap->state & CSAP_STATE_SEND))
    {
        ERROR(CSAP_LOG_FMT " should be called in SEND mode", 
              CSAP_LOG_ARGS(csap));
        return TE_RC(TE_TAD_CSAP, TE_ETADCSAPSTATE);
    }


    tcp_csap_pdu = csap->layers[layer].nds;

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

    CONVERT_FIELD(NDN_TAG_TCP_SRC_PORT, du_src_port); 
    if (spec_data->du_src_port.du_type == TAD_DU_UNDEF)
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

    CONVERT_FIELD(NDN_TAG_TCP_DST_PORT, du_dst_port);
    if (spec_data->du_dst_port.du_type == TAD_DU_UNDEF)
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

    CONVERT_FIELD(NDN_TAG_TCP_SEQN, du_seqn);
    CONVERT_FIELD(NDN_TAG_TCP_ACKN, du_ackn);
    CONVERT_FIELD(NDN_TAG_TCP_HLEN, du_hlen);
    CONVERT_FIELD(NDN_TAG_TCP_FLAGS, du_flags);
    CONVERT_FIELD(NDN_TAG_TCP_WINDOW, du_win_size);
    CONVERT_FIELD(NDN_TAG_TCP_CHECKSUM, du_checksum);
    CONVERT_FIELD(NDN_TAG_TCP_URG, du_urg_p);
#undef CONVERT_FIELD 

    spec_data->opt_bin_len = 0;
    rc = asn_get_descendent(layer_pdu, &options, "options");
    if (rc == 0)
    {
        int i, opt_num = asn_get_length(options, "");

        for (i = 0; i < opt_num; i++)
        {
            rc = asn_get_indexed(options, &option, i, "");
            if (rc == 0)
                spec_data->opt_bin_len += tad_tcp_option_len(option);
        }
        RING(CSAP_LOG_FMT " Options bin len: %d",
             CSAP_LOG_ARGS(csap), (int)spec_data->opt_bin_len);

        spec_data->options = options;
    }
    else
        spec_data->options = NULL;

    return 0;
}


/* See description in tad_ipstack_impl.h */
te_errno
tad_tcp_confirm_ptrn_cb(csap_p csap, unsigned int layer,
                        asn_value *layer_pdu, void **p_opaque)
{ 
    te_errno    rc = 0;

    const asn_value *tcp_csap_pdu;
    const asn_value *du_field;

    tcp_csap_specific_data_t *spec_data = 
        csap_get_proto_spec_data(csap, layer);

    UNUSED(p_opaque);

    if (!(csap->state & CSAP_STATE_RECV))
    {
        ERROR(CSAP_LOG_FMT " should be called in RECV mode", 
              CSAP_LOG_ARGS(csap));
        return TE_RC(TE_TAD_CSAP, TE_ETADCSAPSTATE);
    }

    tcp_csap_pdu = csap->layers[layer].nds;

    if (spec_data->du_src_port.du_type == TAD_DU_UNDEF)
    {
        if (asn_get_child_value(tcp_csap_pdu, &du_field, PRIVATE,
                                NDN_TAG_TCP_REMOTE_PORT) == 0 &&
            (rc = asn_write_component_value(layer_pdu, du_field,
                                           "src-port")) != 0)
        {
            ERROR("%s(): write src-port to tcp layer_pdu failed %r",
                  __FUNCTION__, rc);
            return TE_RC(TE_TAD_CSAP, rc);
        }
    }

    if (spec_data->du_dst_port.du_type == TAD_DU_UNDEF)
    {
        if (asn_get_child_value(tcp_csap_pdu, &du_field, PRIVATE,
                                NDN_TAG_TCP_LOCAL_PORT) == 0 &&
            (rc = asn_write_component_value(layer_pdu, du_field,
                                           "dst-port")) != 0)
        {
            ERROR("%s(): write dst-port to tcp layer_pdu failed %r",
                  __FUNCTION__, rc);
            return TE_RC(TE_TAD_CSAP, rc);
        }
    }

    return 0;
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

    const asn_value *options;
    asn_value *option;

    unsigned int hdr_len = 20 + data->spec_data->opt_bin_len;

    UNUSED(pkt);
    UNUSED(seg_num);
    assert(seg->data_len == hdr_len);

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

#undef PUT_BIN_DATA

    options = data->spec_data->options;
    if (options != NULL)
    {
        uint8_t *opt_start = p;

        asn_tag_class t_cl;
        asn_tag_value t_val;
        asn_value *p_opt;
        size_t opt_b_len;
        int32_t opt_val;

        int i, opt_num = asn_get_length(options, "");

        for (i = 0; i < opt_num; i++)
        {
            rc = asn_get_indexed(options, &option, i, ""); 
            if (rc != 0) continue;

            opt_b_len = tad_tcp_option_len(option);

            rc = asn_get_choice_value(option, &p_opt, &t_cl, &t_val);
            if (rc != 0) continue;
            
            opt_val = 0;
            switch (t_val)
            {
                case NDN_TAG_TCP_OPT_EOL:
                    *p = TE_TCP_OPT_EOL;
                    break;
                case NDN_TAG_TCP_OPT_NOP:
                    *p = TE_TCP_OPT_NOP;
                    break;
                case NDN_TAG_TCP_OPT_MSS:
                    *p = TE_TCP_OPT_MSS;
                    *(p+1) = opt_b_len;
                    asn_read_int32(p_opt, &opt_val, "mss");
                    *((uint16_t *)(p + 2)) = htons((uint16_t)opt_val);
                    break;
                case NDN_TAG_TCP_OPT_WIN_SCALE:
                    *p = TE_TCP_OPT_WIN_SCALE;
                    *(p+1) = opt_b_len;
                    asn_read_int32(p_opt, &opt_val, "win-scale");
                    *(p+2) = opt_val;
                    break;
                case NDN_TAG_TCP_OPT_TIMESTAMP:
                    *p = TE_TCP_OPT_TIMESTAMP;
                    *(p+1) = opt_b_len;
                    asn_read_int32(p_opt, &opt_val, "value");
                    *((uint32_t *)(p + 2)) = htonl(opt_val);
                    opt_val = 0;
                    asn_read_int32(p_opt, &opt_val, "echo-reply");
                    *((uint32_t *)(p + 6)) = htonl(opt_val);
                    break;
                case NDN_TAG_TCP_OPT_SACK_PERM:
                case NDN_TAG_TCP_OPT_SACK_DATA:
                    WARN("%s() SACK TCP option not supported.", 
                         __FUNCTION__, rc);
                    continue;
            }
            p += opt_b_len;
        }
        RING("%s: options bytes: %Tm", __FUNCTION__, opt_start, 
             data->spec_data->opt_bin_len);
    }

    return 0;

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
 

    opaque_data.spec_data = csap_get_proto_spec_data(csap, layer);
    opaque_data.args      = args;
    opaque_data.arg_num   = arg_num;

    /* TCP layer does no fragmentation, just copy all SDUs to PDUs */
    tad_pkts_move(pdus, sdus);

    /*
     * Allocate and add TCP header to all packets.
     * FIXME sizeof(struct tcphdr) instead of 20.
     */
    rc = tad_pkts_add_new_seg(pdus, TRUE, NULL,
                              20 + opaque_data.spec_data->opt_bin_len,
                              NULL);
    if (rc != 0)
        return rc;

    /* Fill in added segment as TCP header */
    rc = tad_pkts_enumerate_first_segs(pdus, tad_tcp_fill_in_hdr,
                                       &opaque_data);

    return rc;
}


/* See description in tad_ipstack_impl.h */
te_errno
tad_tcp_match_bin_cb(csap_p           csap, 
                     unsigned int     layer,
                     const asn_value *ptrn_pdu,
                     void            *ptrn_opaque,
                     tad_recv_pkt    *meta_pkt,
                     tad_pkt         *pdu,
                     tad_pkt         *sdu)
{ 
    asn_value  *options = NULL;
    uint8_t    *data_ptr; 
    uint8_t    *pld_start; 
    uint8_t    *data; 
    size_t      data_len;
    asn_value  *tcp_header_pdu = NULL;
    te_errno    rc;

    uint8_t  tmp8;
    size_t   h_len = 0;

    UNUSED(ptrn_opaque);

    assert(tad_pkt_seg_num(pdu) == 1);
    assert(tad_pkt_first_seg(pdu) != NULL);
    data_ptr = data = tad_pkt_first_seg(pdu)->data_ptr;
    data_len = tad_pkt_first_seg(pdu)->data_len;

    if ((csap->state & CSAP_STATE_RESULTS) &&
        (tcp_header_pdu = meta_pkt->layers[layer].nds =
             asn_init_value(ndn_tcp_header)) == NULL)
    {
        ERROR_ASN_INIT_VALUE(ndn_tcp_header);
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    }

#define CHECK_FIELD(_asn_label, _size) \
    do {                                                        \
        rc = ndn_match_data_units(ptrn_pdu, tcp_header_pdu,     \
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
    rc = ndn_match_data_units(ptrn_pdu, tcp_header_pdu, 
                              &tmp8, 1, "hlen");
    if (rc)
        goto cleanup;
    data ++;

    tmp8 = (*data) & 0x3f;
    rc = ndn_match_data_units(ptrn_pdu, tcp_header_pdu, 
                              &tmp8, 1, "flags");
    if (rc)
        goto cleanup;
    data++;

    CHECK_FIELD("win-size", 2); 
    CHECK_FIELD("checksum", 2); 
    CHECK_FIELD("urg-p", 2);
 
#undef CHECK_FIELD

    if (data_len < h_len * 4)
    {
        ERROR(CSAP_LOG_FMT "Length of data in passed PDU too small: %d",
              CSAP_LOG_ARGS(csap), data_len);
        rc = TE_RC(TE_TAD_CSAP, TE_ETADLESSDATA);
        goto cleanup;
    }

    /* Process TCP options */
    /* Options in pattern are ignored. TODO: filtering by options */

    /* Assume that option length is set correctly in incoming packet */

    pld_start = data_ptr + h_len * 4;

    rc = 0;
    if (tcp_header_pdu != NULL)
    {
        uint32_t opt_val;
        if (data < pld_start)
            VERB("%s(): dump of options: %Tm",
                 __FUNCTION__, data, pld_start - data);

        while ( rc == 0 && data < pld_start)
        {
            asn_value *opt = asn_init_value(ndn_tcp_option);
            if (options == NULL)
                options = asn_init_value(ndn_tcp_options_seq);

            VERB("%s(): foung option with kind %d, offset %d",
                 __FUNCTION__, (int)(*data), data  - data_ptr);
            switch (*data)
            {
                case TE_TCP_OPT_EOL:
                    rc = asn_write_value_field(opt, NULL, 0, "#eol");
                    break;
                case TE_TCP_OPT_NOP:
                    rc = asn_write_value_field(opt, NULL, 0, "#nop");
                    break;
                case TE_TCP_OPT_MSS:
                    rc = asn_write_value_field(opt, data+1, 1, 
                                               "#mss.length.#plain");
                    if (rc) break;
                    opt_val = ntohs(*((uint16_t *)(data + 2)));
                    rc = asn_write_value_field(opt, &opt_val, 4,
                                               "#mss.mss.#plain");
                    data += *(data + 1) - 1;
                    break;
                case TE_TCP_OPT_WIN_SCALE:
                    rc = asn_write_value_field(opt, data + 1, 1, 
                                               "#win-scale.length.#plain");
                    if (rc) break;
                    rc = asn_write_value_field(opt, data + 2, 1,
                                               "#win-scale.scale.#plain");
                    data += *(data + 1) - 1;
                    break;

                case TE_TCP_OPT_SACK_PERM:
                case TE_TCP_OPT_SACK_DATA:
                    F_INFO(CSAP_LOG_FMT "TCP options, SACK not supported",
                          CSAP_LOG_ARGS(csap));
                    data += *(data + 1) - 1;
                    asn_free_value(opt);
                    opt = NULL;
                    break;
                case TE_TCP_OPT_TIMESTAMP:
                    rc = asn_write_value_field(opt, data+1, 1, 
                                               "#timestamp.length.#plain");
                    if (rc) break;
                    opt_val = ntohl(*((uint32_t *)(data + 2)));
                    rc = asn_write_value_field(opt, &opt_val, 4,
                                               "#timestamp.value.#plain");
                    if (rc) break;
                    opt_val = ntohl(*((uint32_t *)(data + 6)));
                    rc = asn_write_value_field(opt, &opt_val, 4,
                                           "#timestamp.echo-reply.#plain");
                    data += *(data + 1) - 1;
                    break;
            }
            data++;
            if (rc) break;

            if (opt != NULL)
                rc = asn_insert_indexed(options, opt, -1, "");

#if 0
        {
            char buf [200] = {0,};
            asn_sprint_value(options, buf, sizeof(buf) - 1, 0);
            RING("dump of parsed options: %s", buf);
        }
#endif
        }
        if (rc == 0)
            rc = asn_put_descendent(tcp_header_pdu, options, "options");
        VERB("%s(), options put, rc %r", __FUNCTION__, rc);
    }
    if (rc)
        goto cleanup;

    /* Passing payload to upper layer */
    rc = tad_pkt_get_frag(sdu, pdu, h_len * 4, data_len - (h_len * 4),
                          TAD_PKT_GET_FRAG_ERROR);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Failed to prepare TCP SDU: %r",
              CSAP_LOG_ARGS(csap), rc);
        return rc;
    }

cleanup:
    if (rc != 0)
    {
        if (TE_RC_GET_ERROR(rc) != TE_ETADNOTMATCH &&
            TE_RC_GET_ERROR(rc) != TE_ETADLESSDATA)
        {
            ERROR("%s: failed at offset %u: %r", __FUNCTION__,
                  (unsigned)(data_ptr - data), rc);
        }
    }
    return rc;
}
