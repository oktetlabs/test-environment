/** @file
 * @brief Test Environment
 *
 * Simple RCF test
 * 
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS
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
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 * 
 * $Id$
 */

#define TE_TEST_NAME    "eth/vlan_recv_send"

#define TEST_START_VARS         TEST_START_ENV_VARS
#define TEST_START_SPECIFIC     TEST_START_ENV
#define TEST_END_SPECIFIC       TEST_END_ENV

#include "te_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "te_defs.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "rcf_api.h"

#include "tapi_test.h"
#include "tapi_env.h"

#include "ndn_eth.h"
#include "tapi_eth.h" 

#include "logger_api.h"


static void
local_eth_frame_handler(const asn_value *packet, int layer,
                        const ndn_eth_header_plain *header, 
                        const uint8_t *payload, uint16_t plen, 
                        void *userdata)
{
    int  i;
    char buffer[100];
    
    UNUSED(packet);
    UNUSED(layer);
    UNUSED(payload);
    UNUSED(userdata);

    RING("Ethernet frame received");
    sprintf(buffer, "dst: ");
    for (i = 0; i < ETHER_ADDR_LEN; i ++ )
        sprintf(buffer, "%02x ", header->dst_addr[i]);
    sprintf(buffer, "src: ");
    for (i = 0; i < ETHER_ADDR_LEN; i ++ )
        sprintf(buffer, "%02x ", header->src_addr[i]);
    RING("addrs: %s", buffer);

    RING("len_type: 0x%x = %d", header->len_type, header->len_type);

    if (header->is_tagged)
    {
        RING("cfi:     %d", (int)header->cfi);
        RING("prio:    %d", (int)header->priority);
        RING("vlan-id: %d", (int)header->vlan_id);
    }

    RING("payload len: %d", plen);
}

int
main(int argc, char *argv[])
{
    tapi_env_host              *host_a = NULL;
    const struct if_nameindex  *if_a = NULL;
    const void                 *hwaddr = NULL;

    csap_handle_t   send_csap = CSAP_INVALID_HANDLE;
    csap_handle_t   recv_csap = CSAP_INVALID_HANDLE;

    ndn_eth_header_plain    plain_hdr;
    uint16_t                eth_type = ETH_P_IP;
    asn_value              *asn_eth_hdr = NULL;
    uint8_t                 payload[100];

    asn_value  *template = NULL;
    asn_value  *csap_spec = NULL;
    asn_value  *pattern = NULL;

    int         num;


    TEST_START;
    TEST_GET_HOST(host_a);
    TEST_GET_IF(if_a);
    TEST_GET_LINK_ADDR(hwaddr);


    /* Prepare plain representation of Ethernet frame header */
    memset(&plain_hdr, 0, sizeof(plain_hdr));
    memcpy(plain_hdr.dst_addr, hwaddr, ETHER_ADDR_LEN);  
    plain_hdr.len_type = ETH_P_IP; 
    plain_hdr.is_tagged = 1;
    plain_hdr.vlan_id = 16;

    /* Convert plain representation of Ethernet header to ASN.1 value.*/
    CHECK_NOT_NULL(asn_eth_hdr = ndn_eth_plain_to_packet(&plain_hdr));

    /* Create traffic template with Ethernet PDU */
    CHECK_RC(tapi_eth_add_pdu(&template, NULL, FALSE, NULL, NULL, NULL,
                              TE_BOOL3_ANY /* tagged/untagged */,
                              TE_BOOL3_ANY /* Ethernet2/LLC */));

    /* Rewrite Ethernet PDU by ASN.1 value created by plain data */
    CHECK_RC(asn_write_component_value(template, asn_eth_hdr,
                                       "pdus.0.#eth"));

    /* Add some payload to traffic template */
    memset(payload, 0, sizeof(payload));
    CHECK_RC(asn_write_value_field(template, payload, 
                                   sizeof(payload),
                                   "payload.#bytes"));

    RING("Ethernet frame template to send created successfully");

    /* Create traffic pattern with Ethernet PDU */
    CHECK_RC(tapi_eth_add_pdu(&pattern, NULL, TRUE, hwaddr, NULL, NULL,
                              TE_BOOL3_ANY /* tagged/untagged */,
                              TE_BOOL3_ANY /* Ethernet2/LLC */));
    RING("Ethernet frame pattern to receive created successfully");


    /* Create send CSAP */
    CHECK_RC(tapi_eth_csap_create(host_a->ta, 0, if_a->if_name,
                                  TAD_ETH_RECV_NO,
                                  hwaddr, NULL, &eth_type,
                                  &send_csap));

    /* Create receive CSAP */
    CHECK_RC(tapi_eth_add_csap_layer(&csap_spec, if_a->if_name,
                                     TAD_ETH_RECV_ALL,
                                     NULL, NULL, NULL,
                                     TE_BOOL3_ANY /* tagged/untagged */,
                                     TE_BOOL3_ANY /* Ethernet2/LLC */));
    CHECK_RC(tapi_tad_csap_create(host_a->ta, 0, "eth", csap_spec,
                                  &recv_csap));
    asn_free_value(csap_spec); csap_spec = NULL;


    /* Start receiver */
    CHECK_RC(tapi_tad_trrecv_start(host_a->ta, 0, recv_csap, pattern, 
                                   TAD_TIMEOUT_INF, 1 /* one packet */,
                                   RCF_TRRECV_PACKETS));

    /* Start sender */
    CHECK_RC(tapi_tad_trsend_start(host_a->ta, 0, send_csap, template,
                                   RCF_MODE_BLOCKING));

    /* Sleep 100 milliseconds */
    MSLEEP(100);

    /* Stop receiver with processing of received frames by callback */
    num = 0;
    CHECK_RC(tapi_tad_trrecv_stop(host_a->ta, 0, recv_csap,
                                  tapi_eth_trrecv_cb_data(
                                      local_eth_frame_handler, NULL),
                                  &num));

    if (num <= 0)
        TEST_FAIL("No received packets");

    TEST_SUCCESS;

cleanup:
    if (host_a != NULL)
    {
        CLEANUP_CHECK_RC(rcf_ta_csap_destroy(host_a->ta, 0, send_csap));
        CLEANUP_CHECK_RC(rcf_ta_csap_destroy(host_a->ta, 0, recv_csap));
    }

    asn_free_value(csap_spec); csap_spec = NULL;
    asn_free_value(template); template = NULL;
    asn_free_value(pattern); pattern = NULL;

    TEST_END; 
}
