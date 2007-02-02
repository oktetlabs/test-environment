/** @file
 * @brief Test Environment
 *
 * Check VLAN support in Configurator
 * 
 * Copyright (C) 2007 Test Environment authors (see file AUTHORS
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
 * $Id: ip4_send_udp.c 34433 2006-12-23 14:00:41Z arybchik $
 */

/** @page ipstack-ip4_send_udp Send UDP/IP4 datagram via udp.ip4.eth CSAP and receive it via DGRAM socket
 *
 * @objective Check that udp.ip4.eth CSAP can send UDP datagrams
 *            with user-specified ports and checksum.
 *
 * @param host_csap     TA with CSAP
 *
 * @par Scenario:
 *
 * -# Create udp.ip4.eth CSAP on @p pco_csap. 
 *
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 */

#ifndef DOXYGEN_TEST_SPEC

#define TE_TEST_NAME    "vlans"

#include "te_config.h"

#if HAVE_STRING_H
#include <string.h>
#endif

#include "tad_common.h"
#include "rcf_rpc.h"
#include "asn_usr.h"
#include "ndn_eth.h"
#include "ndn_ipstack.h"
#include "tapi_ndn.h"
#include "tapi_udp.h"
#include "tapi_env.h"
#include "tapi_rpcsock_macros.h"
#include "tapi_test.h"
#include "tapi_rpc_params.h"
#include "tapi_cfg_base.h"

int
main(int argc, char *argv[])
{
    tapi_env_host              *host_csap = NULL;
    const struct if_nameindex  *csap_if;

    char           *if_name;
    char           *vlan_ifn;
    cfg_handle     *handles;
    unsigned int    n_handles;

    TEST_START; 

    rc = cfg_find_pattern("/agent:Agt_A/interface:*/vlans:*", 
                          &n_handles, &handles); 
    RING("find vlans on Agt_A rc %r, n: %d", rc, n_handles);
    {
        int i;
        char *name;
        for (i = 0; i < n_handles; i++)
        {
            cfg_get_inst_name(handles[i], &name);
            RING("found vlan '%s' on Agt_A", name);
        }
    }

    rc = cfg_find_pattern("/agent:Agt_A/interface:*", 
                          &n_handles, &handles); 
    RING("find interfaces on Agt_A rc %r, n: %d", rc, n_handles);

    {
        int i;
        char *name;
        for (i = 0; i < n_handles; i++)
        {
            cfg_get_inst_name(handles[i], &name);
            RING("found interfaces '%s' on Agt_A", name);
        }
    }
    if (n_handles < 1)
        TEST_FAIL("There is no any accessible interface on Agt_A");


    cfg_get_inst_name(handles[n_handles - 1], &if_name);
    rc = cfg_add_instance_fmt(NULL, CVT_NONE, NULL,
                              "/agent:Agt_A/interface:%s/vlans:12",
                              if_name);
    RING("add vlan on Agt_A:%s rc %r", if_name, rc);
    if (rc != 0)
        TEST_FAIL("Add VLAN failed");


    rc = cfg_find_pattern("/agent:Agt_A/interface:*/vlans:*", 
                          &n_handles, &handles); 
    RING("After add vlan: find vlans on eth1 on Agt_A rc %r, n: %d",
         rc, n_handles);
    {
        int i;
        char *name;
        for (i = 0; i < n_handles; i++)
        {
            cfg_get_inst_name(handles[i], &name);
            RING("found vlan '%s' on Agt_A", name);
        }
    }

    rc = cfg_find_pattern("/agent:Agt_A/interface:*", 
                          &n_handles, &handles); 
    RING("After add vlan: find interfaces on Agt_A rc %r, n: %d", rc, n_handles);

    {
        int i;
        char *name;
        for (i = 0; i < n_handles; i++)
        {
            cfg_get_inst_name(handles[i], &name);
            RING("found interfaces '%s' on Agt_A", name);
        }
    }

    rc = cfg_find_pattern("/agent:Agt_A/rsrc:*", 
                          &n_handles, &handles); 
    RING("find resources on Agt_A rc %r, n: %d", rc, n_handles);
    {
        cfg_val_type type = CVT_STRING;
        char *name = NULL;

        rc = cfg_get_instance_fmt(&type, &name,
                                  "/agent:Agt_A/interface:eth1/vlans:12/ifname:");
        RING("read ifname rc %r, %s", rc, name);
    }

    rc = cfg_del_instance_fmt(FALSE, "/agent:Agt_A/interface:%s/vlans:12",
                              if_name);
    if (rc != 0)
        TEST_FAIL("remove VLAN failed %r", rc);

    rc = tapi_cfg_base_if_add_vlan("Agt_A", if_name, 10, &vlan_ifn);
    if (rc != 0)
        TEST_FAIL("add VLAN with TAPI failed %r", rc);
    RING("ifname of created VLAN: %s", vlan_ifn);



    {
        char cfg_if_oid[200];
        cfg_handle new_addr;
        struct sockaddr_in sa;

        sa.sin_family = AF_INET;
        sa.sin_addr.s_addr = inet_addr("192.168.0.1");
        sprintf(cfg_if_oid, "/agent:Agt_A/interface:%s", vlan_ifn);

        rc = tapi_cfg_base_add_net_addr(cfg_if_oid, SA(&sa),
                           24, FALSE, &new_addr);
        if (rc != 0)
            TEST_FAIL("add IP address on VLAN failed %r", rc);
    }

    rc = tapi_cfg_base_if_del_vlan("Agt_A", if_name, 10);
    if (rc != 0)
        TEST_FAIL("remove VLAN with TAPI failed %r", rc);

    TEST_SUCCESS;

cleanup:


    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
