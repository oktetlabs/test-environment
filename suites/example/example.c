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

#include "te_config.h"

#define ETHER_ADDR_LEN 6
#define TE_TEST_NAME "example"
#define TEST_START_VARS unsigned mtu; \
            char ta[32]; \
            char eth0_oid[128]; \
            int num_interfaces; \
            cfg_handle *interfaces; \
            struct sockaddr *addr; \
            char *dirname; \
            cfg_val_type type = CVT_INTEGER; \
            int flag; \
            int len; \
            int shell_tid; \
            struct sockaddr_in host_addr; \
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
#include "rcf_rpc.h"
#include "tapi_rpc.h"
#include "tapi_rpc_stdio.h"
#include "tapi_rpcsock_macros.h"

int
main(int argc, char *argv[])
{
    rcf_rpc_server *srv = NULL;
    int rc1;
    FILE *f;
    cfg_handle new_client;
    TEST_START;

    len = sizeof(ta);
    CHECK_RC(rcf_get_ta_list(ta, &len));
    RING("Agent is %s", ta);
    CHECK_RC(cfg_set_instance_fmt(CVT_INTEGER, 0, "/agent:%s/radiusserver:", ta));
    CHECK_RC(cfg_get_instance_fmt(&type, &flag, "/agent:%s/radiusserver:", ta));
    CHECK_RC(cfg_add_instance_fmt(&new_client, CVT_NONE, NULL, "/agent:%s/radiusserver:/client:%s", 
                                  ta, "127.0.0.1"));
    CHECK_RC(cfg_set_instance_fmt(CVT_STRING, "secret", "/agent:%s/radiusserver:/client:%s/secret:", 
                                  ta, "127.0.0.1"));
    CHECK_RC(cfg_add_instance_fmt(&new_client, CVT_INTEGER, 1, "/agent:%s/radiusserver:/user:%s", 
                                  ta, "artem"));
    CHECK_RC(cfg_set_instance_fmt(CVT_STRING, "User-Password=\"kuku\"", 
                                  "/agent:%s/radiusserver:/user:%s/check:", 
                                  ta, "artem"));
    CHECK_RC(cfg_set_instance_fmt(CVT_STRING, "Session-Timeout=100,Framed-IP-Address=192.168.1.1", 
                                  "/agent:%s/radiusserver:/user:%s/challenge-attrs:", 
                                  ta, "artem"));
    CHECK_RC(cfg_set_instance_fmt(CVT_STRING, "Framed-IP-Address=192.168.1.1", 
                                  "/agent:%s/radiusserver:/user:%s/accept-attrs:", 
                                  ta, "artem"));
    sleep(60);
    CHECK_RC(cfg_set_instance_fmt(CVT_INTEGER, (void *)1, "/agent:%s/radiusserver:", ta));
    CHECK_RC(cfg_get_instance_fmt(&type, &flag, "/agent:%s/radiusserver:", ta));
    CHECK_RC(cfg_set_instance_fmt(type, 0, "/agent:%s/radiusserver:", ta));
    CHECK_RC(cfg_get_instance_fmt(&type, &flag, "/agent:%s/radiusserver:", ta));

    RING("RADIUS server is %s", flag ? "on" : "off");

    TEST_SUCCESS;

cleanup:
    TEST_END;
}
