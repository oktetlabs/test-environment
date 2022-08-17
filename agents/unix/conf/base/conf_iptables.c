/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Unix Test Agent
 *
 * Unix TA iptables support
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "TA iptables"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#if defined __linux__
#include <linux/version.h>
#endif

#include "te_errno.h"
#include "logger_api.h"
#include "te_defs.h"
#include "te_str.h"
#include "rcf_pch.h"
#include "unix_internal.h"
#include "te_shell_cmd.h"
#include "ta_common.h"
#include "te_string.h"

#if __linux__

#define IPTABLES_CMD_BUF_SIZE    1024

/* iptables tools */
#define IPTABLES_TOOL           "PATH=/sbin:/usr/sbin iptables"
#define IPTABLES_SAVE_TOOL      "PATH=/sbin:/usr/sbin iptables-save"
#define IPTABLES_RESTORE_TOOL   "PATH=/sbin:/usr/sbin iptables-restore"

/* iptables tool extra options */
static char iptables_tool_options[RCF_MAX_VAL];

/*
 * Methods
 */

static te_errno iptables_table_list(unsigned int, const char *,
                                    const char *, char **,
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

static te_errno iptables_chain_list(unsigned int, const char *, const char *,
                                    char **, const char *, const char *,
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
      NULL, NULL, NULL};

RCF_PCH_CFG_NODE_COLLECTION(node_iptables_table, "table",
                            &node_iptables_chain, NULL,
                            NULL, NULL,
                            iptables_table_list, NULL);

RCF_PCH_CFG_NODE_NA(node_iptables, "iptables",
                    &node_iptables_table, NULL);

/**
 * Get the list of iptables tables available in the system.
 *
 * @param table_list    Output string containing available tables separated
 *                      with spaces.
 *
 * @return Status code
 */
static te_errno
iptables_obtain_table_list(te_string *table_list)
{
    const char *tables[] = {"filter", "mangle", "nat", "raw"};
    size_t i;

    te_string_reset(table_list);

    for (i = 0; i < TE_ARRAY_LEN(tables); i++)
    {
        int rc;
        te_errno te_rc;

        rc = ta_system_fmt(IPTABLES_TOOL " -t %s -L >/dev/null", tables[i]);
        if (rc < 0 || !WIFEXITED(rc) || WEXITSTATUS(rc) != 0)
            continue;

        te_rc = te_string_append(table_list, "%s ", tables[i]);
        if (te_rc != 0)
            return te_rc;
    }

    return 0;
}

/**
 * Obtain list of built-in iptables tables.
 *
 * @param gid           group identifier (unused)
 * @param oid           full parent object instence identifier (unused)
 * @param sub_id        ID of the object to be listed (unused)
 * @param list          location for the list pointer
 * @param ifname        interface name (unused)
 *
 * @return              Status code
 */
static te_errno
iptables_table_list(unsigned int  gid, const char *oid,
                    const char *sub_id, char **list,
                    const char *ifname)
{
    static te_string table_list = TE_STRING_INIT;

    UNUSED(sub_id);
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(ifname);

    if (table_list.len == 0)
    {
        te_errno rc = iptables_obtain_table_list(&table_list);

        if (rc != 0)
        {
            ERROR("%s(): failed to obtain iptables table list (%r)",
                  __FUNCTION__, rc);
            te_string_reset(&table_list);
            return rc;
        }
    }

    *list = strdup(table_list.ptr);

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

    TE_SPRINTF(buf,
        IPTABLES_TOOL " %s -t %s -S %s_%s | grep '^-A %s -%c %s -j %s_%s'",
        iptables_tool_options, table, chain, ifname, chain,
        iptables_is_chain_output(chain) ? 'o' : 'i',
        ifname, chain, ifname);
    VERB("Invoke: %s", buf);
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
    if (fp != NULL)
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
    rc = te_snprintf(cmd_buf, sizeof(cmd_buf),
                     IPTABLES_TOOL " %s -t %s -%c %s -%c %s -j %s_%s",
                     iptables_tool_options, table, (enable ? 'I' : 'D'), chain,
                     iptables_is_chain_output(chain) ? 'o' : 'i',
                     ifname, chain, ifname);
    if (rc != 0)
        return rc;

    VERB("Invoke: %s", cmd_buf);
    rc = ta_system(cmd_buf);
    if (rc < 0 || !WIFEXITED(rc) || WEXITSTATUS(rc) != 0)
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
    rc = te_snprintf(cmd_buf, sizeof(cmd_buf),
                     IPTABLES_TOOL " %s -t %s -N %s_%s",
                     iptables_tool_options, table, chain, ifname);
    if (rc != 0)
        return rc;

    VERB("Invoke: %s", cmd_buf);
    rc = ta_system(cmd_buf);
    if (rc < 0 || !WIFEXITED(rc) || WEXITSTATUS(rc) != 0)
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
    rc = te_snprintf(cmd_buf, sizeof(cmd_buf),
                     IPTABLES_TOOL " %s -t %s -F %s_%s",
                     iptables_tool_options, table, chain, ifname);
    if (rc != 0)
        return rc;

    VERB("Invoke: %s", cmd_buf);
    rc = ta_system(cmd_buf);
    if (rc < 0 || !WIFEXITED(rc) || WEXITSTATUS(rc) != 0)
    {
        ERROR("Failed to flush the chain %s_%s, rc=%r", chain, ifname, rc);
        return rc;
    }

    /* Remove all rules which refer to the current chain */
    rc = te_snprintf(cmd_buf, sizeof(cmd_buf),
                     IPTABLES_SAVE_TOOL " | grep -v -- '-j %s_%s' | "
                     IPTABLES_RESTORE_TOOL,
                     chain, ifname);
    if (rc != 0)
        return rc;

    rc = ta_system(cmd_buf);
    if (rc < 0 || !WIFEXITED(rc) || WEXITSTATUS(rc) != 0)
    {
        ERROR("Failed to remove all rules referring to the chain %s_%s, "
              "rc=%r", chain, ifname, rc);
        return rc;
    }

    /* Delete chain */
    rc = te_snprintf(cmd_buf, sizeof(cmd_buf),
                     IPTABLES_TOOL " %s -t %s -X %s_%s",
                     iptables_tool_options, table, chain, ifname);
    if (rc != 0)
        return rc;

    VERB("Invoke: %s", cmd_buf);
    rc = ta_system(cmd_buf);
    if (rc < 0 || !WIFEXITED(rc) || WEXITSTATUS(rc) != 0)
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
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(dummy);

    rc = te_snprintf(value, RCF_MAX_VAL,
                     iptables_perif_chain_is_enabled(ifname, table, chain) ?
                     "1" : "0");

    INFO("%s(): dummy %p, table %p chain %p", __FUNCTION__,
         dummy, table, chain);

    return rc;
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
 * @param sub_id        ID of the object to be listed (unused)
 * @param list          location of the chains list
 * @param ifname        interface name to operate the chain linked to
 * @param dummy         unused value, corresponding to .../iptables: node
 * @param table         table name to look for chains in
 *
 * @return              Status code
 */
static te_errno
iptables_chain_list(unsigned int  gid, const char *oid,
                    const char *sub_id, char **list,
                    const char *ifname, const char *dummy,
                    const char *table)
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
    UNUSED(sub_id);
    UNUSED(dummy);

    INFO("%s started, ifname=%s, table=%s", __FUNCTION__, ifname, table);

    *list = NULL;

    rc = te_snprintf(buf, sizeof(buf),
                     IPTABLES_TOOL " %s -t %s -S | grep '^-N .*_%s' | "
                     "sed -e 's/^-N //g' | sed -e 's/_%s$//g'",
                     iptables_tool_options, table, ifname, ifname);
    if (rc != 0)
        return rc;

    VERB("Invoke: %s", buf);
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

extern void **konst_susp_ptr;

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

    size_t rest_value_space = RCF_MAX_VAL;
    size_t sz;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(dummy);

#if 0
    fprintf(stderr,"%p:%s(ifname=%s, table=%s, chain=%s) started, "
        " konst_susp_ptr %p\n",
         &iptables_rules_get, __FUNCTION__, ifname, table, chain,
         konst_susp_ptr);
#endif
    RING("%s(ifname=%s, table=%s, chain=%s) started",
         __FUNCTION__, ifname, table, chain);

    *value = '\0';

    rc = te_snprintf(buf, sizeof(buf),
                     IPTABLES_TOOL " %s -t %s -S %s_%s | "
                     "grep '^-A %s_%s ' | "
                     "sed -e 's/^-A %s_%s //g'",
                     iptables_tool_options,
                     table, chain, ifname, chain, ifname, chain, ifname);
    if (rc != 0)
        return rc;

    VERB("Invoke: %s", buf);
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

        sz = snprintf(value, rest_value_space, "%s\n", buf);
        if (sz >= rest_value_space)
        {
            WARN("%s(): got value is cut, need print %d bytes, buf '%s'",
                 __FUNCTION__, sz, buf);
            break;
        }
        value += sz;
        rest_value_space -= sz;
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
    rc = te_snprintf(buf, sizeof(buf),
                     IPTABLES_TOOL " %s -t %s -F %s_%s",
                     iptables_tool_options, table, chain, ifname);
    if (rc != 0)
        return rc;

    VERB("Invoke: %s", buf);
    ta_system(buf);

    /* Open iptables-restore session, do not flush all chains */
    rc = te_snprintf(buf, sizeof(buf),
                     IPTABLES_RESTORE_TOOL " -n");
    if (rc != 0)
        return rc;

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
    do {
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
    int         rc = 0;
    char        command;
    const char *val_p;
    const char *parameter_j_p;
    te_string   buf = TE_STRING_INIT;

    static const char *parameter_j = " -j";

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(dummy);

    INFO("%s(ifname=%s, table=%s, chain=%s): %s", __FUNCTION__,
         ifname, table, chain, value);

#define SKIP_SPACES(_p)                                 \
    while (isspace(*(_p))) (_p)++;

    /*
     * Any @p value for iptables must begin with one of the following
     * commands (@c '-A', @c '-I' or @c '-D') and may contain the parameter
     * @c '-j'. The @p value may contain one of two substitutions.
     *
     * The first substitution is located after the command @c '-A',
     * @c '-I' or @c '-D'. The second substitution is located after
     * the parameter @c '-j' without target value. Second substitution takes
     * precedence.
     */
    rc = te_string_append(&buf, IPTABLES_TOOL " %s -t %s ",
                          iptables_tool_options, table);
    if (rc)
    {
        te_string_free(&buf);
        return rc;
    }

    /*
     * Find parameter @c " -j" without target value. If this parameter
     * contains target, then the second substitution is not performed.
     */
    parameter_j_p = strstr(value, parameter_j);
    if (parameter_j_p != NULL)
    {
        te_bool contain_space;

        val_p = parameter_j_p + strlen(parameter_j);

        contain_space = (*val_p == ' ');
        SKIP_SPACES(val_p);

        /*
         * The parameter @c "-j" doesn't have a target in one of two cases:
         * - any number of spaces and end of line;
         * - one or more spaces and new parameter (@c '-').
         */
        if (!((*val_p == '\0') ||
              (contain_space && *val_p == '-')))
            parameter_j_p = NULL;
    }

    val_p = (char *)value;
    SKIP_SPACES(val_p);
    if (*val_p++ != '-')
    {
        ERROR("Invalid rule format");
        te_string_free(&buf);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    command = *val_p++;
    if (!((command == 'A') || (command == 'D') || (command == 'I')))
    {
        ERROR("Unknown iptables rule action");
        te_string_free(&buf);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (parameter_j_p)
    {
        /*
         * The first substitution should not be applied because the second
         * is found.
         */
        SKIP_SPACES(val_p);
        if (*val_p == '-')
        {
            ERROR("iptables rule action has two substitutions");
            te_string_free(&buf);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }

        /*
         * Copy a command and move the pointers to the parameter @c "-j".
         */
        rc = te_string_append(&buf, "-%c %s ", command, val_p);
        if (rc)
        {
            te_string_free(&buf);
            return rc;
        }

        buf.len -= strlen(val_p) - (parameter_j_p - val_p);
        val_p = parameter_j_p + strlen(parameter_j);

        command = 'j';
    }
    rc = te_string_append(&buf, "-%c %s_%s%s", command, chain, ifname,
                          val_p);
    if (rc)
    {
        te_string_free(&buf);
        return rc;
    }

#undef SKIP_SPACES

    VERB("Invoke: %s", buf.ptr);

    rc = ta_system(buf.ptr);
    if (rc < 0 || !WIFEXITED(rc) || WEXITSTATUS(rc) != 0)
    {
        ERROR("Command '%s' returned %r", buf.ptr, rc);
    }

    te_string_free(&buf);
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
    UNUSED(ifname);
    UNUSED(dummy);
    UNUSED(table);
    UNUSED(chain);

    *value = '\0';

    return 0;
}

/**
 * Set the extra options for iptables tool.
 *
 * @param gid           Group identifier (unused).
 * @param oid           Full object instance identifier.
 * @param value         New value.
 *
 * @return Status code.
 */
static te_errno
iptables_tool_opts_set(unsigned int gid, const char *oid, const char *value)
{
    UNUSED(gid);

    INFO("%s, %s = %s", __FUNCTION__, oid, value);

    if (strlen(value) >= RCF_MAX_VAL)
    {
        ERROR("A buffer to save the \"%s\" variable value is too small.",
              oid);
        return TE_RC(TE_TA_UNIX, TE_EOVERFLOW);
    }
    strcpy(iptables_tool_options, value);

    return 0;
}

/**
 * Get the extra options for iptables tool.
 *
 * @param gid           Group identifier (unused).
 * @param oid           Full object instance identifier.
 * @param value         Obtained value.
 *
 * @return Status code.
 */
static te_errno
iptables_tool_opts_get(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);

    INFO("%s, %s = %s", __FUNCTION__, oid, iptables_tool_options);

    strcpy(value, iptables_tool_options);

    return 0;
}

/* iptables tool options Configurator node */
RCF_PCH_CFG_NODE_RW(node_iptables_tool_opts, "iptables_tool_opts",
                    NULL, NULL,
                    iptables_tool_opts_get,
                    iptables_tool_opts_set);


/**
 * Initialize iptables subtree
 *
 * @return              Status code
 */
extern te_errno
ta_unix_conf_iptables_init(void)
{
    rcf_pch_add_node("/agent", &node_iptables_tool_opts);
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


