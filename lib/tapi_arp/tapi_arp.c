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

/* Forward declaration of static functions */
static uint8_t *tapi_arp_create_bin_arp_pkt(const tapi_arp_hdr_t *arp_hdr,
                                            const uint8_t *data,
                                            size_t data_len,
                                            size_t *pkt_len);


/* See the description in tapi_arp.h */
int
tapi_arp_csap_create(const char *ta_name, int sid, const char *device,
                     const uint8_t *remote_addr, const uint8_t *local_addr, 
                     csap_handle_t *arp_csap)
{
    uint16_t eth_type = ETHERTYPE_ARP;

    return tapi_eth_csap_create(ta_name, sid, device,
                                remote_addr, local_addr,
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
    
    i_data = (struct tapi_pkt_handler_data *)malloc(
                 sizeof(struct tapi_pkt_handler_data));
    if (i_data == NULL)
        return TE_RC(TE_TAPI, ENOMEM);
    
    i_data->user_callback = cb;
    i_data->user_data = cb_data;
 
    return tapi_eth_recv_start(ta_name, sid, arp_csap, pattern,
                               (cb != NULL) ? eth_frame_callback : NULL,
                               (cb != NULL) ? i_data : NULL, timeout, num);
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
        info->rc = ENOMEM;
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
            info->rc = ENOMEM;
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
        return EINVAL;

    memset(&info, 0, sizeof(info));
    
    rc = tapi_arp_recv_start(ta_name, sid, arp_csap, pattern,
                             arp_frame_callback, &info, timeout, *num);
    
    if (rc != 0)
    {
        ERROR("tapi_arp_recv_start() returns %X", rc);
        return rc;
    }
    /* Wait until all the packets received or timeout */
    rc = rcf_ta_trrecv_wait(ta_name, arp_csap, &num_tmp);

    if (rc != 0 || info.rc != 0)
    {
        int i;
        
        ERROR("rcf_ta_trrecv_wait() returns %X, info.rc %X", rc, info.rc);
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
int
tapi_arp_prepare_template(const tapi_arp_frame_t *frame, asn_value **templ)
{
    asn_value *traffic_templ;
    asn_value *frame_tmpl;
    asn_value *asn_pdus, *asn_pdu;
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

    traffic_templ = asn_init_value(ndn_traffic_template);
    asn_pdus = asn_init_value(ndn_generic_pdu_sequence);
    asn_pdu = asn_init_value(ndn_generic_pdu);

    frame_tmpl = ndn_eth_plain_to_packet(&(frame->eth_hdr));
    if (frame_tmpl == NULL)
    {
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
        uint8_t   *arp_pkt;
        size_t     arp_pkt_len;

        arp_pkt = tapi_arp_create_bin_arp_pkt(&(frame->arp_hdr),
                                              frame->data,
                                              frame->data_len,
                                              &arp_pkt_len);
        if (arp_pkt == NULL)
            return ENOMEM;

        rc = asn_write_value_field(traffic_templ, arp_pkt, arp_pkt_len,
                                   "payload.#bytes");
        free(arp_pkt);
    }


    if (rc != 0)
    {
        ERROR("Cannot create traffic template %x\n", rc);
        return rc;
    }

    *templ = traffic_templ;
    return 0;
}

/* See the description in tapi_arp.h */
int
tapi_arp_prepare_pattern_eth_only(uint8_t *src_mac, uint8_t *dst_mac,
                                  asn_value **pattern)
{
    asn_value            *frame_hdr;
    ndn_eth_header_plain  eth_hdr;
    int                   syms;
    int                   rc;

    if (pattern == NULL)
        return EINVAL;

    memset(&eth_hdr, 0, sizeof(eth_hdr));

    if (src_mac != NULL)
        memcpy(eth_hdr.src_addr, src_mac, sizeof(eth_hdr.src_addr));
    if (dst_mac != NULL)
        memcpy(eth_hdr.dst_addr, dst_mac, sizeof(eth_hdr.dst_addr));

    eth_hdr.eth_type_len = ETHERTYPE_ARP;

    frame_hdr = ndn_eth_plain_to_packet(&eth_hdr);
    if (frame_hdr == NULL)
        return ENOMEM;

    if (src_mac == NULL)
    {
        rc = asn_free_subvalue(frame_hdr, "src-addr");
        WARN("asn_free_subvalue() returns %x\n", rc);
    }
    if (dst_mac == NULL)
    {
        rc = asn_free_subvalue(frame_hdr, "dst-addr");
        WARN("asn_free_subvalue() returns %x\n", rc);
    }

    rc = asn_parse_value_text("{{ pdus { eth:{ }}}}",
                              ndn_traffic_pattern,
                              pattern, &syms);
    if (rc != 0)
    {
        ERROR("asn_parse_value_text fails %x\n", rc);
        return rc;
    }
    rc = asn_write_component_value(*pattern,
                                   frame_hdr, "0.pdus.0.#eth");
    if (rc != 0)
    {
        ERROR("asn_write_component_value fails %x\n", rc);
        return rc;
    }

    return 0;
}

/* See the description in tapi_arp.h */
int
tapi_arp_prepare_pattern_with_arp(uint8_t *eth_src_mac,
                                  uint8_t *eth_dst_mac,
                                  uint16_t *op_code,
                                  uint8_t *snd_hw_addr,
                                  uint8_t *snd_proto_addr,
                                  uint8_t *tgt_hw_addr,
                                  uint8_t *tgt_proto_addr,
                                  asn_value **pattern)
{
    int               rc;
    asn_value        *eth_pattern;
    uint8_t          *arp_pkt;
    uint8_t          *pkt_mask;
    size_t            arp_pkt_len;
    tapi_arp_frame_t  arp_frame;
    uint8_t           zero_mac[ETH_ALEN] = { 0, };
    uint8_t           zero_ip[sizeof(struct in_addr)] = { 0, };
    uint8_t           all_one_mac[ETH_ALEN] =
                          { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
    uint8_t           all_one_ip[sizeof(struct in_addr)] = 
                          { 0xff, 0xff, 0xff, 0xff };
    size_t            const_hdr_part_len;
    
    if ((rc = tapi_arp_prepare_pattern_eth_only(eth_src_mac, eth_dst_mac,
                                                &eth_pattern)) != 0)
    {
        ERROR("tapi_arp_prepare_pattern_eth_only() returns %x", rc);
        return rc;
    }

    TAPI_ARP_FILL_HDR(&arp_frame, (op_code != NULL) ? *op_code : 0,
                      snd_hw_addr, snd_proto_addr,
                      tgt_hw_addr, tgt_proto_addr);

    /* Create binary ARP header */
    arp_pkt = tapi_arp_create_bin_arp_pkt(&(arp_frame.arp_hdr),
                                          NULL, 0, &arp_pkt_len);
    if (arp_pkt == NULL)
    {
        ERROR("Cannot create binary ARP header");
        return ENOMEM;
    }

    /* Create 'mask' for ethernet payload, which contains ARP header */
    TAPI_ARP_FILL_HDR(&arp_frame, (op_code == NULL) ?  0x0000 : 0xffff,
                      snd_hw_addr == NULL ? zero_mac : all_one_mac,
                      snd_proto_addr == NULL ? zero_ip : all_one_ip,
                      tgt_hw_addr == NULL ? zero_mac : all_one_mac,
                      tgt_proto_addr == NULL ? zero_mac : all_one_mac);

    pkt_mask = tapi_arp_create_bin_arp_pkt(&(arp_frame.arp_hdr),
                                           NULL, 0, &arp_pkt_len);
    if (pkt_mask == NULL)
    {
        ERROR("Cannot create binary mask for ARP header");
        free(arp_pkt);
        return ENOMEM;
    }
    /*
     * Set the first 6 bytes of mask with 0xff - we expect exact matching
     * for constant fields
     */
    const_hdr_part_len = sizeof(arp_frame.arp_hdr.hard_type) + 
                         sizeof(arp_frame.arp_hdr.proto_type) +
                         sizeof(arp_frame.arp_hdr.hard_size) + 
                         sizeof(arp_frame.arp_hdr.proto_size);

    assert(const_hdr_part_len < arp_pkt_len);
    assert(const_hdr_part_len == 6);

    memset(pkt_mask, 0xff, const_hdr_part_len);

    {
        size_t  i;
        char buf[1024] = { 0, };

        for (i = 0; i < arp_pkt_len; i++)
        {
            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
                     "%02x ", pkt_mask[i]);
        }
        WARN("MASK is %s", buf);
    }
    
    if (rc == 0)
        rc = asn_write_value_field(eth_pattern, arp_pkt, arp_pkt_len,
                                   "0.payload.#mask.v"); 
    if (rc == 0)
        rc = asn_write_value_field(eth_pattern, pkt_mask, arp_pkt_len,
                                   "0.payload.#mask.m");

    if (rc != 0)
    {
        ERROR("Cannot create ARP header pattern: %X", rc);
        return rc;
    }

    *pattern = eth_pattern;

    free(pkt_mask);
    free(arp_pkt);

    return 0;
}


/* Static routines */

/**
 * Create ARP header in binary format based on 
 * the 'tapi_arp_hdr_t' data structure.
 *
 * @param arp_hdr   ARP packet header
 * @param data      Pointer to the raw data that should go after ARP header
 * @param data_len  The length of the data
 * @param pkt_len   Binary ARP packet length (OUT)
 *
 * @return Pointer to the binary data formed, or NULL if there is not enough
 *         memory to allocate.
 */
static uint8_t *
tapi_arp_create_bin_arp_pkt(const tapi_arp_hdr_t *arp_hdr,
                            const uint8_t *data, size_t data_len,
                            size_t *pkt_len)
{
    uint8_t   *arp_pkt;
    uint32_t   arp_pkt_len;
    uint16_t  short_var;
    uint8_t  *arp_pkt_start;

    if (arp_hdr == NULL || pkt_len == NULL)
    {
        assert(0);
        return NULL;
    }

    assert((data == NULL && data_len == 0) ||
           (data != NULL && data_len != 0));

    /* Allocate memory under ARP packet */
#define ARP_FIELD_SIZE(fld_) sizeof(arp_hdr->fld_)
    arp_pkt_len = ARP_FIELD_SIZE(hard_type) + ARP_FIELD_SIZE(proto_type) +
                  ARP_FIELD_SIZE(hard_size) + ARP_FIELD_SIZE(proto_size) +
                  ARP_FIELD_SIZE(op_code) + arp_hdr->hard_size * 2 +
                  arp_hdr->proto_size * 2 + data_len;
#undef ARP_FIELD_SIZE

    if ((arp_pkt_start = arp_pkt = (uint8_t *)malloc(arp_pkt_len)) == NULL)
        return NULL;

#define ARP_FILL_SHORT_VAR(fld_) \
    do {                                                \
        short_var = htons(arp_hdr->fld_);               \
        memcpy(arp_pkt, &short_var, sizeof(short_var)); \
        arp_pkt += sizeof(short_var);                   \
    } while (0)

    ARP_FILL_SHORT_VAR(hard_type);
    ARP_FILL_SHORT_VAR(proto_type);

    *(arp_pkt++) = arp_hdr->hard_size;
    *(arp_pkt++) = arp_hdr->proto_size;

    ARP_FILL_SHORT_VAR(op_code);

#undef ARP_FILL_SHORT_VAR

#define ARP_FILL_ARRAY(fld_, size_fld_) \
   do {                                \
        memcpy(arp_pkt, arp_hdr->fld_, \
               arp_hdr->size_fld_);    \
        arp_pkt += arp_hdr->size_fld_; \
    } while (0)

    ARP_FILL_ARRAY(snd_hw_addr, hard_size);
    ARP_FILL_ARRAY(snd_proto_addr, proto_size);
    ARP_FILL_ARRAY(tgt_hw_addr, hard_size);
    ARP_FILL_ARRAY(tgt_proto_addr, proto_size);

#undef ARP_FILL_ARRAY
    if (data != NULL)
    {
        memcpy(arp_pkt, data, data_len);
        arp_pkt += data_len;
    }

    assert((arp_pkt - arp_pkt_start) == (int)arp_pkt_len);

    *pkt_len = arp_pkt_len;

    return arp_pkt_start;
}
