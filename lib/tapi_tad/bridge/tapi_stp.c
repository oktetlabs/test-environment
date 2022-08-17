/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Proteos, Tester API for Ethernet CSAP.
 *
 * Implementation of Tester API for Bridge STP CSAP.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "TAPI STP"

#include "te_config.h"

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
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_ARP_INET_H
#include <arpa/inet.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include "te_errno.h"
#include "logger_api.h"
#include "rcf_api.h"
#include "ndn_bridge.h"
#include "tapi_ndn.h"
#include "tapi_tad.h"
#include "tapi_stp.h"
#include "tapi_eth.h"

#include "tapi_test.h"


/** Bridge Group Address according to IEEE 802.1D, Table 7.9 */
static uint8_t bridge_group_addr[ETHER_ADDR_LEN] =
    {0x01, 0x80, 0xC2, 0x00, 0x00, 0x00};


static te_errno
tapi_bridge_add_csap_layer(asn_value          **csap_spec,
                           const unsigned int  *proto)
{
    asn_value  *layer;

    CHECK_RC(tapi_tad_csap_add_layer(csap_spec, ndn_bridge_csap,
                                     "#bridge", &layer));

    if (proto != NULL)
        CHECK_RC(asn_write_int32(layer, *proto, "proto-id.#plain"));

    return 0;
}

/**
 * Creates STP CSAP that can be used for sending/receiving
 * Configuration and Notification BPDUs specified in
 * Media Access Control (MAC) Bridges ANSI/IEEE Std. 802.1D,
 * 1998 Edition section 9
 * CSAP will be either "RX" or "TX", it will be specified by
 * local/remote MAC addresses on Ethernet layer.
 *
 * @param ta_name        Test Agent name where CSAP will be created
 * @param sid            RCF session
 * @param ifname         Name of an interface the CSAP is attached to
 *                       (frames are sent/captured from/on this interface)
 * @param own_mac_addr   Default source MAC address  of outgoing BPDUs
 *                       should be NULL for RX CSAP
 * @param peer_mac_addr  Source MAC address of incoming into the CSAP BPDU
 *                       should be NULL for TX CSAP
 * @param stp_csap       Created STP CSAP (OUT)
 *
 * @return Status of the operation
 */
int
tapi_stp_plain_csap_create(const char *ta_name, int sid, const char *ifname,
                           const uint8_t *own_mac_addr,
                           const uint8_t *peer_mac_addr,
                           csap_handle_t *stp_csap)
{
    asn_value    *csap_spec = NULL;
    unsigned int  proto = 0;
    te_errno      rc;

    if ((ifname == NULL) ||
        (own_mac_addr != NULL && peer_mac_addr != NULL))
        return TE_RC(TE_TAPI, TE_EINVAL);

    CHECK_RC(tapi_bridge_add_csap_layer(&csap_spec, &proto));

    CHECK_RC(tapi_eth_add_csap_layer(&csap_spec, ifname, TAD_ETH_RECV_ALL,
                                     (peer_mac_addr != NULL) ?
                                         peer_mac_addr : bridge_group_addr,
                                     (own_mac_addr != NULL) ?
                                         own_mac_addr :
                                     (peer_mac_addr != NULL) ?
                                          bridge_group_addr : NULL,
                                     NULL,
                                     TE_BOOL3_ANY /* tagged/untagged */,
                                     TE_BOOL3_FALSE /* LLC */));

    rc = tapi_tad_csap_create(ta_name, sid, "bridge.eth", csap_spec,
                              stp_csap);

    asn_free_value(csap_spec);

    return rc;
}

/**
 * Sends STP BPDU from the specified CSAP
 *
 * @param ta_name       Test Agent name;
 * @param sid           RCF session identifier;
 * @param eth_csap      CSAP handle;
 * @param templ         Traffic template;
 *
 * @return  Status of the operation
 * @retval TE_EINVAL  This code is returned if "peer_mac_addr" value wasn't
 *                 specified on creating the CSAP and "dst_mac_addr"
 *                 parameter is NULL.
 * @retval 0       BPDU is sent
 */
int
tapi_stp_bpdu_send(const char *ta_name, int sid,
                   csap_handle_t stp_csap,
                   const asn_value *templ)
{
    char tmp_name[] = "/tmp/te_stp_trsend.XXXXXX";
    int  rc;

    if ((ta_name == NULL) || (templ == NULL))
        return TE_RC(TE_TAPI, TE_EINVAL);

    if ((rc = te_make_tmp_file(tmp_name)) != 0)
        return TE_RC(TE_TAPI, rc);

    if ((rc = asn_save_to_file(templ, tmp_name)) != 0)
        return TE_RC(TE_TAPI, rc);

    rc = rcf_ta_trsend_start(ta_name, sid, stp_csap, tmp_name,
                             RCF_MODE_BLOCKING);

    VERB("rc from rcf_trsend_start: %x\n", rc);

    unlink(tmp_name);

    return rc;
}

struct tapi_pkt_handler_data {

    tapi_stp_bpdu_callback user_callback; /**< User callback */

    void   *user_data;      /**< Pointer to the user data passed in
                                 tapi_stp_bpdu_recv_start function */
    int     current_call;   /**< Number of already received packets */
    int     total_num;      /**< Total number of the packets user wants
                                 to receive */
};


void
tapi_bpdu_pkt_handler(const char *fn, void *user_param)
{
    struct tapi_pkt_handler_data *i_data =
        (struct tapi_pkt_handler_data *)user_param;

    struct timeval   timestamp;
    ndn_stp_bpdu_t   stp_bpdu;
    asn_value const *stp_pkt_val;
    asn_value       *frame_val;
    int              rc, syms = 0;

    VERB("pkt handler called");

    if (user_param == NULL)
    {
        ERROR("pkt handler: bad user param");
        return;
    }

    if (++(i_data->current_call) > i_data->total_num)
    {
        ERROR("Number of callback calls is more than "
                  "the number of packets wanted by user");
        assert(0);
        return;
    }

    rc = asn_parse_dvalue_in_file(fn, ndn_raw_packet, &frame_val, &syms);
    if (rc)
    {
        ERROR(
                "parse value from file %s failed, rc %x, syms: %d\n",
                fn, rc, syms);
        return;
    }

    rc = ndn_get_timestamp(frame_val, &timestamp);
    if (rc)
    {
        ERROR("get_timestamp rc: %x", rc);
        return;
    }

    rc = asn_get_descendent(frame_val, (asn_value **)&stp_pkt_val,
                            "pdus.0.#bridge");
    if (rc)
    {
        ERROR("%s(): get_subvalue rc %r", __FUNCTION__, rc);
        return;
    }

    rc = ndn_bpdu_asn_to_plain(stp_pkt_val, &stp_bpdu);
    if (rc)
    {
        ERROR("packet to plain error %x\n", rc);
        i_data->user_callback(NULL, &timestamp, i_data->user_data);
        return;
    }

    i_data->user_callback(&stp_bpdu, &timestamp, i_data->user_data);

    asn_free_value(frame_val);

    if (i_data->current_call == i_data->total_num)
    {
        free(i_data);
    }

    return;
}

#if 0
/**
 * See description in tapi_stp.h
 */
int
tapi_stp_bpdu_recv_start(const char *ta_name, int sid,
                         csap_handle_t stp_csap,
                         const asn_value *pattern,
                         tapi_stp_bpdu_callback callback,
                         void *callback_data,
                         unsigned int timeout, int num)
{
    int  rc;
    char tmp_name[] = "/tmp/te_stp_trrecv.XXXXXX";

    struct tapi_pkt_handler_data *i_data;

    if ((ta_name == NULL) || (pattern == NULL))
        return TE_RC(TE_TAPI, TE_EINVAL);

    if ((i_data = malloc(sizeof(*i_data))) == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);

    i_data->user_callback = callback;
    i_data->user_data = callback_data;
    i_data->current_call = 0;
    i_data->total_num = num;

    if ((rc = te_make_tmp_file(tmp_name)) != 0)
    {
        free(i_data);
        return TE_RC(TE_TAPI, rc);
    }

    rc = asn_save_to_file(pattern, tmp_name);
    if (rc)
    {
        free(i_data);
        return TE_RC(TE_TAPI, rc);
    }

    ERROR("time to wait: %d", timeout);
    rc = rcf_ta_trrecv_start(ta_name, sid, stp_csap, tmp_name,
                             (callback != NULL) ? tapi_bpdu_pkt_handler :
                                                  NULL,
                             (callback != NULL) ? (void *)i_data : NULL,
                             timeout, num);

    unlink(tmp_name);

    return rc;
}
#endif
