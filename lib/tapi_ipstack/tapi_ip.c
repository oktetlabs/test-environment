/** @file
 * @brief Test API for TAD. ipstack CSAP
 *
 * Implementation of Test API
 * 
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS in the
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
 * @author: Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER "TAPI IPv4"

#include "te_config.h"

#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include <assert.h>

#include <netinet/in.h>
#include <netinet/ether.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include "rcf_api.h"
#include "conf_api.h"

#include "logger_api.h"

#include "tapi_ip.h"
#include "ndn_ipstack.h"
#include "ndn_eth.h"


typedef struct tapi_ip4_eth_pkt_handler_data {
    ip4_callback  callback;
    void         *user_data;
} tapi_ip4_eth_pkt_handler_data;


/* 
 * Pkt handler for IP packets 
 */
static void
ip4_pkt_handler(asn_value *pkt, void *user_param)
{
    tapi_ip4_eth_pkt_handler_data *cb_data =
        (tapi_ip4_eth_pkt_handler_data *)user_param;

    const asn_value    *ip_pdu;
    tapi_ip4_packet_t   plain_pkt;
    size_t              len;

    int rc = 0;

    int32_t hdr_field;

    if (user_param == NULL)
    {
        WARN("%s called with NULL user param", __FUNCTION__);
        return;
    }

    if (cb_data->callback == NULL)
    {
        WARN("%s called with NULL user cb", __FUNCTION__);
        return; 
    }

#define CHECK_FAIL(msg_...) \
    do {                        \
        if (rc != 0)            \
        {                       \
            ERROR(msg_);        \
            return;             \
        }                       \
    } while (0)

    rc = asn_get_subvalue(pkt, &ip_pdu, "pdus.0.#ip4");
    CHECK_FAIL("%s(): get IP4 PDU fails, rc = %r",
              __FUNCTION__, rc);

    len = sizeof(plain_pkt.src_addr);
    rc = ndn_du_read_plain_oct(ip_pdu, NDN_TAG_IP4_SRC_ADDR, 
                               (uint8_t *)&(plain_pkt.src_addr), &len);
    CHECK_FAIL("%s(): get IP4 src fails, rc = %r",
              __FUNCTION__, rc);

    len = sizeof(plain_pkt.dst_addr);
    rc = ndn_du_read_plain_oct(ip_pdu, NDN_TAG_IP4_DST_ADDR, 
                               (uint8_t *)&(plain_pkt.dst_addr), &len);
    CHECK_FAIL("%s(): get IP4 dst fails, rc = %r",
              __FUNCTION__, rc);

    rc = ndn_du_read_plain_int(ip_pdu, NDN_TAG_IP4_PROTOCOL, &hdr_field);
    CHECK_FAIL("%s(): get IP4 proto fails, rc = %r", __FUNCTION__, rc);
    plain_pkt.ip_proto = hdr_field;

    len = plain_pkt.pld_len = asn_get_length(pkt, "payload");
    plain_pkt.payload = malloc(len);

    rc = asn_read_value_field(pkt, plain_pkt.payload, &len, "payload");

    cb_data->callback(&plain_pkt, cb_data->user_data);

    free(plain_pkt.payload);
#undef CHECK_FAIL
}

/* see description in tapi_ip.h */
int
tapi_ip4_eth_csap_create(const char *ta_name, int sid, const char *eth_dev,
                         const uint8_t *loc_mac_addr,
                         const uint8_t *rem_mac_addr,
                         in_addr_t      loc_ip4_addr, 
                         in_addr_t      rem_ip4_addr,
                         csap_handle_t *ip4_csap)
{
    int         rc = 0;
    char        csap_fname[100] = "/tmp/te_ip4_csap.XXXXXX";
    asn_value_p csap_ip4_level = NULL;
    asn_value_p csap_eth_level = NULL;
    asn_value_p csap_spec = NULL;
    asn_value_p csap_level_spec = NULL;

    unsigned short ip_eth = 0x0800;

    csap_spec       = asn_init_value(ndn_csap_spec);
    csap_level_spec = asn_init_value(ndn_generic_csap_level);
    csap_ip4_level  = asn_init_value(ndn_ip4_csap);
    csap_eth_level  = asn_init_value(ndn_eth_csap);


    do {
        if ((loc_ip4_addr != 0) && 
            (rc = asn_write_value_field(csap_ip4_level,
                                        &loc_ip4_addr, 4,
                                        "local-addr.#plain")) != 0)
            break;

        if ((rem_ip4_addr != 0) &&
            (rc = asn_write_value_field(csap_ip4_level,
                                        &rem_ip4_addr, 4,
                                        "remote-addr.#plain")) != 0)
            break;

        rc = asn_write_component_value(csap_level_spec,
                                       csap_ip4_level, "#ip4");
        if (rc != 0) break;

        rc = asn_insert_indexed(csap_spec, csap_level_spec, 0, "");
        if (rc != 0) break;

        asn_free_value(csap_level_spec);

        csap_level_spec = asn_init_value(ndn_generic_csap_level);

        if (eth_dev)
            rc = asn_write_value_field(csap_eth_level, 
                                eth_dev, strlen(eth_dev),
                                    "device-id.#plain"); 
        if (rc != 0) break;
        rc = asn_write_int32(csap_eth_level, ip_eth,
                             "eth-type.#plain");
        if (loc_mac_addr) 
            rc = asn_write_value_field(csap_eth_level,
                                       loc_mac_addr, 6,
                                       "local-addr.#plain");
        if (rem_mac_addr) 
            rc = asn_write_value_field(csap_eth_level,
                                       rem_mac_addr, 6,
                                       "remote-addr.#plain");

        if (rc != 0) break;
        rc = asn_write_component_value(csap_level_spec,
                                       csap_eth_level, "#eth"); 
        if (rc != 0) break;

        rc = asn_insert_indexed(csap_spec, csap_level_spec, 1, "");
        if (rc != 0) break;

        mktemp(csap_fname);
        rc = asn_save_to_file(csap_spec, csap_fname);
        VERB("TAPI: ip4.eth create csap, save to file %s, rc: %x\n",
                csap_fname, rc);
        if (rc != 0) break;


        rc = rcf_ta_csap_create(ta_name, sid, "ip4.eth", 
                                csap_fname, ip4_csap); 
    } while (0);

    asn_free_value(csap_spec);
    asn_free_value(csap_ip4_level);
    asn_free_value(csap_eth_level);
    asn_free_value(csap_level_spec);

    unlink(csap_fname); 

    return TE_RC(TE_TAPI, rc);
}


/* See description in tapi_ip.h */
te_errno
tapi_ip4_eth_recv_start(const char      *ta_name,
                        int              sid,
                        csap_handle_t    csap,
                        const uint8_t   *src_mac_addr,
                        const uint8_t   *dst_mac_addr,
                        in_addr_t        src_ip4_addr,
                        in_addr_t        dst_ip4_addr,
                        unsigned int     timeout,
                        unsigned int     num,
                        rcf_trrecv_mode  mode)
{
    char        template_fname[] = "/tmp/te_ip4_eth_recv.XXXXXX";
    te_errno    rc;
    FILE       *f;

    const uint8_t      *b;

    mktemp(template_fname);

    f = fopen(template_fname, "w+");
    if (f == NULL)
    {
        ERROR("fopen() of %s failed(%d)", template_fname, errno);
        return TE_OS_RC(TE_TAPI, errno); /* return system errno */
    }

    fprintf(f, "{{ pdus { ip4:{" );
    
    if (src_ip4_addr != INADDR_ANY)
    {
        b = (uint8_t *)&(src_ip4_addr);
        fprintf(f, "src-addr plain:'%02x %02x %02x %02x'H", 
                b[0], b[1], b[2], b[3]);
    }

    if (src_ip4_addr && dst_ip4_addr)
        fprintf(f, ",\n   ");

    if (dst_ip4_addr != INADDR_ANY)
    {
        b = (uint8_t *)&(dst_ip4_addr);
        fprintf(f, " dst-addr plain:'%02x %02x %02x %02x'H", 
                b[0], b[1], b[2], b[3]);
    }

    fprintf(f, "   },\n" ); /* closing  'ip4' */
    fprintf(f, "   eth:{eth-type plain:2048");

    if ((b = src_mac_addr))
        fprintf(f, ",\n    "
                "src-addr plain:'%02x %02x %02x %02x %02x %02x'H", 
                b[0], b[1], b[2], b[3], b[4], b[5]);

    if ((b = dst_mac_addr))
        fprintf(f, ",\n    "
                "dst-addr plain:'%02x %02x %02x %02x %02x %02x'H", 
                b[0], b[1], b[2], b[3], b[4], b[5]);
    fprintf(f, "}\n");
    fprintf(f, "}}}\n");
    fclose(f);

    rc = rcf_ta_trrecv_start(ta_name, sid, csap, template_fname,
                             timeout, num, mode);

    unlink(template_fname); 

    return rc;
}

/* see description in tapi_ip.h */
tapi_tad_trrecv_cb_data *
tapi_ip4_eth_trrecv_cb_data(ip4_callback  callback,
                            void         *user_data)
{
    tapi_ip4_eth_pkt_handler_data   *cb_data;
    tapi_tad_trrecv_cb_data         *res;

    cb_data = (tapi_ip4_eth_pkt_handler_data *)calloc(1, sizeof(*cb_data));
    if (cb_data == NULL)
    {
        ERROR("%s(): failed to allocate memory", __FUNCTION__);
        return NULL;
    }
    cb_data->callback = callback;
    cb_data->user_data = user_data;
    
    res = tapi_tad_trrecv_make_cb_data(ip4_pkt_handler, cb_data);
    if (res == NULL)
        free(cb_data);

    return res;
}

/* see description in tapi_ip.h */
int
tapi_ip4_pdu(in_addr_t  src_ip4_addr, in_addr_t  dst_ip4_addr,
             tapi_ip_frag_spec_t *fragments, size_t num_frags,
             int ttl, int protocol, asn_value **result_value)
{
    int          rc, syms; 
    unsigned int fr_i; 
    asn_value   *ip4_pdu;

    tapi_ip_frag_spec_t *frag;

    if (result_value == NULL)
        return TE_RC(TE_TAPI, TE_EWRONGPTR);

    if ((rc = asn_parse_value_text("ip4:{}", ndn_generic_pdu,
                                   result_value, &syms)) != 0)
        return TE_RC(TE_TAPI, rc);

    if ((rc = asn_get_choice_value(*result_value,
                                   (const asn_value **)&ip4_pdu,
                                   NULL, NULL))
            != 0)
    {
        ERROR("%s(): get ip4 pdu subvalue failed %r", __FUNCTION__, rc);
        return TE_RC(TE_TAPI, rc);
    }

    if (src_ip4_addr != INADDR_ANY &&
        (rc = ndn_du_write_plain_oct(ip4_pdu, NDN_TAG_IP4_SRC_ADDR,
                                     (uint8_t *)&src_ip4_addr, 4)) != 0)
    {
        ERROR("%s(): set IP4 src failed %r", __FUNCTION__, rc);
        return TE_RC(TE_TAPI, rc);
    }

    if (dst_ip4_addr != INADDR_ANY &&
        (rc = ndn_du_write_plain_oct(ip4_pdu, NDN_TAG_IP4_DST_ADDR,
                                     (uint8_t *)&dst_ip4_addr, 4)) != 0)
    {
        ERROR("%s(): set IP4 dst failed %r", __FUNCTION__, rc);
        return TE_RC(TE_TAPI, rc);
    }

    if (ttl >= 0 &&
        (rc = ndn_du_write_plain_int(ip4_pdu, NDN_TAG_IP4_TTL,
                                     ttl)) != 0)
    {
        ERROR("%s(): set IP4 ttl failed %r", __FUNCTION__, rc);
        return TE_RC(TE_TAPI, rc);
    }

    if (protocol >= 0 &&
        (rc = ndn_du_write_plain_int(ip4_pdu, NDN_TAG_IP4_PROTOCOL,
                                     protocol)) != 0)
    {
        ERROR("%s(): set IP4 protocol failed %r", __FUNCTION__, rc);
        return TE_RC(TE_TAPI, rc);
    }

    if (fragments != NULL)
    {
        asn_value *frag_seq = asn_init_value(ndn_ip4_frag_seq);

        for (fr_i = 0, frag = fragments; fr_i < num_frags; fr_i++, frag++)
        {
            asn_value *frag_val = asn_init_value(ndn_ip4_frag_spec);

            asn_write_int32(frag_val, frag->hdr_offset,  "hdr-offset");
            asn_write_int32(frag_val, frag->real_offset, "real-offset");
            asn_write_int32(frag_val, frag->hdr_length,  "hdr-length");
            asn_write_int32(frag_val, frag->real_length, "real-length");
            asn_write_bool(frag_val,  frag->more_frags,  "more-frags");
            asn_write_bool(frag_val,  frag->dont_frag,   "dont-frag");

            asn_insert_indexed(frag_seq, frag_val, fr_i, "");
        }

        asn_put_child_value(ip4_pdu, frag_seq, 
                            PRIVATE, NDN_TAG_IP4_FRAGMENTS);
        /* do NOT free frag_seq here! */
    }

    return 0; 
}

/* see description in tapi_ip.h */
int
tapi_ip4_eth_pattern_unit(const uint8_t *src_mac_addr,
                          const uint8_t *dst_mac_addr,
                          in_addr_t      src_ip4_addr,
                          in_addr_t      dst_ip4_addr,
                          asn_value **pattern_unit)
{
    int rc = 0;
    int num = 0;

    if (pattern_unit == NULL)
        return TE_RC(TE_TAPI, TE_EWRONGPTR);


    rc = asn_parse_value_text("{ pdus { ip4:{}, eth:{}}}", 
                              ndn_traffic_pattern_unit,
                              pattern_unit, &num); 
    if (rc != 0)
    {
        ERROR("%s: parse simple pattern unit fails %X, sym %d",
              __FUNCTION__, rc, num);
    }

#define CHECK_ERROR(dir_, atype_) \
    do {                                                            \
        if (rc != 0)                                                \
        {                                                           \
            ERROR("%s(): write " #dir_ " " #atype_ " addr fails %X",\
                  __FUNCTION__, rc);                                \
            asn_free_value(*pattern_unit);                          \
            *pattern_unit = NULL;                                   \
            return TE_RC(TE_TAPI, rc);                              \
        }                                                           \
    } while (0)

#define FILL_IP4_ADDR(dir_) \
    do {                                                                  \
        if (dir_ ## _ip4_addr != INADDR_ANY)                              \
        {                                                                 \
            rc = asn_write_value_field(*pattern_unit,                     \
                                       &( dir_ ## _ip4_addr),             \
                                       4, "pdus.0." #dir_ "-addr.#plain");\
            CHECK_ERROR(dir_, ip4);                                       \
        }                                                                 \
    } while (0)

#define FILL_MAC_ADDR(dir_) \
    do {                                                                  \
        if (dir_ ## _mac_addr != NULL)                                    \
        {                                                                 \
            rc = asn_write_value_field(*pattern_unit,                     \
                                       dir_ ## _mac_addr,                 \
                                       6, "pdus.1." #dir_ "-addr.#plain");\
            CHECK_ERROR(dir_, mac);                                       \
        }                                                                 \
    } while (0)

    FILL_IP4_ADDR(src);
    FILL_IP4_ADDR(dst); 
    FILL_MAC_ADDR(src);
    FILL_MAC_ADDR(dst);

#undef CHECK_ERROR
#undef FILL_IP4_ADDR
#undef FILL_MAC_ADDR

    return TE_RC(TE_TAPI, rc);
}


/* see description in tapi_ip.h */
int
tapi_ip4_eth_template(const uint8_t *src_mac_addr,
                      const uint8_t *dst_mac_addr,
                      in_addr_t      src_ip4_addr,
                      in_addr_t      dst_ip4_addr,
                      tapi_ip_frag_spec_t *fragments,
                      size_t num_frags,
                      int ttl, int protocol, 
                      const uint8_t *payload,
                      size_t pld_len,
                      asn_value **result_value)
{ 
    int rc, syms;

    asn_value *ip4_pdu;

#define CHECK_RC(msg_...) \
    do {                                \
        if (rc != 0)                    \
        {                               \
            ERROR(msg_);                \
            return TE_RC(TE_TAPI, rc);  \
        }                               \
    } while (0)

    if (result_value == NULL)
        return TE_RC(TE_TAPI, TE_EWRONGPTR); 

    rc = asn_parse_value_text("{ pdus { eth:{} } }",
                              ndn_traffic_template, result_value, &syms); 
    CHECK_RC("%s(): init of traffic template from text failed %X, sym %d",
              __FUNCTION__, rc, syms);

    if ((src_mac_addr != NULL) && 
        ((rc = asn_write_value_field(*result_value,
                                     src_mac_addr, ETHER_ADDR_LEN,
                                     "pdus.0.#eth.src-addr.#plain")) != 0)) 
    {
        ERROR("%s(): src MAC specified, but write error %X", 
              __FUNCTION__, rc);
        return TE_RC(TE_TAPI, rc);
    } 

    if ((dst_mac_addr != NULL) && 
        ((rc = asn_write_value_field(*result_value,
                                     dst_mac_addr, ETHER_ADDR_LEN,
                                     "pdus.0.#eth.dst-addr.#plain")) != 0)) 
    {
        ERROR("%s(): dst MAC specified, but write error %X", 
              __FUNCTION__, rc);
        return TE_RC(TE_TAPI, rc);
    } 
    rc = asn_write_value_field(*result_value, payload, pld_len,
                               "payload.#bytes");
    CHECK_RC("%s(): write payload error %X", __FUNCTION__, rc);

    rc = tapi_ip4_pdu(src_ip4_addr, dst_ip4_addr,
                      fragments, num_frags, ttl, protocol, &ip4_pdu);
    CHECK_RC("%s(): construct IP4 pdu error %X", __FUNCTION__, rc);

    rc = asn_insert_indexed(*result_value, ip4_pdu, 0, "pdus");
    CHECK_RC("%s(): insert IP4 pdu error %X", __FUNCTION__, rc);


    return 0;
}


