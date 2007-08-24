/** @file
 * @brief Test API to configure XEN.
 *
 * Implementation of API to configure XEN.
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Edward Makarov <Edward.Makarov@oktetlabs.ru>
 *
 * $Id:
 */

#define TE_LGR_USER     "TAPI CFG VTund"

#include "te_config.h"

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif

#include "te_defs.h"
#include "logger_api.h"
#include "te_sleep.h"
#include "tapi_sockaddr.h"
#include "tapi_cfg_base.h"
#include "tapi_cfg_xen.h"
#include "tapi_tad.h"
#include "tapi_dhcp.h"

#define DHCP_MAGIC_SIZE        4

#define SERVER_ID_OPTION       54
#define REQUEST_IP_ADDR_OPTION 50

/* Ethernet address length */
#define ETHER_ADDR_LEN 6

/**
 * FIXME: libtapi_tad library should be linked
 * to this one in order to use commented code below
 */
#if 0
/**
 * Tries to send DHCP request and waits for ans answer.
 *
 * @param ta        TA name
 * @param csap      DHCP CSAP handle
 * @param lladdr    Local hardware address
 * @param type      Message type (DHCP Option 53)
 * @param broadcast Whether DHCP request is marked as broadcast
 * @param myaddr    in: current IP address; out: obtained IP address
 * @param srvaddr   in: current IP address; out: obtained IP address
 * @param xid       DHCP XID field
 * @param xid       DHCP XID field
 *
 * @return boolean success
 */
static te_bool
dhcp_request_reply(const char *ta, csap_handle_t csap,
                        const struct sockaddr *lladdr, 
                        int type, te_bool broadcast,
                        struct in_addr *myaddr,
                        struct in_addr *srvaddr,
                        unsigned long *xid, te_bool set_ciaddr)
{
    struct dhcp_message *request = NULL;
    uint16_t             flags = (broadcast ? FLAG_BROADCAST : 0);
    struct dhcp_option **opt;
    int                  i;
    te_errno             rc = 0;

    if ((request = dhcpv4_message_create(type)) == NULL)
    {
        ERROR("Failed to create DHCP message");
        rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
        goto cleanup0;
    }

    dhcpv4_message_set_flags(request, &flags);
    dhcpv4_message_set_chaddr(request, lladdr->sa_data);
    dhcpv4_message_set_xid(request, xid);

    if (set_ciaddr)
        dhcpv4_message_set_ciaddr(request, myaddr);

    if (type != DHCPDISCOVER)
        dhcpv4_message_add_option(request, SERVER_ID_OPTION,
                                  sizeof(srvaddr->s_addr),
                                  &srvaddr->s_addr);
    if (type == DHCPREQUEST)
        dhcpv4_message_add_option(request, REQUEST_IP_ADDR_OPTION,
                                  sizeof(myaddr->s_addr),
                                  &myaddr->s_addr);

    /* Add the 'end' option (RFC2131, chapter 4.1, page 22:
     * "The last option must always be the 'end' option")
     */
    dhcpv4_message_add_option(request, 255, 0, NULL);

    /* Calculate the space in octets currently occupied by options */
    for (i = DHCP_MAGIC_SIZE, opt = &(request->opts);
         *opt != NULL;
         opt = &((*opt)->next))
    {
        int opt_type = (*opt)->type;

        i++;                          /** Option type length       */
        if (opt_type != 255 && opt_type != 0)
            i += 1 + (*opt)->val_len; /** Option len + body length */
    }

    /* Align to 32-octet boundary; no such requirement in RFC,
     * Solaris 'dhcp' server does this way, so the test too:
     * alignment is performed over adding appropriate number
     * of 'pad' options
     */
    for (i = 32 - i % 32; i < 32 && i > 0; i--)
        dhcpv4_message_add_option(request, 0, 0, NULL);

    if (type != DHCPRELEASE)
    {
        const char          *err_msg;
        unsigned int         timeout = 10000; /**< 10 sec */
        struct dhcp_message *reply   = tapi_dhcpv4_send_recv(ta, csap,
                                                             request,
                                                             &timeout,
                                                             &err_msg);
        struct dhcp_option const *server_id_option;

        if (reply == NULL)
        {
            ERROR("Failed send/receive DHCP request/reply: %s", err_msg);
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
            goto cleanup1;
        }

        myaddr->s_addr = dhcpv4_message_get_yiaddr(reply);

        RING("Got address %d.%d.%d.%d", 
             (myaddr->s_addr & 0xFF),
             (myaddr->s_addr >> 8) & 0xFF,
             (myaddr->s_addr >> 16) & 0xFF,
             (myaddr->s_addr >> 24) & 0xFF);

        if (dhcpv4_message_get_xid(reply) != *xid)
        {
            ERROR("Reply XID doesn't match that of request");
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
            goto cleanup2;
        }

        if ((server_id_option = dhcpv4_message_get_option(reply,
                                                          54)) == NULL)
        {
            ERROR("Cannot get ServerID option");
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
            goto cleanup2;
        }

        if (server_id_option->len != 4 || server_id_option->val_len != 4)
        {
            ERROR("Invalid ServerID option value length: %u",
                  server_id_option->val_len);
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
            goto cleanup2;
        }

        memcpy(&srvaddr->s_addr, server_id_option->val, 4);

cleanup2:
        free(reply);
    }
    else
    {
        if (tapi_dhcpv4_message_send(ta, csap, request) == 0)
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

cleanup1:
    free(request);

cleanup0:
    return rc;
}

/**
 * Requests IP address by MAC one via DHCP server.
 *
 * @param ta        TA name
 * @param if_name   Name of the interface to send request from
 * @param mac       Supplied MAC address
 * @param ip_addr   Got IP address
 *
 * @return Status code
 */
static te_errno
dhcp_request_ip_addr(char const *ta, char const *if_name,
                     uint8_t const *mac, struct sockaddr *ip_addr)
{
    int                     rc;
    csap_handle_t           csap = CSAP_INVALID_HANDLE;
    struct in_addr          myaddr = {INADDR_ANY};
    struct in_addr          srvaddr = {INADDR_ANY};
    unsigned long           xid = random();
    struct sockaddr         mac_addr = { .sa_family = AF_LOCAL };

    memcpy(mac_addr.sa_data, mac, ETHER_ADDR_LEN);

    if ((rc = tapi_dhcpv4_plain_csap_create(ta, if_name,
                                            DHCP4_CSAP_MODE_CLIENT,
                                            &csap)) != 0)
    {
        ERROR("Failed to create DHCP client CSAP for interface %s on %s",
              if_name, ta);
        goto cleanup0;
    }

    if ((rc = dhcp_request_reply(ta, csap, &mac_addr, DHCPDISCOVER, TRUE,
                                 &myaddr, &srvaddr, &xid, FALSE)) != 0)
    {
        ERROR("DHCP discovery failed");
        goto cleanup1;
    }

    if ((rc = dhcp_request_reply(ta, csap, &mac_addr, DHCPREQUEST, TRUE,
                                 &myaddr, &srvaddr, &xid, FALSE)) != 0)
    {
        ERROR("DHCP lease cannot be obtained");
        goto cleanup1;
    }

    ip_addr->sa_family = AF_INET;
    SIN(ip_addr)->sin_addr = myaddr;

    RING("Got IP address: %d.%d.%d.%d",
         (ntohl(myaddr.s_addr) >> 24) % 256,
         (ntohl(myaddr.s_addr) >> 16) % 256,
         (ntohl(myaddr.s_addr) >> 8) % 256,
         (ntohl(myaddr.s_addr) >> 0) % 256);

cleanup1:
    tapi_tad_csap_destroy(ta, 0, csap);

cleanup0:
    return rc;
}

/**
 * Releases IP address got previously via DHCP server.
 *
 * @param ta        TA name
 * @param if_name   Name of the interface to send request from
 * @param ip_addr   IP address to release
 *
 * @return Status code
 */
static te_errno
dhcp_release_ip_addr(char const *ta, char const *if_name,
                     struct sockaddr const *ip_addr)
{
    csap_handle_t  csap = CSAP_INVALID_HANDLE;
    struct in_addr myaddr = {INADDR_ANY};
    struct in_addr srvaddr = {INADDR_ANY};
    unsigned long  xid = random();
    te_errno       rc;

    memcpy(&myaddr, &SIN(ip_addr)->sin_addr, sizeof(myaddr));

/* FIXME: Currently crash happens in attempt to create DHCP CSAP */
return 0;

    rc = tapi_dhcpv4_plain_csap_create(ta, if_name,
                                       DHCP4_CSAP_MODE_CLIENT,
                                       &csap);

    if (rc != 0)
    {
        ERROR("Failed to create DHCP client CSAP on %s:%s: %X",
              ta, if_name, rc);
        goto cleanup0;
    }

    if ((rc = dhcp_request_reply(ta, csap, ip_addr, DHCPRELEASE, FALSE,
                                 &myaddr, &srvaddr, &xid, TRUE)) != 0)
    {
        ERROR("Error releasing DHCP lease");
    }

    tapi_tad_csap_destroy(ta, 0, csap);

cleanup0:
    return rc;
}
#endif /* FIXME */

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_get_path(char const *ta, char *path)
{
    cfg_val_type type  = CVT_STRING;
    char const  *value;
    te_errno     rc    = cfg_get_instance_fmt(&type, &value,
                                              "/agent:%s/xen:", ta);

    if (rc == 0)
    {
        strcpy(path, value);
        free((void *)value);
    }
    else
        ERROR("Failed to get XEN path on %s", ta);

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_set_path(char const *ta, char const *path)
{
    te_errno rc = cfg_set_instance_fmt(CFG_VAL(STRING, path),
                                       "/agent:%s/xen:",
                                       ta);

    if (rc != 0)
        ERROR("Failed to set XEN path on %s", ta);

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_create_dom_u(char const *ta, char const *dom_u)
{
    /* Create domU and its directory/disk images in XEN storage */
    te_errno rc = cfg_add_instance_fmt(NULL, CFG_VAL(INTEGER, 1),
                                       "/agent:%s/xen:/dom_u:%s",
                                       ta, dom_u);

    if (rc != 0)
        ERROR("Failed to create domU %s on %s", dom_u, ta);

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_destroy_dom_u(char const *ta, char const *dom_u)
{
    /* Ensure that domU is in "non-running" state */
    te_errno rc = cfg_set_instance_fmt(CFG_VAL(STRING, "non-running"),
                                       "/agent:%s/xen:/dom_u:%s/status:",
                                       ta, dom_u);

    if (rc != 0)
    {
        ERROR("Failed to shutdown domU %s on %s", dom_u, ta);
        goto cleanup0;
    }

    /* Remove directory/disk images of domU from XEN storage */
    if ((rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, 0),
                                   "/agent:%s/xen:/dom_u:%s",
                                   ta, dom_u)) != 0)
    {
        ERROR("Failed to remove directory/images of domU %s on %s",
              dom_u, ta);
        goto cleanup0;
    }

    /* Destroy domU */
    if ((rc = cfg_del_instance_fmt(FALSE,
                                   "/agent:%s/xen:/dom_u:%s",
                                   ta, dom_u)) != 0)
    {
        ERROR("Failed to destroy domU %s on %", dom_u, ta);
    }

cleanup0:
    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_dom_u_get_status(char const *ta, char const *dom_u,
                              char *status)
{
    cfg_val_type type  = CVT_STRING;
    char const  *value;

    te_errno rc = cfg_get_instance_fmt(&type, &value,
                                       "/agent:%s/xen:/dom_u:%s/status:",
                                       ta, dom_u);

    if (rc == 0)
    {
        strcpy(status, value);
        free((void *)value);
    }
    else
        ERROR("Failed to get status for domU %s on %s", dom_u, ta);

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_dom_u_set_status(char const *ta, char const *dom_u,
                              char const *status)
{
    te_errno rc = cfg_set_instance_fmt(CFG_VAL(STRING, status),
                                       "/agent:%s/xen:/dom_u:%s/status:",
                                       ta, dom_u);

    if (rc != 0)
        ERROR("Failed to set status for domU %s on %s, rc = %d: %r", dom_u, ta, rc, rc);

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_dom_u_get_ip_addr(char const *ta, char const *dom_u,
                               struct sockaddr *ip_addr)
{
    cfg_val_type           type  = CVT_ADDRESS;
    struct sockaddr const *value;

    te_errno rc = cfg_get_instance_fmt(&type, &value,
                                       "/agent:%s/xen:/dom_u:%s/ip_addr:",
                                       ta, dom_u);

    if (rc == 0)
    {
        memcpy(ip_addr, value, sizeof(*value));
        free((void *)value);
    }
    else
        ERROR("Failed to get IP address for domU %s on %s", dom_u, ta);

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_dom_u_set_ip_addr(char const *ta, char const *dom_u,
                               struct sockaddr const *ip_addr)
{
    te_errno rc = cfg_set_instance_fmt(CFG_VAL(ADDRESS, ip_addr),
                                       "/agent:%s/xen:/dom_u:%s/ip_addr:",
                                       ta, dom_u);

    if (rc != 0)
        ERROR("Failed to set IP address for domU %s on %s", dom_u, ta);

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_dom_u_get_mac_addr(char const *ta, char const *dom_u, uint8_t *mac)
{
    cfg_val_type     type = CVT_ADDRESS;
    struct sockaddr *addr;

    te_errno rc = cfg_get_instance_fmt(&type, &addr,
                                       "/agent:%s/xen:/dom_u:%s/mac_addr:",
                                       ta, dom_u);

    if (rc == 0)
    {
        memcpy(mac, addr->sa_data, ETHER_ADDR_LEN);
        free(addr);
    }
    else
        ERROR("Failed to get MAC address of domU %s on %s", dom_u, ta);

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_dom_u_set_mac_addr(char const *ta, char const *dom_u, uint8_t const *mac)
{
    struct sockaddr addr = { .sa_family = AF_LOCAL };
    te_errno        rc;

    memcpy(addr.sa_data, mac, ETHER_ADDR_LEN);

    if ((rc = cfg_set_instance_fmt(CFG_VAL(ADDRESS, &addr),
                                   "/agent:%s/xen:/dom_u:%s/mac_addr:",
                                   ta, dom_u)) != 0)
    {
        ERROR("Failed to set MAC address of domU %s on %s", dom_u, ta);
    }

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_dom_u_migrate(char const *from_ta, char const *to_ta,
                           char const *dom_u, char const *host,
                           te_bool live)
{
    cfg_val_type  type1 = CVT_STRING;
    char const   *xen_path1;
    char const   *xen_path2;

    uint8_t         mac[ETHER_ADDR_LEN];
    struct sockaddr ip;

    te_errno rc;

    /* Cannot migrate to itself */
    if (strcmp(from_ta, to_ta) == 0)
    {
        ERROR("Failed to migrate from %s to itself", from_ta);
        rc = TE_RC(TE_TA_UNIX, TE_EINVAL);
        goto cleanup0;
    }

    /* Check that XEN paths are identical for both dom0 agents */
    if ((rc = cfg_get_instance_fmt(&type1, &xen_path1,
                                   "/agent:%s/xen:", from_ta)) != 0)
    {
        ERROR("Failed to get XEN path on %s", from_ta);
        goto cleanup0;
    }

    if ((rc = cfg_get_instance_fmt(&type1, &xen_path2,
                                   "/agent:%s/xen:", to_ta)) != 0)
    {
        ERROR("Failed to get XEN path on %s", to_ta);
        goto cleanup1;
    }

    if (strcmp(xen_path1, xen_path2) != 0)
    {
        ERROR("XEN path differs between %s and %s", from_ta, to_ta);
        goto cleanup2;
    }

    /* Save MAC address */
    if ((rc = tapi_cfg_xen_dom_u_get_mac_addr(from_ta, dom_u, mac)) != 0)
    {
        ERROR("Failed to get domU %s MAC address on %s", dom_u, from_ta);
        goto cleanup2;
    }

    /* Save IP address */
    if ((rc = tapi_cfg_xen_dom_u_get_ip_addr(from_ta, dom_u, &ip)) != 0)
    {
        ERROR("Failed to get domU %s IP address on %s", dom_u, from_ta);
        goto cleanup2;
    }

    /* Set kind of migration (live/non-live) */
    if ((rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, live),
                                   "/agent:%s/xen:/dom_u:%s/"
                                   "migrate:/kind:",
                                   from_ta, dom_u)) != 0)
    {
        ERROR("Failed to set migrate kind for domU %s on %s",
              dom_u, from_ta);
        goto cleanup2;
    }

    /* Perform migration */
    if ((rc = cfg_set_instance_fmt(CFG_VAL(STRING, host),
                                   "/agent:%s/xen:/dom_u:%s/migrate:",
                                   from_ta, dom_u)) != 0)
    {
        ERROR("Failed to migrate domU %s from %s to %s (host %s)",
              dom_u, from_ta, to_ta, host);
        goto cleanup2;
    }

    /* Delete domU item from the source agent configurator tree */
    if ((rc = cfg_del_instance_fmt(FALSE, "/agent:%s/xen:/dom_u:%s",
                                   from_ta, dom_u)) != 0)
    {
        ERROR("Failed to destroy domU %s on %s", dom_u, from_ta);
        goto cleanup2;
    }

    /* Create domU on target agent (domU will have "non-running" state) */
    if ((rc = cfg_add_instance_fmt(NULL, CFG_VAL(INTEGER, 1),
                                   "/agent:%s/xen:/dom_u:%s",
                                   to_ta, dom_u)) != 0)
    {
        ERROR("Failed to accept domU %s migrated from %s to %s "
              "(host %s)", dom_u, from_ta, to_ta, host);
        goto cleanup2;
    }

    /* Set MAC address saved previously */
    if ((rc = tapi_cfg_xen_dom_u_set_mac_addr(to_ta, dom_u, mac)) != 0)
    {
        ERROR("Failed to set MAC address for domU %s on %s",
              dom_u, to_ta);
        goto cleanup2;
    }

    /* Set IP address saved previously */
    if ((rc = tapi_cfg_xen_dom_u_set_ip_addr(to_ta, dom_u, &ip)) != 0)
    {
        ERROR("Failed to set IP address " "%d.%d.%d.%d for domU %s on %s",
              ((ntohl(SIN(&ip)->sin_addr.s_addr)) >> 24) % 256,
              ((ntohl(SIN(&ip)->sin_addr.s_addr)) >> 16) % 256,
              ((ntohl(SIN(&ip)->sin_addr.s_addr)) >> 8) % 256,
              ((ntohl(SIN(&ip)->sin_addr.s_addr)) >> 0) % 256,
              dom_u, to_ta);
        goto cleanup2;
    }

    /**
     * Set "migrated" pseudo-status; that is set domU status to
     * "running" in case migration has proceeded successfully and
     * domU exists within dom0, that is controlled by the target agent
     */
    if ((rc = cfg_set_instance_fmt(CFG_VAL(STRING, "migrated"),
                                   "/agent:%s/xen:/dom_u:%s/status:",
                                   to_ta, dom_u)) != 0)
    {
        ERROR("Failed to set migrated pseudo-status for domU %s on %s",
              dom_u, to_ta);
    }

cleanup2:
    free((void *)xen_path2);

cleanup1:
    free((void *)xen_path1);

cleanup0:
    return rc;
}

/**
 * FIXME: libtapi_tad library should be linked
 * to this one in order to use commented code below
 */
#if 0
/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_create_dom_u_dhcp(char const *ta, char const *dom_u,
                               uint8_t const *mac)
{
    struct sockaddr ip_addr;
    te_errno        rc = dhcp_request_ip_addr(ta, "eth0", mac, &ip_addr);

    if (rc != 0)
    {
        ERROR("Failed to request IP address over DHCP");
        goto cleanup0;
    }

    if ((rc = cfg_add_instance_fmt(NULL, CFG_VAL(INTEGER, 1),
                                   "/agent:%s/xen:/dom_u:%s",
                                   ta, dom_u)) != 0)
    {
        ERROR("Failed to create domU %s on %s", dom_u, ta);
        goto cleanup0;
    }

    if ((rc = tapi_cfg_xen_dom_u_set_mac_addr(ta, dom_u, mac)) != 0)
    {
        ERROR("Failed to set MAC address "
              "%02x:%02x:%02x:%02x:%02x:%02x for domU %s on %s",
              dom_u, ta, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        goto cleanup0;
    }

    if ((rc = tapi_cfg_xen_dom_u_set_ip_addr(ta, dom_u, &ip_addr)) != 0)
    {
        ERROR("Failed to set IP address "
              "%d.%d.%d.%d for domU %s on %s",
              ((ntohl(SIN(&ip_addr)->sin_addr.s_addr)) >> 24) % 256,
              ((ntohl(SIN(&ip_addr)->sin_addr.s_addr)) >> 16) % 256,
              ((ntohl(SIN(&ip_addr)->sin_addr.s_addr)) >> 8) % 256,
              ((ntohl(SIN(&ip_addr)->sin_addr.s_addr)) >> 0) % 256,
              dom_u, ta);
        goto cleanup0;
    }

cleanup0:
    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_destroy_dom_u_dhcp(char const *ta, char const *dom_u)
{
    cfg_val_type           type = CVT_ADDRESS;
    struct sockaddr const *addr;
    te_errno               rc;

    if ((rc = cfg_get_instance_fmt(&type, &addr,
                                   "/agent:%s/xen:/dom_u:%s/ip_addr:",
                                   ta, dom_u)) == 0)
    {
        if (dhcp_release_ip_addr(ta, dom_u, addr) != 0)
            ERROR("Failed to release IP address %d.%d.%d.%d over DHCP",
                  ((ntohl(SIN(&addr)->sin_addr.s_addr)) >> 24) % 256,
                  ((ntohl(SIN(&addr)->sin_addr.s_addr)) >> 16) % 256,
                  ((ntohl(SIN(&addr)->sin_addr.s_addr)) >> 8) % 256,
                  ((ntohl(SIN(&addr)->sin_addr.s_addr)) >> 0) % 256);

        free((void *)addr);
    }
    else
    {
        ERROR("Failed to get IP address of domU %s on %s", dom_u, ta);
        goto cleanup0;
    }

    if ((rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, 0),
                                   "/agent:%s/xen:/dom_u:%s",
                                   ta, dom_u)) != 0)
    {
        ERROR("Failed to delete domU %s disk images on %s", dom_u, ta);
        goto cleanup0;
    }

    if ((rc = cfg_del_instance_fmt(FALSE,
                                   "/agent:%s/xen:/dom_u:%s",
                                   ta, dom_u)) != 0)
    {
        ERROR("Failed to destroy domU %s on %s", dom_u, ta);
        goto cleanup0;
    }

cleanup0:
    return rc;
}
#endif /* FIXME */
