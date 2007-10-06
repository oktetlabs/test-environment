/** @file
 * @brief XEN Test Suite
 *
 * Common definitions for RPC test suite.
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
 * $Id: xen.h $
 */

#ifndef __XEN_H__
#define __XEN_H__

#include "tapi_tad.h"
#include "tapi_dhcp.h"

#include "tapi_cfg_xen.h"

/* FIXME: Comments should be added elswhere in the file */

#define ERR_FLG(fmt...) \
    do {                \
        flg = TRUE;     \
        ERROR(fmt);     \
    } while (0)


static inline uint8_t
get_hex_digit(char const **string)
{
    char c = toupper(*(*string)++);

    if (c < '0' || c > 'F' || (c > '9' && c < 'A'))
        TEST_FAIL("Invalid MAC address string: hex digit is expected");

    return c < 'A' ? (c - '0') : (c - 'A' + 10);
}

static inline uint8_t
get_2_hex_digits(char const **string)
{
    uint8_t u = get_hex_digit(string);

    return (u << 4) + get_hex_digit(string);
}

static inline void
get_mac_by_mac_string(char const *mac_string, uint8_t *mac)
{
    unsigned u;

    for (u = 0; u < ETHER_ADDR_LEN; u++)
    {
        if (u > 0 && *mac_string++ != ':')
            TEST_FAIL("Invalid MAC address string: ':' is expected");

        mac[u] = get_2_hex_digits(&mac_string);
    }

    if (*mac_string != '\0')
        TEST_FAIL("Invalid MAC address string: extra characters on line");
}

static inline void
request_ip_addr_via_dhcp(rcf_rpc_server *pco, char const *rpc_ifname,
                         uint8_t const *mac, struct sockaddr *ip)
{
    if (tapi_dhcp_request_ip_addr(pco->ta, rpc_ifname, mac, ip) != 0)
    {
        TEST_FAIL("DHCP request from interface '%s' on %s using MAC "
                  "address %02X:%02X:%02X:%02X:%02X:%02X has failed",
                  rpc_ifname, pco->ta, mac[0], mac[1], mac[2], mac[3],
                  mac[4], mac[5]);
    }

}

static inline void
release_ip_addr_via_dhcp(rcf_rpc_server *pco, char const *rpc_ifname,
                         struct sockaddr const *ip)
{
    if (tapi_dhcp_release_ip_addr(pco->ta, rpc_ifname, ip) != 0)
    {
        TEST_FAIL("DHCP release from interface '%s' on %s of IP "
                  "address %s has failed", rpc_ifname, pco->ta,
                  inet_ntoa(SIN(ip)->sin_addr));
    }

}

static inline te_bool
cmd(char const *cmdline, char *output, size_t n)
{
    char *s;
    FILE *pipe = NULL;

    /* Invoke shell and execute the ssh command */
    if ((pipe = popen(cmdline, "r")) == NULL)
    {
        ERROR("Failed to open a process on the script host");
        return FALSE;
    }

    /* Read ssh command output */
    fread(output, 1, n, pipe);
    pclose(pipe);

    /* Remove trailing '\n's of ssh command output */
    s = output + strlen(output) - 1;

    while (s != output && *s == '\n')
        *s-- = '\0';

    return TRUE;
}

static inline te_bool
ssh(rcf_rpc_server *pco, char const *dom_u, char const *host)
{
    char cmdline[64]; /**< Probably this length is enough */

    struct utsname utsname;
    char           sysname [sizeof(utsname.sysname)]  = { '\0' };
    char           nodename[sizeof(utsname.nodename)] = { '\0' };

    /* Get sysname of the host from the agent */
    if (rpc_uname(pco, &utsname) != 0)
    {
        ERROR("Failed to get uname of %s", pco->ta);
        return FALSE;
    }

    /* Prepare correspondent ssh comand line */
    TE_SPRINTF(cmdline, "/usr/bin/ssh %s /bin/uname -s", host);

    if (!cmd(cmdline, sysname, sizeof(sysname)))
        return FALSE;

    /* Prepare correspondent ssh comand line */
    TE_SPRINTF(cmdline, "/usr/bin/ssh %s /bin/hostname", host);

    if (!cmd(cmdline, nodename, sizeof(nodename)))
        return FALSE;

    /* Check that sysnames are equal */
    if (strcmp(sysname, utsname.sysname) != 0)
    {
        ERROR("Sysname '%s' got by %s differs from the one '%s' "
              "got over SSH", utsname.sysname, pco->ta, sysname);
        return FALSE;
    }

    if (strcmp(nodename, dom_u) != 0)
    {
        ERROR("Nodename '%s' set by %s differs from the one '%s' "
              "got over SSH", dom_u, pco->ta, nodename);
        return FALSE;
    }

    return TRUE;
}


#endif /* __XEN_H__ */
