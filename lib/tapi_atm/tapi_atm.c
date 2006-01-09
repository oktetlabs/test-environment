/** @file
 * @brief Tester API for ATM CSAP
 *
 * Implementation of Tester API for ATM CSAP.
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
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
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id: tapi_arp.c 20870 2005-11-14 13:04:46Z arybchik $
 */ 

#define TE_LGR_USER     "TAPI ATM"

#include "te_config.h"

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H 
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif

#include "te_stdint.h"
#include "te_errno.h"
#include "logger_api.h"
#include "rcf_api.h"
#include "ndn.h"
#include "ndn_socket.h"
#include "ndn_atm.h"
#include "tapi_tad.h"

#include "tapi_atm.h"

#include "tapi_test.h"


/* See the description in tapi_atm.h */
te_errno 
tapi_atm_csap_layer(ndn_atm_type     type,
                    const uint16_t  *vpi,
                    const uint16_t  *vci,
                    te_bool         *congestion,
                    te_bool         *clp,
                    asn_value      **atm_layer)
{
    if (atm_layer == NULL)
    {
        ERROR("Location for created ASN.1 value have to be provided");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    *atm_layer = asn_init_value(ndn_atm_csap);
    if (*atm_layer == NULL)
    {
        ERROR("Failed to initialize ASN.1 value for CSAP ATM layer");
        return TE_RC(TE_TAPI, TE_ENOMEM);
    }

    CHECK_RC(asn_write_int32(*atm_layer, type, "type"));

    if (vpi != NULL)
        CHECK_RC(asn_write_int32(*atm_layer, *vpi, "vpi.#plain"));
    if (vci != NULL)
        CHECK_RC(asn_write_int32(*atm_layer, *vci, "vci.#plain"));
    if (congestion != NULL)
        CHECK_RC(asn_write_int32(*atm_layer, *congestion,
                                 "congestion.#plain"));
    if (clp != NULL)
        CHECK_RC(asn_write_int32(*atm_layer, *clp, "clp.#plain"));

    return 0;
}

/* See the description in tapi_atm.h */
te_errno 
tapi_aal5_csap_layer(const uint8_t  *cpcs_uu,
                     const uint8_t  *cpi,
                     asn_value     **aal5_layer)
{
    if (aal5_layer == NULL)
    {
        ERROR("%s(): Location for created ASN.1 value have to be provided",
              __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    *aal5_layer = asn_init_value(ndn_aal5_csap);
    if (*aal5_layer == NULL)
    {
        ERROR("Failed to initialize ASN.1 value for CSAP AAL5 layer");
        return TE_RC(TE_TAPI, TE_ENOMEM);
    }

    if (cpcs_uu != NULL)
        CHECK_RC(asn_write_int32(*aal5_layer, *cpcs_uu, "cpcs-uu.#plain"));
    if (cpi != NULL)
        CHECK_RC(asn_write_int32(*aal5_layer, *cpi, "cpi.#plain"));

    return 0;
}


/* See the description in tapi_atm.h */
te_errno 
tapi_atm_csap_create(const char     *ta_name,
                     int             sid,
                     int             fd,
                     ndn_atm_type    type,
                     const uint16_t *vpi,
                     const uint16_t *vci,
                     te_bool        *congestion,
                     te_bool        *clp,
                     csap_handle_t  *csap)
{
    te_errno    rc;
    asn_value  *nds = NULL;
    asn_value  *layer = NULL;
    asn_value  *gen_layer = NULL;

    if (ta_name == NULL)
    {
        ERROR("%s(): TA name have to be specified", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    if (fd < 0)
    {
        ERROR("%s(): Valid file descriptor have to be specified",
              __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    if (csap == NULL)
    {
        ERROR("%s(): Location for created CSAP handle have to be "
              "provided", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    nds = asn_init_value(ndn_csap_spec);
    if (nds == NULL)
    {
        ERROR("%s(): Failed to initialize CSAP NDS", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_ENOMEM);
    }

    rc = tapi_atm_csap_layer(type, vpi, vci, congestion, clp, &layer);
    if (rc != 0)
    {
        ERROR("%s(): Failed to create CSAP ATM layer specification: %r",
              __FUNCTION__, rc);
        asn_free_value(nds);
        return rc;
    }
    CHECK_NOT_NULL(gen_layer = asn_init_value(ndn_generic_csap_level));
    CHECK_RC(asn_write_component_value(gen_layer, layer, "#atm"));
    rc = asn_insert_indexed(nds, gen_layer, 0, "");
    if (rc != 0)
    {
        ERROR("%s(): Failed to insert ATM layer in CSAP NDS: %r",
              __FUNCTION__, rc);
        asn_free_value(layer);
        asn_free_value(nds);
        return rc;
    }
    layer = NULL;

    CHECK_NOT_NULL(layer = asn_init_value(ndn_socket_csap));
    CHECK_RC(asn_write_int32(layer, fd, "type.#file-descr"));
    CHECK_NOT_NULL(gen_layer = asn_init_value(ndn_generic_csap_level));
    CHECK_RC(asn_write_component_value(gen_layer, layer, "#socket"));
    rc = asn_insert_indexed(nds, gen_layer, 1, "");
    if (rc != 0)
    {
        ERROR("%s(): Failed to insert Socket layer in CSAP NDS: %r",
              __FUNCTION__, rc);
        asn_free_value(layer);
        asn_free_value(nds);
        return rc;
    }
    layer = NULL;

    rc = tapi_tad_csap_create(ta_name, sid, "atm.socket", nds, csap);
    if (rc != 0)
    {
        ERROR("%s(): Failed to create 'atm.socket' CSAP: %r",
              __FUNCTION__, rc);
    }

    asn_free_value(nds);

    return rc;
}

/* See the description in tapi_atm.h */
te_errno 
tapi_aal5_atm_csap_create(const char     *ta_name,
                          int             sid,
                          int             fd,
                          ndn_atm_type    type,
                          const uint16_t *vpi,
                          const uint16_t *vci,
                          te_bool        *congestion,
                          te_bool        *clp,
                          const uint8_t  *cpcs_uu,
                          const uint8_t  *cpi,
                          csap_handle_t  *csap)
{
    te_errno    rc;
    asn_value  *nds = NULL;
    asn_value  *layer = NULL;
    asn_value  *gen_layer = NULL;

    if (ta_name == NULL)
    {
        ERROR("%s(): TA name have to be specified", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    if (fd < 0)
    {
        ERROR("%s(): Valid file descriptor have to be specified",
              __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    if (csap == NULL)
    {
        ERROR("%s(): Location for created CSAP handle have to be "
              "provided", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    nds = asn_init_value(ndn_csap_spec);
    if (nds == NULL)
    {
        ERROR("%s(): Failed to initialize CSAP NDS", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_ENOMEM);
    }

    rc = tapi_aal5_csap_layer(cpcs_uu, cpi, &layer);
    if (rc != 0)
    {
        ERROR("%s(): Failed to create CSAP AAL5 layer specification: %r",
              __FUNCTION__, rc);
        asn_free_value(nds);
        return rc;
    }
    CHECK_NOT_NULL(gen_layer = asn_init_value(ndn_generic_csap_level));
    CHECK_RC(asn_write_component_value(gen_layer, layer, "#aal5"));
    rc = asn_insert_indexed(nds, gen_layer, -1, "");
    if (rc != 0)
    {
        ERROR("%s(): Failed to insert AAL5 layer in CSAP NDS: %r",
              __FUNCTION__, rc);
        asn_free_value(layer);
        asn_free_value(nds);
        return rc;
    }
    layer = NULL;

    rc = tapi_atm_csap_layer(type, vpi, vci, congestion, clp, &layer);
    if (rc != 0)
    {
        ERROR("%s(): Failed to create CSAP ATM layer specification: %r",
              __FUNCTION__, rc);
        asn_free_value(nds);
        return rc;
    }
    CHECK_NOT_NULL(gen_layer = asn_init_value(ndn_generic_csap_level));
    CHECK_RC(asn_write_component_value(gen_layer, layer, "#atm"));
    rc = asn_insert_indexed(nds, gen_layer, -1, "");
    if (rc != 0)
    {
        ERROR("%s(): Failed to insert ATM layer in CSAP NDS: %r",
              __FUNCTION__, rc);
        asn_free_value(layer);
        asn_free_value(nds);
        return rc;
    }
    layer = NULL;

    CHECK_NOT_NULL(layer = asn_init_value(ndn_socket_csap));
    CHECK_RC(asn_write_int32(layer, fd, "type.#file-descr"));
    CHECK_NOT_NULL(gen_layer = asn_init_value(ndn_generic_csap_level));
    CHECK_RC(asn_write_component_value(gen_layer, layer, "#socket"));
    rc = asn_insert_indexed(nds, gen_layer, -1, "");
    if (rc != 0)
    {
        ERROR("%s(): Failed to insert Socket layer in CSAP NDS: %r",
              __FUNCTION__, rc);
        asn_free_value(layer);
        asn_free_value(nds);
        return rc;
    }
    layer = NULL;

    rc = tapi_tad_csap_create(ta_name, sid, "aal5.atm.socket", nds, csap);
    if (rc != 0)
    {
        ERROR("%s(): Failed to create 'aal5.atm.socket' CSAP: %r",
              __FUNCTION__, rc);
    }

    asn_free_value(nds);

    return rc;
}


/* See the description in tapi_atm.h */
te_errno
tapi_atm_simple_template(const uint8_t   *gfc,
                         const uint16_t  *vpi,
                         const uint16_t  *vci,
                         const uint8_t   *payload_type,
                         te_bool         *clp,
                         size_t           pld_len,
                         const uint8_t   *pld,
                         asn_value      **tmpl)
{
    asn_value  *atm_hdr;
    asn_value  *asn_pdus, *asn_pdu;
    uint8_t     payload[ATM_PAYLOAD_LEN];

    if (pld_len > ATM_PAYLOAD_LEN)
    {
        ERROR("Too long (%u) ATM cell payload", (unsigned)pld_len);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    if (tmpl == NULL)
    {
        ERROR("%s(): Location for created traffic tempalte have to be "
              "provided", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    
    CHECK_NOT_NULL(*tmpl = asn_init_value(ndn_traffic_template));

    CHECK_NOT_NULL(asn_pdus = asn_init_value(ndn_generic_pdu_sequence));
    CHECK_NOT_NULL(asn_pdu = asn_init_value(ndn_generic_pdu));
    CHECK_NOT_NULL(atm_hdr = asn_init_value(ndn_atm_header));

    if (gfc != NULL)
        CHECK_RC(asn_write_int32(atm_hdr, *gfc, "gfc.#plain"));
    if (vpi != NULL)
        CHECK_RC(asn_write_int32(atm_hdr, *vpi, "vpi.#plain"));
    if (vci != NULL)
        CHECK_RC(asn_write_int32(atm_hdr, *vci, "vci.#plain"));
    if (payload_type != NULL)
        CHECK_RC(asn_write_int32(atm_hdr, *payload_type,
                                 "payload-type.#plain"));
    if (clp != NULL)
        CHECK_RC(asn_write_int32(atm_hdr, *clp, "clp.#plain"));

    CHECK_RC(asn_write_component_value(asn_pdu, atm_hdr, "#atm"));
    CHECK_RC(asn_insert_indexed(asn_pdus, asn_pdu, 0, ""));
    CHECK_RC(asn_write_component_value(*tmpl, asn_pdus, "pdus"));

    memcpy(payload, pld, pld_len);
    memset(payload + pld_len, 0, ATM_PAYLOAD_LEN - pld_len);
    CHECK_RC(asn_write_value_field(*tmpl, payload, sizeof(payload),
                                   "payload.#bytes"));

    return 0;
}


#if 0
/* Forward declaration of static functions */
static uint8_t *tapi_arp_create_bin_arp_pkt(
                    const ndn_arp_header_plain *arp_hdr,
                    const uint8_t *data, size_t data_len,
                    size_t *pkt_len);


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
int
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

    return tapi_eth_prepare_pattern(src_mac, dst_mac, &eth_type, pattern);
}

/* See the description in tapi_arp.h */
int
tapi_arp_prepare_pattern_with_arp(const uint8_t *eth_src_mac,
                                  const uint8_t *eth_dst_mac,
                                  const uint16_t *opcode,
                                  const uint8_t *snd_hw_addr,
                                  const uint8_t *snd_proto_addr,
                                  const uint8_t *tgt_hw_addr,
                                  const uint8_t *tgt_proto_addr,
                                  asn_value **pattern)
{
    int               rc;
    asn_value        *eth_pattern;
    uint8_t          *arp_pkt;
    uint8_t          *pkt_mask;
    size_t            arp_pkt_len;
    tapi_arp_frame_t  arp_frame;
    uint8_t           zero_mac[ETHER_ADDR_LEN] = { 0, };
    uint8_t           zero_ip[sizeof(struct in_addr)] = { 0, };
    uint8_t           all_one_mac[ETHER_ADDR_LEN] =
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

    TAPI_ARP_FILL_HDR(&arp_frame, (opcode != NULL) ? *opcode : 0,
                      snd_hw_addr, snd_proto_addr,
                      tgt_hw_addr, tgt_proto_addr);

    /* Create binary ARP header */
    arp_pkt = tapi_arp_create_bin_arp_pkt(&(arp_frame.arp_hdr),
                                          NULL, 0, &arp_pkt_len);
    if (arp_pkt == NULL)
    {
        ERROR("Cannot create binary ARP header");
        return TE_ENOMEM;
    }

    /* Create 'mask' for ethernet payload, which contains ARP header */
    TAPI_ARP_FILL_HDR(&arp_frame, (opcode == NULL) ?  0x0000 : 0xffff,
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
        return TE_ENOMEM;
    }
    /*
     * Set the first 6 bytes of mask with 0xff - we expect exact matching
     * for constant fields
     */
    const_hdr_part_len = sizeof(arp_frame.arp_hdr.hw_type) + 
                         sizeof(arp_frame.arp_hdr.proto_type) +
                         sizeof(arp_frame.arp_hdr.hw_size) + 
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
        VERB("MASK is %s", buf);
    }
    
    if (rc == 0)
        rc = asn_write_value_field(eth_pattern, arp_pkt, arp_pkt_len,
                                   "0.payload.#mask.v"); 
    if (rc == 0)
        rc = asn_write_value_field(eth_pattern, pkt_mask, arp_pkt_len,
                                   "0.payload.#mask.m");
    if (rc == 0)
    {
        int val = 0;
        rc = asn_write_int32(eth_pattern, val, "0.payload.#mask.exact-len");
    }

    if (rc != 0)
    {
        ERROR("Cannot create ARP header pattern: %r", rc);
        return rc;
    }

    *pattern = eth_pattern;

    asn_save_to_file(eth_pattern, "/tmp/arp-pattern.asn");

    free(pkt_mask);
    free(arp_pkt);

    return 0;
}


/* Static routines */

/**
 * Create ARP header in binary format based on 
 * the 'ndn_arp_header_plain' data structure.
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
tapi_arp_create_bin_arp_pkt(const ndn_arp_header_plain *arp_hdr,
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
    arp_pkt_len = ARP_FIELD_SIZE(hw_type) + ARP_FIELD_SIZE(proto_type) +
                  ARP_FIELD_SIZE(hw_size) + ARP_FIELD_SIZE(proto_size) +
                  ARP_FIELD_SIZE(opcode) + arp_hdr->hw_size * 2 +
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

    ARP_FILL_SHORT_VAR(hw_type);
    ARP_FILL_SHORT_VAR(proto_type);

    *(arp_pkt++) = arp_hdr->hw_size;
    *(arp_pkt++) = arp_hdr->proto_size;

    ARP_FILL_SHORT_VAR(opcode);

#undef ARP_FILL_SHORT_VAR

#define ARP_FILL_ARRAY(fld_, size_fld_) \
   do {                                \
        memcpy(arp_pkt, arp_hdr->fld_, \
               arp_hdr->size_fld_);    \
        arp_pkt += arp_hdr->size_fld_; \
    } while (0)

    ARP_FILL_ARRAY(snd_hw_addr, hw_size);
    ARP_FILL_ARRAY(snd_proto_addr, proto_size);
    ARP_FILL_ARRAY(tgt_hw_addr, hw_size);
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
#endif
