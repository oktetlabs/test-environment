/** @file
 * @brief XEN Test Suite
 *
 * XEN Test Suite
 * 
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
 *
 * @author Edward Makarov <Edward.Makarov@oktetlabs.ru>
 * 
 * $Id: xen_dhcp.c $
 */


#define TE_TEST_NAME "xen/xen_dhcp"

#include "xen_suite.h"
#include "xen.h"

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

    TEST_START;

    TEST_GET_PCO(pco_iut);
    TEST_GET_PCO(pco_aux);

    TEST_GET_STRING_PARAM(rpc_ifname);
    TEST_GET_STRING_PARAM(mac_addr);

    get_mac_by_mac_string(mac_addr, mac);

    /* Omit CSAP reports that are responses to 'trrecv_get' from log */
    rcf_tr_op_log(FALSE);

    /* Request IP addresses */
    request_ip_addr_via_dhcp(pco_iut, rpc_ifname, mac, &ip_iut);

    mac[ETHER_ADDR_LEN - 1]++; /** "Generate" another MAC */

    request_ip_addr_via_dhcp(pco_aux, rpc_ifname, mac, &ip_aux);

    /* Release IP addresses */
    release_ip_addr_via_dhcp(pco_iut, rpc_ifname, &ip_iut);
    release_ip_addr_via_dhcp(pco_aux, rpc_ifname, &ip_aux);

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
