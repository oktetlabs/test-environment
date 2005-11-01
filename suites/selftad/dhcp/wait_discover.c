/* 
 * Test Package: CableHome Provisioning Test (PSP-01)
 *
 * Copyright (C) 2003 OKTET Ltd., St.-Petersburg, Russia
 * 
 * $Id$
 */

/** @page 
 *
 * @objective Tests TE TAD DHCP support 
 *
 * @type Dummy
 *
 * @requirement Template for Test Specification
 *
 * @reference @ref TS
 *
 * @param fake - fake parameter for template
 *
 * @pre Reader should read main page example
 *
 *
 * Specify @b type of the test (see list of types above). This item
 * may be omitted, if it's completely equal to the same item of the
 * parent test.
 * 
 * Put identifiers of the applicability constrains here. It's assumed
 * that set of subtest requirements is an improper superset of the parent
 * test requirements. Hence this item may be omitted if no additional
 * requirements are used by this test.
 *
 * Put requirement identifier and/or specification identifier/section
 * to be tested. In a case of specification reference should be
 * detailed as much as it possible. Also it's allowed to quote
 * specification/standard in small volume. It's not recommended to repeat
 * parent-test references without any additional details.
 *
 * Describe parameters supported by the test.
 *
 * Use @b pre to specify preconditions of the test.
 *
 * Further describe actions of the test.
 * It's recommended to use present tense to describe the test.
 *
 * Specify all the tests here. Test should be described in enough detail
 * that someone else can understand what it does and implement it.
 *
 * Specify pass/fail criteria: try to specify pattern of the result to
 * be verified. Also timeout values should be defined to wait for expected
 * result (if it's applicable).
 *
 * Use @b post to specify postconditions of the test.
 *
 *
 * @post Reader can write test specification in sources of the test package
 * 
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 */

#define TE_TEST_NAME    "dhcp/wait_discover"

#include "te_config.h"
#include "config.h"

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif 

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include <sys/time.h>
#include <unistd.h>

#include "te_defs.h"
#include "te_errno.h"
#include "conf_api.h"
#include "ndn_dhcp.h"
#include "tapi_test.h"

#include "tapi_dhcp.h"


int
main(int argc, char *argv[])
{
    char   ta[32];
    size_t len = sizeof(ta); 
    char  *pattern_fname;
    int    num = 0;

    csap_handle_t        dhcp_csap = CSAP_INVALID_HANDLE;
    struct dhcp_message *dhcp_msg; 

    TEST_START;

    if (rcf_get_ta_list(ta, &len) != 0)
    {
        VERB("rcf_get_ta_list failed\n");
        return 1;
    }
    VERB("Agent: %s\n", ta);

    if ((rc = tapi_dhcpv4_plain_csap_create(ta, "eth0",
                                            DHCP4_CSAP_MODE_SERVER,
                                            &dhcp_csap)) 

         != 0)
    {
        TEST_FAIL("Cannot create DHCP CSAP rc = %X", rc);
    }

    dhcp_msg = dhcpv4_message_create(DHCPDISCOVER);
    rc = dhcpv4_prepare_traffic_pattern(dhcp_msg, &pattern_fname);

    rc = rcf_ta_trrecv_start(ta, 0, dhcp_csap, pattern_fname, 
                             5000, 1, RCF_TRRECV_COUNT);
    if (rc != 0)
    {
        TEST_FAIL("dhcpv4_message_start_recv returns %X", rc);
    }

    rc = rcf_ta_trrecv_wait(ta, 0, dhcp_csap, NULL, NULL, &num);
    switch (rc)
    {
        case 0:
            INFO("Wait for DHCP message successful, num %d", num);
            break;
        case TE_RC(TE_TAD_CSAP, TE_ETIMEDOUT):
            INFO("Wait for DHCP message timedout");
            break;
        default:
            TEST_FAIL("Wait for DHCP message failed %X", rc);
            break;
    } 
    
    TEST_SUCCESS;

cleanup:

    if (dhcp_csap != CSAP_INVALID_HANDLE)
        rcf_ta_csap_destroy(ta, 0, dhcp_csap);

    TEST_END; 
}


