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

struct ps_entry {
    SLIST_ENTRY(ps_entry)   links;
    char                   *name;
    char                   *exe;
    pid_t                   id;
};

static SLIST_HEAD(, ps_entry) processes = SLIST_HEAD_INITIALIZER(processes);

static te_bool
ps_is_running(struct ps_entry *ps)
{
    return ps->id < 0 ? FALSE : (ta_waitpid(ps->id, NULL, WNOHANG) == 0);
}

static te_errno
ps_start(struct ps_entry *ps)
{
    ps->id = te_exec_child(ps->exe, (char *const[]){ps->exe, NULL}, NULL,
                             (uid_t)-1, NULL, NULL, NULL);

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

RCF_PCH_CFG_NODE_RW(node_ps_exe, "exe", NULL, NULL,
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
