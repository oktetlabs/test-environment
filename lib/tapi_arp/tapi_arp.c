/** @file
 * @brief TAPI TAD ARP
 *
 * Implementation of test API for ARP TAD.
 *
 * Copyright (C) 2004-2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * Licensealong with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */ 

#define TE_LGR_USER     "TAPI ARP"

#include "te_config.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H 
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif


#include <stdio.h>
#include <assert.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include "te_errno.h"
#include "logger_api.h"
#include "rcf_api.h"
#include "ndn_eth.h"
#include "tapi_arp.h"
#include "tapi_eth.h"
#include "tapi_tad.h"
#include "tapi_ndn.h"

#include "tapi_test.h"



/* See the description in tapi_arp.h */
te_errno
tapi_arp_add_csap_layer_eth(asn_value      **csap_spec,
                            const char      *device,
                            const uint8_t   *remote_addr,
                            const uint8_t   *local_addr)
{
    uint16_t    eth_type = ETHERTYPE_ARP;

    return tapi_eth_add_csap_layer(csap_spec, device, ETH_RECV_DEF,
                                   remote_addr, local_addr, &eth_type);
}


/* See the description in tapi_arp.h */
int
tapi_arp_eth_csap_create(const char     *ta_name,
                         int             sid,
                         const char     *device,
                         const uint8_t  *remote_addr,
                         const uint8_t  *local_addr, 
                         const uint16_t *hw_type,
                         const uint16_t *proto_type,
                         const uint8_t  *hw_size,
                         const uint8_t  *proto_size,
                         csap_handle_t  *arp_csap)
{
    te_errno    rc;
    asn_value  *nds = NULL;

    CHECK_RC(tapi_arp_add_csap_layer(&nds, hw_type, proto_type,
                                     hw_size, proto_size));
    CHECK_RC(tapi_arp_add_csap_layer_eth(&nds, device,
                                         remote_addr, local_addr));

    rc = tapi_tad_csap_create(ta_name, sid, "arp.eth", nds, arp_csap);

    asn_free_value(nds);

    return rc;
}


typedef struct tapi_arp_pkt_handler_data {
    tapi_arp_frame_callback  callback;
    void                    *user_data;
} tapi_arp_pkt_handler_data;

static void
eth_frame_callback(const ndn_eth_header_plain *header,
                   const uint8_t *payload, uint16_t plen,
                   void *user_data)
{
    struct tapi_arp_pkt_handler_data *i_data = 
        (struct tapi_arp_pkt_handler_data *)user_data;

    tapi_arp_frame_t  arp_frame;
    uint16_t          short_var;
    /* 
     * Flag meaning is payload length minimal. 
     * Minimal Eth frame length = 64 bytes, 
     *         Eth header + tailing checksum = 18 bytes. 
     */
    te_bool  plen_minimal = (plen <= (64 - 18));

    memset(&arp_frame, 0, sizeof(arp_frame));

    memcpy(&(arp_frame.eth_hdr), header, sizeof(arp_frame.eth_hdr));


    /* Parse ARP packet structure */
#define ARP_GET_SHORT_VAR(fld_) \
    do {                                                           \
        if (plen < sizeof(short_var))                              \
        {                                                          \
            ERROR("ARP Header is truncated at '%s' field", #fld_); \
            return;                                                \
        }                                                          \
        memcpy(&short_var, payload, sizeof(short_var));            \
        arp_frame.arp_hdr.fld_ = ntohs(short_var);                 \
        payload += sizeof(short_var);                              \
        plen -= sizeof(short_var);                                 \
    } while (0)

    ARP_GET_SHORT_VAR(hw_type);
    ARP_GET_SHORT_VAR(proto_type);

    if (plen < sizeof(arp_frame.arp_hdr.hw_size))
    {
        ERROR("ARP Header is truncated at 'hw_size' field");
        return;
    }
    arp_frame.arp_hdr.hw_size = *(payload++);
    plen--;
    if (plen < sizeof(arp_frame.arp_hdr.proto_size))
    {
        ERROR("ARP Header is truncated at 'proto_size' field");
        return;
    }
    arp_frame.arp_hdr.proto_size = *(payload++);
    plen--;

    ARP_GET_SHORT_VAR(opcode);

#undef ARP_GET_SHORT_VAR

#define ARP_GET_ARRAY(fld_, size_fld_) \
    do {                                                           \
        if (arp_frame.arp_hdr.size_fld_ >                          \
            sizeof(arp_frame.arp_hdr.fld_))                        \
        {                                                          \
            ERROR("The length of '%s' field is too big to "        \
                  "fit in TAPI data structure", #fld_);            \
            return;                                                \
        }                                                          \
        if (plen < arp_frame.arp_hdr.size_fld_)                    \
        {                                                          \
            ERROR("ARP Header is truncated at '%s' field", #fld_); \
            return;                                                \
        }                                                          \
        memcpy(arp_frame.arp_hdr.fld_, payload,                    \
               arp_frame.arp_hdr.size_fld_);                       \
        payload += arp_frame.arp_hdr.size_fld_;                    \
        plen -= arp_frame.arp_hdr.size_fld_;                       \
    } while (0)

    ARP_GET_ARRAY(snd_hw_addr, hw_size);
    ARP_GET_ARRAY(snd_proto_addr, proto_size);
    ARP_GET_ARRAY(tgt_hw_addr, hw_size);
    ARP_GET_ARRAY(tgt_proto_addr, proto_size);

#undef ARP_GET_ARRAY

    if (!plen_minimal && plen > 0)
    {
        INFO("ARP frame has some data after ARP header, plen %d", plen);
        if ((arp_frame.data = (uint8_t *)malloc(plen)) == NULL)
        {
            ERROR("Cannot allocate memory under ARP payload");
            return;
        }
        memcpy(arp_frame.data, payload, plen);
        arp_frame.data_len = plen;
    }

    i_data->callback(&arp_frame, i_data->user_data);

    free(arp_frame.data);
    return;
}


/* See the description in tapi_arp.h */
tapi_tad_trrecv_cb_data *
tapi_arp_trrecv_cb_data(tapi_arp_frame_callback  callback,
                        void                    *user_data)
{
    tapi_arp_pkt_handler_data  *cb_data;
    tapi_tad_trrecv_cb_data    *res;

    cb_data = (tapi_arp_pkt_handler_data *)calloc(1, sizeof(*cb_data));
    if (cb_data == NULL)
    {
        ERROR("%s(): failed to allocate memory", __FUNCTION__);
        return NULL;
    }
    cb_data->callback = callback;
    cb_data->user_data = user_data;
    
    res = tapi_eth_trrecv_cb_data(eth_frame_callback, cb_data);
    if (res == NULL)
        free(cb_data);
    
    return res;
}


typedef struct arp_frame_cb_info {
    tapi_arp_frame_t *frames;
    int               num;
    int               rc;
} arp_frame_cb_info_t;

static void
arp_frame_callback(const tapi_arp_frame_t *arp_frame, void *userdata)
{
    arp_frame_cb_info_t *info = (arp_frame_cb_info_t *)userdata;
    tapi_arp_frame_t    *tmp_ptr = info->frames;

    assert((arp_frame->data == NULL && arp_frame->data_len == 0) ||
           (arp_frame->data != NULL && arp_frame->data_len != 0));

    if (info->rc != 0)
        return;

    /*
     * Copy ARP frame because it is freed after returning
     * from the function.
     */
    info->num++;
    info->frames = 
        (tapi_arp_frame_t *)realloc(info->frames,
                                    sizeof(tapi_arp_frame_t) * info->num);
    if (info->frames == NULL)
    {
        info->frames = tmp_ptr;
        info->num--;
        info->rc = TE_ENOMEM;
        return;
    }
    memcpy(&info->frames[info->num - 1], arp_frame, sizeof(*arp_frame));
    if (arp_frame->data != NULL)
    {
        /* Allocate memory under ARP payload - which is not usual */
        info->frames[info->num - 1].data = 
            (uint8_t *)malloc(arp_frame->data_len);
        if (info->frames[info->num - 1].data == NULL)
        {
            info->rc = TE_ENOMEM;
            return;
        }
        memcpy(info->frames[info->num - 1].data, arp_frame->data,
               arp_frame->data_len);
    }
}

/* See the description in tapi_arp.h */
int
tapi_arp_recv(const char *ta_name, int sid, csap_handle_t arp_csap,
              const asn_value *pattern, unsigned int timeout,
              tapi_arp_frame_t **frames, int *num)
{
    int                 rc;
    arp_frame_cb_info_t info;
    int                 num_tmp;
    
    if (frames == NULL || num == NULL)
        return TE_EINVAL;

    memset(&info, 0, sizeof(info));
    
    rc = tapi_tad_trrecv_start(ta_name, sid, arp_csap, pattern,
                               timeout, *num, RCF_TRRECV_PACKETS);
    
    if (rc != 0)
    {
        ERROR("tapi_arp_recv_start() returns %r", rc);
        return rc;
    }
    /* Wait until all the packets received or timeout */
    rc = tapi_tad_trrecv_wait(ta_name, sid, arp_csap,
                              tapi_arp_trrecv_cb_data(arp_frame_callback,
                                                      &info),
                              &num_tmp);

    if (rc != 0 || info.rc != 0)
    {
        int i;
        
        ERROR("rcf_ta_trrecv_wait() returns %X, info.rc %r", rc, info.rc);
        for (i = 0; i < info.num; i++)
        {
            free(info.frames[i].data);
        }
        free(info.frames);

        return (rc != 0) ? rc : info.rc;
    }
    WARN("info.num = %d, num_tmp = %d", info.num, num_tmp);
    
    assert(info.num == num_tmp);

    *num = info.num;
    *frames = info.frames;

    return 0;
}


/* See the description in tapi_arp.h */
te_errno
tapi_arp_prepare_template(const tapi_arp_frame_t *frame, asn_value **templ)
{
    asn_value *traffic_templ;
    asn_value *hdr_tmpl;
    asn_value *asn_pdus, *asn_pdu;

    if (frame == NULL || templ == NULL)
        return TE_EINVAL;
    
    if ((frame->data == NULL) != (frame->data_len == 0))
    {
        ERROR("'data' and 'data_len' fields should be 'NULL' and zero, "
              "or non 'NULL' and not zero.");
        return TE_EINVAL;
    }

    if (frame->arp_hdr.hw_size > sizeof(frame->arp_hdr.snd_hw_addr))
    {
        ERROR("The value of 'hw_size' field is more than the length of "
              "'snd_hw_addr' and 'tgt_hw_addr' fields");
        return TE_EINVAL;
    }
    if (frame->arp_hdr.proto_size > sizeof(frame->arp_hdr.snd_proto_addr))
    {
        ERROR("The value of 'proto_size' field is more than the length of "
              "'snd_proto_addr' and 'tgt_proto_addr' fields");
        return TE_EINVAL;
    }

    CHECK_NOT_NULL(traffic_templ = asn_init_value(ndn_traffic_template));
    CHECK_NOT_NULL(asn_pdus = asn_init_value(ndn_generic_pdu_sequence));

    CHECK_NOT_NULL(hdr_tmpl = ndn_arp_plain_to_packet(&(frame->arp_hdr)));
    CHECK_NOT_NULL(asn_pdu = asn_init_value(ndn_generic_pdu));
    CHECK_RC(asn_write_component_value(asn_pdu, hdr_tmpl, "#arp"));
    CHECK_RC(asn_insert_indexed(asn_pdus, asn_pdu, 0, ""));

    CHECK_NOT_NULL(hdr_tmpl = ndn_eth_plain_to_packet(&(frame->eth_hdr)));
    CHECK_NOT_NULL(asn_pdu = asn_init_value(ndn_generic_pdu));
    CHECK_RC(asn_write_component_value(asn_pdu, hdr_tmpl, "#eth"));
    CHECK_RC(asn_insert_indexed(asn_pdus, asn_pdu, 1, ""));

    CHECK_RC(asn_write_component_value(traffic_templ, asn_pdus, "pdus"));

    /** Create ARP header and add it as the payload to Ethernet frame */
    if (frame->data != NULL)
    {
        CHECK_RC(asn_write_value_field(traffic_templ,
                                       frame->data, frame->data_len,
                                       "payload.#bytes"));
    }

    *templ = traffic_templ;

    return 0;
}

/* See the description in tapi_arp.h */
int
tapi_arp_prepare_pattern_eth_only(const uint8_t *src_mac,
                                  const uint8_t *dst_mac,
                                  asn_value **pattern)
{
    uint16_t eth_type = ETHERTYPE_ARP;

    return tapi_eth_add_pdu(pattern, TRUE, dst_mac, src_mac, &eth_type);
}


/* See the description in tapi_arp.h */
te_errno 
tapi_arp_add_csap_layer(asn_value      **csap_spec,
                        const uint16_t  *hw_type,
                        const uint16_t  *proto_type,
                        const uint8_t   *hw_size,
                        const uint8_t   *proto_size)
{
    asn_value  *layer;

    CHECK_RC(tapi_tad_csap_add_layer(csap_spec, ndn_arp_csap, "#arp",
                                     &layer));

    if (hw_type != NULL)
        CHECK_RC(asn_write_int32(layer, *hw_type, "hw-type.#plain"));
    if (proto_type != NULL)
        CHECK_RC(asn_write_int32(layer, *proto_type, "proto-type.#plain"));
    if (hw_size != NULL)
        CHECK_RC(asn_write_int32(layer, *hw_size, "hw-size.#plain"));
    if (proto_size != NULL)
        CHECK_RC(asn_write_int32(layer, *proto_size, "proto-size.#plain"));

    return 0;
}

/* See the description in tapi_arp.h */
te_errno
tapi_arp_add_csap_layer_eth_ip4(asn_value **csap_spec)
{
    uint16_t    hw_type = ARPHRD_ETHER;
    uint16_t    proto_type = ETHERTYPE_IP;
    uint8_t     hw_size = ETHER_ADDR_LEN;
    uint8_t     proto_size = sizeof(in_addr_t);

    return tapi_arp_add_csap_layer(csap_spec, &hw_type, &proto_type,
                                   &hw_size, &proto_size);
}

/* See the description in tapi_arp.h */
te_errno
tapi_arp_add_pdu_eth_ip4(asn_value      **tmpl_or_ptrn,
                         te_bool          is_pattern,
                         const uint16_t  *opcode,
                         const uint8_t   *snd_hw_addr,
                         const uint8_t   *snd_proto_addr,
                         const uint8_t   *tgt_hw_addr,
                         const uint8_t   *tgt_proto_addr)
{
    asn_value  *pdu;

    CHECK_RC(tapi_tad_tmpl_ptrn_add_layer(tmpl_or_ptrn, is_pattern,
                                          ndn_arp_header, "#arp",
                                          &pdu));

    if (opcode != NULL)
        CHECK_RC(asn_write_int32(pdu, *opcode, "opcode.#plain"));
    if (snd_hw_addr != NULL)
        CHECK_RC(asn_write_value_field(pdu, snd_hw_addr, ETHER_ADDR_LEN,
                                       "snd-hw-addr.#plain"));
    if (snd_proto_addr != NULL)
        CHECK_RC(asn_write_value_field(pdu, snd_proto_addr,
                                       sizeof(in_addr_t),
                                       "snd-proto-addr.#plain"));
    if (tgt_hw_addr != NULL)
        CHECK_RC(asn_write_value_field(pdu, tgt_hw_addr, ETHER_ADDR_LEN,
                                       "tgt-hw-addr.#plain"));
    if (tgt_proto_addr != NULL)
        CHECK_RC(asn_write_value_field(pdu, tgt_proto_addr,
                                       sizeof(in_addr_t),
                                       "tgt-proto-addr.#plain"));

    return 0;
}
