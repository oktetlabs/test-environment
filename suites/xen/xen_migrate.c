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
 * $Id: xen_migrate.c $
 */


#define TE_TEST_NAME "xen/xen_migrate"

#include "xen_suite.h"
#include "xen.h"

static inline void
test_core(rcf_rpc_server *pco_src, rcf_rpc_server *pco_dst,
          char const *xen_path, char const *dom_u,
          uint8_t const *mac, struct sockaddr const *ip,
          te_bool live_migration)
{
    te_bool  flg = FALSE;
    char     ip_addr[64];

    struct utsname src;
    struct utsname dst;

    rcf_rpc_server *pco = pco_src; /**< Dom0/agent that domU
                                        currently resides on */

    TE_SPRINTF(ip_addr, "%s", inet_ntoa(SIN(ip)->sin_addr));

    /* Get hostnames for src and dst agents/dom0s (used in migration) */
    if (rpc_uname(pco_src, &src) != 0)
        TEST_FAIL("Failed to get utsname from %s", pco_src->ta);

    if (rpc_uname(pco_dst, &dst) != 0)
        TEST_FAIL("Failed to get utsname from %s", pco_dst->ta);

    /* Set xen path on both dom0s */
    if (tapi_cfg_xen_set_path(pco_src->ta, xen_path) != 0)
    {
        TEST_FAIL("Failed to set XEN path to '%s' on %s",
                  xen_path, pco_src->ta);
    }

    if (tapi_cfg_xen_set_path(pco_dst->ta, xen_path) != 0)
    {
        TEST_FAIL("Failed to set XEN path to '%s' on %s",
                  xen_path, pco_dst->ta);
    }

    /* Create and start domU on the src agent/dom0 */
    if (tapi_cfg_xen_create_dom_u(pco_src->ta, dom_u) != 0)
    {
        ERR_FLG("Failed to create '%s' domU on %s", dom_u, pco_src->ta);
        goto cleanup0;
    }

    if (tapi_cfg_xen_dom_u_set_mac_addr(pco_src->ta, dom_u, mac) != 0)
    {
        ERR_FLG("Failed to set '%s' domU MAC address "
                "%02X:%02X:%02X:%02X:%02X:%02X on %s",
                dom_u, mac[0], mac[1], mac[2], mac[3],
                mac[4], mac[5], pco_src->ta);
        goto cleanup1;
    }

    if (tapi_cfg_xen_dom_u_set_ip_addr(pco_src->ta, dom_u, ip) != 0)
    {
        ERR_FLG("Failed to set '%s' domU IP address %s on %s",
                dom_u, ip_addr, pco_src->ta);
        goto cleanup1;
    }

    if (tapi_cfg_xen_dom_u_set_status(pco_src->ta, dom_u,
                                      "running") != 0)
    {
        ERR_FLG("Failed to start '%s' domU on %s", dom_u, pco_src->ta);
        goto cleanup1;
    }

    /* Check that SSH works */
    if (!ssh(pco_src, dom_u, ip_addr))
    {
        ERR_FLG("SSH is failed");
        goto cleanup1;
    }

    /* Migrate from src to dst */
    if (tapi_cfg_xen_dom_u_migrate(pco_src->ta, pco_dst->ta, dom_u,
                                   dst.nodename, live_migration) != 0)
    {
        ERR_FLG("Failed to migrate '%s' domU from %s to %s",
                dom_u, pco_src->ta, pco_dst->ta);
        goto cleanup1;
    }

    pco = pco_dst; /** Dom0/agent where domU currently resides */

    /* Check that SSH works again */
    if (!ssh(pco_dst, dom_u, ip_addr))
    {
        ERR_FLG("SSH is failed");
        goto cleanup1;
    }

    /* Migrate back from dst to src */
    if (tapi_cfg_xen_dom_u_migrate(pco_dst->ta, pco_src->ta, dom_u,
                                   src.nodename, live_migration) != 0)
    {
        ERR_FLG("Failed to migrate '%s' domU from %s to %s",
                dom_u, pco_dst->ta, pco_src->ta);
        goto cleanup1;
    }

    pco = pco_src; /** Dom0/agent where domU currently resides */

    /* And again check that SSH works */
    if (!ssh(pco_src, dom_u, ip_addr))
    {
        ERR_FLG("SSH is failed");
        goto cleanup1;
    }

    if (tapi_cfg_xen_dom_u_set_status(pco_src->ta, dom_u,
                                      "non-running") != 0)
    {
        ERR_FLG("Failed to stop '%s' domU on %s", dom_u, pco_src->ta);
        goto cleanup1;
    }

cleanup1:

    /* Use that dom0/agent where domU currently resides */
    if (tapi_cfg_xen_destroy_dom_u(pco->ta, dom_u) != 0)
        ERR_FLG("Failed to destroy '%s' domU on %s", dom_u, pco->ta);

cleanup0:

    if (tapi_cfg_xen_set_path(pco_src->ta, "") != 0)
        ERR_FLG("Failed to reset XEN path on %s", xen_path, pco_src->ta);

    if (tapi_cfg_xen_set_path(pco_dst->ta, "") != 0)
        ERR_FLG("Failed to reset XEN path on %s", xen_path, pco_dst->ta);

    if (flg)
        TEST_FAIL("There are errors");
}

int
main(int argc, char *argv[])
{
    rcf_rpc_server *pco_iut = NULL;
    rcf_rpc_server *pco_aux = NULL;

    uint8_t mac_iut[ETHER_ADDR_LEN];
    uint8_t mac_aux[ETHER_ADDR_LEN];

    struct sockaddr ip_iut;
    struct sockaddr ip_aux;

    char const *rpc_ifname     = NULL;
    char const *mac_addr       = NULL;
    char const *xen_path       = NULL;
    char const *dom_u          = NULL;
    te_bool     live_migration = FALSE;

    TEST_START;

    TEST_GET_PCO(pco_iut);
    TEST_GET_PCO(pco_aux);

    TEST_GET_STRING_PARAM(rpc_ifname);
    TEST_GET_STRING_PARAM(mac_addr);
    TEST_GET_STRING_PARAM(xen_path);
    TEST_GET_STRING_PARAM(dom_u);
    TEST_GET_BOOL_PARAM(live_migration);

    get_mac_by_mac_string(mac_addr, mac_iut);
    memcpy(mac_aux, mac_iut, sizeof(mac_iut));
    mac_aux[ETHER_ADDR_LEN - 1]++; /** "Generate" another MAC */

    /* Omit CSAP reports that are responses to 'trrecv_get' from log */
    rcf_tr_op_log(FALSE);

    /* Request IP addresses */
    request_ip_addr_via_dhcp(pco_iut, rpc_ifname, mac_iut, &ip_iut);
    request_ip_addr_via_dhcp(pco_aux, rpc_ifname, mac_aux, &ip_aux);

    /* Perform testing both TAs */
    test_core(pco_iut, pco_aux, xen_path, dom_u, mac_iut, &ip_iut,
              live_migration);
    test_core(pco_aux, pco_iut, xen_path, dom_u, mac_aux, &ip_aux,
              live_migration);

    /* Release IP addresses (this operation may be omitted) */
    release_ip_addr_via_dhcp(pco_iut, rpc_ifname, &ip_iut);
    release_ip_addr_via_dhcp(pco_aux, rpc_ifname, &ip_aux);

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
