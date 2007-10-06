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
 * $Id: xen_create_destroy.c $
 */


#define TE_TEST_NAME "xen/xen_create_destroy"

#include "xen_suite.h"
#include "xen.h"

static inline void
test_core(rcf_rpc_server *pco_src, rcf_rpc_server *pco_dst,
          char const *xen_path, char const *dom_u,
          struct sockaddr const *ip)
{
    te_bool  flg = FALSE;
    te_errno rc;
    char     ip_addr[64];

    TE_SPRINTF(ip_addr, "%s", inet_ntoa(SIN(ip)->sin_addr));

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

    if (tapi_cfg_xen_create_dom_u(pco_src->ta, dom_u) != 0)
    {
        ERR_FLG("Failed to create '%s' domU on %s", dom_u, pco_src->ta);
        goto cleanup0;
    }

    if ((rc = tapi_cfg_xen_set_path(pco_src->ta, "")) == 0)
    {
        ERR_FLG("XEN path reset attempt in case when '%s' domU exists "
                "succeeded on %s", dom_u, pco_src->ta);
    }

    if (rc != TE_RC(TE_TA_UNIX, TE_EBUSY)) /** Should be TE_EBUSY */
    {
        ERR_FLG("XEN path reset attempt in case when '%s' domU exists "
                "returned wrong error code on %s", dom_u, pco_src->ta);
    }

    if ((rc = tapi_cfg_xen_dom_u_set_status(pco_src->ta, dom_u,
                                            "saved")) == 0)
    {
        ERR_FLG("The \"non-running\" -> \"saved\" transition iattempt "
                "succeeded for '%s' domU on %s", dom_u, pco_src->ta);
        goto cleanup1;
    }

    if (rc != TE_RC(TE_TA_UNIX, TE_EINVAL)) /** Should be TE_EINVAL */
    {
        ERR_FLG("The \"non-running\" -> \"saved\" transition attempt "
                "returned wrong error code for '%s' domU on %s", dom_u,
                pco_src->ta);
        goto cleanup1;
    }

    if ((rc = tapi_cfg_xen_dom_u_migrate(pco_src->ta, pco_dst->ta,
                                         dom_u, ip_addr, FALSE)) == 0)
    {
        ERR_FLG("Non-live migration attempt from %s to %s succeeded for "
                "'%s' domU", pco_src->ta, pco_dst->ta, dom_u);
        goto cleanup1;
    }

    if (rc != TE_RC(TE_TA_UNIX, TE_EINVAL)) /** Should be TE_EINVAL */
    {
        ERR_FLG("Non-live migration attempt from %s to %s "
                "returned wrong error code for '%s' domU",
                pco_src->ta, pco_dst->ta, dom_u);
        goto cleanup1;
    }

    if ((rc = tapi_cfg_xen_dom_u_migrate(pco_src->ta, pco_dst->ta,
                                         dom_u, ip_addr, TRUE)) == 0)
    {
        ERR_FLG("Live migration attempt from %s to %s succeeded for "
                "'%s' domU", pco_src->ta, pco_dst->ta, dom_u);
        goto cleanup1;
    }

    if (rc != TE_RC(TE_TA_UNIX, TE_EINVAL)) /** Should be TE_EINVAL */
    {
        ERR_FLG("Live migration attempt from %s to %s "
                "returned wrong error code for '%s' domU",
                pco_src->ta, pco_dst->ta, dom_u);
        goto cleanup1;
    }

cleanup1:

    if (tapi_cfg_xen_destroy_dom_u(pco_src->ta, dom_u) != 0)
        ERR_FLG("Failed to destroy '%s' domU on %s", dom_u, pco_src->ta);

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

    uint8_t mac[ETHER_ADDR_LEN];

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

    get_mac_by_mac_string(mac_addr, mac);

    /* Omit CSAP reports that are responses to 'trrecv_get' from log */
    rcf_tr_op_log(FALSE);

    /* Request IP addresses */
    request_ip_addr_via_dhcp(pco_iut, rpc_ifname, mac, &ip_iut);

    mac[ETHER_ADDR_LEN - 1]++; /** "Generate" another MAC */

    request_ip_addr_via_dhcp(pco_aux, rpc_ifname, mac, &ip_aux);

    /* Perform testing both TAs */
    test_core(pco_iut, pco_aux, xen_path, dom_u, &ip_aux);
    test_core(pco_aux, pco_iut, xen_path, dom_u, &ip_iut);

    /* Release IP addresses (this operation may be omitted) */
    release_ip_addr_via_dhcp(pco_iut, rpc_ifname, &ip_iut);
    release_ip_addr_via_dhcp(pco_aux, rpc_ifname, &ip_aux);

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
