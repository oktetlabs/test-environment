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

#include "config.h"
/*
#ifdef HAVE_STDLIB_H
*/
#include <stdlib.h>
/*
#endif
#ifdef HAVE_STRING_H
*/
#include <string.h>
/*
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
*/

#include <stdio.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <unistd.h>

#include "conf_api.h"
#include "tapi_dhcp.h"

#define TEST_FAIL(_args...) \
    do {                               \
        fprintf(stderr, _args);        \
        exit(1);                       \
    } while (0)

int
main()
{
    int rc;
    
    struct dhcp_message *dhcp_req;
    struct dhcp_message *dhcp_ack;
    const struct dhcp_option  *opt;

    csap_handle_t        dhcp_csap;
    csap_handle_t        dhcp_csap_tx;

    struct timeval tv = {10, 0}; /* 10 sec */
    const char *ta_name = "valens";

    uint8_t ps_wan_man_mac[MACADDR_LEN];
    uint32_t ps_wan_man_addr;
    uint32_t tftp_serv_addr = inet_addr("192.168.249.2");
    uint32_t dhcp_serv_addr = inet_addr("192.168.249.2");
    uint32_t tod_serv_addr = inet_addr("192.168.249.2");
    const char *ps_cfg_file = "PSP-01-Basic.cfg";
    unsigned int i;

    /* Register on receiving DHCP DISCOVER message from WAN-Man MAC address */
    /*@todo Create a handle to operate with DHCP CSAP (WAN-Man MAC) */
    if ((rc = dhcpv4_plain_csap_create(ta_name, &dhcp_csap, "eth0")) 
         != 0)
    {
        TEST_FAIL("Cannot create DHCP CSAP rc = %d\n", rc);
    }
#if 1
    if ((rc = dhcpv4_message_start_recv(ta_name, dhcp_csap, DHCPDISCOVER)) != 0)
    {
        TEST_FAIL("dhcpv4_message_start_recv returns %d\n", rc);
    }

    /* Reboot the PS */
    printf("Before sleep\n");
    fflush(stdout);
    /* Wait for DHCP DISCOVER message (use CSAP created before) */
    sleep(10);
    printf("Before traffic stop\n");
    fflush(stdout);
#endif
    
    return 0;
}


