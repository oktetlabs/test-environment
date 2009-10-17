/** @file
 * @brief Unix Test Agent
 *
 * Unix TA iptables support
 *
 * Copyright (C) 2004-2007 Test Environment authors (see file AUTHORS
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
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TA iptables"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if defined __linux__
#include <linux/version.h>
#endif

#include "te_errno.h"
#include "logger_api.h"
#include "te_defs.h"
#include "rcf_pch.h"
#include "unix_internal.h"
#include "te_shell_cmd.h"
#include "ta_common.h"

#if __linux__

#define IPTABLES_CHAIN_NAME_SIZE 128
#define IPTABLES_CMD_BUF_SIZE    1024

/*
 * Methods
 */

static te_errno iptables_table_list(unsigned int, const char *, char **,
                                    const char *);

static te_errno iptables_rule_add(unsigned int, const char *, const char *,
                                  const char *, const char *, const char *,
                                  const char *);

static te_errno iptables_rule_del(unsigned int, const char *, const char *,
                                  const char *, const char *, const char *);

static te_errno iptables_rule_list(unsigned int, const char *, char **,
                                   const char *, const char *, const char *);

/*
 * Nodes
 */

RCF_PCH_CFG_NODE_COLLECTION(node_iptables_rule, "rule",
                            NULL, NULL,
                            iptables_rule_add,
                            iptables_rule_del,
                            iptables_rule_list,
                            NULL);

RCF_PCH_CFG_NODE_COLLECTION(node_iptables_table, "table",
                            &node_iptables_rule, NULL,
                            NULL, NULL,
                            iptables_table_list, NULL);

RCF_PCH_CFG_NODE_NA(node_iptables, "iptables",
                    &node_iptables_table, NULL);


te_errno
iptables_table_list(unsigned int  gid, const char *oid, char **list,
                    const char *ifname)
{
    RING("%s started", __FUNCTION__);

    *list = (char *)calloc(4, strlen("filter") + 1);
    if (*list == NULL)
    {
        ERROR("out of memory");
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }
    sprintf(*list, "filter mangle nat raw");

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(ifname);

    return 0;
}

/* Create $ifname_$chain chain and jump there */
te_errno
iptables_perif_chain_flush(const char *ifname, const char *table,
                           const char *chain)
{
    te_errno rc;

    char chain_name[IPTABLES_CHAIN_NAME_SIZE];
    char cmd_buf[IPTABLES_CMD_BUF_SIZE];

    RING("%s(%s, %s, %s) started", __FUNCTION__, ifname, table, chain);

    snprintf(chain_name, IPTABLES_CHAIN_NAME_SIZE, "%s_%s",
             ifname, chain);

    /* Create new chain first */
    snprintf(cmd_buf, IPTABLES_CMD_BUF_SIZE, "iptables -t %s -F %s",
             table, chain_name);
    if ((rc = ta_system(cmd_buf)) != 0)
    {
        return rc;
    }

    return 0;
}


/* Create $ifname_$chain chain and jump there */
te_errno
iptables_perif_chain_create(const char *ifname, const char *table,
                            const char *chain)
{
    te_errno rc;

    char chain_name[IPTABLES_CHAIN_NAME_SIZE];
    char cmd_buf[IPTABLES_CMD_BUF_SIZE];

    RING("%s(%s, %s, %s) started", __FUNCTION__, ifname, table, chain);

    snprintf(chain_name, IPTABLES_CHAIN_NAME_SIZE, "%s_%s",
             ifname, chain);

    /* Create new chain first */
    snprintf(cmd_buf, IPTABLES_CMD_BUF_SIZE, "iptables -t %s -N %s",
             table, chain_name);
    if ((rc = ta_system(cmd_buf)) != 0)
    {
        return rc;
    }

    /* Add rule to jump to new chain */
    snprintf(cmd_buf, IPTABLES_CMD_BUF_SIZE,
             "iptables -t %s -I %s -%c %s -j %s",
             table, chain,
             ((strcmp(chain, "POSTROUTING") == 0) ||
              (strcmp(chain, "OUTPUT")      == 0)) ? 'o' : 'i',
             ifname, chain_name);
    if ((rc = ta_system(cmd_buf)) != 0)
    {
        ERROR("Failed to execute command: %s", cmd_buf);
        return rc;
    }

    return 0;
}


/* Create $ifname_$chain chain and jump there */
te_errno
iptables_perif_chain_destroy(const char *ifname, const char *table,
                             const char *chain)
{
    te_errno rc;

    char chain_name[IPTABLES_CHAIN_NAME_SIZE];
    char cmd_buf[IPTABLES_CMD_BUF_SIZE];

    RING("%s(%s, %s, %s) started", __FUNCTION__, ifname, table, chain);

    snprintf(chain_name, IPTABLES_CHAIN_NAME_SIZE, "%s_%s",
             ifname, chain);

    /* Delete jump rule */
    snprintf(cmd_buf, IPTABLES_CMD_BUF_SIZE,
             "iptables -t %s -D %s -%c %s -j %s",
             table, chain,
             ((strcmp(chain, "POSTROUTING") == 0) ||
              (strcmp(chain, "OUTPUT")      == 0)) ? 'o' : 'i',
             ifname, chain_name);
    if ((rc = ta_system(cmd_buf)) != 0)
    {
        ERROR("Failed to execute command: %s", cmd_buf);
        return rc;
    }

    /* Flush chain */
    snprintf(cmd_buf, IPTABLES_CMD_BUF_SIZE, "iptables -t %s -F %s",
             table, chain_name);
    if ((rc = ta_system(cmd_buf)) != 0)
    {
        ERROR("Failed to execute command: %s", cmd_buf);
        return rc;
    }

    /* Delete chain */
    snprintf(cmd_buf, IPTABLES_CMD_BUF_SIZE, "iptables -t %s -X %s",
             table, chain_name);
    if ((rc = ta_system(cmd_buf)) != 0)
    {
        ERROR("Failed to execute command: %s", cmd_buf);
        return rc;
    }

    return 0;
}


te_errno
iptables_interface_init(const char *ifname)
{
    int rc = 0;

    RING("%s(%s) started", __FUNCTION__, ifname);

    /* filter table chains: INPUT, FORWARD, OUTPUT */
    if ((rc = iptables_perif_chain_create(ifname, "filter", "INPUT")) != 0)
    {
        return rc;
    }

    if ((rc = iptables_perif_chain_create(ifname, "filter", "FORWARD")) != 0)
    {
        return rc;
    }

    if ((rc = iptables_perif_chain_create(ifname, "filter", "OUTPUT")) != 0)
    {
        return rc;
    }

    /* mangle table chains: PREROUTING, INPUT, FORWARD, OUTPUT, POSTROUTING */
    if ((rc = iptables_perif_chain_create(ifname, "mangle", "PREROUTING")) != 0)
    {
        return rc;
    }

    if ((rc = iptables_perif_chain_create(ifname, "mangle", "INPUT")) != 0)
    {
        return rc;
    }

    if ((rc = iptables_perif_chain_create(ifname, "mangle", "FORWARD")) != 0)
    {
        return rc;
    }

    if ((rc = iptables_perif_chain_create(ifname, "mangle", "OUTPUT")) != 0)
    {
        return rc;
    }

    if ((rc = iptables_perif_chain_create(ifname, "mangle", "POSTROUTING")) != 0)
    {
        return rc;
    }

    /* nat table chains: PREROUTING, POSTROUTING, OUTPUT */
    if ((rc = iptables_perif_chain_create(ifname, "nat", "PREROUTING")) != 0)
    {
        return rc;
    }

    if ((rc = iptables_perif_chain_create(ifname, "nat", "POSTROUTING")) != 0)
    {
        return rc;
    }

    if ((rc = iptables_perif_chain_create(ifname, "nat", "OUTPUT")) != 0)
    {
        return rc;
    }

    /* raw table chains: PREROUTING, OUTPUT */
    if ((rc = iptables_perif_chain_create(ifname, "raw", "PREROUTING")) != 0)
    {
        return rc;
    }

    if ((rc = iptables_perif_chain_create(ifname, "raw", "OUTPUT")) != 0)
    {
        return rc;
    }

    return 0;
}

void
iptables_interface_fini(const char *ifname)
{
    /* filter table chains: INPUT, FORWARD, OUTPUT */
    iptables_perif_chain_destroy(ifname, "filter", "INPUT");
    iptables_perif_chain_destroy(ifname, "filter", "FORWARD");
    iptables_perif_chain_destroy(ifname, "filter", "OUTPUT");

    /* mangle table chains: PREROUTING, INPUT, FORWARD, OUTPUT, POSTROUTING */
    iptables_perif_chain_destroy(ifname, "mangle", "PREROUTING");
    iptables_perif_chain_destroy(ifname, "mangle", "INPUT");
    iptables_perif_chain_destroy(ifname, "mangle", "FORWARD");
    iptables_perif_chain_destroy(ifname, "mangle", "OUTPUT");
    iptables_perif_chain_destroy(ifname, "mangle", "POSTROUTING");

    /* nat table chains: PREROUTING, POSTROUTING, OUTPUT */
    iptables_perif_chain_destroy(ifname, "nat", "PREROUTING");
    iptables_perif_chain_destroy(ifname, "nat", "POSTROUTING");
    iptables_perif_chain_destroy(ifname, "nat", "OUTPUT");

    /* raw table chains: PREROUTING, OUTPUT */
    iptables_perif_chain_destroy(ifname, "raw", "PREROUTING");
    iptables_perif_chain_destroy(ifname, "raw", "OUTPUT");
}


static char *
iptables_rule_to_id(const char *rule)
{
    char *id = strdup(rule);
    char *p  = id;

    for ( ; *p != '\0' ; p++)
    {
        if (*p == ' ')
        {
            *p = ';';
        }
    }

    return id;
}

static char *
iptables_id_to_rule(const char *id)
{
    char *rule = strdup(id);
    char *p  = rule;

    for ( ; *p != '\0' ; p++)
    {
        if (*p == ';')
        {
            *p = ' ';
        }
    }

    return rule;
}

static te_errno iptables_rule_list(unsigned int  gid, const char *oid, char **list,
                                   const char   *ifname, const char *dummy,
                                   const char   *table)
{
    int   rc        = 0;
    FILE *fp;
    int   out_fd;
    char  buf[IPTABLES_CMD_BUF_SIZE];
    int   list_size = 0;
    int   list_len  = 0;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(dummy);

    RING("%s started, ifname=%s, table=%s", __FUNCTION__, ifname, table);

    *list = NULL;

    if ((ifname == NULL) || !ta_interface_is_mine(ifname))
    {
        return TE_RC(TE_TA_UNIX, TE_ENODEV);
    }

    snprintf(buf, IPTABLES_CMD_BUF_SIZE,
             "iptables-save -t %s | grep '^-A %s_' | sed -e 's/^-A //g'",
             table, ifname);
    if ((rc = te_shell_cmd(buf, -1, NULL, &out_fd, NULL)) < 0)
    {
        ERROR("failed to execute command line while getting: %s: "
              "rc=%d (%s)", buf, rc, strerror(errno));
        return rc;
    }
    rc = 0;

    if ((fp = fdopen(out_fd, "r")) == NULL)
    {
        ERROR("failed to get shell command execution result");
        rc = TE_RC(TE_TA_UNIX, TE_EFAULT);
        goto cleanup;
    }

    list_size = IPTABLES_CMD_BUF_SIZE;
    *list     = (char *) calloc(1, list_size);
    if (*list == NULL)
    {
        rc = TE_RC(TE_TA_UNIX, TE_ENOMEM);
        goto cleanup;
    }
    list_len = 0;

    while (fgets(buf, sizeof(buf), fp) != NULL)
    {
        char *id = iptables_rule_to_id(buf);
        if (id == NULL)
        {
            rc = TE_RC(TE_TA_UNIX, TE_ENOMEM);
            goto cleanup;
        }
        if (list_len + strlen(id) + 1 >= list_size)
        {
            list_size                               *= 2;
            if ((*list = realloc(*list, list_size)) == NULL)
            {
                rc = TE_RC(TE_TA_UNIX, TE_ENOMEM);
                goto cleanup;
            }
        }
        list_len += sprintf(*list + list_len, " %s", id);
    }

cleanup:
    fclose(fp);
    close(out_fd);

    return rc;
}


/**
 * Add iptables rule.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier
 * @param value         value string
 * @param ifname        device  name, over it VLAN should be added
 * @param table         device  name, over it VLAN should be added
 * @param rule_id       iptables rule id
 *
 * @return              Status code
 */
static te_errno
iptables_rule_add(unsigned int  gid, const char *oid, const char *value,
                  const char *ifname, const char *dummy, const char *table,
                  const char *rule_id)
{
    char     *rule = NULL;
    char      cmd_buf[IPTABLES_CMD_BUF_SIZE];

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    UNUSED(dummy);

    RING("%s started", __FUNCTION__);

    if ((ifname == NULL) || !ta_interface_is_mine(ifname))
    {
        return TE_RC(TE_TA_UNIX, TE_ENODEV);
    }

    if (strncmp(rule_id, ifname, strlen(ifname)) != 0)
    {
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    rule = iptables_id_to_rule(rule_id);
    snprintf(cmd_buf, sizeof(cmd_buf), "iptables -t %s -A %s",
             table, rule);
    free(rule);

    if (ta_system(cmd_buf) != 0)
    {
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    return 0;
}

/**
 * Delete iptables rule.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier
 * @param value         value string
 * @param ifname        device  name, over it VLAN should be added
 * @param table         device  name, over it VLAN should be added
 * @param rule_id       iptables rule id
 *
 * @return              Status code
 */
static te_errno
iptables_rule_del(unsigned int  gid, const char *oid,
                  const char *ifname, const char *dummy,
                  const char *table, const char *rule_id)
{
    char     *rule = NULL;
    char      cmd_buf[IPTABLES_CMD_BUF_SIZE];

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(dummy);

    RING("%s started", __FUNCTION__);

    if ((ifname == NULL) || !ta_interface_is_mine(ifname))
    {
        return TE_RC(TE_TA_UNIX, TE_ENODEV);
    }

    if (strncmp(rule_id, ifname, strlen(ifname)) != 0)
    {
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    rule = iptables_id_to_rule(rule_id);
    snprintf(cmd_buf, sizeof(cmd_buf), "iptables -t %s -D %s",
             table, rule);
    free(rule);

    if (ta_system(cmd_buf) != 0)
    {
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    return 0;
}



/**
 * Initialize iptables subtree
 */
extern te_errno
ta_unix_conf_iptables_init(void)
{
    return rcf_pch_add_node("/agent/interface", &node_iptables);
}

#else
extern te_errno
ta_unix_conf_iptables_init(void)
{
    ERROR("Dummy %s started", __FUNCTION__);
    return 0;
}
#endif

