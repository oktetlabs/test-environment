/** @file
 * @brief Proteos, Tester API for ARP CSAP.
 *
 * Implementation of Tester API for Ethernet CSAP.
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */ 

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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

#include "rcf_api.h"
#include "ndn_eth.h"
#include "tapi_arp.h"
#include "tapi_eth.h"

#define TE_LGR_USER     "TAPI ARP"
#include "logger_api.h"

/* See the description in tapi_arp.h */
int
tapi_arp_csap_create(const char *ta_name, int sid, const char *device,
                     const uint8_t *remote_addr, const uint8_t *local_addr, 
                     csap_handle_t *arp_csap)
{
    uint16_t eth_type = ETHERTYPE_ARP;

    return tapi_eth_csap_create(ta_name, sid, device, remote_addr, local_addr,
                                &eth_type, arp_csap);
}

/* See the description in tapi_arp.h */
int
tapi_arp_send(const char *ta_name, int sid,
              csap_handle_t arp_csap, const asn_value *templ)
{
    return tapi_eth_send(ta_name, sid, arp_csap, templ);
}

struct tapi_pkt_handler_data
{
    tapi_arp_frame_callback  user_callback;
    void                    *user_data;
};

static void
eth_frame_callback(const ndn_eth_header_plain *header,
                   const uint8_t *payload, uint16_t plen,
                   void *user_data)
{
    struct tapi_pkt_handler_data *i_data = 
        (struct tapi_pkt_handler_data *)user_data;
    tapi_arp_frame_t              arp_frame;
    uint16_t                      short_var;

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

    ARP_GET_SHORT_VAR(hard_type);
    ARP_GET_SHORT_VAR(proto_type);

    if (plen < sizeof(arp_frame.arp_hdr.hard_size))
    {
        ERROR("ARP Header is truncated at 'hard_size' field");
        return;
    }
    arp_frame.arp_hdr.hard_size = *(payload++);
    plen--;
    if (plen < sizeof(arp_frame.arp_hdr.proto_size))
    {
        ERROR("ARP Header is truncated at 'proto_size' field");
        return;
    }
    arp_frame.arp_hdr.proto_size = *(payload++);
    plen--;

    ARP_GET_SHORT_VAR(op_code);

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

    ARP_GET_ARRAY(snd_hw_addr, hard_size);
    ARP_GET_ARRAY(snd_proto_addr, proto_size);
    ARP_GET_ARRAY(tgt_hw_addr, hard_size);
    ARP_GET_ARRAY(tgt_proto_addr, proto_size);

#undef ARP_GET_ARRAY

    if (plen > 0)
    {
        WARN("ARP frame has some data after ARP header");
        if ((arp_frame.data = (uint8_t *)malloc(plen)) == NULL)
        {
            ERROR("Cannot allocate memory under ARP payload");
            return;
        }
        memcpy(arp_frame.data, payload, plen);
        arp_frame.data_len = plen;
    }

    i_data->user_callback(&arp_frame, i_data->user_data);

    free(arp_frame.data);
    return;
}

/* See the description in tapi_arp.h */
int
tapi_arp_recv_start(const char *ta_name, int sid, csap_handle_t arp_csap,
                    const asn_value *pattern,
                    tapi_arp_frame_callback cb, void *cb_data,
                    unsigned int timeout, int num)
{
    struct tapi_pkt_handler_data *i_data;
    
    if ((i_data = (struct tapi_pkt_handler_data *)malloc(sizeof(struct tapi_pkt_handler_data))) == NULL)
        return TE_RC(TE_TAPI, ENOMEM);
    
    i_data->user_callback = cb;
    i_data->user_data = cb_data;
 
    return tapi_eth_recv_start(ta_name, sid, arp_csap, pattern,
                               (cb != NULL) ? eth_frame_callback : NULL,
                               (cb != NULL) ? i_data : NULL, timeout, num);
}

int
tapi_arp_prepare_template(const tapi_arp_frame_t *frame, asn_value **templ)
{
    asn_value *traffic_templ;
    asn_value *frame_tmpl;
    asn_value *asn_pdus, *asn_pdu;
    uint8_t   *arp_pkt;
    uint32_t   arp_pkt_len;
    int        rc = 0;

    if (frame == NULL || templ == NULL)
        return EINVAL;
    
    if ((uint32_t)(frame->data) ^ (uint32_t)(frame->data_len))
    {
        ERROR("'data' and 'data_len' fields should be 'NULL' and zero, "
              "or non 'NULL' and not zero.");
        return EINVAL;
    }

    if (frame->arp_hdr.hard_size > sizeof(frame->arp_hdr.snd_hw_addr))
    {
        ERROR("The value of 'hard_size' field is more than the length of "
              "'snd_hw_addr' and 'tgt_hw_addr' fields");
        return EINVAL;
    }
    if (frame->arp_hdr.proto_size > sizeof(frame->arp_hdr.snd_proto_addr))
    {
        ERROR("The value of 'proto_size' field is more than the length of "
              "'snd_proto_addr' and 'tgt_proto_addr' fields");
        return EINVAL;
    }

    /* Allocate memory under ARP packet */
#define ARP_FIELD_SIZE(fld_) sizeof(frame->arp_hdr.fld_)
    arp_pkt_len = ARP_FIELD_SIZE(hard_type) + ARP_FIELD_SIZE(proto_type) +
                  ARP_FIELD_SIZE(hard_size) + ARP_FIELD_SIZE(proto_size) +
                  ARP_FIELD_SIZE(op_code) + frame->arp_hdr.hard_size * 2 +
                  frame->arp_hdr.proto_size * 2 + frame->data_len;
#undef ARP_FIELD_SIZE
    if ((arp_pkt = (uint8_t *)malloc(arp_pkt_len)) == NULL)
        return ENOMEM;

    traffic_templ = asn_init_value(ndn_traffic_template);
    asn_pdus = asn_init_value(ndn_generic_pdu_sequence);
    asn_pdu = asn_init_value(ndn_generic_pdu);

    frame_tmpl = ndn_eth_plain_to_packet(&(frame->eth_hdr));
    if (frame_tmpl == NULL)
    {
        free(arp_pkt);
        return ENOMEM;
    }

    rc = asn_write_component_value(asn_pdu, frame_tmpl, "#eth");

    if (rc == 0)
        rc = asn_insert_indexed(asn_pdus, asn_pdu, -1, "");

    if (rc == 0)
        rc = asn_write_component_value(traffic_templ, asn_pdus, "pdus");

    /** Create ARP header and add it as the payload to Ethernet frame */
    if (rc == 0)
    {
        uint16_t  short_var;
        uint8_t  *arp_pkt_start = arp_pkt;
        
#define ARP_FILL_SHORT_VAR(fld_) \
        do {                                                \
            short_var = htons(frame->arp_hdr.fld_);         \
            memcpy(arp_pkt, &short_var, sizeof(short_var)); \
            arp_pkt += sizeof(short_var);                   \
        } while (0)
    
        ARP_FILL_SHORT_VAR(hard_type);
        ARP_FILL_SHORT_VAR(proto_type);

        *(arp_pkt++) = frame->arp_hdr.hard_size;
        *(arp_pkt++) = frame->arp_hdr.proto_size;

        ARP_FILL_SHORT_VAR(op_code);

#undef ARP_FILL_SHORT_VAR

#define ARP_FILL_ARRAY(fld_, size_fld_) \
        do {                                     \
            memcpy(arp_pkt, frame->arp_hdr.fld_, \
                   frame->arp_hdr.size_fld_);    \
            arp_pkt += frame->arp_hdr.size_fld_; \
        } while (0)

        ARP_FILL_ARRAY(snd_hw_addr, hard_size);
        ARP_FILL_ARRAY(snd_proto_addr, proto_size);
        ARP_FILL_ARRAY(tgt_hw_addr, hard_size);
        ARP_FILL_ARRAY(tgt_proto_addr, proto_size);

#undef ARP_FILL_ARRAY

        assert((arp_pkt - arp_pkt_start) == arp_pkt_len);
        rc = asn_write_value_field(traffic_templ, arp_pkt_start, arp_pkt_len,
                                   "payload.#bytes");
        arp_pkt = arp_pkt_start;
    }

    free(arp_pkt);

    if (rc != 0)
    {
        VERB("Cannot create traffic template %x\n", rc);
        return rc;
    }

    *templ = traffic_templ;
    return 0;
}
