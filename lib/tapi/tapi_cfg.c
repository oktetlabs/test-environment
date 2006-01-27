/** @file
 * @brief Test API to access Configurator
 *
 * Implementation
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "Configuration TAPI"

#include "te_config.h"

#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif

#ifdef HAVE_NET_ROUTE_H
#include <net/route.h>
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "te_stdint.h"
#include "conf_api.h"
#include "logger_api.h"
#include "rcf_api.h"

#include "tapi_sockaddr.h"
#include "tapi_cfg_base.h"
#include "tapi_cfg.h"


/** Operations with routing/ARP table */
enum tapi_cfg_oper {
    OP_ADD,   /**< Add operation */
    OP_DEL,   /**< Delete operation */
    OP_GET,   /**< Get operation */
    OP_MODIFY /**< Modify operation */
};

/* Forward declarations */
static int tapi_cfg_route_op(enum tapi_cfg_oper op, const char *ta,
                             int addr_family,
                             const void *dst_addr, int prefix,
                             const void *gw_addr, const char *dev,
                             uint32_t flags, int metric, int tos, int mtu,
                             int win, int irtt, cfg_handle *cfg_hndl);

static int tapi_cfg_neigh_op(enum tapi_cfg_oper op, const char *ta,
                             const char *ifname, 
                             const struct sockaddr *net_addr, 
                             const void *link_addr,
                             void *ret_addr, te_bool *is_static, 
                             cs_neigh_entry_state *state);


/* See description in tapi_cfg.h */
int
tapi_cfg_get_son_mac(const char *father, const char *subid,
                     const char *name, uint8_t *p_mac)
{
    int                 rc;
    cfg_handle          handle;
    cfg_val_type        type;
    struct sockaddr    *p_addr;


    rc = cfg_find_fmt(&handle, "%s/%s:%s", father, subid, name);
    if (rc != 0)
    {
        ERROR("Failed(%x) to get handle of '%s:' son of %s",
            rc, subid, father);
        return rc;
    }
    
    type = CVT_ADDRESS;
    rc = cfg_get_instance(handle, &type, &p_addr);
    if (rc != 0)
    {
        ERROR("Failed(%x) to get MAC address using OID %s/%s:%s",
            rc, father, subid, name);
        return rc;
    }
    if (p_addr->sa_family != AF_LOCAL)
    {
        ERROR("Unexpected address family %d", p_addr->sa_family);
    }
    else
    {
        memcpy(p_mac, p_addr->sa_data, ETHER_ADDR_LEN);
    }
    free(p_addr);

    return rc;
}



/*
 * Temporaraly located here
 */

const char * const cfg_oid_ta_port_admin_status_fmt =
    "/agent:%s/port:%u/admin:/status:";
const char * const cfg_oid_ta_port_oper_status_fmt =
    "/agent:%s/port:%u/state:/status:";
const char * const cfg_oid_ta_port_oper_speed_fmt =
    "/agent:%s/port:%u/state:/speed:";

const char * const cfg_oid_oper_status_fmt =
    "%s/state:/status:";
const char * const cfg_oid_oper_speed_fmt =
    "%s/state:/speed:";

/** Format string for Switch VLAN OID */
static const char * const cfg_oid_ta_vlan_fmt =
    "/agent:%s/vlan:%u";
/** Format string for Switch VLAN port OID */
static const char * const cfg_oid_ta_vlan_port_fmt =
    "/agent:%s/vlan:%u/port:%u";


/**
 * Add VLAN on switch.
 *
 * @param ta_name   - name of the test agent
 * @param vid       - VLAN identifier
 *
 * @return Status code.
 */
int
tapi_cfg_switch_add_vlan(const char *ta_name, uint16_t vid)
{
    /* 
     * Format string without specifiers + TA name + maximum length
     * of printed VID + '\0'
     */
    char        oid[strlen(cfg_oid_ta_vlan_fmt) - 4 +
                    strlen(ta_name) + 5 + 1];
    int         rc;
    cfg_handle  handle;

    
    ENTRY("ta_name=%s vid=%u", ta_name, vid);
    /* Prepare OID */
    sprintf(oid, cfg_oid_ta_vlan_fmt, ta_name, vid);

    rc = cfg_find_str(oid, &handle);
    if (rc == 0)
    {
        int             state;
        cfg_val_type    type;

        VERB("VLAN %u already exists on TA %s", vid, ta_name);
        type = CVT_INTEGER;
        rc = cfg_get_instance(handle, &type, &state);
        if (rc != 0)
        {
            VERB("cfg_get_instance() failed(%r)", rc);
            EXIT("%r", rc);
            return rc;
        }
        if (state != 1)
        {
            ERROR("VLAN %u disabled on TA %s", vid, ta_name);
            EXIT("TE_EENV");
            return TE_EENV;
        }
        rc = TE_EEXIST;
        EXIT("EEXIST");
    }
    else
    {
        VERB("Add instance '%s'", oid);
        rc = cfg_add_instance_str(oid, &handle, CVT_INTEGER, 1);
        if (rc != 0)
        {
            ERROR("Addition of VLAN %u on TA %s failed(%r)",
                vid, ta_name, rc);
        }
        EXIT("%r", rc);
    }

    return rc;
}


/**
 * Delete VLAN from switch.
 *
 * @param ta_name   - name of the test agent
 * @param vid       - VLAN identifier
 *
 * @return Status code.
 */
int
tapi_cfg_switch_del_vlan(const char *ta_name, uint16_t vid)
{
    /* 
     * Format string without specifiers + TA name + maximum length
     * of printed VID + '\0'
     */
    char        oid[strlen(cfg_oid_ta_vlan_fmt) - 4 + 
                    strlen(ta_name) + 5 + 1];
    int         rc;
    cfg_handle  handle;


    /* Prepare OID */
    sprintf(oid, cfg_oid_ta_vlan_fmt, ta_name, vid);

    rc = cfg_find_str(oid, &handle);
    if (rc == 0)
    {
        rc = cfg_del_instance(handle, FALSE);
        if (rc != 0)
        {
            ERROR("Delete of VLAN %u on TA %s failed(%r)",
                vid, ta_name, rc);
        }
    }
    else
    {
        ERROR("VLAN %u on TA %s not found (error %r)",
            vid, ta_name, rc);
    }

    return rc;
}

/**
 * Add port to VLAN on switch.
 *
 * @param ta_name   - name of the test agent
 * @param vid       - VLAN identifier
 * @param port      - port number
 *
 * @return Status code.
 */
int
tapi_cfg_switch_vlan_add_port(const char *ta_name, uint16_t vid,
                              unsigned int port)
{
    /* 
     * Format string without specifiers + TA name + maximum length
     * of printed VID + maximum length of printed port + \0'
     */
    char        oid[strlen(cfg_oid_ta_vlan_port_fmt) - 4 +
                    strlen(ta_name) + 5 + 10 + 1];
    int         rc;
    cfg_handle  handle;


    /* Prepare OID */
    sprintf(oid, cfg_oid_ta_vlan_port_fmt, ta_name, vid, port);

    rc = cfg_find_str(oid, &handle);
    if (rc == 0)
    {
        VERB("Port %u already in VLAN %u on TA %s", port, vid, ta_name);
        rc = TE_EEXIST;
    }
    else
    {
        rc = cfg_add_instance_str(oid, &handle, CVT_NONE);
        if (rc != 0)
        {
            ERROR("Addition of port %u to VLAN %u on TA %s failed(%r)",
                port, vid, ta_name, rc);
        }
    }

    return rc;
}

/**
 * Delete port from VLAN on switch.
 *
 * @param ta_name   - name of the test agent
 * @param vid       - VLAN identifier
 * @param port      - port number
 *
 * @return Status code.
 */
int
tapi_cfg_switch_vlan_del_port(const char *ta_name, uint16_t vid,
                              unsigned int port)
{
    /* 
     * Format string without specifiers + TA name + maximum length
     * of printed VID + maximum length of printed port + \0'
     */
    char        oid[strlen(cfg_oid_ta_vlan_port_fmt) - 4 +
                    strlen(ta_name) + 5 + 10 + 1];
    int         rc;
    cfg_handle  handle;


    /* Prepare OID */
    sprintf(oid, cfg_oid_ta_vlan_port_fmt, ta_name, vid, port);

    rc = cfg_find_str(oid, &handle);
    if (rc == 0)
    {
        rc = cfg_del_instance(handle, FALSE);
        if (rc != 0)
        {
            ERROR("Delete of port %u from VLAN %u on TA %s failed(%r)",
                port, vid, ta_name, rc);
        }
    }
    else
    {
        ERROR("Port %u not in VLAN %u on TA %s (error %r)",
            port, vid, ta_name, rc);
    }

    return rc;
}

/*
 * END of Temporaraly located here
 */

/**
 * Parses instance name and converts its value into routing table entry 
 * data structure.
 *
 * @param inst_name  Instance name that keeps route information
 * @param rt         Routing entry location to be filled in (OUT)
 */
static int
route_parse_inst_name(const char *inst_name, tapi_rt_entry_t *rt)
{
    char        *tmp, *tmp1;
    int          prefix;
    char        *ptr;
    char        *end_ptr;
    char        *term_byte; /* Pointer to the trailing zero byte 
                               in 'inst_name' */
    static char  inst_copy[RCF_MAX_VAL];

    int          family = (strchr(inst_name, ':') == NULL) ?
                          AF_INET : AF_INET6;

    strncpy(inst_copy, inst_name, sizeof(inst_copy));
    inst_copy[sizeof(inst_copy) - 1] = '\0';

    if ((tmp = strchr(inst_copy, '|')) == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    *tmp ='\0';
    rt->dst.ss_family = family;

    if ((family == AF_INET &&
         inet_pton(family, inst_copy, &(SIN(&(rt->dst))->sin_addr)) <= 0) ||
        (family == AF_INET6 &&
         inet_pton(family, inst_copy, &(SIN6(&(rt->dst))->sin6_addr)) <= 0))
    {
        ERROR("Incorrect 'destination address' value in route %s",
              inst_name);
        return TE_RC(TE_TAPI, TE_ENOENT);
    }
    tmp++;
    if (*tmp == '-' || (prefix = strtol(tmp, &tmp1, 10), tmp == tmp1) ||
         ((rt->dst.ss_family == AF_INET && prefix > 32) ||
          (rt->dst.ss_family == AF_INET6 && prefix > 128)))
    {
        ERROR("Incorrect 'prefix length' value in route %s", inst_name);
        return TE_RC(TE_TAPI, TE_ENOENT);
    }
    tmp = tmp1;

    rt->prefix = prefix;

    term_byte = (char *)(tmp + strlen(tmp));

    if ((ptr = strstr(tmp, "metric=")) != NULL)
    {
        end_ptr = ptr += strlen("metric=");
        rt->metric = atoi(end_ptr);
        rt->flags |= TAPI_RT_METRIC;
    }
    
    if ((ptr = strstr(tmp, "tos=")) != NULL)
    {
        end_ptr = ptr += strlen("tos=");
        rt->metric = atoi(end_ptr);
        rt->flags |= TAPI_RT_TOS;
    }       
    
    return 0;
}

/* See the description in tapi_cfg.h */
int
tapi_cfg_get_route_table(const char *ta, int addr_family,
                         tapi_rt_entry_t **rt_tbl, unsigned int *n)
{
    int              rc;
    cfg_handle       handle1;
    cfg_handle       handle2;
    cfg_handle      *handles;
    tapi_rt_entry_t *tbl;
    char            *rt_name = NULL;
    unsigned int     num;
    unsigned int     i;
    
    UNUSED(addr_family);

    if (ta == NULL || rt_tbl == NULL || n == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    rc = cfg_find_pattern_fmt(&num, &handles, "/agent:%s/route:*", ta);
    if (rc != 0)
        return rc;
    
    if (num == 0)
    {
        *rt_tbl = NULL;
        *n = 0;
        return 0;
    }

    if ((tbl = (tapi_rt_entry_t *)calloc(num, sizeof(*tbl))) == NULL)
    {
        free(handles);
        return TE_RC(TE_TAPI, TE_ENOMEM);
    }

    for (i = 0; i < num; i++)
    {
        struct sockaddr  *addr;
        
        if ((rc = cfg_get_inst_name(handles[i], &rt_name)) != 0)
        {
            ERROR("%s: Route handle cannot be processed", __FUNCTION__);
            break;
        }

        rc = route_parse_inst_name(rt_name, &tbl[i]);

        free(rt_name);
        
        assert(rc == 0);

        if ((rc = cfg_get_instance(handles[i], NULL, &addr)) != 0)
        {
            ERROR("%s: Cannot obtain route instance value", __FUNCTION__);
            break;
        }        

        if ((addr->sa_family == AF_INET &&
             SIN(addr)->sin_addr.s_addr != htonl(INADDR_ANY)) ||
            (addr->sa_family == AF_INET6 &&
             !IN6_IS_ADDR_UNSPECIFIED(&SIN6(addr)->sin6_addr)))
        {
            tbl[i].flags |= TAPI_RT_GW;
            memcpy(&tbl[i].gw, addr, sizeof(struct sockaddr_storage));
        }
        
        /* Get route attributes */
        if ((rc = cfg_get_son(handles[i], &handle2)) != 0 ||
            handle2 == CFG_HANDLE_INVALID)
        {
            ERROR("%s: Cannot find any attribute of the route %r",
                  __FUNCTION__, rc);
            break;
        }
        
        do {
            cfg_val_type  type;
            char         *name;
            cfg_oid      *oid;
            void         *val_p;

            handle1 = handle2;
            
            if ((rc = cfg_get_oid(handle1, &oid)) != 0)
            {
                ERROR("%s: Cannot get route attribute name %r",
                      __FUNCTION__, rc);
                break;
            }
            name = ((cfg_inst_subid *)
                            (oid->ids))[oid->len - 1].subid;

            type = CVT_INTEGER;
            if (strcmp(name, "dev") == 0)
            {
                val_p = &tbl[i].dev;
                type = CVT_STRING;
            }
            else if (strcmp(name, "mtu") == 0)
                val_p = &tbl[i].mtu;
            else if (strcmp(name, "win") == 0)
                val_p = &tbl[i].win;
            else if (strcmp(name, "irtt") == 0)
                val_p = &tbl[i].irtt;
            else
            {
                ERROR("%s(): Unknown route attribute found %s",
                      __FUNCTION__, name);
                free(oid);
                rc = TE_RC(TE_TAPI, TE_EINVAL);
                break;
            }

            if ((rc = cfg_get_instance(handle1, &type, val_p)) != 0)
            {
                ERROR("%s(): Cannot get value of %s route attribute %r",
                      __FUNCTION__, name, rc);
                free(oid);
                break;
            }
            free(oid);

            rc = cfg_get_brother(handle1, &handle2);
            if (rc != 0)
            {
                ERROR("%s(): Cannot get brother of route attribute %r",
                      __FUNCTION__, rc);
                break;
            }
        } while (handle2 != CFG_HANDLE_INVALID);

        if (rc != 0)
            break;
            
        assert(handle2 == CFG_HANDLE_INVALID);

        tbl[i].hndl = handles[i];
    }
    free(handles);
    
    if (rc != 0)
    {
        free(tbl);
        return rc;
    }

    *rt_tbl = tbl;
    *n = num;

    return 0;
}

/* See the description in tapi_cfg.h */
int
tapi_cfg_add_route(const char *ta, int addr_family,
                   const void *dst_addr, int prefix,
                   const void *gw_addr, const char *dev,
                   uint32_t flags, int metric, int tos, int mtu,
                   int win, int irtt, cfg_handle *cfg_hndl)
{
    return tapi_cfg_route_op(OP_ADD, ta, addr_family,
                             dst_addr, prefix, gw_addr, dev,
                             flags, metric, tos, mtu, win, irtt, cfg_hndl);
}

/* See the description in tapi_cfg.h */
int
tapi_cfg_modify_route(const char *ta, int addr_family,
                   const void *dst_addr, int prefix,
                   const void *gw_addr, const char *dev,
                   uint32_t flags, int metric, int tos, int mtu,
                   int win, int irtt, cfg_handle *cfg_hndl)
{
    return tapi_cfg_route_op(OP_MODIFY, ta, addr_family,
                             dst_addr, prefix, gw_addr, dev,
                             flags, metric, tos, mtu, win, irtt, cfg_hndl);
}


/* See the description in tapi_cfg.h */
int
tapi_cfg_del_route_tmp(const char *ta, int addr_family,
                       const void *dst_addr, int prefix, 
                       const void *gw_addr, const char *dev,
                       uint32_t flags, int metric, int tos,
                       int mtu, int win, int irtt)
{
    return tapi_cfg_route_op(OP_DEL, ta, addr_family,
                             dst_addr, prefix, gw_addr, dev,
                             flags, metric, tos, mtu, win, irtt, NULL);
}

/* See the description in tapi_cfg.h */
int
tapi_cfg_del_route(cfg_handle *rt_hndl)
{
    int rc;

    if (rt_hndl == NULL)
        return TE_EINVAL;

    if (*rt_hndl == CFG_HANDLE_INVALID)
        return 0;
    
    rc = cfg_del_instance(*rt_hndl, FALSE);
    if (rc == 0)
    {
        *rt_hndl = CFG_HANDLE_INVALID;
    }

    return rc;
}

/* See the description in tapi_cfg.h */
te_errno
tapi_cfg_get_neigh_entry(const char *ta, const char *ifname, 
                         const struct sockaddr *net_addr,
                         void *ret_addr, te_bool *is_static, 
                         cs_neigh_entry_state *state)
{
    return tapi_cfg_neigh_op(OP_GET, ta, ifname, net_addr, NULL, 
                             ret_addr, is_static, state);
}

/* See the description in tapi_cfg.h */
te_errno
tapi_cfg_add_neigh_entry(const char *ta, const char *ifname, 
                         const struct sockaddr *net_addr,
                         const void *link_addr, te_bool is_static)
{
    return tapi_cfg_neigh_op(OP_ADD, ta, ifname, net_addr, link_addr, NULL,
                             &is_static, NULL);
}

/* See the description in tapi_cfg.h */
te_errno
tapi_cfg_del_neigh_entry(const char *ta, const char *ifname, 
                         const struct sockaddr *net_addr)
{
    return tapi_cfg_neigh_op(OP_DEL, ta, ifname, net_addr, NULL, 
                             NULL, NULL, NULL);
}

/* See the description in tapi_cfg.h */
te_errno
tapi_cfg_del_neigh_dynamic(const char *ta, const char *ifname)
{
    cfg_handle     *hndls = NULL;
    unsigned int    num;
    unsigned int    i;
    te_errno        rc;
    te_errno        result = 0;
    
    if (ifname == NULL)
    {
        if ((rc = cfg_find_pattern_fmt(&num, &hndls,
                                       "/agent:%s/interface:*", 
                                       ta)) != 0)
        {
            return rc;
        }
        for (i = 0; i < num; i++)
        {
            char *name = NULL; 
            
            if ((rc = cfg_get_inst_name(hndls[i], &name)) != 0 ||
                (rc = tapi_cfg_del_neigh_dynamic(ta, name)) != 0)
            {
                TE_RC_UPDATE(result, rc);
            }
            free(name);
        }
        free(hndls);
        
        return result;
    }

    if ((rc = cfg_synchronize_fmt(TRUE, "/agent:%s/interface:%s", 
                                  ta, ifname)) != 0)
        return rc;

    if ((rc = cfg_find_pattern_fmt(&num, &hndls,
                                   "/agent:%s/interface:*/neigh_dynamic:*", 
                                   ta)) != 0)
    {
        return rc;
    }
    for (i = 0; i < num; i++)
    {
        if ((rc = cfg_del_instance(hndls[i], FALSE)) != 0)
        {
            TE_RC_UPDATE(result, rc);
        }
    }

    free(hndls);

    return result;
}

/**
 * Perform specified operation with routing table
 * 
 * @param op            Operation
 * @param ta            Test agent
 * @param addr_family   Address family 
 * @param dst_addr      Destination address of the route
 * @param prefix        Prefix for dst_addr
 * @param gw_addr       Gateway address of the route
 * @param cfg_hndl      Handle of the route in Configurator DB (OUT)
 *
 * @return Status code
 *
 * @retval 0  on success
 */                   
static int
tapi_cfg_route_op(enum tapi_cfg_oper op, const char *ta, int addr_family,
                  const void *dst_addr, int prefix, const void *gw_addr,
                  const char *dev, uint32_t flags,
                  int metric, int tos, int mtu, int win, int irtt,
                  cfg_handle *cfg_hndl)
{
    cfg_handle  handle;
    char        dst_addr_str[INET6_ADDRSTRLEN];
    char        dst_addr_str_orig[INET6_ADDRSTRLEN];
    char        route_inst_name[1024];
    int         rc;
    int         netaddr_size = netaddr_get_size(addr_family);
    uint8_t    *dst_addr_copy;
    int         i;
    int         diff;
    uint8_t     mask;

    struct sockaddr_storage ss;

    UNUSED(flags);

    if (netaddr_size < 0)
    {
        ERROR("%s() unknown address family value", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    
    if (prefix < 0 || prefix > (netaddr_size << 3))
    {
        ERROR("%s() fails: Incorrect prefix value specified %d", prefix);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    if ((dst_addr_copy = (uint8_t *)malloc(netaddr_size)) == NULL)
    {
        ERROR("%s() cannot allocate %d bytes for the copy of "
              "the network address", __FUNCTION__,
              netaddr_get_size(addr_family));

        return TE_RC(TE_TAPI, TE_ENOMEM);
    }

    if (inet_ntop(addr_family, dst_addr, dst_addr_str_orig, 
                  sizeof(dst_addr_str_orig)) == NULL)
    {
        ERROR("%s() fails converting binary destination address "
              "into a character string", __FUNCTION__);
        return TE_OS_RC(TE_TAPI, errno);
    }

    memcpy(dst_addr_copy, dst_addr, netaddr_size);
    
    /* Check that dst_addr & netmask == dst_addr */
    for (i = 0; i < netaddr_size; i++)
    {
        diff = ((i + 1) << 3) - prefix;
        
        if (diff < 0)
        {
            /* i-th byte is fully under the mask, so skip it */
            continue;
        }
        if (diff < 8)
            mask = 0xff << diff;
        else
            mask = 0;
        
        if ((dst_addr_copy[i] & mask) != dst_addr_copy[i])
        {
            dst_addr_copy[i] &= mask;
        }
    }
    if (memcmp(dst_addr, dst_addr_copy, netaddr_size) != 0)
    {
        inet_ntop(addr_family, dst_addr_copy, dst_addr_str, 
                  sizeof(dst_addr_str));

        WARN("Destination address specified in the route does not "
             "cleared according to the prefix: prefix length %d, "
             "addr: %s expected to be %s. "
             "[The address is cleared anyway]",
             prefix, dst_addr_str_orig, dst_addr_str);
    }

    if (inet_ntop(addr_family, dst_addr_copy, dst_addr_str, 
                  sizeof(dst_addr_str)) == NULL)
    {
        ERROR("%s() fails converting binary destination address "
              "into a character string", __FUNCTION__);
        free(dst_addr_copy);
        return TE_OS_RC(TE_TAPI, errno);
    }
    free(dst_addr_copy);
    
#define PUT_INTO_BUF(buf_, args...) \
    snprintf((buf_) + strlen(buf_), sizeof(buf_) - strlen(buf_), args)

    route_inst_name[0] = '\0';
    PUT_INTO_BUF(route_inst_name, "%s|%d", dst_addr_str, prefix);
    
    if (metric > 0)
        PUT_INTO_BUF(route_inst_name, ",metric=%d", metric);

    if (tos > 0)
        PUT_INTO_BUF(route_inst_name, ",tos=%d", tos);

    /* Prepare structure with gateway address */
    memset(&ss, 0, sizeof(ss));
    ss.ss_family = addr_family;
           
    if (gw_addr != NULL)
    {
        sockaddr_set_netaddr(SA(&ss), gw_addr);
    }
    
    switch (op)
    {
        case OP_MODIFY:
        {
            
/*
 * Macro makes local set for route attribute whose name specified
 * with 'field_' parameter.
 * Macro uses 'break' statement and is intended to be called inside
 * 'do {} while (FALSE)' brace.
 *
 * You may use this macro with trailing ';', which will just 
 * add empty statement, but do not worry about that.
 */
#define CFG_RT_SET_LOCAL(field_) \
            if ((rc = cfg_set_instance_local_fmt(               \
                          CFG_VAL(INTEGER, field_),             \
                          "/agent:%s/route:%s/" #field_ ":",    \
                          ta, route_inst_name)) != 0)           \
            {                                                   \
                ERROR("%s() fails to set " #field_ " to %d "    \
                      "on route %s on '%s' Agent errno = %r",   \
                      __FUNCTION__,                             \
                      field_, route_inst_name, ta, rc);         \
                break;                                          \
            }
            
            do {
                if ((rc = cfg_set_instance_local_fmt(CFG_VAL(ADDRESS, &ss),
                                                     "/agent:%s/route:%s",
                                                     ta, route_inst_name))
                    != 0)
                {
                    ERROR("%s() fails to set value of route %s on '%s'"
                           " Agent errno = %r",
                           __FUNCTION__, dev, route_inst_name, ta, rc);
                    break;
                }

                if ((dev != NULL) &&
                    ((rc = cfg_set_instance_local_fmt(CFG_VAL(STRING, dev),
                          "/agent:%s/route:%s/dev:", ta, route_inst_name))
                      != 0))
                {
                    ERROR("%s() fails to set dev to %s "
                          "on route %s on '%s' Agent errno = %r",
                          __FUNCTION__, dev, route_inst_name, ta, rc);
                    break;
                }
                
                CFG_RT_SET_LOCAL(win);
                CFG_RT_SET_LOCAL(mtu);
                CFG_RT_SET_LOCAL(irtt);
            } while (FALSE);

#undef CFG_RT_SET_LOCAL

            if (rc == 0)
            {
                rc = cfg_commit_fmt("/agent:%s/route:%s",
                                    ta, route_inst_name);
            }

            break;
        }

        case OP_ADD:
        {
             
            rc = cfg_add_instance_local_fmt(&handle,
                                            CFG_VAL(ADDRESS, &ss),
                                            "/agent:%s/route:%s",
                                            ta, route_inst_name);
             
            if (rc != 0)
            {
                ERROR("%s() fails adding a new route %s on '%s' Agent "
                      "errno = %r", __FUNCTION__,
                      route_inst_name, ta, rc);
                break;
            }

#define CFG_RT_SET_LOCAL(field_) \
            if (field_ != 0 &&                                  \
                (rc = cfg_set_instance_local_fmt(               \
                          CFG_VAL(INTEGER, field_),               \
                          "/agent:%s/route:%s/" #field_ ":",    \
                          ta, route_inst_name)) != 0)           \
            {                                                   \
                ERROR("%s() fails to set " #field_ " to %d "    \
                      "on a new route %s on '%s' "              \
                      "Agent errno = %r", __FUNCTION__,         \
                      field_, route_inst_name, ta, rc);         \
                break;                                          \
            }

            do {
                if ((dev != NULL) &&
                    (rc = cfg_set_instance_local_fmt(
                              CFG_VAL(STRING, dev),
                              "/agent:%s/route:%s/dev:",
                              ta, route_inst_name)) != 0)
                {
                    ERROR("%s() fails to set dev to %s "
                          "on a new route %s on '%s' Agent errno = %r",
                          __FUNCTION__, dev, route_inst_name, ta, rc);
                    break;
                }
                
                CFG_RT_SET_LOCAL(win);
                CFG_RT_SET_LOCAL(mtu);
                CFG_RT_SET_LOCAL(irtt);
           } while (FALSE);

#undef CFG_RT_SET_LOCAL

            if (rc != 0)
            {
                cfg_del_instance(handle, TRUE);
                break;
            }

            if ((rc = cfg_commit_fmt("/agent:%s/route:%s",
                                     ta, route_inst_name)) != 0)
            {
                ERROR("%s() fails to commit a new route %s "
                      "on '%s' Agent errno = %r",
                      __FUNCTION__, route_inst_name, ta, rc);
            }

            if (rc == 0 && cfg_hndl != NULL)
                *cfg_hndl = handle;
            break;
        }

        case OP_DEL:
            if ((rc = cfg_del_instance_fmt(FALSE, "/agent:%s/route:%s",
                                           ta, route_inst_name)) != 0)
            {
                ERROR("%s() fails deleting route %s on '%s' Agent "
                      "errno = %r", __FUNCTION__, route_inst_name, ta, rc);
            }
            break;

        default:
            ERROR("%s(): Operation %d is not supported", __FUNCTION__, op);
            rc = TE_RC(TE_TAPI, TE_EOPNOTSUPP);
            break;
    }

#undef PUT_INTO_BUF

    return rc;
}

/**
 * Perform specified operation with neighbour cache.
 * 
 * @param op            Operation
 * @param ta            Test agent
 * @param ifname        Interface name
 * @param net_addr      Network address
 * @param link_addr     Link-leyer address
 * @param ret_addr      Returned by OP_GET operation link_addr
 * @param state         NULL or location for state of dynamic entry
 *
 * @return Status code
 *
 * @retval 0  on success
 */                   
static int
tapi_cfg_neigh_op(enum tapi_cfg_oper op, const char *ta, 
                  const char *ifname, const struct sockaddr *net_addr,
                  const void *link_addr, void *ret_addr, 
                  te_bool *is_static, cs_neigh_entry_state *state)
{
    cfg_handle handle;
    char       net_addr_str[64];
    int        rc = 0;

    if (ta == NULL || net_addr == NULL || ifname == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);
    
    if (inet_ntop(net_addr->sa_family, 
                  net_addr->sa_family == AF_INET ? 
                      (void *)&SIN(net_addr)->sin_addr :
                      (void *)&SIN6(net_addr)->sin6_addr,
                  net_addr_str, sizeof(net_addr_str)) == NULL)
    {
        ERROR("%s() fails converting binary IPv4 address  "
              "into a character string", __FUNCTION__);
        return TE_OS_RC(TE_TAPI, errno);
    }

    switch (op)
    {
        case OP_GET:
        {
            struct sockaddr *lnk_addr = NULL;

            rc = cfg_get_instance_fmt(NULL, &lnk_addr,
                                      "/agent:%s/interface:%s/"
                                      "neigh_static:%s",
                                      ta, ifname, net_addr_str);
                                      
            if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
            {
                rc = cfg_synchronize_fmt(TRUE, "/agent:%s/interface:%s", 
                                         ta, ifname);
                if (rc != 0)
                    break;

                rc = cfg_get_instance_fmt(NULL, &lnk_addr,
                                          "/agent:%s/interface:%s/"
                                          "neigh_dynamic:%s",
                                          ta, ifname, net_addr_str);
                if (rc == 0) 
                {
                    if (is_static != NULL)
                        *is_static = FALSE;
                        
                    if (state != NULL)
                    {
                        rc = cfg_get_instance_fmt(NULL, state, 
                                                  "/agent:%s/interface:%s/"
                                                  "neigh_dynamic:%s/state:",
                                                  ta, ifname, net_addr_str);
                    }
                }
            }
            else if ((rc == 0) && (is_static != NULL))
            {
                *is_static = TRUE;
            }

            if (rc == 0)
            {
                if (ret_addr != NULL)
                {
                    memcpy(ret_addr, &(lnk_addr->sa_data), IFHWADDRLEN);
                }
                free(lnk_addr);
            }
            else if (TE_RC_GET_ERROR(rc) != TE_ENOENT)
            {
                ERROR("%s() cfg_get_instance_fmt() failed for neighbour "
                      "entry %s on interface %s of TA %s with error %r",
                      __FUNCTION__, net_addr_str, ifname, ta, rc);
            }
            break;
        }

        case OP_ADD:
        {
            struct sockaddr lnk_addr;

            if (link_addr == NULL || is_static == NULL)
                return TE_RC(TE_TAPI, TE_EINVAL);

            memset(&lnk_addr, 0, sizeof(lnk_addr));
            lnk_addr.sa_family = AF_LOCAL;
            memcpy(&(lnk_addr.sa_data), link_addr, IFHWADDRLEN);

            rc = cfg_add_instance_fmt(&handle,
                                      CFG_VAL(ADDRESS, &lnk_addr),
                                      "/agent:%s/interface:%s/neigh_%s:%s",
                                      ta, ifname, 
                                      (*is_static) ? "static" :
                                                     "dynamic",
                                      net_addr_str);
            /* Error is logged by CS */          
            break;
        }
            
        case OP_DEL:
            rc = cfg_find_fmt(&handle, 
                              "/agent:%s/interface:%s/neigh_static:%s",
                              ta, ifname, net_addr_str);
            if (rc != 0)
            {
                if (TE_RC_GET_ERROR(rc) != TE_ENOENT)
                    break;

                rc = cfg_synchronize_fmt(TRUE, "/agent:%s/interface:%s", 
                                         ta, ifname);
                if (rc != 0)
                    break;
                              
                rc = cfg_find_fmt(&handle, 
                                  "/agent:%s/interface:%s/neigh_dynamic:%s",
                                   ta, ifname, net_addr_str);
                
                if (rc != 0)
                {                                   
                    if (TE_RC_GET_ERROR(rc) != TE_ENOENT)
                        break;
                                   
                    RING("%s: there is no neighbour entry for %s"
                         " on interface %s of TA %s", __FUNCTION__, 
                         net_addr_str, ifname, ta);
                    rc = 0;
                    break;
                }
            }
            rc = cfg_del_instance(handle, FALSE);
            /* Error is logged by CS */                
            break;

        default:
            ERROR("%s(): Operation %d is not supported", __FUNCTION__, op);
            rc = TE_RC(TE_TAPI, TE_EOPNOTSUPP);
            break;
    }

    return rc;
}

/* See the description in tapi_cfg.h */
int
tapi_cfg_get_hwaddr(const char *ta,
                    const char *ifname,
                    void *hwaddr, unsigned int *hwaddr_len)
{
    char     buf[1024];
    int      rc;
    char   *ifname_bkp;
    char   *ptr;

    if (hwaddr == NULL || hwaddr_len == NULL)
    {
        ERROR("%s(): It is not allowed to have NULL 'hwaddr' or "
              "'hwaddr_len' parameter", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    if (*hwaddr_len < IFHWADDRLEN)
    {
        ERROR("%s(): 'hwaddr' is too short", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EMSGSIZE);
    }

    /**
     * Configuration model does not support alias interfaces,
     * so that we should truncate trailing :XX part from the interface name.
     */
    if ((ifname_bkp = (char *)malloc(strlen(ifname) + 1)) == NULL)
    {
        return TE_RC(TE_TAPI, TE_ENOMEM);
    }
    memcpy(ifname_bkp, ifname, strlen(ifname) + 1);

    if ((ptr = strchr(ifname_bkp, ':')) != NULL)
        *ptr = '\0';

    snprintf(buf, sizeof(buf), "/agent:%s/interface:%s",
             ta, ifname_bkp);
    free(ifname_bkp);
    if ((rc = tapi_cfg_base_if_get_mac(buf, hwaddr)) != 0)
    {
        return rc;
    }

    return 0;
}

/* See the description in tapi_cfg.h */
te_errno
tapi_cfg_set_hwaddr(const char *ta,
                    const char *ifname,
                    const void *hwaddr, unsigned int hwaddr_len)
{
    char        buf[1024];
    char       *ifname_bkp;
    char       *ptr;

    if (hwaddr == NULL || hwaddr_len == 0)
    {
        ERROR("%s(): It is not allowed to have NULL 'hwaddr' or "
              "'hwaddr_len' parameter", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    if (hwaddr_len < IFHWADDRLEN)
    {
        ERROR("%s(): 'hwaddr' is too short", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EMSGSIZE);
    }

    /**
     * Configuration model does not support alias interfaces,
     * so that we should truncate trailing :XX part from the interface name.
     */
    if ((ifname_bkp = (char *)malloc(strlen(ifname) + 1)) == NULL)
    {
        return TE_RC(TE_TAPI, TE_ENOMEM);
    }
    memcpy(ifname_bkp, ifname, strlen(ifname) + 1);

    if ((ptr = strchr(ifname_bkp, ':')) != NULL)
        *ptr = '\0';

    snprintf(buf, sizeof(buf), "/agent:%s/interface:%s",
             ta, ifname_bkp);
    free(ifname_bkp);
    return tapi_cfg_base_if_set_mac(buf, hwaddr);
}


/* See the description in tapi_cfg.h */
static int
tapi_cfg_alloc_entry_by_handle(cfg_handle parent, cfg_handle *entry)
{
    int             rc;
    cfg_handle      handle;
    cfg_val_type    type = CVT_INTEGER;
    int             value;

    if (entry == NULL)
    {
        ERROR("%s(): Invalid argument", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    *entry = CFG_HANDLE_INVALID;

    for (rc = cfg_get_son(parent, &handle);
         rc == 0 && handle != CFG_HANDLE_INVALID;
         rc = cfg_get_brother(handle, &handle))
    {
        rc = cfg_get_instance(handle, &type, &value);
        if (rc != 0)
        {
            ERROR("%s: Failed to get integer value by handle 0x%x: %r",
                  __FUNCTION__, handle, rc);
            break;
        }
        if (value == 0)
        {
            rc = cfg_set_instance(handle, type, 1);
            if (rc != 0)
            {
                ERROR("%s: Failed to set value of handle 0x%x to 1: %r",
                      __FUNCTION__, handle, rc);
            }
            break;
        }
    }

    if (rc == 0)
    {
        if (handle != CFG_HANDLE_INVALID)
        {
            *entry = handle;
            INFO("Pool 0x%x entry with Cfgr handle 0x%x allocated",
                 parent, *entry);
        }
        else
        {
            INFO("No free entries in pool 0x%x", parent);
            rc = TE_RC(TE_TAPI, TE_ENOENT);
        }
    }
#if 0
    else if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
    {
        INFO("No free entries in pool 0x%x", parent);
    }
#endif
    else
    {
        ERROR("Failed to allocate entry in 0x%x: %r", parent, rc);
    }

    return rc;
}

/* See the description in tapi_cfg.h */
int
tapi_cfg_alloc_entry(const char *parent_oid, cfg_handle *entry)
{
    int         rc;
    cfg_handle  parent;

    rc = cfg_find_str(parent_oid, &parent);
    if (rc != 0)
    {
        ERROR("%s: Failed to convert OID '%s' to handle: %r",
              __FUNCTION__, parent_oid, rc);
        return rc;
    }
    
    return tapi_cfg_alloc_entry_by_handle(parent, entry);
}

/* See the description in tapi_cfg.h */
int
tapi_cfg_free_entry(cfg_handle *entry)
{
    int rc;

    if (entry == NULL)
    {
        ERROR("%s(): Invalid Cfgr handle pointer", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    if (*entry == CFG_HANDLE_INVALID)
    {
        return 0;
    }

    rc = cfg_set_instance(*entry, CVT_INTEGER, 0);
    if (rc != 0)
    {
    ERROR("Failed to free entry by handle 0x%x: %r", *entry, rc);
    }
    else
    {
        INFO("Pool entry with Cfgr handle 0x%x freed", *entry);
        *entry= CFG_HANDLE_INVALID;
    }
    return rc;
}

/* See the description in tapi_cfg.h */
int
tapi_cfg_add_ip4_net(struct sockaddr_in *ip4_net_addr, int prefix, 
                     int state, cfg_handle *entry)
{
    int        rc;
    cfg_handle pool;
    cfg_handle net;
    cfg_handle handle;
    char       buf[INET_ADDRSTRLEN];
    char       oid[64];
    struct sockaddr_in addr;

    *entry = CFG_HANDLE_INVALID;
    rc = cfg_find_str("/ip4_net_pool:", &pool);
    if (rc != 0)
    {
        ERROR("%s: Failed to find /ip4_net_pool instance: %r",
              __FUNCTION__, rc);
        return rc;
    }

    memcpy(&addr, ip4_net_addr, sizeof(addr));
    addr.sin_addr.s_addr = htonl(ntohl(addr.sin_addr.s_addr)
                                       & PREFIX2MASK(prefix));
    inet_ntop(addr.sin_family, &addr.sin_addr, buf, sizeof(buf));

    /* Check for interference with existing nets in the pool */
    for (rc = cfg_get_son(pool, &net);
         rc == 0 && net != CFG_HANDLE_INVALID;
         rc = cfg_get_brother(net, &net))
    {
        cfg_val_type        val_type;
        char               *net_oid;
        struct sockaddr    *sa;
        struct sockaddr_in *net_addr;
        int                 net_prefix;
        struct in_addr      net_mask;

        rc = cfg_get_inst_name_type(net, CVT_ADDRESS, 
                                    (cfg_inst_val *)&sa);
        if (rc != 0)
        {
            ERROR("%s: Cannot get pool net name by handle 0x%x "
                  "as address: %r", __FUNCTION__, net, rc);
            return rc;
        }
        net_addr = SIN(sa);
        rc = cfg_get_oid_str(net, &net_oid);
        if (rc != 0)
        {
            ERROR("%s: Cannot get pool net OID by handle 0x%x: %r",
                  __FUNCTION__, net, rc);
            free(net_addr);
            return rc;
        }
        val_type = CVT_INTEGER;
        rc = cfg_get_instance_fmt(&val_type, &net_prefix,
                                  "%s/prefix:", net_oid);
        if (rc != 0)
        {
            ERROR("%s: Cannot get pool net prefix for %s: %r",
                  __FUNCTION__, net_oid, rc);
            free(net_addr);
            free(net_oid);
            return rc;
        }
        
        /* Compare net from pool with net to be added */
        net_mask.s_addr = htonl(PREFIX2MASK(net_prefix < prefix ? 
                                            net_prefix : prefix));
        if ((net_addr->sin_addr.s_addr & net_mask.s_addr) ==
            (addr.sin_addr.s_addr & net_mask.s_addr))
        {
            ERROR("%s: Cannot add network %s/%d to pool: it interferes "
                  "with %s", __FUNCTION__, buf, prefix, net_oid);
            free(net_addr);
            free(net_oid);
            return TE_EEXIST;
        }

        free(net_addr);
        free(net_oid);
    }

    /* Add new entry to the pool */
    snprintf(oid, sizeof(oid), "/ip4_net_pool:/entry:%s", buf);
    rc = cfg_add_instance_str(oid, &net, CVT_INTEGER, state);
    if (rc != 0)
    {
        ERROR("%s: Failed to add %s to the pool: %r",
              __FUNCTION__, oid, rc);
        return rc;
    }
    rc = cfg_add_instance_fmt(&handle, CFG_VAL(INTEGER, prefix),
                              "%s/prefix:", oid);
    if (rc != 0)
    {
        ERROR("%s: Failed to add %s/prefix to the pool: %r",
              __FUNCTION__, oid, rc);
        return rc;
    }
    rc = cfg_add_instance_fmt(&handle, CFG_VAL(INTEGER, 0),
                              "%s/n_entries:", oid);
    if (rc != 0)
    {
        ERROR("%s: Failed to add %s/n_instance to the pool: %r",
              __FUNCTION__, oid, rc);
        return rc;
    }

    *entry = net;
    RING("Network %s/%d is added to the pool", buf, prefix);

    return 0;
}

/**
 * Internal implementation of tapi_cfg_add_ip4_addr() and
 * tapi_cfg_alloc_ip4_addr(). Add new entry in IPv4 subnet
 * from IPv4 subnets pool.
 *
 * @param ip4_net       IPv4 subnet handle
 * @param ip4_addr      IPv4 address to add or NULL to allocate any
 *                      free address from pool
 * @param p_entry       Location for Cfgr handle of new entry
 * @param addr          Location for added address
 *
 * @return Status code.
 */
static int
tapi_cfg_insert_ip4_addr(cfg_handle ip4_net, struct sockaddr_in *ip4_addr,
                         cfg_handle *p_entry, struct sockaddr_in **addr)
{
    int             rc;
    char           *ip4_net_oid;
    cfg_handle      pool;
    cfg_handle      entry;
    int             n_entries;
    cfg_handle      n_entries_hndl;
    cfg_val_type    val_type;
    int             prefix;
    char            buf[INET_ADDRSTRLEN];
    int             entry_state;


    rc = cfg_get_oid_str(ip4_net, &ip4_net_oid);
    if (rc != 0)
    {
        ERROR("Failed to get OID by handle 0x%x: %r", ip4_net, rc);
        return rc;
    }

    /* Find or create pool of IPv4 subnet addresses */
    rc = cfg_find_fmt(&pool, "%s/pool:", ip4_net_oid);
    if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
    {
        rc = cfg_add_instance_fmt(&pool, CFG_VAL(NONE, NULL),
                                  "%s/pool:", ip4_net_oid);
        if (rc != 0)
        {
            ERROR("Failed to add object instance '%s/pool:': %r",
                  ip4_net_oid, rc);
            free(ip4_net_oid);
            return rc;
        }
    }
    else if (rc != 0)
    {
        ERROR("Failed to find object instance '%s/pool:': %r",
              ip4_net_oid, rc);
        free(ip4_net_oid);
        return rc;
    }
    
    rc = tapi_cfg_alloc_entry_by_handle(pool, &entry);
    if (TE_RC_GET_ERROR(rc) != TE_ENOENT)
    {
        free(ip4_net_oid);
        if (rc == 0)
        {
            /* Get address */
            rc = cfg_get_inst_name_type(entry, CVT_ADDRESS,
                                        (cfg_inst_val *)addr);
            if (rc != 0)
            {
                ERROR("Failed to get IPv4 address as instance name of "
                      "0x%x: %r", entry, rc);
            }
            else if (p_entry != NULL)
            {
                *p_entry = entry;
            }
        }
        return rc;
    }
    
    /* No available entries */

    /* Get number of entries in the pool */
    rc = cfg_find_fmt(&n_entries_hndl, "%s/n_entries:", ip4_net_oid);
    if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
    {
        rc = cfg_add_instance_fmt(&n_entries_hndl, CFG_VAL(INTEGER, 0),
                                  "%s/n_entries:", ip4_net_oid);
        if (rc != 0)
        {
            ERROR("Failed to add object instance '%s/n_entries:': %r",
                  ip4_net_oid, rc);
            free(ip4_net_oid);
            return rc;
        }
        n_entries = 0;
    }
    else if (rc != 0)
    {
        ERROR("Failed to find object instance '%s/n_entries:': %r",
              ip4_net_oid, rc);
        free(ip4_net_oid);
        return rc;
    }
    else
    {
        rc = cfg_get_instance(n_entries_hndl, CVT_INTEGER, &n_entries);
        if (rc != 0)
        {
            ERROR("Failed to get number of entries in the pool: %r",
                  rc);
            free(ip4_net_oid);
            return rc;
        }
    }

    /* Create one more entry */
    n_entries++;

    /* Get network prefix length */
    val_type = CVT_INTEGER;
    rc = cfg_get_instance_fmt(&val_type, &prefix,
                              "%s/prefix:", ip4_net_oid);
    if (rc != 0)
    {
        ERROR("Failed to get prefix length of '%s': %r",
              ip4_net_oid, rc);
        free(ip4_net_oid);
        return rc;
    }

    /* Check for sufficient space */
    assert((prefix >= 0) && (prefix <= 32));
    if (n_entries > ((1 << ((sizeof(struct in_addr) << 3) - prefix)) - 2))
    {
        ERROR("All addresses of the subnet '%s' are used",
              ip4_net_oid);
        free(ip4_net_oid);
        return TE_RC(TE_TAPI, TE_ENOENT);
    }

    /* Get subnet address */
    rc = cfg_get_inst_name_type(ip4_net, CVT_ADDRESS,
                                (cfg_inst_val *)addr);
    if (rc != 0)
    {
        ERROR("Failed to get IPv4 subnet address from '%s': %r",
              ip4_net_oid, rc);
        free(ip4_net_oid);
        return rc;
    }

    if (ip4_addr == NULL)
    {
        /* Dynamic allocation of IP */
        /* TODO: Optimize free address search */
        do
        {
            /* Make address from subnet address */
            (*addr)->sin_addr.s_addr =
                htonl(ntohl((*addr)->sin_addr.s_addr) + 1);
            inet_ntop(AF_INET, &(*addr)->sin_addr, buf, sizeof(buf));

            /* Check if the entry already exists */
            val_type = CVT_INTEGER;
            rc = cfg_get_instance_fmt(&val_type, &entry_state,
                                      "%s/pool:/entry:%s",
                                      ip4_net_oid, buf);
        } while (rc == 0);
    }
    else
    {
        /* Insert predefined IP */
        uint32_t    mask = PREFIX2MASK(prefix);

        inet_ntop(AF_INET, &ip4_addr->sin_addr, buf, sizeof(buf));
        if ((ntohl(ip4_addr->sin_addr.s_addr) & mask) !=
            (ntohl((*addr)->sin_addr.s_addr) & mask))
        {
            ERROR("Cannot add address %s to '%s': does not fit",
                  buf, ip4_net_oid);
            free(ip4_net_oid);
            return TE_EINVAL;
        }
        
        /* Check if the entry already exists */
        val_type = CVT_INTEGER;
        rc = cfg_get_instance_fmt(&val_type, &entry_state,
                                  "%s/pool:/entry:%s", ip4_net_oid, buf);
    }

    if (TE_RC_GET_ERROR(rc) != TE_ENOENT)
    {
        ERROR("Failed to get '%s/pool:/entry:%s' instance while "
              "checking for free address: %r", ip4_net_oid, buf, rc);
        free(ip4_net_oid);
        return rc;
    }

    /* Add used entry in the pool */
    rc = cfg_add_instance_fmt(&entry, CFG_VAL(INTEGER, 1),
                              "%s/pool:/entry:%s", ip4_net_oid, buf);
    if (rc != 0)
    {
        ERROR("Failed to add entry in IPv4 subnet pool '%s': %r",
              ip4_net_oid, rc);
        free(ip4_net_oid);
        return rc;
    }

    /* Update number of entries ASAP */
    rc = cfg_set_instance(n_entries_hndl, CVT_INTEGER, n_entries);
    if (rc != 0)
    {
        ERROR("Failed to get number of entries in the pool: %r",
              rc);
        free(ip4_net_oid);
        return rc;
    }
    RING("Address %s is added to pool entry '%s'", buf, ip4_net_oid);
    free(ip4_net_oid);

    if (p_entry != NULL)
        *p_entry = entry;

    return 0;
}

/* See the description in tapi_cfg.h */
int
tapi_cfg_add_ip4_addr(cfg_handle ip4_net, struct sockaddr_in *ip4_addr,
                      cfg_handle *p_entry)
{
    int rc;
    struct sockaddr_in *addr = NULL;
    
    rc = tapi_cfg_insert_ip4_addr(ip4_net, ip4_addr, p_entry, &addr);
    free(addr);

    return rc;
}

/* See the description in tapi_cfg.h */
int
tapi_cfg_alloc_ip4_addr(cfg_handle ip4_net, cfg_handle *p_entry,
                        struct sockaddr_in **addr)
{
    return tapi_cfg_insert_ip4_addr(ip4_net, NULL, p_entry, addr);
}


/* See the description in tapi_cfg.h */
te_errno
tapi_cfg_env_local_to_agent(void)
{
    const char * const  pattern = "/local:*/env:*";

    te_errno        rc;
    unsigned int    num;
    cfg_handle     *handles = NULL;
    unsigned int    i;
    cfg_oid        *oid = NULL;
    cfg_val_type    type = CVT_STRING;
    char           *new_value = NULL;
    char           *old_value = NULL;

    rc = cfg_find_pattern(pattern, &num, &handles);
    if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
    {
        return 0;
    }
    else if (rc != 0)
    {
        ERROR("Failed to find by pattern '%s': %r", pattern, rc);
        return rc;
    }

    for (i = 0; i < num; ++i)
    {
        rc = cfg_get_instance(handles[i], &type, &new_value);
        if (rc != 0)
        {
            ERROR("%s(): cfg_get_instance() failed for #%u: %r",
                  __FUNCTION__, i, rc);
            break;
        }
        rc = cfg_get_oid(handles[i], &oid);
        if (rc != 0)
        {
            ERROR("%s(): cfg_get_oid() failed for #%u: %r",
                  __FUNCTION__, i, rc);
            break;
        }
        rc = cfg_get_instance_fmt(&type, &old_value,
                                  "/agent:%s/env:%s",
                                  CFG_OID_GET_INST_NAME(oid, 1),
                                  CFG_OID_GET_INST_NAME(oid, 2));
        if (rc == 0)
        {
            if (strcmp(new_value, old_value) != 0)
            {
                rc = cfg_set_instance_fmt(type, new_value,
                                          "/agent:%s/env:%s",
                                          CFG_OID_GET_INST_NAME(oid, 1),
                                          CFG_OID_GET_INST_NAME(oid, 2));
            }
        }
        else if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
        {
            rc = cfg_add_instance_fmt(NULL, type, new_value,
                                      "/agent:%s/env:%s",
                                      CFG_OID_GET_INST_NAME(oid, 1),
                                      CFG_OID_GET_INST_NAME(oid, 2));
        }
        if (rc != 0)
        {
            ERROR("%s(): Failed on #%u: %r", __FUNCTION__, i, rc);
            break;
        }
        free(new_value); new_value = NULL;
        cfg_free_oid(oid); oid = NULL;
        free(old_value); old_value = NULL;
    }

    free(new_value);
    cfg_free_oid(oid);
    free(old_value);
    free(handles);

    return rc;
}
