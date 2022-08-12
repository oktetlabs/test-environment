/** @file
 * @brief Basic Configuration Model TAPI
 *
 * Implementation of test API for basic configuration model
 * (storage/cm/cm_base.xml).
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 */

#define TE_LGR_USER     "Configuration TAPI"

#include "te_config.h"

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "te_stdint.h"
#include "te_alloc.h"
#include "te_str.h"
#include "te_sleep.h"
#include "logger_api.h"
#include "conf_api.h"
#include "te_sockaddr.h"
#include "te_enum.h"

#include "tapi_cfg_base.h"
#include "tapi_cfg_sys.h"
#include "tapi_cfg_phy.h"
#include "tapi_host_ns.h"
#include "tapi_mem.h"
#include "tapi_test_behaviour.h"

/* See the description in tapi_cfg_base.h */
char *
tapi_cfg_base_get_ta_dir(const char *ta, tapi_cfg_base_ta_dir kind)
{
    char *value = NULL;
    te_errno rc;
    static const te_enum_map dir_map[] = {
        {.name = "dir", .value = TAPI_CFG_BASE_TA_DIR_AGENT},
        {.name = "tmp_dir", .value = TAPI_CFG_BASE_TA_DIR_TMP},
        {.name = "lib_mod_dir", .value = TAPI_CFG_BASE_TA_DIR_KMOD},
        {.name = "lib_bin_dir", .value = TAPI_CFG_BASE_TA_DIR_BIN},
        TE_ENUM_MAP_END
    };

    rc = cfg_get_instance_string_fmt(&value, "/agent:%s/%s:",
                                     ta, te_enum_map_from_value(dir_map, kind));
    if (rc != 0)
    {
        ERROR("Cannot get /agent:%s/%s: %r", ta,
              te_enum_map_from_value(dir_map, kind), rc);
        return NULL;
    }

    return value;
}

/* See the description in tapi_cfg_base.h */
te_errno
tapi_cfg_base_ipv4_fw(const char *ta, te_bool enable)
{
    return tapi_cfg_sys_set_int(ta, enable, NULL, "net/ipv4/ip_forward");
}

/* See the description in tapi_cfg_base.h */
te_errno
tapi_cfg_base_ipv4_fw_enabled(const char *ta, te_bool *enabled)
{
    te_errno rc;
    int val;

    rc = tapi_cfg_sys_get_int(ta, &val, "net/ipv4/ip_forward");
    if (rc == 0)
        *enabled = val;

    return rc;
}

/* See the description in tapi_cfg_base.h */
te_errno
tapi_cfg_ipv4_fw_set(const char *ta, const char *ifname, te_bool enable)
{
    return tapi_cfg_sys_set_int(ta, enable, NULL,
                                "net/ipv4/conf/%s/forwarding", ifname);
}

/* See the description in tapi_cfg_base.h */
te_errno
tapi_cfg_ipv4_fw_get(const char *ta, const char *ifname, te_bool *enabled)
{
    te_errno rc;
    int val;

    rc = tapi_cfg_sys_get_int(ta, &val, "net/ipv4/conf/%s/forwarding", ifname);
    if (rc == 0)
        *enabled = val;

    return rc;
}

/* See the description in tapi_cfg_base.h */
te_errno
tapi_cfg_ipv6_fw_set(const char *ta, const char *ifname, te_bool enable)
{
    return tapi_cfg_sys_set_int(ta, enable, NULL,
                                "net/ipv6/conf/%s/forwarding", ifname);
}

/* See the description in tapi_cfg_base.h */
te_errno
tapi_cfg_ipv6_fw_get(const char *ta, const char *ifname, te_bool *enabled)
{
    te_errno rc;
    int val;

    rc = tapi_cfg_sys_get_int(ta, &val, "net/ipv6/conf/%s/forwarding", ifname);
    if (rc == 0)
        *enabled = val;

    return rc;
}

/* See the description in tapi_cfg_base.h */
int
tapi_cfg_base_if_get_link_addr(const char *ta, const char *dev,
                               struct sockaddr *link_addr)
{
    char             inst_name[CFG_OID_MAX];
    cfg_handle       handle;
    cfg_val_type     type = CVT_ADDRESS;
    struct sockaddr *addr = NULL;
    int              rc;

    snprintf(inst_name, sizeof(inst_name),
             "/agent:%s/interface:%s/link_addr:",
             ta, dev);
    rc = cfg_find_str(inst_name, &handle);
    if (rc != 0)
    {
        ERROR("%s(): Failed to find MAC address OID handle for %s",
              __FUNCTION__, inst_name);
        return rc;
    }

    rc = cfg_get_instance(handle, &type, &addr);
    if (rc != 0)
    {
        ERROR("%s(): Failed to get MAC address using OID %s",
              __FUNCTION__, inst_name);
        return rc;
    }
    memcpy(link_addr, addr, sizeof(struct sockaddr));
    free(addr);
    return 0;
}

/* See description in tapi_cfg_base.h */
int
tapi_cfg_base_if_get_mac(const char *oid, uint8_t *mac)
{
    int                 rc;
    char                buf[strlen(oid) + strlen("/link_addr:") + 1];
    cfg_handle          handle;
    cfg_val_type        type = CVT_ADDRESS;
    struct sockaddr    *addr = NULL;

    TE_SPRINTF(buf, "%s/link_addr:", oid);
    rc = cfg_find_str(buf, &handle);
    if (rc != 0)
    {
        ERROR("%s(): Failed to find MAC address OID handle for %s",
              __FUNCTION__, oid);
        return rc;
    }

    rc = cfg_get_instance(handle, &type, &addr);
    if (rc != 0)
    {
        ERROR("%s(): Failed to get MAC address using OID %s",
              __FUNCTION__, buf);
        return rc;
    }
    memcpy(mac, addr->sa_data, ETHER_ADDR_LEN);

    free(addr);

    return rc;
}

/* See description in tapi_cfg_base.h */
int
tapi_cfg_base_if_set_mac(const char *oid, const uint8_t *mac)
{
    int                 rc;
    char                buf[strlen(oid) + strlen("/link_addr:") + 1];
    cfg_handle          handle;
    struct sockaddr     addr;

    memset(&addr, 0, sizeof(struct sockaddr));
    addr.sa_family = AF_LOCAL;
    TE_SPRINTF(buf, "%s/link_addr:", oid);
    rc = cfg_find_str(buf, &handle);
    if (rc != 0)
    {
        ERROR("%s(): Failed to find MAC address OID handle for %s",
              __FUNCTION__, oid);
        return rc;
    }

    memcpy(addr.sa_data, mac, ETHER_ADDR_LEN);

    rc = cfg_set_instance(handle, CVT_ADDRESS, &addr);
    if (rc != 0)
    {
        ERROR("%s(): Failed to set MAC address using OID %s",
              __FUNCTION__, buf);
        return rc;
    }

    return rc;
}

/* See description in tapi_cfg_base.h */
int
tapi_cfg_base_if_get_bcast_mac(const char *oid, uint8_t *bcast_mac)
{
    int                 rc;
    char                buf[strlen(oid) + strlen("/bcast_link_addr:") + 1];
    cfg_handle          handle;
    cfg_val_type        type = CVT_ADDRESS;
    struct sockaddr    *addr = NULL;

    TE_SPRINTF(buf, "%s/bcast_link_addr:", oid);
    rc = cfg_find_str(buf, &handle);
    if (rc != 0)
    {
        ERROR("%s(): Failed to find MAC address OID handle for %s",
              __FUNCTION__, oid);
        return rc;
    }

    rc = cfg_get_instance(handle, &type, &addr);
    if (rc != 0)
    {
        ERROR("%s(): Failed to get MAC address using OID %s",
              __FUNCTION__, buf);
        return rc;
    }
    memcpy(bcast_mac, addr->sa_data, ETHER_ADDR_LEN);

    free(addr);

    return rc;
}

/* See description in tapi_cfg_base.h */
int
tapi_cfg_base_if_set_bcast_mac(const char *oid,
                               const uint8_t *bcast_mac)
{
    int                 rc;
    char                buf[strlen(oid) +
                             strlen("/bcast_link_addr:") + 1];
    cfg_handle          handle;
    struct sockaddr     addr;

    memset(&addr, 0, sizeof(struct sockaddr));
    addr.sa_family = AF_LOCAL;
    TE_SPRINTF(buf, "%s/bcast_link_addr:", oid);
    rc = cfg_find_str(buf, &handle);
    if (rc != 0)
    {
        ERROR("%s(): Failed to find broadcast MAC "
              "address OID handle for %s", __FUNCTION__, oid);
        return rc;
    }

    memcpy(addr.sa_data, bcast_mac, ETHER_ADDR_LEN);

    rc = cfg_set_instance(handle, CVT_ADDRESS, &addr);
    if (rc != 0)
    {
        ERROR("%s(): Failed to set bcast MAC address using OID %s",
              __FUNCTION__, buf);
        return rc;
    }

    return rc;
}


static const char *
tapi_cfg_mac2str(const uint8_t *mac)
{
    static char macbuf[ETHER_ADDR_LEN * 3];

    int  i;

    for (i = 0; i < ETHER_ADDR_LEN; i++)
    {
        sprintf(macbuf + 3 * i, "%2.2x:", mac[i]);
    }
    macbuf[ETHER_ADDR_LEN * 3 - 1] = '\0';

    return macbuf;
}

/* See description in tapi_cfg_base.h */
int
tapi_cfg_base_if_add_mcast_mac(const char *oid,
                               const uint8_t *mcast_mac)
{
    return cfg_add_instance_fmt(NULL, CFG_VAL(NONE, NULL),
                                "%s/mcast_link_addr:%s", oid,
                                tapi_cfg_mac2str(mcast_mac));
}


/* See description in tapi_cfg_base.h */
int
tapi_cfg_base_if_del_mcast_mac(const char *oid,
                               const uint8_t *mcast_mac)
{
    if (mcast_mac != NULL)
    {
        return cfg_del_instance_fmt(TRUE, "%s/mcast_link_addr:%s", oid,
                                    tapi_cfg_mac2str(mcast_mac));
    }
    else
    {
        te_errno        rc = 0;
        cfg_handle     *addrs;
        unsigned int    addr_num;
        unsigned int    i;

        if ((rc = cfg_find_pattern_fmt(&addr_num, &addrs,
                                       "%s/mcast_link_addr:*",
                                       oid)) != 0)
        {
            ERROR("%s(): Failed to get mcast_link_addr list for %s",
                  __FUNCTION__, oid);
            return rc;
        }
        for (i = 0; i < addr_num; i++)
        {
            char *inst_name;
            rc = cfg_get_inst_name(addrs[i], &inst_name);
            if (rc != 0)
            {
                ERROR("%s(): Unable to enumerate multicast addresses: %r",
                      __FUNCTION__, rc);
                break;
            }

            if ((rc = cfg_del_instance(addrs[i], TRUE)) != 0)
            {
                ERROR("%s(): Failed to delete address with handle %#x: %r",
                      __FUNCTION__, addrs[i], rc);
                break;
            }
        }
        free(addrs);

        return rc;
    }
}

/* See description in tapi_cfg_base.h */
int
tapi_cfg_base_if_get_mtu(const char *oid, unsigned int *p_mtu)
{
    int                 rc;
    char                buf[strlen(oid) + strlen("/mtu:") + 1];
    cfg_handle          handle;
    cfg_val_type        type = CVT_INTEGER;
    int                 mtu;

    TE_SPRINTF(buf, "%s/mtu:", oid);
    rc = cfg_find_str(buf, &handle);
    if (rc != 0)
    {
        ERROR("%s(): Failed to find MTU OID handle for %s",
              __FUNCTION__, oid);
        return rc;
    }

    rc = cfg_get_instance(handle, &type, &mtu);
    if (rc != 0)
    {
        ERROR("%s(): Failed to get MTU using OID %s. %r",
              __FUNCTION__, buf, rc);
        return rc;
    }
    assert(mtu >= 0);
    *p_mtu = (unsigned int)mtu;

    return rc;
}

/* See description in tapi_cfg_base.h */
int
tapi_cfg_base_add_net_addr(const char *oid, const struct sockaddr *addr,
                           int prefix, te_bool set_bcast,
                           cfg_handle *cfg_hndl)
{
    char    buf[INET6_ADDRSTRLEN];
    int     rc;


    if (addr->sa_family != AF_INET && addr->sa_family != AF_INET6)
    {
        ERROR("%s(): AF_INET and AF_INET6 address families are supported only.",
              __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EAFNOSUPPORT);
    }

    if (prefix == -1)
    {
        rc = cfg_add_instance_fmt(cfg_hndl, CFG_VAL(NONE, NULL),
                                  "%s/net_addr:%s", oid,
                                  inet_ntop(addr->sa_family,
                                            te_sockaddr_get_netaddr(addr),
                                            buf, sizeof(buf)));
    }
    else
    {
        rc = cfg_add_instance_fmt(cfg_hndl, CFG_VAL(INTEGER, prefix),
                                  "%s/net_addr:%s", oid,
                                  inet_ntop(addr->sa_family,
                                            te_sockaddr_get_netaddr(addr),
                                            buf, sizeof(buf)));
    }
    if (rc == 0)
    {
        if (addr->sa_family == AF_INET && set_bcast)
        {
            struct sockaddr_in  bcast;
            uint32_t            nmask;

            if (prefix > 0)
            {
                nmask = (1 << ((sizeof(struct in_addr) << 3) - prefix)) - 1;
            }
            else
            {
                uint32_t    inaddr = ntohl(SIN(addr)->sin_addr.s_addr);

                if (IN_CLASSA(inaddr))
                    nmask = IN_CLASSA_HOST;
                else if (IN_CLASSB(inaddr))
                    nmask = IN_CLASSA_HOST;
                else if (IN_CLASSC(inaddr))
                    nmask = IN_CLASSA_HOST;
                else
                {
                    ERROR("%s(): Invalid IPv4 address - unknown class",
                          __FUNCTION__);
                    return TE_EINVAL;
                }
            }

            memcpy(&bcast, addr, sizeof(bcast));
            bcast.sin_addr.s_addr |= htonl(nmask);

            /* Set broadcast address */
            rc = cfg_set_instance_fmt(CFG_VAL(ADDRESS, &bcast),
                                      "%s/net_addr:%s/broadcast:", oid,
                                      inet_ntop(addr->sa_family,
                                                &SIN(addr)->sin_addr,
                                                buf, sizeof(buf)));
            if (rc != 0)
            {
                int rc2;

                ERROR("%s(): Failed to set broadcast address: %r",
                      __FUNCTION__, rc);
                rc2 = cfg_del_instance_fmt(TRUE,
                                           "%s/net_addr:%s", oid,
                                           inet_ntop(addr->sa_family,
                                                     &SIN(addr)->sin_addr,
                                                     buf, sizeof(buf)));
                if (rc2 != 0)
                {
                    ERROR("%s(): Failed to delete address to rollback: %r",
                          __FUNCTION__, rc2);
                }
                return rc;
            }
        }
    }
    else if (TE_RC_GET_ERROR(rc) == TE_EEXIST)
    {
        WARN("%s already has address %s", oid,
             inet_ntop(addr->sa_family, te_sockaddr_get_netaddr(addr),
                       buf, sizeof(buf)));
    }
    else
    {
        ERROR("%s(): Failed to add address for %s: %r", __FUNCTION__, oid, rc);
    }

    return rc;
}

/* See description in tapi_cfg_base.h */
te_errno
tapi_cfg_del_if_ip4_addresses(const char *ta,
                              const char *if_name,
                              const struct sockaddr *addr_to_save)
{
    return tapi_cfg_save_del_if_ip4_addresses(ta, if_name,
                                              addr_to_save, TRUE,
                                              NULL, NULL, NULL, NULL);
}

/* See description in tapi_cfg_base.h */
te_errno
tapi_cfg_restore_if_ip4_addresses(const char *ta,
                                  const char *if_name,
                                  struct sockaddr *saved_addrs,
                                  int *saved_prefixes,
                                  te_bool *saved_broadcasts,
                                  int saved_count)
{
    return tapi_cfg_restore_if_addresses(ta, if_name, saved_addrs,
                                         saved_prefixes, saved_broadcasts,
                                         saved_count);
}

te_errno
tapi_cfg_save_del_if_addresses(const char *ta,
                                      const char *if_name,
                                      const struct sockaddr *addr_to_save,
                                      te_bool save_first,
                                      struct sockaddr **saved_addrs,
                                      int **saved_prefixes,
                                      te_bool **saved_broadcasts,
                                      int *saved_count,
                                      int addr_fam)
{
    cfg_handle                 *addrs = NULL;
    unsigned int                addr_num = 0;
    cfg_handle                 *broadcasts;
    unsigned int                brd_num = 0;
    unsigned int                i;
    unsigned int                j;
    unsigned int                prefix;
    char                       *addr_str = NULL;
    struct sockaddr_storage     addr;
    te_errno                    rc = 0;

    if ((addr_fam != AF_INET) && (addr_fam != AF_INET6))
    {
        ERROR("%s(): Invalid addr_fam parameter value %d",
              __FUNCTION__, addr_fam);
        return TE_RC(TE_TAPI, TE_EAFNOSUPPORT);
    }

    if (saved_count != NULL)
        *saved_count = 0;
    if (saved_addrs != NULL)
        *saved_addrs = NULL;
    if (saved_prefixes != NULL)
        *saved_prefixes = NULL;
    if (saved_broadcasts != NULL)
        *saved_broadcasts = NULL;

    if (addr_to_save != NULL && addr_to_save->sa_family != addr_fam)
    {
        ERROR("%s(): Invalid family %d of the address to save",
              __FUNCTION__, (int)addr_to_save->sa_family);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    if ((rc = cfg_find_pattern_fmt(&addr_num, &addrs,
                                   "/agent:%s/interface:%s/net_addr:*",
                                   ta, if_name)) != 0)
    {
        ERROR("%s(): Failed to get net_addr list for /agent:%s/interface:%s/",
              __FUNCTION__, ta, if_name);
        return rc;
    }

    if (saved_addrs != NULL)
    {
        struct sockaddr *tmp_addrs;
        int             *tmp_prefixes;
        te_bool         *tmp_broadcasts;

        tmp_addrs = TE_ALLOC(sizeof(struct sockaddr) * addr_num);
        if (tmp_addrs == NULL)
        {
            free(addrs);
            return TE_RC(TE_TAPI, TE_ENOMEM);
        }

        if (saved_prefixes != NULL)
        {
            tmp_prefixes = TE_ALLOC(sizeof(int) * addr_num);
            if (saved_prefixes == NULL)
            {
                free(addrs);
                free(tmp_addrs);
                return TE_RC(TE_TAPI, TE_ENOMEM);
            }
        }

        if (saved_broadcasts != NULL)
        {
            tmp_broadcasts = TE_ALLOC(sizeof(te_bool) * addr_num);
            if (saved_broadcasts == NULL)
            {
                free(addrs);
                free(tmp_addrs);
                free(tmp_prefixes);
                return TE_RC(TE_TAPI, TE_ENOMEM);
            }
        }

        *saved_addrs = tmp_addrs;
        if (saved_prefixes != NULL)
            *saved_prefixes = tmp_prefixes;
        if (saved_broadcasts != NULL)
            *saved_broadcasts = tmp_broadcasts;
    }

    for (i = 0, j = 0; i < addr_num; i++)
    {
        free(addr_str);
        addr_str = NULL;

        if ((rc = cfg_get_inst_name(addrs[i], &addr_str)) != 0)
        {
            ERROR("%s(): Failed to get instance name: %r", __FUNCTION__, rc);
            break;
        }
        if ((rc = te_sockaddr_netaddr_from_string(addr_str,
                                                  SA(&addr))) != 0)
        {
            ERROR("%s(): Failed to convert address from string '%s': %r",
                  __FUNCTION__, addr_str, rc);
            break;
        }

        if (addr.ss_family != addr_fam)
            continue;

        /*
         * Do not remove IPv6 link-local addresses, IPv6
         * requires them always to be present.
         */
        if (addr.ss_family == AF_INET6 &&
            IN6_IS_ADDR_LINKLOCAL(&SIN6(&addr)->sin6_addr))
        {
            continue;
        }

        if (addr_to_save == NULL && save_first)
        {
            /* Just to mark that one address is saved */
            addr_to_save = SA(&addr);
            continue;
        }

        if (addr_to_save != NULL && addr_to_save != SA(&addr))
        {
            if (addr_fam == AF_INET)
            {
                if (memcmp(&SIN(addr_to_save)->sin_addr,
                           &SIN(&addr)->sin_addr,
                           sizeof(SIN(&addr)->sin_addr)) == 0)
                {
                    continue;
                }
            }
            else
            {
                if (memcmp(&SIN6(addr_to_save)->sin6_addr,
                           &SIN6(&addr)->sin6_addr,
                           sizeof(SIN6(&addr)->sin6_addr)) == 0)
                {
                    continue;
                }
            }
        }

        if ((rc = cfg_get_instance(addrs[i], CVT_INTEGER, &prefix)) != 0)
        {
            ERROR("%s(): Failed to get prefix of address with handle %#x: %r",
                  __FUNCTION__, addrs[i], rc);
            break;
        }

        if ((rc = cfg_find_pattern_fmt(&brd_num, &broadcasts,
                                       "/agent:%s/interface:%s/net_addr:"
                                       "%s/broadcast:*",
                                       ta, if_name, addr_str)) != 0)
        {
            ERROR("%s(): Failed to get broadcast address for /agent:%s/"
                  "interface:%s/net_addr:%s/broadcast:*",
                  __FUNCTION__, ta, if_name, addr_str);
            break;
        }

        free(broadcasts);

        if ((rc = cfg_del_instance(addrs[i], FALSE)) != 0)
        {
            ERROR("%s(): Failed to delete address with handle %#x: %r",
                  __FUNCTION__, addrs[i], rc);
            break;
        }
        else if (saved_addrs != NULL)
        {
            if (saved_prefixes != NULL)
                (*saved_prefixes)[j] = prefix;
            if (saved_broadcasts != NULL)
                (*saved_broadcasts)[j] = brd_num > 0 ? TRUE : FALSE;

            memcpy(&((*saved_addrs)[j++]), SA(&addr),
                   sizeof(struct sockaddr));
        }
    }

    free(addrs);
    free(addr_str);

    if (saved_count != NULL)
        *saved_count = j;

    return rc;
}

/* See description in tapi_cfg_base.h */
te_errno
tapi_cfg_save_del_if_ip4_addresses(const char *ta,
                                   const char *if_name,
                                   const struct sockaddr *addr_to_save,
                                   te_bool save_first,
                                   struct sockaddr **saved_addrs,
                                   int **saved_prefixes,
                                   te_bool **saved_broadcasts,
                                   int *saved_count)
{
    return tapi_cfg_save_del_if_addresses(ta, if_name, addr_to_save,
                                          save_first, saved_addrs,
                                          saved_prefixes,
                                          saved_broadcasts, saved_count,
                                          AF_INET);
}

/* See description in tapi_cfg_base.h */
te_errno
tapi_cfg_save_del_if_ip6_addresses(const char *ta,
                                   const char *if_name,
                                   const struct sockaddr *addr_to_save,
                                   te_bool save_first,
                                   struct sockaddr **saved_addrs,
                                   int **saved_prefixes,
                                   te_bool **saved_broadcasts,
                                   int *saved_count)
{
    return tapi_cfg_save_del_if_addresses(ta, if_name, addr_to_save,
                                          save_first, saved_addrs,
                                          saved_prefixes,
                                          saved_broadcasts, saved_count,
                                          AF_INET6);
}

/* See description in tapi_cfg_base.h */
te_errno
tapi_cfg_del_if_ip6_addresses(const char *ta,
                              const char *if_name,
                              const struct sockaddr *addr_to_save)
{
    return tapi_cfg_save_del_if_ip6_addresses(ta, if_name,
                                              addr_to_save, TRUE,
                                              NULL, NULL, NULL, NULL);
}

/* See description in tapi_cfg_base.h */
te_errno
tapi_cfg_restore_if_addresses(const char *ta,
                              const char *if_name,
                              struct sockaddr *saved_addrs,
                              int *saved_prefixes,
                              te_bool *saved_broadcasts,
                              int saved_count)
{
    int        i;
    te_errno   rc = 0;
    cfg_handle addr;

    for (i = 0; i < saved_count; i++)
    {
        if ((rc = tapi_cfg_base_if_add_net_addr(ta, if_name,
                                                (const struct sockaddr *)
                                                    &saved_addrs[i],
                                                saved_prefixes[i],
                                                saved_broadcasts[i],
                                                &addr)) != 0)
        {
            ERROR("Failed to restore address: %r", rc);
            break;
        }
    }

    return rc;
}

/* See description in tapi_cfg_base.h */
te_errno
tapi_cfg_base_if_add_vlan(const char *ta, const char *if_name,
                          uint16_t vid, char **vlan_ifname)
{
    te_errno rc = 0;

    if ((rc = cfg_add_instance_fmt(NULL, CVT_NONE, NULL,
                                   "/agent:%s/interface:%s/vlans:%d",
                                   ta, if_name, vid)) != 0)
    {
        ERROR("%s(): Failed to add VLAN with VID=%d to %s",
              __FUNCTION__, vid, if_name);
        return rc;
    }

    if ((rc = cfg_get_instance_string_fmt(vlan_ifname,
                         "/agent:%s/interface:%s/vlans:%d/ifname:",
                         ta, if_name, vid)) != 0)
    {
        ERROR("%s(): Failed to get interface name for VLAN interface "
              "with VID=%d on %s", __FUNCTION__, vid, if_name);
        return rc;
    }

    rc = tapi_cfg_base_if_add_rsrc(ta, *vlan_ifname);
    if (rc != 0)
    {
        ERROR("%s(): Failed to grab VLAN interface %s", __FUNCTION__,
              *vlan_ifname);
        return rc;
    }

    if (tapi_host_ns_enabled())
        rc = tapi_host_ns_if_add(ta, *vlan_ifname, if_name);

    return rc;
}

/* See description in tapi_cfg_base.h */
te_errno
tapi_cfg_base_if_del_vlan(const char *ta, const char *if_name,
                          uint16_t vid)
{
    char    *vlan_ifname = NULL;
    te_errno rc;
    te_errno rc2 = 0;

    rc = cfg_get_instance_fmt(NULL, &vlan_ifname,
                               "/agent:%s/interface:%s/vlans:%d/ifname:",
                               ta, if_name, vid);
    if (rc != 0)
    {
        ERROR("%s(): Failed to get interface name for VLAN interface "
              "with VID=%hu on %s", __FUNCTION__, vid, if_name);
    }
    else
    {
        if (tapi_host_ns_enabled())
            rc = tapi_host_ns_if_del(ta, vlan_ifname, TRUE);

        rc2 = tapi_cfg_base_if_del_rsrc(ta, vlan_ifname);
        if (rc2 == TE_RC(TE_CS, TE_ENOENT))
            rc2 = 0;
        else if (rc2 != 0)
        {
            ERROR("%s(): Failed to release VLAN interface %s", __FUNCTION__,
                  vlan_ifname);
        }
        if (rc == 0)
            rc = rc2;

        free(vlan_ifname);
    }

    rc2 = cfg_del_instance_fmt(FALSE, "/agent:%s/interface:%s/vlans:%d",
                              ta, if_name, vid);
    if (rc2 != 0)
        ERROR("%s(): Failed to delete VLAN with VID=%d from %s",
              __FUNCTION__, vid, if_name);
    if (rc == 0)
        rc = rc2;

    return rc;
}

/* See description in tapi_cfg_base.h */
te_errno
tapi_cfg_base_if_get_mtu_u(const char *agent, const char *interface,
                           int *mtu)
{
    te_errno rc;

    if ((rc = cfg_get_instance_int_fmt(mtu, "/agent:%s/interface:%s/mtu:",
                                       agent, interface)) != 0)
        ERROR("%s(): Failed to get MTU value for %s on %s: %r",
              __FUNCTION__, interface, agent, rc);

    return rc;
}

/* See description in tapi_cfg_base.h */
te_errno
tapi_cfg_base_if_set_mtu_ext(const char *agent, const char *interface,
                             int mtu, int *old_mtu, te_bool fast)
{
    te_errno        rc;
    int             old_mtu_l = 0;
    int             assigned_mtu;

#define MTU_ERR(_error_msg...) \
do { \
    ERROR(_error_msg); \
    return rc; \
} while(0)

    if ((rc = tapi_cfg_base_if_get_mtu_u(agent, interface,
                                         &old_mtu_l)) != 0)
        MTU_ERR("%s(): Failed to get old MTU value for %s on %s: %r",
                __FUNCTION__, interface, agent, rc);

    if (old_mtu != NULL)
        *old_mtu = old_mtu_l;

    if ((rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, mtu),
                                   "/agent:%s/interface:%s/mtu:",
                                   agent, interface)) != 0)
        MTU_ERR("%s(): Failed to set new MTU for %s on %s: %r",
                __FUNCTION__, interface, agent, rc);

    /**
     * IPv6 doesn't support MTU value less then 1280, IPv6 address
     * is removed from interface if set such MTU. The address doesn't
     * come back automatically if then MTU is set back. But it returns
     * if restart the network interface.
     */
    if (old_mtu_l < 1280 && mtu >= 1280)
    {
        RING("Network interface %s on %s will put down/up to avoid "
             "configurator-IPv6 problems.", interface, agent);
        if ((rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, 0),
                                       "/agent:%s/interface:%s/status:",
                                        agent, interface)) != 0)
            MTU_ERR("%s(): Failed to put down interface %s on %s: %r",
                    __FUNCTION__, interface, agent, rc);
        sleep(1);

        if ((rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, 1),
                                       "/agent:%s/interface:%s/status:",
                                       agent, interface)) != 0)
            MTU_ERR("%s(): Failed to put up interface %s on %s: %r",
                    __FUNCTION__, interface, agent, rc);
        if (fast)
        {
            usleep(100000);
        }
        else
        {
            CFG_WAIT_CHANGES;

            /*
             * For some types of interfaces (bonding, teaming) it may take
             * more than 10 seconds before they are really UP, and
             * CFG_WAIT_CHANGES may be not enough in such case.
             */
            rc = tapi_cfg_phy_state_wait_up(agent, interface, 60000);
            if (rc != 0)
            {
                if (rc == TE_RC(TE_TAPI, TE_EOPNOTSUPP) ||
                    rc == TE_RC(TE_CS, TE_ENOENT))
                {
                    WARN("interface:/phy:/state: is not registered or not "
                         "supported, cannot check whether the interface %s "
                         "on the agent %s is UP", interface, agent);
                    rc = 0;
                }
                else
                {
                    MTU_ERR("%s(): failed to wait until the interface %s on "
                            "the agent %s becomes UP", __FUNCTION__,
                            interface, agent);
                }
            }
        }
    }

    if (tapi_cfg_base_if_get_mtu_u(agent, interface, &assigned_mtu) != 0)
        MTU_ERR("%s(): Failed to get assigned MTU value for %s on %s: %r",
                __FUNCTION__, interface, agent, rc);

    if (assigned_mtu != mtu)
    {
        if (assigned_mtu == old_mtu_l)
            ERROR("%s(): MTU was set to %d, but currently it is equal to old "
                  "MTU %d", __FUNCTION__, mtu, assigned_mtu);
        else
            ERROR("%s(): MTU was set to %d, but currently it is %d",
                  __FUNCTION__, mtu, assigned_mtu);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

#undef MTU_ERR

    return rc;
}

/* See description in 'tapi_cfg_base.h' */
te_errno
tapi_cfg_base_if_set_mtu_leastwise(const char   *ta,
                                   const char   *ifname,
                                   unsigned int  mtu)
{
    int      old_mtu;
    te_errno rc;

    rc = tapi_cfg_base_if_get_mtu_u(ta, ifname, &old_mtu);
    if (rc != 0)
        return rc;

    assert(old_mtu >= 0);
    if ((unsigned int)old_mtu < mtu)
        rc = tapi_cfg_base_if_set_mtu(ta, ifname, mtu, NULL);

    return rc;
}

/* See description in tapi_cfg_base.h */
te_errno
tapi_cfg_base_if_add_rsrc(const char *ta, const char *ifname)
{
    char if_oid[CFG_OID_MAX];

    snprintf(if_oid, CFG_OID_MAX, "/agent:%s/interface:%s", ta, ifname);
    return cfg_add_instance_fmt(NULL, CVT_STRING, if_oid,
                                "/agent:%s/rsrc:%s", ta, ifname);
}

/* See description in tapi_cfg_base.h */
te_errno
tapi_cfg_base_if_check_add_rsrc(const char *ta, const char *ifname)
{
    if (cfg_get_instance_string_fmt(NULL, "/agent:%s/rsrc:%s", ta, ifname) == 0)
        return 0;

    return tapi_cfg_base_if_add_rsrc(ta, ifname);
}

/* See description in tapi_cfg_base.h */
te_errno
tapi_cfg_base_if_del_rsrc(const char *ta, const char *ifname)
{
    return cfg_del_instance_fmt(FALSE, "/agent:%s/rsrc:%s", ta, ifname);
}

/* See description in tapi_cfg_base.h */
te_errno
tapi_cfg_base_if_add_macvlan(const char *ta, const char *link,
                             const char *ifname, const char *mode)
{
    int rc;

    rc = cfg_add_instance_fmt(NULL, CFG_VAL(STRING, mode),
                              "/agent:%s/interface:%s/macvlan:%s",
                              ta, link, ifname);
    if (rc != 0)
        return rc;

    rc = tapi_cfg_base_if_add_rsrc(ta, ifname);
    if (rc != 0)
        return rc;

    rc = tapi_cfg_base_if_up(ta, ifname);

    if (rc == 0 && tapi_host_ns_enabled())
        rc = tapi_host_ns_if_add(ta, ifname, link);

    return rc;
}

/* See description in tapi_cfg_base.h */
te_errno
tapi_cfg_base_if_del_macvlan(const char *ta, const char *link,
                             const char *ifname)
{
    int rc;
    rc = cfg_del_instance_fmt(FALSE, "/agent:%s/interface:%s/macvlan:%s",
                              ta, link, ifname);
    if (rc != 0)
        return rc;

    rc = tapi_cfg_base_if_del_rsrc(ta, ifname);
    if (rc == TE_RC(TE_CS, TE_ENOENT))
        rc = 0;

    if (tapi_host_ns_enabled())
    {
        te_errno rc2;
        rc2 = tapi_host_ns_if_del(ta, ifname, TRUE);
        if (rc == 0)
            rc = rc2;
    }

    return rc;
}

/* See description in tapi_cfg_base.h */
te_errno
tapi_cfg_base_if_get_macvlan_mode(const char *ta, const char *link,
                                  const char *ifname, char **mode)
{
    return cfg_get_instance_fmt(NULL, mode,
                                "/agent:%s/interface:%s/macvlan:%s",
                                ta, link, ifname);
}

/* See description in tapi_cfg_base.h */
te_errno
tapi_cfg_base_if_set_macvlan_mode(const char *ta, const char *link,
                                  const char *ifname, const char *mode)
{
    return cfg_set_instance_fmt(CFG_VAL(STRING, mode),
                                "/agent:%s/interface:%s/macvlan:%s",
                                ta, link, ifname);
}

/* See description in tapi_cfg_base.h */
te_errno
tapi_cfg_base_if_add_ipvlan(const char *ta, const char *link,
                            const char *ifname, const char *mode,
                            const char *flag)
{
    int         rc;
    te_string   mode_flag = TE_STRING_INIT_STATIC(64); /* i.e. 'l3s:private' */

    if ((rc = te_string_append(&mode_flag, "%s:%s",
                           mode != NULL ? mode : TAPI_CFG_IPVLAN_MODE_DEFAULT,
                           flag != NULL ? flag : TAPI_CFG_IPVLAN_FLAG_DEFAULT
                           )) != 0)
    {
        return rc;
    }

    rc = cfg_add_instance_fmt(NULL, CFG_VAL(STRING, mode_flag.ptr),
                              "/agent:%s/interface:%s/ipvlan:%s",
                              ta, link, ifname);
    if (rc != 0)
        return rc;

    rc = tapi_cfg_base_if_add_rsrc(ta, ifname);
    if (rc != 0)
        return rc;

    rc = tapi_cfg_base_if_up(ta, ifname);

    if (rc == 0 && tapi_host_ns_enabled())
        rc = tapi_host_ns_if_add(ta, ifname, link);

    return rc;
}

/* See description in tapi_cfg_base.h */
te_errno
tapi_cfg_base_if_del_ipvlan(const char *ta, const char *link,
                            const char *ifname)
{
    int rc;
    rc = cfg_del_instance_fmt(FALSE, "/agent:%s/interface:%s/ipvlan:%s",
                              ta, link, ifname);
    if (rc != 0)
        return rc;

    rc = tapi_cfg_base_if_del_rsrc(ta, ifname);
    if (rc == TE_RC(TE_CS, TE_ENOENT))
        rc = 0;

    if (tapi_host_ns_enabled())
    {
        te_errno rc2;
        rc2 = tapi_host_ns_if_del(ta, ifname, TRUE);
        if (rc == 0)
            rc = rc2;
    }

    return rc;
}

/* See description in tapi_cfg_base.h */
te_errno
tapi_cfg_base_if_get_ipvlan_mode(const char *ta, const char *link,
                                 const char *ifname, char **mode,
                                 char **flag)
{
    char *mode_flag = NULL;
    char *val = NULL;
    int   rc;

    if ((rc = cfg_get_instance_string_fmt(&mode_flag,
                                          "/agent:%s/interface:%s/ipvlan:%s",
                                          ta, link, ifname)) != 0)
    {
        return rc;
    }

    if ((val = strtok(mode_flag, ":")) == NULL)
    {
        ERROR("%s(): unexpected ipvlan mode", __FUNCTION__);
        free(mode_flag);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    if ((val = strtok(NULL, ":")) == NULL)
    {
        ERROR("%s(): unexpected ipvlan flag", __FUNCTION__);
        free(mode_flag);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    *mode = mode_flag;
    *flag = tapi_strdup(val);

    return rc;
}

/* See description in tapi_cfg_base.h */
te_errno
tapi_cfg_base_if_set_ipvlan_mode(const char *ta, const char *link,
                                 const char *ifname, const char *mode,
                                 const char *flag)
{
    int         rc;
    te_string   mode_flag = TE_STRING_INIT_STATIC(64); /* i.e. 'l3s:private' */

    if ((rc = te_string_append(&mode_flag, "%s:%s",
                           mode != NULL ? mode : TAPI_CFG_IPVLAN_MODE_DEFAULT,
                           flag != NULL ? flag : TAPI_CFG_IPVLAN_FLAG_DEFAULT
                           )) != 0)
    {
        return rc;
    }
    return cfg_set_instance_fmt(CFG_VAL(STRING, mode_flag.ptr),
                                "/agent:%s/interface:%s/ipvlan:%s",
                                ta, link, ifname);
}

/* See description in tapi_cfg_base.h */
te_errno
tapi_cfg_base_if_add_veth(const char *ta, const char *ifname,
                          const char *peer)
{
    int rc;
    char veth_oid[CFG_OID_MAX];

    snprintf(veth_oid, CFG_OID_MAX, "/agent:%s/veth:%s", ta, ifname);
    rc = cfg_add_instance_fmt(NULL, CFG_VAL(STRING, peer), "%s", veth_oid);
    if (rc != 0)
        return rc;

    rc = tapi_cfg_base_if_add_rsrc(ta, ifname);
    if (rc != 0)
        return rc;

    rc = tapi_cfg_base_if_add_rsrc(ta, peer);
    if (rc != 0)
        return rc;

    rc = cfg_add_instance_fmt(NULL, CVT_STRING, veth_oid,
                              "/agent:%s/rsrc:veth_%s", ta, ifname);
    if (rc != 0)
        return rc;

    rc = tapi_cfg_base_if_up(ta, ifname);
    if (rc != 0)
        return rc;

    rc = tapi_cfg_base_if_up(ta, peer);

    if (rc == 0 && tapi_host_ns_enabled())
        rc = tapi_host_ns_if_add(ta, ifname, peer);

    return rc;
}

/* See description in tapi_cfg_base.h */
te_errno
tapi_cfg_base_if_get_veth_peer(const char *ta, const char *ifname,
                               char **peer)
{
    return cfg_get_instance_fmt(NULL, peer, "/agent:%s/veth:%s", ta, ifname);
}

/* See description in tapi_cfg_base.h */
te_errno
tapi_cfg_base_if_del_veth(const char *ta, const char *ifname)
{
    char *peer = NULL;
    int   rc;
    int   rc2;

    rc = tapi_cfg_base_if_get_veth_peer(ta, ifname, &peer);

    /* Try to delete veth and release resources even if a call fails. */
    rc2 = cfg_del_instance_fmt(FALSE, "/agent:%s/veth:%s", ta, ifname);
    if (rc == 0)
        rc = rc2;

    rc2 = cfg_del_instance_fmt(TRUE, "/agent:%s/rsrc:veth_%s", ta, ifname);
    if (rc == 0)
        rc = rc2;

    if (peer != NULL)
    {
        rc2 = tapi_cfg_base_if_del_rsrc(ta, peer);
        if (rc2 == TE_RC(TE_CS, TE_ENOENT))
            rc2 = 0;
        if (rc == 0)
            rc = rc2;
    }

    rc2 = tapi_cfg_base_if_del_rsrc(ta, ifname);
    if (rc2 == TE_RC(TE_CS, TE_ENOENT))
        rc2 = 0;
    if (rc == 0)
        rc = rc2;

    if (tapi_host_ns_enabled())
    {
        rc2 = tapi_host_ns_if_del(ta, ifname, TRUE);
        if (rc == 0)
            rc = rc2;
    }

    free(peer);

    return rc;
}

/* See description in tapi_cfg_base.h */
te_errno
tapi_cfg_base_if_down_up(const char *ta, const char *ifname)
{
    int rc;

    rc = tapi_cfg_base_if_down(ta, ifname);
    if (rc != 0)
        return rc;

    if (test_behaviour_storage.iface_toggle_delay_ms > 0)
        te_motivated_msleep(test_behaviour_storage.iface_toggle_delay_ms,
                            "wait before bringing the interface up");

    return tapi_cfg_base_if_up(ta, ifname);
}
