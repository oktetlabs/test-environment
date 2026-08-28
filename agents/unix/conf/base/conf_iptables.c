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

#include "te_alloc.h"
#include "te_errno.h"
#include "logger_api.h"
#include "te_defs.h"
#include "te_str.h"
#include "rcf_pch.h"
#include "rcf_pch_tree.h"
#include "unix_internal.h"
#include "te_shell_cmd.h"
#include "ta_common.h"
#include "te_string.h"
#include "te_vector.h"

#if __linux__

#define IPTABLES_CMD_BUF_SIZE    1024

/* iptables tools */
#define IPTABLES_TOOL           "PATH=/sbin:/usr/sbin iptables"
#define IPTABLES_SAVE_TOOL      "PATH=/sbin:/usr/sbin iptables-save"
#define IPTABLES_RESTORE_TOOL   "PATH=/sbin:/usr/sbin iptables-restore"

#define IP6TABLES_TOOL           "PATH=/sbin:/usr/sbin ip6tables"
#define IP6TABLES_SAVE_TOOL      "PATH=/sbin:/usr/sbin ip6tables-save"
#define IP6TABLES_RESTORE_TOOL   "PATH=/sbin:/usr/sbin ip6tables-restore"

/* iptables tool extra options */
static char iptables_tool_options[RCF_MAX_VAL];

/*
 * Methods
 */

/**
 * Split a space-separated list produced in-place in @p buf_in into
 * individual heap-allocated names appended to @p names.
 *
 * @param buf_in        Space-separated names (modified in place)
 * @param names         Vector of heap-allocated names to append to
 */
static void
buf_append_names(char *buf_in, te_vec *names)
{
    char *saveptr;
    char *tok;

    for (tok = strtok_r(buf_in, " ", &saveptr); tok != NULL;
         tok = strtok_r(NULL, " ", &saveptr))
    {
        char *name = TE_STRDUP(tok);

        TE_VEC_APPEND(names, name);
    }
}

/**
 * Obtain list of supported IP versions.
 *
 * @param ctx           request context (unused, interface is irrelevant)
 * @param names         vector of heap-allocated names to append to
 *
 * @return              Status code
 */
static te_errno
iptables_iptables_list(const ta_conf_ctx *ctx, te_vec *names)
{
    char *ip4 = TE_STRDUP("4");
    char *ip6 = TE_STRDUP("6");

    UNUSED(ctx);

    TE_VEC_APPEND(names, ip4);
    TE_VEC_APPEND(names, ip6);

    return 0;
}

static const char *
iptables_get_tool(const char *ip)
{
    return *ip == '4' ? IPTABLES_TOOL : IP6TABLES_TOOL;
}

static const char *
iptables_get_save_tool(const char *ip)
{
    return *ip == '4' ? IPTABLES_SAVE_TOOL : IP6TABLES_SAVE_TOOL;
}

static const char *
iptables_get_restore_tool(const char *ip)
{
    return *ip == '4' ? IPTABLES_RESTORE_TOOL : IP6TABLES_RESTORE_TOOL;
}

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

        rc = ta_system_fmt(IPTABLES_TOOL " -t %s -L >/dev/null", tables[i]);
        if (rc < 0 || !WIFEXITED(rc) || WEXITSTATUS(rc) != 0)
            continue;

        te_string_append(table_list, "%s ", tables[i]);
    }

    return 0;
}

/**
 * Obtain list of built-in iptables tables.
 *
 * @param ctx           request context (unused, interface/IP version
 *                      are irrelevant)
 * @param names         vector of heap-allocated names to append to
 *
 * @return              Status code
 */
static te_errno
iptables_table_list(const ta_conf_ctx *ctx, te_vec *names)
{
    static te_string table_list = TE_STRING_INIT;
    char *copy;

    UNUSED(ctx);

    if (table_list.len == 0)
    {
        /*
         * Assume that the sets of available tables are the same
         * for IPv4 and IPv6.
         */
        te_errno rc = iptables_obtain_table_list(&table_list);

        if (rc != 0)
        {
            ERROR("%s(): failed to obtain iptables table list (%r)",
                  __FUNCTION__, rc);
            te_string_reset(&table_list);
            return rc;
        }
    }

    if (table_list.ptr == NULL)
        return 0;

    /* Tokenize a copy: table_list is a persistent cache. */
    copy = TE_STRDUP(table_list.ptr);
    buf_append_names(copy, names);
    free(copy);

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
static bool
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
 * @param ip            IP version
 * @param table         table name to check chains in
 * @param chain         chain name to check status of
 *
 * @return              Check status (boolean value)
 */
static bool
iptables_perif_chain_is_enabled(const char *ifname, const char *ip,
                                const char *table, const char *chain)
{
    FILE    *fp;
    int      out_fd;
    char     buf[IPTABLES_CMD_BUF_SIZE];
    bool enabled = false;
    pid_t    pid;
    int      status;

    INFO("%s started, ifname=%s, table=%s", __FUNCTION__, ifname, table);

    TE_SPRINTF(buf,
        "%s %s -t %s -S %s_%s | grep '^-A %s -%c %s -j %s_%s'",
        iptables_get_tool(ip), iptables_tool_options, table, chain, ifname,
        chain, iptables_is_chain_output(chain) ? 'o' : 'i', ifname,
        chain, ifname);
    VERB("Invoke: %s", buf);
    if ((pid = te_shell_cmd(buf, -1, NULL, &out_fd, NULL)) < 0)
    {
        ERROR("failed to execute command line while getting: %s: "
              "rc=%r (%s)", buf, pid, strerror(errno));
        return false;
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
 * @param enable        state of chain jumping rule (if @c true, jumping rule
 *                      should be installed)
 *
 * @return              Status code
 */
static te_errno
iptables_perif_chain_set(const char *ifname,
                         const char *ip,
                         const char *table,
                         const char *chain,
                         bool enable)
{
    int  rc;
    char cmd_buf[IPTABLES_CMD_BUF_SIZE];

    INFO("%s(%s, %s, %s, %s, %s) started", __FUNCTION__,
         ifname, ip, table, chain, enable ? "ON" : "OFF");

    if (enable == iptables_perif_chain_is_enabled(ifname, ip, table, chain))
        return 0;

    /* Add rule to jump to new chain */
    rc = te_snprintf(cmd_buf, sizeof(cmd_buf),
                     "%s %s -t %s -%c %s -%c %s -j %s_%s",
                     iptables_get_tool(ip), iptables_tool_options, table,
                     (enable ? 'I' : 'D'), chain,
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
 * @param ctx           request context
 * @param val           any nonzero value enables the jumping rule
 *
 * @return              Status code
 */
static te_errno
iptables_chain_add(const ta_conf_ctx *ctx, int32_t val)
{
    const char *ifname = ta_conf_ctx_inst(ctx, "interface");
    const char *ip = ta_conf_ctx_inst(ctx, "iptables");
    const char *table = ta_conf_ctx_inst(ctx, "table");
    const char *chain = ta_conf_ctx_inst(ctx, "chain");
    char        cmd_buf[IPTABLES_CMD_BUF_SIZE];
    int         enable = (val != 0);
    te_errno    rc;

    INFO("%s(%s, %s, %s, %s) started", __FUNCTION__, ifname, ip, table, chain);

    /* Create new chain first */
    rc = te_snprintf(cmd_buf, sizeof(cmd_buf),
                     "%s %s -t %s -N %s_%s",
                     iptables_get_tool(ip), iptables_tool_options, table,
                     chain, ifname);
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
        if ((rc = iptables_perif_chain_set(ifname, ip, table,
                                           chain, true)) != 0)
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
 * @param ctx           request context
 *
 * @return              Status code
 */
static te_errno
iptables_chain_del(const ta_conf_ctx *ctx)
{
    const char *ifname = ta_conf_ctx_inst(ctx, "interface");
    const char *ip = ta_conf_ctx_inst(ctx, "iptables");
    const char *table = ta_conf_ctx_inst(ctx, "table");
    const char *chain = ta_conf_ctx_inst(ctx, "chain");
    char        cmd_buf[IPTABLES_CMD_BUF_SIZE];
    te_errno    rc;

    INFO("%s(%s, %s, %s, %s) started", __FUNCTION__, ifname, ip, table, chain);

    /* Delete jump rule */
    if (iptables_perif_chain_is_enabled(ifname, ip, table, chain))
    {
        if ((rc = iptables_perif_chain_set(ifname, ip, table,
                                           chain, false)) != 0)
        {
            ERROR("Failed to remove jumping rule for chain %s_%s, rc=%r",
                  chain, ifname, rc);
            return rc;
        }
    }

    /* Flush chain */
    rc = te_snprintf(cmd_buf, sizeof(cmd_buf),
                     "%s %s -t %s -F %s_%s",
                     iptables_get_tool(ip), iptables_tool_options, table,
                     chain, ifname);
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
                     "%s | grep -v -- '-j %s_%s' | %s",
                     iptables_get_save_tool(ip), chain, ifname,
                     iptables_get_restore_tool(ip));
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
                     "%s %s -t %s -X %s_%s",
                     iptables_get_tool(ip), iptables_tool_options, table,
                     chain, ifname);
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
 * @param ctx           request context
 * @param val           any nonzero value installs the jumping rule,
 *                      zero removes it
 *
 * @return              Status code
 */
static te_errno
iptables_chain_set(const ta_conf_ctx *ctx, int32_t val)
{
    const char *ifname = ta_conf_ctx_inst(ctx, "interface");
    const char *ip = ta_conf_ctx_inst(ctx, "iptables");
    const char *table = ta_conf_ctx_inst(ctx, "table");
    const char *chain = ta_conf_ctx_inst(ctx, "chain");
    int enable = (val != 0);

    return iptables_perif_chain_set(ifname, ip, table, chain, enable);
}

/**
 * Get the status of per-interface chain jumping rule (installed or not)
 *
 * @param ctx           request context
 * @param val           location to returned status of jumping rule
 *
 * @return              Status code
 */
static te_errno
iptables_chain_get(const ta_conf_ctx *ctx, int32_t *val)
{
    const char *ifname = ta_conf_ctx_inst(ctx, "interface");
    const char *ip = ta_conf_ctx_inst(ctx, "iptables");
    const char *table = ta_conf_ctx_inst(ctx, "table");
    const char *chain = ta_conf_ctx_inst(ctx, "chain");

    *val = iptables_perif_chain_is_enabled(ifname, ip, table, chain) ? 1 : 0;

    INFO("%s(): ip %p, table %p chain %p", __FUNCTION__,
         ip, table, chain);

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
 * @param ctx           request context (parent instance is the table)
 * @param names         vector of heap-allocated names to append to
 *
 * @return              Status code
 */
static te_errno
iptables_chain_list(const ta_conf_ctx *ctx, te_vec *names)
{
    const char *ifname = ta_conf_ctx_inst(ctx, "interface");
    const char *ip = ta_conf_ctx_inst(ctx, "iptables");
    const char *table = ta_conf_ctx_inst(ctx, "table");
    int       rc        = 0;
    FILE     *fp;
    int       out_fd;
    char      buf[IPTABLES_CMD_BUF_SIZE];
    pid_t     pid;
    int       status;

    INFO("%s started, ifname=%s, ip=%s, table=%s", __FUNCTION__,
         ifname, ip, table);

    rc = te_snprintf(buf, sizeof(buf),
                     "%s %s -t %s -S | grep '^-N .*_%s' | "
                     "sed -e 's/^-N //g' | sed -e 's/_%s$//g'",
                     iptables_get_tool(ip), iptables_tool_options, table,
                     ifname, ifname);
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

    while (fgets(buf, sizeof(buf), fp) != NULL)
    {
        char *name;

        /* Remove trailing newline */
        chomp(buf);

        name = TE_STRDUP(buf);
        TE_VEC_APPEND(names, name);

        INFO("Found chain %s", buf);
    }

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
 * @param ctx           request context
 * @param val           location for the rules list
 *
 * @return              Status code
 */
static te_errno
iptables_rules_get(const ta_conf_ctx *ctx, te_string *val)
{
    const char *ifname = ta_conf_ctx_inst(ctx, "interface");
    const char *ip = ta_conf_ctx_inst(ctx, "iptables");
    const char *table = ta_conf_ctx_inst(ctx, "table");
    const char *chain = ta_conf_ctx_inst(ctx, "chain");
    int   rc = 0;
    FILE *fp = NULL;
    int   out_fd;
    char  buf[IPTABLES_CMD_BUF_SIZE] = {0, };
    pid_t pid;
    int   status;
    /*
     * Written into directly (bypassing te_string bookkeeping) to
     * preserve the legacy graceful truncation below: overflowing a
     * te_string external buffer via te_string_append() is fatal.
     */
    char *value = val->ptr;

    size_t rest_value_space = RCF_MAX_VAL;
    size_t sz;

#if 0
    fprintf(stderr,"%p:%s(ifname=%s, table=%s, chain=%s) started, "
        " konst_susp_ptr %p\n",
         &iptables_rules_get, __FUNCTION__, ifname, table, chain,
         konst_susp_ptr);
#endif
    RING("%s(ifname=%s, ip=%s, table=%s, chain=%s) started",
         __FUNCTION__, ifname, ip, table, chain);

    *value = '\0';

    rc = te_snprintf(buf, sizeof(buf),
                     "%s %s -t %s -S %s_%s | "
                     "grep '^-A %s_%s ' | "
                     "sed -e 's/^-A %s_%s //g'",
                     iptables_get_tool(ip), iptables_tool_options,
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
 * @param ctx           request context
 * @param value         rules list without chain name and delimited by '|'
 *
 * @return              Status code
 */
static te_errno
iptables_rules_set(const ta_conf_ctx *ctx, const char *value)
{
    const char *ifname = ta_conf_ctx_inst(ctx, "interface");
    const char *ip = ta_conf_ctx_inst(ctx, "iptables");
    const char *table = ta_conf_ctx_inst(ctx, "table");
    const char *chain = ta_conf_ctx_inst(ctx, "chain");
    int   rc = 0;
    FILE *fp = NULL;
    int   in_fd;
    char  buf[IPTABLES_CMD_BUF_SIZE];
    const char *p = NULL;
    pid_t pid;
    int   status;

    INFO("%s started, ifname=%s, ip=%s, table=%s", __FUNCTION__,
         ifname, ip, table);

    /* Flush the chain */
    rc = te_snprintf(buf, sizeof(buf),
                     "%s %s -t %s -F %s_%s",
                     iptables_get_tool(ip), iptables_tool_options,
                     table, chain, ifname);
    if (rc != 0)
        return rc;

    VERB("Invoke: %s", buf);
    ta_system(buf);

    /* Open iptables-restore session, do not flush all chains */
    rc = te_snprintf(buf, sizeof(buf),
                     "%s -n", iptables_get_restore_tool(ip));
    if (rc != 0)
        return rc;

    if ((pid = te_shell_cmd(buf, -1, &in_fd, NULL, NULL)) < 0)
    {
        ERROR("failed to execute command line while getting: %s: "
              "rc=%r (%s)", buf, pid, strerror(errno));
        return pid;
    }

    if ((fp = fdopen(in_fd, "w")) == NULL)
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
        fprintf(fp, buf, chain, ifname, value);

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
 * @param ctx           request context
 * @param value         iptables command to execute, chain name in the
 *                      command should be omitted to avoid ambiguity
 *
 * @return              Status code
 */
static te_errno
iptables_cmd_set(const ta_conf_ctx *ctx, const char *value)
{
    const char *ifname = ta_conf_ctx_inst(ctx, "interface");
    const char *ip = ta_conf_ctx_inst(ctx, "iptables");
    const char *table = ta_conf_ctx_inst(ctx, "table");
    const char *chain = ta_conf_ctx_inst(ctx, "chain");
    int         rc = 0;
    char        command;
    const char *val_p;
    const char *parameter_j_p;
    te_string   buf = TE_STRING_INIT;

    static const char *parameter_j = " -j";

    INFO("%s(ifname=%s, ip=%s, table=%s, chain=%s): %s", __FUNCTION__,
         ifname, ip, table, chain, value);

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
    te_string_append(&buf, "%s %s -t %s ",
                     iptables_get_tool(ip), iptables_tool_options, table);

    /*
     * Find parameter @c " -j" without target value. If this parameter
     * contains target, then the second substitution is not performed.
     */
    parameter_j_p = strstr(value, parameter_j);
    if (parameter_j_p != NULL)
    {
        bool contain_space;

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
        te_string_append(&buf, "-%c %s ", command, val_p);

        buf.len -= strlen(val_p) - (parameter_j_p - val_p);
        val_p = parameter_j_p + strlen(parameter_j);

        command = 'j';
    }
    te_string_append(&buf, "-%c %s_%s%s", command, chain, ifname, val_p);

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
 * @param ctx           request context
 * @param val           location to the returned empty value
 *
 * @return              Status code
 */
static te_errno
iptables_cmd_get(const ta_conf_ctx *ctx, te_string *val)
{
    UNUSED(ctx);
    UNUSED(val);

    return 0;
}

/**
 * Set the extra options for iptables tool.
 *
 * @param ctx           Request context.
 * @param value         New value.
 *
 * @return Status code.
 */
static te_errno
iptables_tool_opts_set(const ta_conf_ctx *ctx, const char *value)
{
    INFO("%s, %s = %s", __FUNCTION__, ta_conf_ctx_oid(ctx), value);

    if (strlen(value) >= RCF_MAX_VAL)
    {
        ERROR("A buffer to save the \"%s\" variable value is too small.",
              ta_conf_ctx_oid(ctx));
        return TE_RC(TE_TA_UNIX, TE_EOVERFLOW);
    }
    strcpy(iptables_tool_options, value);

    return 0;
}

/**
 * Get the extra options for iptables tool.
 *
 * @param ctx           Request context.
 * @param val           Obtained value.
 *
 * @return Status code.
 */
static te_errno
iptables_tool_opts_get(const ta_conf_ctx *ctx, te_string *val)
{
    INFO("%s, %s = %s", __FUNCTION__, ta_conf_ctx_oid(ctx),
        iptables_tool_options);

    te_string_append(val, "%s", iptables_tool_options);

    return 0;
}

static const ta_conf_node *const node_iptables =
    TA_CONF_LIST("iptables", iptables_iptables_list,
        TA_CONF_LIST("table", iptables_table_list,
            TA_CONF_COLL_INT32_RW("chain", iptables_chain_get,
                                iptables_chain_set, iptables_chain_add,
                                iptables_chain_del, iptables_chain_list,
                TA_CONF_RW_STR("cmd", iptables_cmd_get,
                               iptables_cmd_set),
                TA_CONF_RW_STR("rules", iptables_rules_get,
                               iptables_rules_set))));

static const ta_conf_node *const node_iptables_tool_opts =
    TA_CONF_RW_STR("iptables_tool_opts", iptables_tool_opts_get,
                   iptables_tool_opts_set);

/**
 * Initialize iptables subtree
 *
 * @return              Status code
 */
extern te_errno
ta_unix_conf_iptables_init(void)
{
    te_errno rc;

    rc = ta_conf_register("/agent", node_iptables_tool_opts);
    if (rc != 0)
        return rc;

    return ta_conf_register("/agent/interface", node_iptables);
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


