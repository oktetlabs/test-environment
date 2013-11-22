/** @file
 * @brief Unix Test Agent
 *
 * PPPoE client support
 *
 * Copyright (C) 2004-2013 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
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
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 * 
 * $Id: conf_eth.c 83223 2013-07-05 15:51:56Z andrey $
 */

#define TE_LGR_USER     "PPPoE Client"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "te_errno.h"
#include "te_defs.h"
#include "te_ethernet.h"
#include "rcf_pch.h"
#include "logger_api.h"
#include "unix_internal.h"
#include "te_shell_cmd.h"
#include "conf_daemons.h"

#define PPPD_EXEC "/usr/sbin/pppd"
#define PPPOE_CLIENT_EXEC "/usr/sbin/pppoe"

struct pppoe_client;
struct pppoe_if_group;

/** PPPoE client structure */
typedef struct pppoe_client {
    /** Pointer to the next PPoE client */
    struct pppoe_client   *next;
    /** Pointer to group structure */
    struct pppoe_if_group *grp;
    /** PPPoE client name */
    char                  *name;
    /** Source MAC address to use in PPPoE session */
    uint8_t                mac[ETHER_ADDR_LEN];
    /** Whether PPPoE client is active or not */
    te_bool                active;
    /** PID of running PPPoE client */
    pid_t                  pid;
} pppoe_client;

/** The list of interfaces with links to PPPoE clients */
typedef struct pppoe_if_group {
    /** Pointer to the next interface group */
    struct pppoe_if_group *next;
    /** Interface name for this group of PPPoE clients */
    char                  *if_name;
    /** The list of PPPoE clients */
    struct pppoe_client   *clients;
} pppoe_if_group;

/** Head of the interface list */
static pppoe_if_group *if_group = NULL;

extern int link_addr_a2n(uint8_t *lladdr, int len, const char *str);

static te_errno pppoe_mac_addr_set(unsigned int gid, const char *oid,
                                   const char *value,
                                   const char *if_name,
                                   const char *pppoe_name);
static te_errno pppoe_mac_addr_get(unsigned int gid, const char *oid,
                                   char *value,
                                   const char *if_name,
                                   const char *pppoe_name);
static te_errno pppoe_list(unsigned int gid, const char *oid,
                           char **list, const char *if_name);
static te_errno pppoe_add(unsigned int gid, const char *oid,
                          const char *value,
                          const char *if_name,
                          const char *pppoe_name);
static te_errno pppoe_del(unsigned int gid, const char *oid,
                          const char *if_name,
                          const char *pppoe_name);
static te_errno pppoe_set(unsigned int gid, const char *oid,
                          const char *value,
                          const char *if_name,
                          const char *pppoe_name);
static te_errno pppoe_get(unsigned int gid, const char *oid,
                          char *value,
                          const char *if_name,
                          const char *pppoe_name);

static te_errno pppoe_stop(pppoe_client *clnt);

RCF_PCH_CFG_NODE_RW(node_pppoe_mac_addr, "mac_addr",
                    NULL, NULL /*&node_pppoe_ac_name*/,
                    &pppoe_mac_addr_get,
                    &pppoe_mac_addr_set);

static rcf_pch_cfg_object node_pppoe_client =
    { "pppoe", 0, &node_pppoe_mac_addr, NULL,
      (rcf_ch_cfg_get)&pppoe_get,
      (rcf_ch_cfg_set)&pppoe_set,
      (rcf_ch_cfg_add)&pppoe_add,
      (rcf_ch_cfg_del)&pppoe_del,
      (rcf_ch_cfg_list)&pppoe_list, NULL, NULL };

/**
 * Create PPPoE client over the particular interface
 * with the particular name.
 *
 * @param if_name  interface name over which to run PPPoE client
 * @param name     the name of PPPoE client instance
 *
 * @return Status of the operation
 */
static te_errno
pppoe_client_create(const char *if_name, const char *name,
                    pppoe_client **clnt_out)
{
    pppoe_if_group **p_grp = &if_group;
    pppoe_client   **p_clnt;

    while (*p_grp != NULL)
    {
        if (strcmp((*p_grp)->if_name, if_name) == 0)
            break;

        p_grp = &(*p_grp)->next;
    }

    if (*p_grp == NULL)
    {
        *p_grp = malloc(sizeof(**p_grp));
        if (*p_grp == NULL)
            return TE_ENOMEM;

        (*p_grp)->next = NULL;
        (*p_grp)->if_name = strdup(if_name);
        (*p_grp)->clients = NULL;
    }

    p_clnt = &(*p_grp)->clients;
    while (*p_clnt != NULL)
    {
        if (strcmp((*p_clnt)->name, name) == 0)
            return TE_EEXIST;

        p_clnt = &(*p_clnt)->next;
    }

    *p_clnt = malloc(sizeof(**p_clnt));
    if (*p_clnt == NULL)
        return TE_ENOMEM;

    (*p_clnt)->next = NULL;
    (*p_clnt)->grp = (*p_grp);
    (*p_clnt)->name = strdup(name);
    memset(&(*p_clnt)->mac, 0, sizeof((*p_clnt)->mac));
    (*p_clnt)->active = FALSE;
    (*p_clnt)->pid = (pid_t)(-1);

    *clnt_out = *p_clnt;

    return 0;
}

/**
 * Find PPPoE client structure based on interface name and client name.
 *
 * @param if_name   interface name
 * @param name      PPPoE client instance name
 * @param clnt_out  found PPPoE client structure (OUT)
 *
 * @return Status of the operation
 */
static te_errno
pppoe_client_find(const char *if_name, const char *name,
                  pppoe_client **clnt_out)
{
    pppoe_if_group *grp = if_group;
    pppoe_client   *clnt;

    while (grp != NULL)
    {
        if (strcmp(grp->if_name, if_name) == 0)
            break;
        grp = grp->next;
    }
    if (grp == NULL)
        return TE_ENOENT;
    
    clnt = grp->clients;
    while (clnt != NULL)
    {
        if (strcmp(clnt->name, name) == 0)
        {
            *clnt_out = clnt;
            return 0;
        }
        clnt = clnt->next;
    }
    return TE_ENOENT;
}

/**
 * Get instance list for object "/agent/interface/pppoe".
 *
 * @param gid      request's group identifier (unused)
 * @param oid      full object instance identifier (unused)
 * @param list     location for the list pointer
 * @param if_name  interface name
 *
 * @return Status code
 */
static te_errno
pppoe_list(unsigned int gid, const char *oid, char **list, const char *if_name)
{
    pppoe_if_group *grp = if_group;
    char           *buf = NULL;
    size_t          buf_len = 0;
    size_t          name_len;
    pppoe_client   *clnt;

    UNUSED(oid);
    UNUSED(gid);


    while (grp != NULL)
    {
        if (strcmp(grp->if_name, if_name) == 0)
            break;
        grp = grp->next;
    }

    if (grp == NULL || grp->clients == NULL)
    {
        *list = NULL;
        return 0;
    }

    clnt = grp->clients;
    while (clnt != NULL)
    {
        name_len = strlen(clnt->name);
        buf = realloc(buf, buf_len + name_len + 2);
        if (buf == NULL)
            return TE_ENOMEM;

        snprintf(buf + buf_len, name_len + 2, "%s ", clnt->name);
        buf_len += (name_len + 1);

        clnt = clnt->next;
    }

    *list = buf;
    return 0;
}

static te_errno
pppoe_add(unsigned int gid, const char *oid, const char *value,
          const char *if_name, const char *pppoe_name)
{
    pppoe_client *clnt;
    te_errno      rc;

    UNUSED(oid);
    UNUSED(gid);

    if (strcmp(value, "0") != 0)
    {
        ERROR("PPPoE client start-up at creation is not supported!");
        return TE_EINVAL;
    }

    if ((rc = pppoe_client_create(if_name, pppoe_name, &clnt)) != 0)
        return rc;

    return 0;
}

static te_errno
pppoe_del(unsigned int gid, const char *oid,
          const char *if_name, const char *pppoe_name)
{
    pppoe_if_group **p_grp = &if_group;
    pppoe_client   **p_clnt;

    UNUSED(oid);
    UNUSED(gid);

    while (*p_grp != NULL)
    {
        if (strcmp((*p_grp)->if_name, if_name) == 0)
            break;

        p_grp = &(*p_grp)->next;
    }

    if (*p_grp == NULL)
        return TE_ENOENT;

    p_clnt = &(*p_grp)->clients;
    while (*p_clnt != NULL)
    {
        if (strcmp((*p_clnt)->name, pppoe_name) == 0)
        {
            pppoe_if_group *grp = *p_grp;
            pppoe_client   *clnt = *p_clnt;

            if (clnt->active)
            {
                pppoe_stop(clnt);
            }
            *p_clnt = clnt->next;
            free(clnt);

            if (grp->clients == NULL)
            {
                *p_grp = grp->next;
                free(grp);
            }
            return 0;
        }
        p_clnt = &(*p_clnt)->next;
    }

    return TE_ENOENT;
}

static te_errno
pppoe_stop(pppoe_client *clnt)
{
    if (!clnt->active)
        return 0;

    if (ta_kill_death(clnt->pid) != 0)
        ERROR("PPPoE client terminated abnormally");
    clnt->pid = (pid_t)(-1);

    clnt->active = FALSE;
    return 0;
}

static te_errno
pppoe_start(pppoe_client *clnt)
{
    uint8_t  zero_mac[ETHER_ADDR_LEN] = {};
    char     src_mac_buf[32];
    char    *src_mac_str;
    char     cmd[128];

    if (memcmp(clnt->mac, zero_mac, sizeof(clnt->mac)) != 0)
    {
        snprintf(src_mac_buf, sizeof(src_mac_buf),
                 " -H %02X:%02X:%02X:%02X:%02X:%02X",
                 clnt->mac[0], clnt->mac[1], clnt->mac[2],
                 clnt->mac[3], clnt->mac[4], clnt->mac[5]);
        src_mac_str = src_mac_buf;
    }
    else
    {
        /*
         * With default MAC we should add '-U' option that
         * adds Host-Uniq tag in discovery packets in order to
         * be able to run multiple pppoe daemons.
         */
        src_mac_str = " -U";
    }

    snprintf(cmd, sizeof(cmd), "%s pty '%s -I %s%s' noauth",
             PPPD_EXEC, PPPOE_CLIENT_EXEC,
             clnt->grp->if_name, src_mac_str);

    clnt->pid = te_shell_cmd(cmd, -1, NULL, NULL, NULL);

    if (clnt->pid <= 0)
        return TE_EFAULT;

    clnt->active = TRUE;
    return 0;
}

static te_errno
pppoe_set(unsigned int gid, const char *oid, const char *value,
          const char *if_name, const char *pppoe_name)
{
    pppoe_client *clnt;
    te_bool       set_active;
    te_errno      rc;

    UNUSED(oid);
    UNUSED(gid);

    if ((rc = pppoe_client_find(if_name, pppoe_name, &clnt)) != 0)
        return rc;

    if (strcmp(value, "1") == 0)
        set_active = TRUE;
    else if (strcmp(value, "0") == 0)
        set_active = FALSE;
    else
        return TE_EINVAL;

    if (set_active == clnt->active)
        return 0;

    if (set_active)
    {
        if ((rc = pppoe_start(clnt)) != 0)
            return rc;
    }
    else
    {
        pppoe_stop(clnt);
    }

    return 0;
}

static te_errno
pppoe_get(unsigned int gid, const char *oid, char *value,
          const char *if_name, const char *pppoe_name)
{
    pppoe_client *clnt;
    te_errno      rc;

    UNUSED(oid);
    UNUSED(gid);

    if ((rc = pppoe_client_find(if_name, pppoe_name, &clnt)) != 0)
        return rc;

    sprintf(value, "%s", (clnt->active) ? "1" : "0");
    return 0;
}

static te_errno
pppoe_mac_addr_set(unsigned int gid, const char *oid, const char *value,
                   const char *if_name, const char *pppoe_name)
{
    uint8_t       mac[ETHER_ADDR_LEN];
    pppoe_client *clnt;
    te_errno      rc;
    int i;

    UNUSED(oid);
    UNUSED(gid);

    if ((rc = pppoe_client_find(if_name, pppoe_name, &clnt)) != 0)
        return rc;

    memcpy(mac, clnt->mac, sizeof(mac));
    if (link_addr_a2n(clnt->mac, sizeof(clnt->mac), value) == -1)
    {
        ERROR("%s: Link layer address conversation issue", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (clnt->active)
    {
        pppoe_stop(clnt);
        if ((rc = pppoe_start(clnt)) != 0)
        {
            ERROR("Failed to start PPPoE client after MAC address change!");
            memcpy(clnt->mac, mac, sizeof(clnt->mac));
            return rc;
        }
    }

    return 0;
}

static te_errno
pppoe_mac_addr_get(unsigned int gid, const char *oid, char *value,
                   const char *if_name, const char *pppoe_name)
{
    pppoe_client *clnt;
    te_errno      rc;

    UNUSED(oid);
    UNUSED(gid);

    if ((rc = pppoe_client_find(if_name, pppoe_name, &clnt)) != 0)
        return rc;

    snprintf(value, RCF_MAX_VAL, "%02x:%02x:%02x:%02x:%02x:%02x",
             clnt->mac[0], clnt->mac[1], clnt->mac[2],
             clnt->mac[3], clnt->mac[4], clnt->mac[5]);

    return 0;
}

/**
 * Initialize PPPoE client configuration nodes
 */
te_errno
pppoe_client_add(void)
{
    const char *pppd_paths[] = { PPPD_EXEC };
    const char *pppoe_paths[] = { PPPOE_CLIENT_EXEC };
    te_errno    rc = 0;

    INFO("%s()", __FUNCTION__);

    /* Find PPPoE client executables */
    rc = find_file(1, pppd_paths, TRUE);
    if (rc < 0)
    {
        ERROR("Failed to find PPPD executable necessary for PPPoE client"
              " - PPPoE client will not be available");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }
    rc = find_file(1, pppoe_paths, TRUE);
    if (rc < 0)
    {
        ERROR("Failed to find PPPOE executable"
              " - PPPoE client will not be available");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    return rcf_pch_add_node("/agent/interface", &node_pppoe_client);
}

