/** @file
 * @brief Proteos, Tester API for Ethernet CSAP.
 *
 * Implementation of Tester API for Ethernet CSAP.
 *
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include "rcf_api.h"
#include "ndn_eth.h"
#include "tapi_eth.h"

#define TE_LGR_USER     "TAPI Ethernet"
#include "logger_api.h"


/**
 * Print ethernet address to the specified file stream
 *
 * @param f     File stream handle
 * @param addr  Pointer to the array with Ethernet MAC address
 *
 * @return nothing.
 */
void
tapi_eth_fprint_mac (FILE *f, const uint8_t *addr)
{
    if ((f != NULL) && (addr != NULL))
    {
        fprintf(f, "'%02x %02x %02x %02x %02x %02x'H",
                addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
    }
}

/**
 * Create common Ethernet CSAP.
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF session
 * @param device        Interface name on TA host.
 * @param remote_addr   Default remote MAC address, may be NULL - in this
 *                      case frames will be sent only dst is specified in
 *                      template, and frames from all src's will be catched
 * @param local_addr    Default local MAC address. may be NULL - in this
 *                      case frames will be sent with src specifed in
 *                      template or native for outgoing device (if not
 *                      present in template).
 *                      If NULL, CSAP will have remote address unconfigured
 *                      and will require it in traffic template.
 * @param eth_type_len  Default Ethernet type/length field.
 *                      See description in IEEE 802.3.
 *                      If NULL, CSAP will have eth-type/len unconfigured
 *                      and will require it in traffic template.
 * @param eth_csap      Identifier of created CSAP (OUT)
 *
 * @return Zero on success, otherwise standard or common TE error code.
 */
int
tapi_eth_csap_create(const char *ta_name, int sid,
                     const char *device,
                     const uint8_t *remote_addr,
                     const uint8_t *local_addr,
                     const uint16_t *eth_type_len,
                     csap_handle_t *eth_csap)
{
    return tapi_eth_csap_create_with_mode (ta_name, sid, device,
            ETH_RECV_ALL & (~ETH_RECV_OUTGOING),
            remote_addr, local_addr, eth_type_len, eth_csap);
}

/**
 * Create common Ethernet CSAP.
 *
 * @param ta_name       Test Agent name;
 * @param sid           RCF session;
 * @param device        Interface name on TA host.
 * @param recv_mode     Receive mode, bit scale defined by elements of
 *                      'enum eth_csap_receive_mode' in ndn_eth.h.
 * @param remote_addr   Default remote MAC address, may be NULL - in this
 *                      case frames will be sent only dst is specified in
 *                      template, and frames from all src's will be
 *                      catched.
 * @param local_addr    Default local MAC address. may be NULL - in this
 *                      case frames will be sent with src specifed in
 *                      template or native for outgoing device (if not
 *                      present in template).
 *                      If NULL, CSAP will have remote address unconfigured
 *                      and will require it in traffic template.
 * @param eth_type_len  Default Ethernet type/length field.
 *                      See description in IEEE 802.3.
 *                      If NULL, CSAP will have eth-type/len unconfigured
 *                      and will require it in traffic template.
 * @param eth_csap      Identifier of created CSAP (OUT).
 *
 * @return zero on success, otherwise standard or common TE error code.
 */
int
tapi_eth_csap_create_with_mode(const char *ta_name, int sid,
                               const char *device,
                               uint8_t recv_mode,
                               const uint8_t *remote_addr,
                               const uint8_t *local_addr,
                               const uint16_t *eth_type_len,
                               csap_handle_t *eth_csap)
{
    int     rc;
    char    tmp_name[100];
    FILE   *f;

    if ((ta_name == NULL) || (device == NULL) || (eth_csap == NULL))
        return TE_RC(TE_TAPI, EINVAL);

    strcpy(tmp_name, "/tmp/te_eth_csap_create.XXXXXX");
    mkstemp(tmp_name);

    VERB("file: %s\n", tmp_name);

    f = fopen(tmp_name, "w+");
    if (f == NULL)
    {
        ERROR("fopen() of %s failed(%d)", tmp_name, errno);
        return TE_RC(TE_TAPI, errno); /* return system errno */
    }

    fprintf(f,    "{ eth:{ device-id   plain:\"%s\",\n", device);
    fprintf(f,    "        receive-mode %d", recv_mode);
    if (local_addr != NULL)
    {
        fprintf(f, ",\n        local-addr plain:");
        tapi_eth_fprint_mac(f, local_addr);
    }
    if (remote_addr != NULL)
    {
        fprintf(f, ",\n        remote-addr plain:");
        tapi_eth_fprint_mac(f, remote_addr);
    }
    if (eth_type_len != NULL)
    {
        fprintf(f, ",\n        eth-type    plain:%d", (int)*eth_type_len);
    }
    fprintf(f, "}}\n");
    fclose(f);

    rc = rcf_ta_csap_create(ta_name, sid, "eth", tmp_name, eth_csap);
    if (rc != 0)
    {
        ERROR("rcf_ta_csap_create() failed(0x%x) on TA %s:%d file %s",
              rc, ta_name, sid, tmp_name);
    }

#if !(TAPI_DEBUG)
    unlink(tmp_name);
#endif

    return rc;
}


/**
 * Create Ethernet CSAP for processing tagged frames.
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF session
 * @param device        Interface name on TA host
 * @param remote_addr   Default remote MAC address, may be NULL - in this
 *                      case frames will be sent only dst is specified in
 *                      template, and frames from all src's will be
 *                      catched.
 * @param local_addr    Default local MAC address. may be NULL - in this
 *                      case frames will be sent with src specifed in
 *                      template or native for outgoing device (if not
 *                      present in template).
 * @param eth_type_len  Default Ethernet type/length field.
 *                      See description in IEEE 802.3.
 * @param cfi           CFI field in VLAN header extension.
 * @param pririty       Priority field in VLAN header extension.
 * @param vlan_id       VLAN identifier field in VLAN header extension.
 * @param eth_csap      Identifier of created CSAP (OUT).
 *
 * @return zero on success, otherwise standard or common TE error code.
 */
int
tapi_eth_tagged_csap_create(const char *ta_name, int sid,
                            const char *device,
                            uint8_t    *remote_addr,
                            uint8_t    *local_addr,
                            uint16_t    eth_type_len,
                            int         cfi,
                            uint8_t     priority,
                            uint16_t    vlan_id,
                            csap_handle_t *eth_csap)
{
    int rc;
    char tmp_name[100];
    FILE *f;

    if (
        (ta_name == NULL) || (device == NULL) ||
        (remote_addr == NULL) || (local_addr == NULL) ||
        (eth_csap == NULL)
       )
        return TE_RC(TE_TAPI, EINVAL);

    strcpy(tmp_name, "/tmp/te_eth_csap_create.XXXXXX");
    mkstemp(tmp_name);
    f = fopen (tmp_name, "w+");
    if (f == NULL)
    {
        ERROR("fopen() of %s failed(%d)", tmp_name, errno);
        return TE_RC(TE_TAPI, errno); /* return system errno */
    }

    fprintf(f,    "{ eth:{ device-id   plain:\"%s\",\n"
                  "        local-addr plain:", device);
    tapi_eth_fprint_mac (f, local_addr);
    fprintf(f, ",\n        remote-addr plain:");
    tapi_eth_fprint_mac (f, remote_addr);
    fprintf(f, ",\n        eth-type    plain:%d,\n", (int)eth_type_len);
    fprintf(f, "        cfi         %s,\n", cfi ? "true" : "false" );
    fprintf(f, "        priority    plain:%d,\n", (int) priority);
    fprintf(f, "        vlan-id     plain:%d}\n}\n", (int)vlan_id);
    fclose(f);

    rc = rcf_ta_csap_create(ta_name, sid, "eth", tmp_name, eth_csap);
    if (rc != 0)
    {
        ERROR("rcf_ta_csap_create() failed(0x%x) on TA %s:%d file %s",
              rc, ta_name, sid, tmp_name);
    }

#if !(TAPI_DEBUG)
    unlink(tmp_name);
#endif

    return rc;
}

static int
tapi_internal_eth_send(const char *ta_name, int sid, csap_handle_t eth_csap,
                       const asn_value *templ, rcf_call_mode_t blk_mode)
{
    int rc;
    char tmp_name[100];

    if ( (ta_name == NULL) || (templ == NULL) )
        return TE_RC(TE_TAPI, EINVAL);

    strcpy(tmp_name, "/tmp/te_eth_trsend.XXXXXX");
    mkstemp(tmp_name);
    rc = asn_save_to_file(templ, tmp_name);
    if (rc)
        return TE_RC(TE_TAPI, rc);

    rc = rcf_ta_trsend_start(ta_name, sid, eth_csap, tmp_name, blk_mode);
    if (rc != 0)
    {
        ERROR("rcf_ta_trsend_start() failed(0x%x) on TA %s:%d CSAP %d "
              "file %s", rc, ta_name, sid, eth_csap, tmp_name);
    }

#if !(TAPI_DEBUG)
    unlink(tmp_name);
#endif

    return rc;
}

/**
 * Sends traffic template from specified CSAP
 * (the template can represent tagged and ordinary frames).
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF session identifier
 * @param eth_csap      CSAP handle
 * @param templ         Traffic template
 *
 * @return Zero on success, otherwise standard or common TE error code.
 */
int tapi_eth_send_start(const char *ta_name, int sid,
                        csap_handle_t eth_csap, const asn_value *templ)
{
    return tapi_internal_eth_send(ta_name, sid, eth_csap, templ,
                                  RCF_MODE_NONBLOCKING);
}

/**
 * Sends traffic template from specified CSAP.
 * (the template can represent tagged and ordinary frames)
 *
 * @param ta_name       Test Agent name;
 * @param sid           RCF session identifier;
 * @param eth_csap      CSAP handle;
 * @param templ         Traffic template;
 *
 * @return zero on success, otherwise standard or common TE error code.
 */
int
tapi_eth_send(const char *ta_name, int sid, csap_handle_t eth_csap,
              const asn_value *templ)
{
    return tapi_internal_eth_send(ta_name, sid, eth_csap, templ,
                                  RCF_MODE_BLOCKING);
}

struct tapi_pkt_handler_data
{
    tapi_eth_frame_callback user_callback;
    void                    *user_data;
};

static void
tapi_eth_pkt_handler(char *fn, void *user_param)
{
    struct tapi_pkt_handler_data *i_data =
        (struct tapi_pkt_handler_data *)user_param;

    ndn_eth_header_plain header;
    int rc, syms = 0;
    asn_value *frame_val;
    asn_value *eth_hdr_val;

    int p_len;
    uint8_t *payload;

    if (user_param == NULL)
    {
        fprintf(stderr, "TAPI ETH, pkt handler: bad user param!!!\n");
        return;
    }

    rc = asn_parse_dvalue_in_file(fn, ndn_raw_packet, &frame_val, &syms);
    if (rc)
    {
        fprintf(stderr,
                "Parse value from file %s failed, rc %x, syms: %d\n",
                fn, rc, syms);
        return;
    }

    eth_hdr_val = asn_read_indexed (frame_val, 0, "pdus");
    if (eth_hdr_val == NULL)
    {
        fprintf (stderr, "tapi eth int cb, read_indexed error\n");
        return;
    }

    rc = ndn_eth_packet_to_plain(eth_hdr_val, &header);
    if (rc)
    {
        fprintf(stderr, "tapi eth int cb, packet to plain error %x\n", rc);
        return;
    }

    p_len = asn_get_length(frame_val, "payload.#bytes");
    if (p_len < 0)
    {
        fprintf (stderr, "tapi eth int cb, get_len error \n");
        return;
    }

    payload = malloc(p_len);
    rc = asn_read_value_field(frame_val, payload, &p_len, "payload.#bytes");

    if (rc)
    {
        fprintf (stderr, "tapi eth int cb, read payload error %x\n", rc);
        return;
    }

    i_data->user_callback(&header, payload, p_len, i_data->user_data);

    free(payload);
    asn_free_value(frame_val);
    asn_free_value(eth_hdr_val);

    return;
}

/**
 * See the description in tapi_eth.h
 */
int
tapi_eth_recv_start(const char *ta_name, int sid,
                    csap_handle_t eth_csap,
                    const asn_value *pattern,
                    tapi_eth_frame_callback cb, void *cb_data,
                    unsigned int timeout, int num)
{
    int    rc;
    char   tmp_name[100];
    struct tapi_pkt_handler_data *i_data;

    if (ta_name == NULL || pattern == NULL)
        return TE_RC(TE_TAPI, EINVAL);

    if ((i_data = malloc(sizeof(*i_data))) == NULL)
        return TE_RC(TE_TAPI, ENOMEM);

    i_data->user_callback = cb;
    i_data->user_data = cb_data;

    strcpy(tmp_name, "/tmp/te_eth_trrecv.XXXXXX");
    mkstemp(tmp_name);
    rc = asn_save_to_file(pattern, tmp_name);
    if (rc)
    {
        free(i_data);
        return TE_RC(TE_TAPI, rc);
    }

    rc = rcf_ta_trrecv_start(ta_name, sid, eth_csap, tmp_name,
                             (cb != NULL) ? tapi_eth_pkt_handler : NULL,
                             (cb != NULL) ? (void *) i_data : NULL,
                             timeout, num);
    if (rc != 0)
    {
        ERROR("rcf_ta_trrecv_start() failed(0x%x) on TA %s:%d CSAP %d "
              "file %s", rc, ta_name, sid, eth_csap, tmp_name);
    }

#if !(TAPI_DEBUG)
    unlink(tmp_name);
#endif

    return rc;
}
