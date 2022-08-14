/** @file
 * @brief Test Environment
 *
 * Simple RAW Ethernet test: receive first broadcast ethernet frame
 * from first ('eth0') network card.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "te_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "te_stdint.h"
#include "te_errno.h"
#include "rcf_api.h"

#include "ndn_eth.h"
#include "ndn_bridge.h"
#include "tapi_stp.h"

#undef TE_LOG_LEVEL
#define TE_LOG_LEVEL 255
#include "logger_ten.h"

#define TE_LOG_LEVEL 255

#define CHECK_STATUS(_rc, x...) \
    do if (_rc) {\
        printf (x);\
        VERB(x);\
        if (bpdu_csap) \
            rcf_ta_csap_destroy(ta, sid, bpdu_csap);\
        if (bpdu_listen_csap) \
            rcf_ta_csap_destroy(ta, sid, bpdu_listen_csap);\
        return (_rc);\
    } while (0)



int
main()
{
    char ta[32];
    int  len = sizeof(ta);
    int  sid;

    VERB("Starting test\n");
    if (rcf_get_ta_list(ta, &len) != 0)
    {
        fprintf(stderr, "rcf_get_ta_list failed\n");
        return 1;
    }
    VERB(" Using agent: %s\n", ta);

    /* Type test */
    {
        char type[16];
        if (rcf_ta_name2type(ta, type) != 0)
        {
            fprintf(stderr, "rcf_ta_name2type failed\n");
            VERB("rcf_ta_name2type failed\n");
            return 1;
        }
        VERB("TA type: %s\n", type);
    }

    /* Session */
    {
        if (rcf_ta_create_session(ta, &sid) != 0)
        {
            fprintf(stderr, "rcf_ta_create_session failed\n");
            VERB("rcf_ta_create_session failed\n");
            return 1;
        }
        VERB("Test: Created session: %d\n", sid);
    }

    /* CSAP tests */
    do {
        int rc, syms = 4;
        uint8_t payload [2000];
        int p_len = 100; /* for test */
        csap_handle_t bpdu_csap = 0, bpdu_listen_csap = 0;
        asn_value *pattern;
        asn_value *template;
        asn_value *asn_pdus;
        asn_value *asn_pdu;
        asn_value *asn_bpdu;
        asn_value *asn_eth_hdr;
        char eth_device[] = "eth0";

        uint8_t own_addr[6] = {0x01,0x02,0x03,0x04,0x05,0x06};
        uint8_t rem_addr[6] = {0x01,0x02,0x03,0x04,0x05,0x06};

        uint8_t root_id[] = {0x12, 0x13, 0x14, 0x15, 0, 0, 0, 0};
        ndn_stp_bpdu_t plain_bpdu;

        memset (&plain_bpdu, 0, sizeof (plain_bpdu));

        plain_bpdu.cfg.root_path_cost = 10;
        memcpy (plain_bpdu.cfg.root_id, root_id, sizeof(root_id));

        asn_bpdu = ndn_bpdu_plain_to_asn(&plain_bpdu);
        CHECK_STATUS(asn_bpdu == NULL,
                     "Create ASN bpdu from plain fails\n");

        template = asn_init_value(ndn_traffic_template);
        asn_pdus = asn_init_value(ndn_generic_pdu_sequence);
        asn_pdu = asn_init_value(ndn_generic_pdu);
        asn_eth_hdr = asn_init_value(ndn_eth_header);

        rc = asn_write_component_value(asn_pdu, asn_bpdu, "#bridge");
        if (rc == 0)
            rc = asn_insert_indexed(asn_pdus, asn_pdu, 0, "");

        asn_pdu = asn_init_value(ndn_generic_pdu);
        if (rc == 0)
            rc = asn_write_component_value(asn_pdu, asn_eth_hdr, "#eth");
        if (rc == 0)
            rc = asn_insert_indexed(asn_pdus, asn_pdu, 1, "");

        if (rc == 0)
            rc = asn_write_component_value(template, asn_pdus, "pdus");

        CHECK_STATUS(rc, "Template create failed with rc %x\n", rc);

        rc = tapi_stp_plain_csap_create(ta, sid, eth_device, own_addr,
                                        NULL, &bpdu_csap);
        CHECK_STATUS(rc, "csap create failed with rc %x\n", rc);


        rc = tapi_stp_plain_csap_create(ta, sid, eth_device, NULL,
                                        own_addr, &bpdu_listen_csap);
        CHECK_STATUS(rc, "csap listen create failed with rc %x\n", rc);

        rc = asn_parse_value_text(
                "{{ pdus {  bridge:{ version-id plain:0}, eth:{ }}}}",
                            ndn_traffic_pattern, &pattern, &syms);
        CHECK_STATUS(rc, "parse pattern fails:  %x on sym %d\n", rc, syms);


        rc = tapi_tad_trrecv_start(ta, sid, bpdu_listen_csap, pattern,
                                   20000, 10, RCF_TRRECV_COUNT);
        CHECK_STATUS(rc, "bpdu recv start rc: %x\n", rc);


        rc = tapi_stp_bpdu_send(ta, sid, bpdu_csap, template);
        CHECK_STATUS(rc, "BDPU send failed with rc %x\n", rc);

        sleep(1);

        rc = rcf_ta_trrecv_stop(ta, sid, bpdu_listen_csap,
                                NULL, NULL, &syms);
        printf ("trrecv stop rc: %x, num: %d\n", rc, syms);
        CHECK_STATUS(rc, "trrecv stop rc: %x, num of pkts: %d\n", rc, syms);

        rc = rcf_ta_csap_destroy(ta, sid, bpdu_csap);
        CHECK_STATUS(rc, "csap destroy failed with rc %x\n", rc);

        rcf_ta_csap_destroy(ta, sid, bpdu_listen_csap);

    } while (0);

    return 0;
}
