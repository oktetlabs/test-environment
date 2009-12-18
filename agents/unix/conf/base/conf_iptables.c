/** @file
 * @brief Unix Test Agent
 *
 * Unix TA iptables support
 *
 * Copyright (C) 2004-2009 Test Environment authors (see file AUTHORS
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

#define IPTABLES_CMD_BUF_SIZE    1024

/*
 * Methods
 */

static te_errno iptables_table_list(unsigned int, const char *, char **,
                                    const char *);

static te_errno iptables_chain_get(unsigned int, const char *, char *,
                                   const char *, const char *, const char *,
                                   const char *);

static te_errno iptables_chain_set(unsigned int, const char *, const char *,
                                   const char *, const char *, const char *,
                                   const char *);

static te_errno iptables_chain_add(unsigned int, const char *, const char *,
                                   const char *, const char *, const char *,
                                   const char *);

static te_errno iptables_chain_del(unsigned int, const char *, const char *,
                                   const char *, const char *,
                                   const char *);

static te_errno iptables_chain_list(unsigned int, const char *, char **,
                                    const char *, const char *,
                                    const char *);

static te_errno iptables_rules_get(unsigned int, const char *, char *,
                                   const char *, const char *, const char *,
                                   const char *);

static te_errno iptables_rules_set(unsigned int, const char *, const char *,
                                   const char *, const char *, const char *,
                                   const char *);

static te_errno iptables_cmd_get(unsigned int, const char *, char *,
                                 const char *, const char *, const char *,
                                 const char *);

static te_errno iptables_cmd_set(unsigned int, const char *, const char *,
                                 const char *, const char *, const char *,
                                 const char *);



/*
 * Nodes
 */

RCF_PCH_CFG_NODE_RW(node_iptables_rules, "rules",
                    NULL, NULL,
                    iptables_rules_get,
                    iptables_rules_set);

RCF_PCH_CFG_NODE_RW(node_iptables_cmd, "cmd",
                    NULL, &node_iptables_rules,
                    iptables_cmd_get,
                    iptables_cmd_set);

static rcf_pch_cfg_object node_iptables_chain = 
{ "chain", 0, &node_iptables_cmd, NULL,
      (rcf_ch_cfg_get)iptables_chain_get,
      (rcf_ch_cfg_set)iptables_chain_set,
      (rcf_ch_cfg_add)iptables_chain_add,
      (rcf_ch_cfg_del)iptables_chain_del,
      (rcf_ch_cfg_list)iptables_chain_list,
      NULL, NULL};

RCF_PCH_CFG_NODE_COLLECTION(node_iptables_table, "table",
                            &node_iptables_chain, NULL,
                            NULL, NULL,
                            iptables_table_list, NULL);

RCF_PCH_CFG_NODE_NA(node_iptables, "iptables",
                    &node_iptables_table, NULL);


/**
 * Obtain list of built-in iptables tables.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param list          location for the list pointer
 * @param ifname        interface name (unused)
 *
 * @return              Status code
 */
static te_errno
iptables_table_list(unsigned int  gid, const char *oid, char **list,
                    const char *ifname)
{
    static char *table_list = "filter mangle nat raw";

    INFO("%s started", __FUNCTION__);

    *list = strdup(table_list);
    if (*list == NULL)
    {
        ERROR("out of memory");
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(ifname);

    return 0;
}

/**
 * Check if per-interface chain is output
 *
 * @param ifname        interface name to check chain linked to
 * @param table         table name to check chains in
 * @param chain         chain name to check status of
 *
 * @return              Check status (boolean value)
 */
static te_bool
iptables_is_chain_output(const char *chain)
{
    return ((strcmp(chain, "POSTROUTING") == 0) ||
            (strcmp(chain, "OUTPUT")      == 0) ||
            (strcmp(chain, "FORWARD_OUTPUT") == 0));
}

/**
 * Check if per-interface chain jumping rule is installed.
 *
 * @param ifname        interface name to check chain linked to
 * @param table         table name to check chains in
 * @param chain         chain name to check status of
 *
 * @return              Check status (boolean value)
 */
static te_bool
iptables_perif_chain_is_enabled(const char *ifname, const char *table,
                                const char *chain)
{
    FILE    *fp;
    int      out_fd;
    char     buf[IPTABLES_CMD_BUF_SIZE];
    te_bool  enabled = FALSE;
    pid_t    pid;
    int      status;

    INFO("%s started, ifname=%s, table=%s", __FUNCTION__, ifname, table);

    snprintf(buf, IPTABLES_CMD_BUF_SIZE,
             "iptables -t %s -S %s | grep '^-A %s -%c %s -j %s_%s'",
             table, chain, chain,
             iptables_is_chain_output(chain) ? 'o' : 'i',
             ifname, chain, ifname);
    if ((pid = te_shell_cmd(buf, -1, NULL, &out_fd, NULL)) < 0)
    {
        ERROR("failed to execute command line while getting: %s: "
              "rc=%r (%s)", buf, pid, strerror(errno));
        return FALSE;
    }

    if ((fp = fdopen(out_fd, "r")) == NULL)
    {
        ERROR("failed to get shell command execution result");
        goto cleanup;
    }

    enabled = (fgets(buf, sizeof(buf), fp) != NULL);

cleanup:
    fclose(fp);
    close(out_fd);
    ta_waitpid(pid, &status, 0);

    return enabled;
}

/**
 * Change status of (install/remove) per-interface chain jumping rule.
 *
 * @param ifname        interface name to operate the chain linked to
 * @param table         table name to operate chains in
 * @param chain         chain name to work with
 * @param enable        state of chain jumping rule (if TRUE, jumping rule
 *                      should be installed)
 *
 * @return              Status code
 */
static te_errno
iptables_perif_chain_set(const char *ifname,
                         const char *table,
                         const char *chain,
                         te_bool enable)
{
    int  rc;
    char cmd_buf[IPTABLES_CMD_BUF_SIZE];

    INFO("%s(%s, %s, %s, %s) started", __FUNCTION__,
         ifname, table, chain, enable ? "ON" : "OFF");

    if (enable == iptables_perif_chain_is_enabled(ifname, table, chain))
        return 0;

    /* Add rule to jump to new chain */
    snprintf(cmd_buf, IPTABLES_CMD_BUF_SIZE,
             "iptables -t %s -%c %s -%c %s -j %s_%s",
             table, (enable) ? 'I' : 'D', chain,
             iptables_is_chain_output(chain) ? 'o' : 'i',
             ifname, chain, ifname);

    if ((rc = ta_system(cmd_buf)) != 0)
    {
        ERROR("Command '%s' returned %r", cmd_buf, rc);
    }

    return rc;
}

/**
 * Add per-interface chain and install jumping rule if required 
 *
 * @param gid           group identifier (unused)
 * @param oid           full identifier of the father instance
 * @param value         boolean value, if we should install jumping rule
 * @param ifname        interface name to operate the chain linked to
 * @param dummy         unused value, corresponding to .../iptables: node
 * @param table         table name to operate chains in
 * @param chain         chain name to add (without ifname prefix)
 *
 * @return              Status code
 */
static te_errno
iptables_chain_add(unsigned int  gid, const char *oid,
                   const char   *value, const char *ifname,
                   const char   *dummy, const char   *table,
                   const char   *chain)
{
    char        cmd_buf[IPTABLES_CMD_BUF_SIZE];
    int         enable = atoi(value);
    te_errno    rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(dummy);

    INFO("%s(%s, %s, %s) started", __FUNCTION__, ifname, table, chain);

    /* Create new chain first */
    snprintf(cmd_buf, IPTABLES_CMD_BUF_SIZE, "iptables -t %s -N %s_%s",
             table, chain, ifname);
    if ((rc = ta_system(cmd_buf)) != 0)
    {
        ERROR("Failed to add the chain %s_%s, rc=%r", chain, ifname, rc);
        return rc;
    }

    if (enable)
    {
        if ((rc = iptables_perif_chain_set(ifname, table,
                                           chain, TRUE)) != 0)
        {
            ERROR("Failed to add jumping rule for chain %s_%s",
                  chain, ifname);
            return rc;
        }
    }

    return 0;
}

/**
 * Delete per-interface chain and remove jumping rule
 *
 * @param gid           group identifier (unused)
 * @param oid           full identifier of the father instance
 * @param value         boolean value, if we should install jumping rule
 * @param ifname        interface name to operate the chain linked to
 * @param dummy         unused value, corresponding to .../iptables: node
 * @param table         table name to operate chains in
 * @param chain         chain name to delete (without ifname prefix)
 *
 * @return              Status code
 */
static te_errno
iptables_chain_del(unsigned int  gid, const char *oid,
                   const char   *ifname, const char *dummy,
                   const char   *table, const char *chain)
{
    char        cmd_buf[IPTABLES_CMD_BUF_SIZE];
    te_errno    rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(dummy);

    INFO("%s(%s, %s, %s) started", __FUNCTION__, ifname, table, chain);

    /* Delete jump rule */
    if (iptables_perif_chain_is_enabled(ifname, table, chain))
    {
        if ((rc = iptables_perif_chain_set(ifname, table,
                                           chain, FALSE)) != 0)
        {
            ERROR("Failed to remove jumping rule for chain %s_%s, rc=%r",
                  chain, ifname, rc);
            return rc;
        }
    }

    /* Flush chain */
    snprintf(cmd_buf, IPTABLES_CMD_BUF_SIZE, "iptables -t %s -F %s_%s",
             table, chain, ifname);
    if ((rc = ta_system(cmd_buf)) != 0)
    {
        ERROR("Failed to flush the chain %s_%s, rc=%r", chain, ifname, rc);
        return rc;
    }

    /* Delete chain */
    snprintf(cmd_buf, IPTABLES_CMD_BUF_SIZE, "iptables -t %s -X %s_%s",
             table, chain, ifname);
    if ((rc = ta_system(cmd_buf)) != 0)
    {
        ERROR("Failed to delete the chain %s_%s, rc=%r", chain, ifname, rc);
        return rc;
    }

    return 0;
}


/**
 * Install/remove per-interface chain jumping rule
 *
 * @param gid           group identifier (unused)
 * @param oid           full identifier of the father instance
 * @param value         boolean value, if the jumping rule should be
 *                      installed (1) or removed (0)
 * @param ifname        interface name to operate the chain linked to
 * @param dummy         unused value, corresponding to .../iptables: node
 * @param table         table name to operate chains in
 * @param chain         chain name (without ifname prefix)
 *
 * @return              Status code
 */
static te_errno
iptables_chain_set(unsigned int  gid, const char *oid,
                   const char   *value, const char *ifname,
                   const char   *dummy, const char   *table,
                   const char   *chain)
{
    int enable = atoi(value);

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(dummy);

    return iptables_perif_chain_set(ifname, table, chain, enable);
}

/**
 * Get the status of per-interface chain jumping rule (installed or not)
 *
 * @param gid           group identifier (unused)
 * @param oid           full identifier of the father instance
 * @param value         location to returned status of jumping rule
 * @param ifname        interface name to operate the chain linked to
 * @param dummy         unused value, corresponding to .../iptables: node
 * @param table         table name to operate chains in
 * @param chain         chain name to check (without ifname prefix)
 *
 * @return              Status code
 */
static te_errno
iptables_chain_get(unsigned int  gid, const char *oid,
                   char   *value, const char *ifname,
                   const char *dummy, const char   *table,
                   const char *chain)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(dummy);

    sprintf(value, iptables_perif_chain_is_enabled(ifname, table,
                                                   chain) ? "1" : "0");

    return 0;
}

/**
 * Remove trailing newline and spaces at the end of string
 *
 * @param buf   string location
 *
 * @return      N/A
 */
static void
chomp(char *buf)
{
    char *p;

    if (buf == NULL)
        return;

    for (p = buf + strlen(buf) - 1;
         (p > buf) && isspace(*p) ; *p-- = '\0');
}

/**
 * Get the list of per-interface chains.
 *
 * @param gid           group identifier (unused)
 * @param oid           full identifier of the father instance
 * @param list          location of the chains list
 * @param ifname        interface name to operate the chain linked to
 * @param dummy         unused value, corresponding to .../iptables: node
 * @param table         table name to look for chains in
 *
 * @return              Status code
 */
static te_errno
iptables_chain_list(unsigned int  gid, const char *oid, char **list,
                    const char   *ifname, const char *dummy,
                    const char   *table)
{
    int       rc        = 0;
    FILE     *fp;
    int       out_fd;
    char      buf[IPTABLES_CMD_BUF_SIZE];
    uint32_t  list_size = 0;
    uint32_t  list_len  = 0;
    pid_t     pid;
    int       status;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(dummy);

    INFO("%s started, ifname=%s, table=%s", __FUNCTION__, ifname, table);

    *list = NULL;

    snprintf(buf, IPTABLES_CMD_BUF_SIZE,
             "iptables -t %s -S | grep '^-N .*_%s' | "
             "sed -e 's/^-N //g' | sed -e 's/_%s$//g'",
             table, ifname, ifname);
    if ((pid = te_shell_cmd(buf, -1, NULL, &out_fd, NULL)) < 0)
    {
        ERROR("failed to execute command line while getting: %s: "
              "rc=%r (%s)", buf, rc, strerror(errno));
        return pid;
    }

    if ((fp = fdopen(out_fd, "r")) == NULL)
    {
        ERROR("Failed to get shell command execution result");
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
        /* Remove trailing newline */
        chomp(buf);

        if (list_len + strlen(buf) + 1 >= list_size)
        {
            list_size *= 2;
            if ((*list = realloc(*list, list_size)) == NULL)
            {
                rc = TE_RC(TE_TA_UNIX, TE_ENOMEM);
                goto cleanup;
            }
        }

        list_len += sprintf(*list + list_len, "%s ", buf);

        INFO("Found chain %s", buf);
    }

    if (strlen(*list) > 0)
        INFO("Chains list for %s table on %s: %s", table, ifname, *list);

cleanup:
    if (fp != NULL)
        fclose(fp);
    close(out_fd);
    ta_waitpid(pid, &status, 0);

    return rc;
}


/**
 * Get the list of rules in the per-interface chain as a single value.
 *
 * @param gid           group identifier (unused)
 * @param oid           full identifier of the father instance
 * @param value         location for the rules list
 * @param ifname        interface name to operate the chain linked to
 * @param dummy         unused value, corresponding to .../iptables: node
 * @param table         table name to operate with chains in
 * @param chain         chain name to get the list of rules in
 *
 * @return              Status code
 */
static te_errno
iptables_rules_get(unsigned int  gid, const char *oid,
                   char         *value, const char *ifname,
                   const char   *dummy, const char *table,
                   const char   *chain)
{
    int   rc = 0;
    FILE *fp = NULL;
    int   out_fd;
    char  buf[IPTABLES_CMD_BUF_SIZE] = {0, };
    pid_t pid;
    int   status;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(dummy);

    INFO("%s(ifname=%s, table=%s, chain=%s) started",
         __FUNCTION__, ifname, table, chain);

    *value = '\0';

    snprintf(buf, IPTABLES_CMD_BUF_SIZE,
             "iptables -t %s -S %s_%s | "
             "grep '^-A %s_%s ' | "
             "sed -e 's/^-A %s_%s //g'",
             table, chain, ifname, chain, ifname, chain, ifname);
    if ((pid = te_shell_cmd(buf, -1, NULL, &out_fd, NULL)) < 0)
    {
        ERROR("failed to execute command line while getting: %s: "
              "rc=%r (%s)", buf, pid, strerror(errno));
        return pid;
    }

    if ((fp = fdopen(out_fd, "r")) == NULL)
    {
        ERROR("failed to get shell command execution result");
        rc = TE_RC(TE_TA_UNIX, TE_EFAULT);
        goto cleanup;
    }

    while (fgets(buf, sizeof(buf), fp) != NULL)
    {
        /* Remove trailing newline */
        chomp(buf);

        INFO("Rule(ifname:%s, table:%s, chain:%s): %s",
             ifname, table, chain, buf);

        value += sprintf(value, "%s\n", buf);
    }

cleanup:
    if (fp != NULL)
        fclose(fp);
    close(out_fd);
    ta_waitpid(pid, &status, 0);

    return rc;
}


/**
 * Flush and setup the list of rules for the per-interface chain.
 *
 * @param gid           group identifier (unused)
 * @param oid           full identifier of the father instance
 * @param value         rules list without chain name and delimited by '|'
 * @param ifname        interface name to operate the chain linked to
 * @param dummy         unused value, corresponding to .../iptables: node
 * @param table         table name to operate with chain in
 * @param chain         chain name to update the list of rules in
 *
 * @return              Status code
 */
static te_errno
iptables_rules_set(unsigned int  gid, const char *oid,
                   const char   *value, const char *ifname,
                   const char   *dummy, const char *table,
                   const char   *chain)
{
    int   rc = 0;
    FILE *fp = NULL;
    int   in_fd;
    char  buf[IPTABLES_CMD_BUF_SIZE];
    char *p = NULL;
    pid_t pid;
    int   status;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(dummy);

    INFO("%s started, ifname=%s, table=%s", __FUNCTION__, ifname, table);

    /* Flush the chain */
    snprintf(buf, IPTABLES_CMD_BUF_SIZE,
             "iptables -t %s -F %s_%s", table, chain, ifname);
    ta_system(buf);

    /* Open iptables-restore session, do not flush all chains */
    snprintf(buf, IPTABLES_CMD_BUF_SIZE,
             "iptables-restore -n");
    if ((pid = te_shell_cmd(buf, -1, &in_fd, NULL, NULL)) < 0)
    {
        ERROR("failed to execute command line while getting: %s: "
              "rc=%r (%s)", buf, pid, strerror(errno));
        return pid;
    }

    if ((fp = fdopen(in_fd, "r")) == NULL)
    {
        ERROR("failed to get shell command execution result");
        rc = TE_RC(TE_TA_UNIX, TE_EFAULT);
        goto cleanup;
    }

    /* Fill the table */
    fprintf(fp, "*%s\n", table);
    do
    {
        int len;

        p = strchr(value, '\n');

        len = (p != NULL) ? (p - value) : (int)strlen(value);
        /* prepare format string */
        sprintf(buf, "-A %%s_%%s %%.%ds\n", len);
        fprintf(fp, buf, ifname, chain, value);

        if (p != NULL)
            value = p + 1;
    } while (p != NULL);

    /* Commit changes */
    fprintf(fp, "COMMIT\n\n");

cleanup:
    if (fp != NULL)
        fclose(fp);
    close(in_fd);
    ta_waitpid(pid, &status, 0);

    return rc;
}



/**
 * Add/Delete/Insert iptables rule into the specific per-interface chain.
 *
 * @param gid           group identifier (unused)
 * @param oid           full identifier of the father instance
 * @param value         iptables command to execute, chain name in the
 *                      command should be omitted to avoid ambiguity
 * @param ifname        interface name to operate the chain linked to
 * @param dummy         unused value, corresponding to .../iptables: node
 * @param table         table name to operate with chain in
 * @param chain         chain name to operate with
 *
 * @return              Status code
 */
static te_errno
iptables_cmd_set(unsigned int  gid, const char *oid,
                 const char   *value, const char *ifname,
                 const char   *dummy, const char *table,
                 const char   *chain)
{
    int rc = 0;
    char buf[IPTABLES_CMD_BUF_SIZE] = {0, };
    char *val_p = (char *)value;
    char *cmd_p = (char *)buf;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(dummy);

    INFO("%s(ifname=%s, table=%s, chain=%s): %s", __FUNCTION__,
         ifname, table, chain, value);

    cmd_p += sprintf(cmd_p, "iptables -t %s ", table);

#define SKIP_SPACES(_p)                                 \
    while (isspace(*(_p))) (_p)++;

    SKIP_SPACES(val_p);
    if (*val_p != '-')
    {
        ERROR("Invalid rule format");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    val_p++;
    if ((*val_p == 'A') || (*val_p == 'D') || (*val_p == 'I'))
    {
        cmd_p += sprintf(cmd_p, "-%c ", *val_p++);
    }
    else
    {
        ERROR("Unknown iptables rule action");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    SKIP_SPACES(val_p);
    cmd_p += sprintf(cmd_p, " %s_%s %s", chain, ifname, val_p);
#undef SKIP_SPACES

    INFO("Execute cmd: '%s'", buf);
    if ((rc = ta_system(buf)) != 0)
    {
        ERROR("Command '%s' returned %r", buf, rc);
    }

    return rc;
}

/**
 * Dummy get method for volatile write-only object.
 *
 * @param gid           group identifier (unused)
 * @param oid           full identifier of the father instance
 * @param value         location to the returned empty value
 * @param ifname        interface name to operate the chain linked to
 * @param dummy         unused value, corresponding to .../iptables: node
 * @param table         table name to operate with chain in
 * @param chain         chain name to operate with
 *
 * @return              Status code
 */
static te_errno
iptables_cmd_get(unsigned int  gid, const char *oid,
                 char   *value, const char *ifname,
                 const char   *dummy, const char *table,
                 const char   *chain)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(dummy);
    UNUSED(chain);

    INFO("%s started, ifname=%s, table=%s", __FUNCTION__, ifname, table);

    *value = '\0';

    return 0;
}

/**
 * Initialize iptables subtree
 *
 * @return              Status code
 */
extern te_errno
ta_unix_conf_iptables_init(void)
{
    return rcf_pch_add_node("/agent/interface", &node_iptables);
}

#else
/**
 * Dummy initialization of iptables subtree if not __linux__
 *
 * @return              Status code
 */
extern te_errno
ta_unix_conf_iptables_init(void)
{
    WARN("iptables functionality is not supported", __FUNCTION__);
    return 0;
}
#endif  /* __linux__ */


