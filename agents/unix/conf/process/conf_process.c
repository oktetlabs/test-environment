/** @file
 * @brief Unix Test Agent
 *
 * Unix TA processes support
 *
 * Copyright (C) 2020 OKTET Labs. All rights reserved.
 *
 * @author Dilshod Urazov <Dilshod.Urazov@oktetlabs.ru>
 */

#define TE_LGR_USER     "Unix Conf Process"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "te_errno.h"
#include "logger_api.h"
#include "te_defs.h"
#include "te_queue.h"
#include "te_alloc.h"
#include "te_string.h"
#include "te_shell_cmd.h"
#include "te_str.h"
#include "rcf_pch.h"
#include "agentlib.h"
#include "conf_common.h"
#include "conf_process.h"

struct ps_arg_entry {
    SLIST_ENTRY(ps_arg_entry)     links;
    char                         *value;
    unsigned int                  order;
};

typedef SLIST_HEAD(ps_arg_list_t, ps_arg_entry) ps_arg_list_t;

struct ps_entry {
    SLIST_ENTRY(ps_entry)   links;
    char                   *name;
    char                   *exe;
    ps_arg_list_t           args;
    unsigned int            argc;
    pid_t                   id;
};

static SLIST_HEAD(, ps_entry) processes = SLIST_HEAD_INITIALIZER(processes);

static te_bool
ps_is_running(struct ps_entry *ps)
{
    return ps->id < 0 ? FALSE : (ta_waitpid(ps->id, NULL, WNOHANG) == 0);
}

static int
ps_arg_compare(const void *a, const void *b)
{
    const struct ps_arg_entry **arg_a = (const struct ps_arg_entry **)a;
    const struct ps_arg_entry **arg_b = (const struct ps_arg_entry **)b;

    return (*arg_a)->order - (*arg_b)->order;
}

static te_errno
ps_get_argv(struct ps_entry *ps, char ***argv)
{
    char **tmp;
    struct ps_arg_entry *arg;
    struct ps_arg_entry **args;
    unsigned int i;

    args = TE_ALLOC(ps->argc * sizeof(*args));
    if (args == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    i = 0;
    SLIST_FOREACH(arg, &ps->args, links)
    {
        args[i] = arg;
        i++;
    }

    qsort(args, ps->argc, sizeof(*args), ps_arg_compare);

    tmp = TE_ALLOC((ps->argc + 2) * sizeof(char *));
    if (tmp == NULL)
    {
        free(args);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    tmp[0] = ps->exe;
    tmp[ps->argc + 1] = NULL;
    for (i = 1; i < ps->argc + 1; i++)
        tmp[i] = args[i - 1]->value;

    *argv = tmp;

    free(args);

    return 0;
}

static te_errno
ps_start(struct ps_entry *ps)
{
    char **argv;
    te_errno rc;

    rc = ps_get_argv(ps, &argv);
    if (rc != 0)
        return rc;

    ps->id = te_exec_child(ps->exe, argv, NULL,
                             (uid_t)-1, NULL, NULL, NULL);

    free(argv);

    return (ps->id < 0) ? TE_RC(TE_TA_UNIX, TE_ECHILD) : 0;
}

static te_errno
ps_stop(struct ps_entry *ps)
{
    if (ta_kill_death(ps->id) != 0)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return 0;
}

static struct ps_entry *
ps_find(const char *ps_name)
{
    struct ps_entry *ps;

    SLIST_FOREACH(ps, &processes, links)
    {
        if (strcmp(ps_name, ps->name) == 0)
            return ps;
    }

    return NULL;
}

static te_errno
ps_list(unsigned int gid, const char *oid, const char *sub_id, char **list)
{
    struct ps_entry *ps;
    size_t len = 0;
    char *p;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    SLIST_FOREACH(ps, &processes, links)
        len += strlen(ps->name) + 1;

    if (len == 0)
    {
        *list = NULL;
        return 0;
    }

    *list = TE_ALLOC(len);
    if (*list == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    p = *list;
    SLIST_FOREACH(ps, &processes, links)
    {
        if (p != *list)
            *p++ = ' ';
        len = strlen(ps->name);
        memcpy(p, ps->name, len);
        p += len;
    }
    *p = '\0';

    return 0;
}

static te_errno
ps_add(unsigned int gid, const char *oid, const char *value,
       const char *ps_name)
{
    struct ps_entry *ps;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    ENTRY("%s", ps_name);

    if (ps_find(ps_name) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    ps = TE_ALLOC(sizeof(*ps));
    if (ps == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    ps->name = strdup(ps_name);
    if (ps->name == NULL)
    {
        free(ps);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    ps->id = -1;
    ps->argc = 0;

    SLIST_INIT(&ps->args);

    SLIST_INSERT_HEAD(&processes, ps, links);

    return 0;
}

static te_errno
ps_del(unsigned int gid, const char *oid,
       const char *ps_name)
{
    struct ps_entry *ps;

    UNUSED(gid);
    UNUSED(oid);

    ENTRY("%s", ps_name);

    ps = ps_find(ps_name);
    if (ps == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (ps_is_running(ps))
        ps_stop(ps);

    SLIST_REMOVE(&processes, ps, ps_entry, links);

    free(ps->name);
    free(ps->exe);
    free(ps);

    return 0;
}

static te_errno
ps_exe_get(unsigned int gid, const char *oid, char *value,
           const char *ps_name)
{
    struct ps_entry *ps;

    UNUSED(gid);
    UNUSED(oid);

    ENTRY("%s", ps_name);

    ps = ps_find(ps_name);
    if (ps == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(value, RCF_MAX_VAL, "%s", ps->exe);

    return 0;
}

static te_errno
ps_exe_set(unsigned int gid, const char *oid, const char *value,
           const char *ps_name)
{
    struct ps_entry *ps;

    UNUSED(gid);
    UNUSED(oid);

    ENTRY("%s", ps_name);

    ps = ps_find(ps_name);
    if (ps == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (ps_is_running(ps))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    return string_replace(&ps->exe, value);
}

static te_errno
ps_status_get(unsigned int gid, const char *oid, char *value,
              const char *ps_name)
{
    struct ps_entry *ps;

    UNUSED(gid);
    UNUSED(oid);

    ENTRY("%s", ps_name);

    ps = ps_find(ps_name);
    if (ps == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(value, RCF_MAX_VAL, "%d", ps_is_running(ps));

    return 0;
}

static te_errno
ps_status_set(unsigned int gid, const char *oid, const char *value,
              const char *ps_name)
{
    struct ps_entry *ps;
    te_bool enable = !!atoi(value);
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);

    ENTRY("%s", ps_name);

    ps = ps_find(ps_name);
    if (ps == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (enable == ps_is_running(ps))
        return 0;

    rc = enable ? ps_start(ps) : ps_stop(ps);

    return rc;
}

static void
ps_arg_free(struct ps_arg_entry *arg)
{
    free(arg->value);
    free(arg);
}

static struct ps_arg_entry *
ps_arg_find(const struct ps_entry *ps, unsigned int order)
{
    struct ps_arg_entry *p;

    if (ps == NULL)
        return NULL;

    SLIST_FOREACH(p, &ps->args, links)
    {
        if (p->order == order)
            return p;
    }

    return NULL;
}

static te_errno
ps_arg_list(unsigned int gid, const char *oid, const char *sub_id, char **list,
            const char *ps_name)
{
    te_string result = TE_STRING_INIT;
    struct ps_entry *ps;
    struct ps_arg_entry *arg;
    te_bool first = TRUE;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    if((ps = ps_find(ps_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_FOREACH(arg, &ps->args, links)
    {
        rc = te_string_append(&result, "%s%u", first ? "" : " ", arg->order);
        first = FALSE;
        if (rc != 0)
        {
            te_string_free(&result);
            return rc;
        }
    }

    *list = result.ptr;
    return 0;
}

static te_errno
ps_arg_get(unsigned int gid, const char *oid, char *value,
           const char *ps_name, const char *arg_name)
{
    struct ps_entry *ps;
    struct ps_arg_entry *arg = NULL;
    unsigned int order;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);

    if ((ps = ps_find(ps_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    rc = te_strtoui(arg_name, 10, &order);
    if (rc != 0)
        return rc;

    arg = ps_arg_find(ps, order);
    if (arg == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(value, RCF_MAX_VAL, "%s", arg->value);

    return 0;
}

static te_errno
ps_arg_add(unsigned int gid, const char *oid, const char *value,
           const char *ps_name, const char *arg_name)
{
    struct ps_entry *ps;
    struct ps_arg_entry *arg;
    unsigned int order;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);

    if ((ps = ps_find(ps_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (ps_is_running(ps))
        return TE_RC(TE_TA_UNIX, ETXTBSY);

    rc = te_strtoui(arg_name, 10, &order);
    if (rc != 0)
        return rc;

    if (ps_arg_find(ps, order) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    arg = TE_ALLOC(sizeof(*arg));
    if (arg == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    arg->value = strdup(value);
    if (arg->value == NULL)
    {
        free(arg);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    arg->order = order;

    SLIST_INSERT_HEAD(&ps->args, arg, links);
    ps->argc++;

    return 0;
}

static te_errno
ps_arg_del(unsigned int gid, const char *oid,
           const char *ps_name, const char *arg_name)
{
    struct ps_entry *ps;
    struct ps_arg_entry *arg;
    unsigned int order;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);

    if((ps = ps_find(ps_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (ps_is_running(ps))
        return TE_RC(TE_TA_UNIX, ETXTBSY);

    rc = te_strtoui(arg_name, 10, &order);
    if (rc != 0)
        return rc;

    arg = ps_arg_find(ps, order);
    if (arg == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_REMOVE(&ps->args, arg, ps_arg_entry, links);
    ps->argc--;

    ps_arg_free(arg);

    return 0;
}


RCF_PCH_CFG_NODE_RW_COLLECTION(node_ps_arg, "arg", NULL, NULL,
                               ps_arg_get, NULL, ps_arg_add,
                               ps_arg_del, ps_arg_list, NULL);

RCF_PCH_CFG_NODE_RW(node_ps_exe, "exe", NULL, &node_ps_arg,
                    ps_exe_get, ps_exe_set);

RCF_PCH_CFG_NODE_RW(node_ps_status, "status", NULL, &node_ps_exe,
                    ps_status_get, ps_status_set);

RCF_PCH_CFG_NODE_COLLECTION(node_ps, "process", &node_ps_status, NULL,
                            ps_add, ps_del, ps_list, NULL);

te_errno
ta_unix_conf_ps_init(void)
{
    return rcf_pch_add_node("/agent", &node_ps);
}
