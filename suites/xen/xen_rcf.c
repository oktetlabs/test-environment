/** @file
 * @brief XEN Test Suite
 *
 * XEN Test Suite
 * 
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
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
 * @author Edward Makarov <Edward.Makarov@oktetlabs.ru>
 * 
 * $Id: xen_rcf.c $
 */

#define TE_TEST_NAME "xen/xen_rcf"

#include "xen_suite.h"
#include "xen.h"

static inline void
test_core(rcf_rpc_server *pco, char const *xen_path, char const *dom_u,
          uint8_t const *mac, struct sockaddr const *ip)
{
    te_bool  flg = FALSE;
    char     ip_addr[64];

    TE_SPRINTF(ip_addr, "%s", inet_ntoa(SIN(ip)->sin_addr));

    if (tapi_cfg_xen_set_path(pco->ta, xen_path) != 0)
    {
        TEST_FAIL("Failed to set XEN path to '%s' on %s",
                  xen_path, pco->ta);
    }

    if (tapi_cfg_xen_create_dom_u(pco->ta, dom_u) != 0)
    {
        ERR_FLG("Failed to create '%s' domU on %s", dom_u, pco->ta);
        goto cleanup0;
    }

    if (tapi_cfg_xen_dom_u_set_mac_addr(pco->ta, dom_u, mac) != 0)
    {
        ERR_FLG("Failed to set '%s' domU MAC address "
                "%02X:%02X:%02X:%02X:%02X:%02X on %s",
                dom_u, mac[0], mac[1], mac[2], mac[3],
                mac[4], mac[5], pco->ta);
        goto cleanup1;
    }

    if (tapi_cfg_xen_dom_u_set_ip_addr(pco->ta, dom_u, ip) != 0)
    {
        ERR_FLG("Failed to set '%s' domU IP address %s on %s",
                dom_u, ip_addr, pco->ta);
        goto cleanup1;
    }

    if (tapi_cfg_xen_dom_u_set_status(pco->ta, dom_u, "running") != 0)
    {
        ERR_FLG("Failed to start '%s' domU on %s", dom_u, pco->ta);
        goto cleanup1;
    }

    if (!ssh(pco, dom_u, ip_addr))
    {
        ERR_FLG("SSH is failed");
        goto cleanup1;
    }

    if (tapi_cfg_xen_dom_u_set_status(pco->ta, dom_u, "non-running") != 0)
    {
        ERR_FLG("Failed to stop '%s' domU on %s", dom_u, pco->ta);
        goto cleanup1;
    }

cleanup1:

    if (tapi_cfg_xen_destroy_dom_u(pco->ta, dom_u) != 0)
        ERR_FLG("Failed to destroy '%s' domU on %s", dom_u, pco->ta);

cleanup0:

    if (tapi_cfg_xen_set_path(pco->ta, "") != 0)
        ERR_FLG("Failed to reset XEN path on %s", xen_path, pco->ta);

    if (flg)
        TEST_FAIL("There are errors");
}

int fun(void)
{
    char ta[32] = { '\0' };
    size_t len = sizeof(ta);
    int sid[32];
    unsigned tas = 0;
    te_errno rc;

    if ((rc = rcf_get_ta_list(ta, &len)) != 0)
    {
        printf("rcf_get_ta_list failed\n");
        ERROR("rcf_get_ta_list failed: %r", rc);
        return 1;
    }

    {
        char *p = ta;

        for (tas = 0; *p != '\0' && tas < 32; p += strlen(p) + 1, tas++)
        {
            char type[16] = { '\0' };
            char val[32768] = { '\0' };

            if ((rc = rcf_ta_name2type(p, type)) != 0)
            {
                printf("rcf_ta_name2type failed\n");
                ERROR("rcf_ta_name2type failed: %r", rc);
                return 1;
            }

            if ((rc = rcf_ta_create_session(p, sid + tas)) != 0)
            {
                printf("rcf_ta_create_session failed\n");
                ERROR("rcf_ta_create_session failed: %r", rc);
                return 1;
            }

            printf("Agent[%u]: '%s', type '%s', session %i\n", tas, p, type, sid[tas]);

            if ((rc = rcf_ta_cfg_get(p, sid[tas], "*", val, sizeof(val))) != 0)
            {
                printf("rcf_ta_cfg_get failed\n");
                ERROR("rcf_ta_cfg_get failed: %r", rc);
                return 1;
            }

            printf("Objects: <%.128s>\n", val);

            if ((rc = rcf_ta_cfg_get(p, sid[tas], "*:*", val, sizeof(val))) != 0)
            {
                printf("rcf_ta_cfg_get failed\n");
                ERROR("rcf_ta_cfg_get failed: %r", rc);
                return 1;
            }

            printf("Instances: <%.128s>\n", val);
        }
    }

    return 0;
}

int
main(int argc, char *argv[])
{
    char b[32] = { '\0' };
    char const *p;
    unsigned int u;

    rcf_rpc_server *pco_iut = NULL;
    rcf_rpc_server *pco_aux = NULL;

    uint8_t mac_iut[ETHER_ADDR_LEN];
    uint8_t mac_aux[ETHER_ADDR_LEN];

    struct sockaddr ip_iut;
    struct sockaddr ip_aux;

    char const *rpc_ifname = NULL;
    char const *mac_addr   = NULL;
    char const *xen_path   = NULL;
    char const *dom_u      = NULL;

    TEST_START;

    TEST_GET_PCO(pco_iut);
    TEST_GET_PCO(pco_aux);

    TEST_GET_STRING_PARAM(rpc_ifname);
    TEST_GET_STRING_PARAM(mac_addr);
    TEST_GET_STRING_PARAM(xen_path);
    TEST_GET_STRING_PARAM(dom_u);

    printf("\n\nStarting test:\n");

    CHECK_RC(rcf_get_ta_list(b, (u = sizeof(b), &u)));
    b[u] = b[u + 1] = '\0';
    for (p = b, u = 0; p < b + sizeof(b) && *p; p += strlen(p) + 1, u++)
        printf("Agt[%u] = '%s'\n", u, p);

    printf("\n");

    printf("rcf_add_ta_unix(\"Agt_C\", \"linux\", \"kili0\", 18007, 0, 0, 0);\n\n");
    CHECK_RC(rcf_add_ta_unix("Agt_C", "linux", "kili0", 18007, 0, 0, 0));

sleep(3);
    CHECK_RC(rcf_get_ta_list(b, (u = sizeof(b), &u)));
    b[u] = b[u + 1] = '\0';
    for (p = b, u = 0; p < b + sizeof(b) && *p; p += strlen(p) + 1, u++)
        printf("Agt[%u] = '%s'\n", u, p);

    printf("\n");

    if (fun() != 0)
        TEST_FAIL("The first part of the test is failed");

    printf("\nContinuing test:\n");

    if (fun() != 0)
        TEST_FAIL("The second part of the test is failed");

#if 0
    get_mac_by_mac_string(mac_addr, mac_iut);
    memcpy(mac_aux, mac_iut, sizeof(mac_iut));
    mac_aux[ETHER_ADDR_LEN - 1]++; /** "Generate" another MAC */

    /* Omit CSAP reports that are responses to 'trrecv_get' from log */
    rcf_tr_op_log(FALSE);

    /* Request IP addresses */
    request_ip_addr_via_dhcp(pco_iut, rpc_ifname, mac_iut, &ip_iut);
    request_ip_addr_via_dhcp(pco_aux, rpc_ifname, mac_aux, &ip_aux);

#if 0
    /* Perform testing both TAs */
    test_core(pco_iut, xen_path, dom_u, mac_iut, &ip_iut);
    test_core(pco_aux, xen_path, dom_u, mac_aux, &ip_aux);
#endif

    /* Release IP addresses (this operation may be omitted) */
    release_ip_addr_via_dhcp(pco_iut, rpc_ifname, &ip_iut);
    release_ip_addr_via_dhcp(pco_aux, rpc_ifname, &ip_aux);
#endif

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
