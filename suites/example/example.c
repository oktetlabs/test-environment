/** @file
 * @brief Test Environment
 *
 * A test example
 * 
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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
 * @author Artem V. Andreev <Artem.Andreev@oktetlabs.ru>
 * 
 * $Id$
 */

/** @page example An example test
 *
 *  @objective Example smells example whether it's called example or not.
 *
 */

#include "config.h"

#define ETHER_ADDR_LEN 6
#define TE_TEST_NAME "example"
#define TEST_START_VARS unsigned mtu; \
            char ta[32]; \
            char eth0_oid[128]; \
            int num_interfaces; \
            cfg_handle *interfaces; \
            struct sockaddr *addr; \
            char *dirname; \
            cfg_val_type type = CVT_ADDRESS; \
            int flag; \
            int len; \
            uint8_t mac[ETHER_ADDR_LEN + 1];

#include <stdarg.h>
#include <stdio.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include "tapi_test.h"
#include "tapi_cfg_base.h"
#include "tapi_sockaddr.h"
#include "rcf_api.h"

int
main(int argc, char *argv[])
{
    TEST_START;
    CHECK_RC(rcf_get_ta_list(ta, &len));
    INFO("Agent is %s", ta);
    CHECK_RC(cfg_get_instance_fmt(&type, &addr, "/agent:%s/dns:", ta)); 
    RING("DNS is %s", sockaddr2str(addr));
    type = CVT_INTEGER;
    CHECK_RC(cfg_get_instance_fmt(&type, &flag, "/agent:%s/dnsserver:", ta));
    RING("DNS server is %s", flag ? "running" : "not running");
    type = CVT_STRING;
    CHECK_RC(cfg_get_instance_fmt(&type, &dirname, "/agent:%s/dnsserver:/directory:", ta));
    RING("DNS server directory is %s", dirname);
    free(addr);
    free(dirname);
    type = CVT_ADDRESS;
    CHECK_RC(cfg_get_instance_fmt(&type, &addr, "/agent:%s/dnsserver:/forwarder:", ta));
    RING("DNS server forwarder is %s", sockaddr2str(addr));
    free(addr);
    type = CVT_INTEGER;
    CHECK_RC(cfg_get_instance_fmt(&type, &flag, "/agent:%s/dnsserver:/recursive:", ta));
    RING("DNS server is %s", flag ? "recursive" : "not recursive");
    flag = 1;
    CHECK_RC(cfg_set_instance_fmt(type, &flag, "/agent:%s/dnsserver:/recursive:", ta));


    snprintf(eth0_oid, sizeof(eth0_oid) - 1, "/agent:%s/interface:*", ta);
    CHECK_RC(cfg_find_pattern(eth0_oid, &num_interfaces, &interfaces));
    {
        int i;
        char *oid;
        
        for (i = 0; i < num_interfaces; i++)
        {
            CHECK_RC(cfg_get_oid_str(interfaces[i], &oid));

            CHECK_RC(tapi_cfg_base_if_get_mtu(oid, &mtu));
            RING("MTU for %s is %u", oid, mtu);
            free(oid);
        }
    }
    free(interfaces);
#if 0
    CHECK_RC(rcf_ta_put_file(ta, 0, "/home/artem/test.txt", \
                             "/home/artem/tmp/test.txt"));
#endif
    TEST_SUCCESS;

cleanup:
    TEST_END;
}
