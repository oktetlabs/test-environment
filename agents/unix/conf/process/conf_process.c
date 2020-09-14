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

extern char **environ;

struct ps_arg_entry {
    SLIST_ENTRY(ps_arg_entry)     links;
    char                         *value;
    unsigned int                  order;
};

struct ps_env_entry {
    SLIST_ENTRY(ps_env_entry)     links;
    char                         *name;
    char                         *value;
};

struct ps_opt_entry {
    SLIST_ENTRY(ps_opt_entry)     links;
    char                         *name;
    char                         *value;
    te_bool                       is_long;
};

typedef SLIST_HEAD(ps_arg_list_t, ps_arg_entry) ps_arg_list_t;
typedef SLIST_HEAD(ps_env_list_t, ps_env_entry) ps_env_list_t;
typedef SLIST_HEAD(ps_opt_list_t, ps_opt_entry) ps_opt_list_t;

struct ps_entry {
    SLIST_ENTRY(ps_entry)   links;
    char                   *name;
    char                   *exe;
    ps_arg_list_t           args;
    unsigned int            argc;
    ps_env_list_t           envs;
    ps_opt_list_t           opts;
    te_bool                 long_opt_sep;
    pid_t                   id;
};

static SLIST_HEAD(, ps_entry) processes = SLIST_HEAD_INITIALIZER(processes);

static te_bool
ps_is_running(struct ps_entry *ps)
{
    pid_t ret;

    if (ps->id == -1)
        return FALSE;

    do {
        ret = ta_waitpid(ps->id, NULL, WNOHANG);
    } while (ret == -1 && errno == EINTR);

    if (ret != 0)
    {
        ps->id = -1;
        return FALSE;
    }

    return TRUE;
}

static int
ps_arg_compare(const void *a, const void *b)
{
    const struct ps_arg_entry **arg_a = (const struct ps_arg_entry **)a;
    const struct ps_arg_entry **arg_b = (const struct ps_arg_entry **)b;

    return (*arg_a)->order - (*arg_b)->order;
}

static void
ps_free_argv(struct ps_entry *ps, char **argv)
{
    struct ps_opt_entry *opt;
    unsigned int i = 1;


    SLIST_FOREACH(opt, &ps->opts, links)
    {
        free(argv[i]);
        /*
         * Memory in argv is allocated for option names and only
         * for such values that go with a long option after '='.
         * So skip argv[j] assigned to some opt->value.
        */
        i += ((opt->value[0] == '\0') ||
              (ps->long_opt_sep && opt->is_long)) ? 1 : 2;
    }

    free(argv);
}

static te_errno
ps_get_argv(struct ps_entry *ps, char ***argv)
{
    char **tmp;
    struct ps_arg_entry *arg;
    struct ps_arg_entry **args;
    struct ps_opt_entry *opt;
    te_errno rc;
    unsigned int i;
    unsigned int len = 0;

    SLIST_FOREACH(opt, &ps->opts, links)
    {
        len += ((opt->value[0] == '\0') ||
                (ps->long_opt_sep && opt->is_long)) ? 1 : 2;
    }

    tmp = TE_ALLOC((ps->argc + len + 2) * sizeof(char *));
    if (tmp == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    i = 1;
    SLIST_FOREACH(opt, &ps->opts, links)
    {
        rc = te_asprintf(&tmp[i], "-%s%s%s%s",
                         opt->is_long ? "-" : "",
                         opt->name,
                         ps->long_opt_sep && opt->is_long &&
                         opt->value[0] != '\0' ?
                         "=" : "",
                         opt->is_long && opt->value[0] != '\0' ?
                         opt->value : "");
        if (rc < 0)
        {
            rc = errno;
            ps_free_argv(ps, tmp);
            return TE_OS_RC(TE_TA_UNIX, rc);
        }
        if (!opt->is_long && opt->value[0] != '\0')
        {
            i++;
            tmp[i] = opt->value;
        }

        i++;
    }

    args = TE_ALLOC(ps->argc * sizeof(*args));
    if (args == NULL)
    {
        ps_free_argv(ps, tmp);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    i = 0;
    SLIST_FOREACH(arg, &ps->args, links)
    {
        args[i] = arg;
        i++;
    }

    qsort(args, ps->argc, sizeof(*args), ps_arg_compare);

    tmp[0] = ps->exe;
    tmp[ps->argc + len + 1] = NULL;
    for (i = 1; i < ps->argc + 1; i++)
        tmp[i + len] = args[i - 1]->value;

    *argv = tmp;

    free(args);

    return 0;
}

static void
ps_free_envp(char **envp)
{
    char **env;

    for (env = envp; *env != NULL; env++)
        free(*env);

    free(envp);
}

static te_errno
ps_get_envp(struct ps_entry *ps, char ***envp)
{
    char **tmp;
    struct ps_env_entry *env;
    unsigned int len = 0;
    unsigned int env_len;
    unsigned int name_len;
    unsigned int i;
    te_bool found;
    char *sub;

    for (env_len = 0; environ[env_len] != NULL; env_len++)
        ;

    SLIST_FOREACH(env, &ps->envs, links)
        len++;

    tmp = TE_ALLOC((env_len + len + 1) * sizeof(char *));
    if (tmp == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    for (len = 0; len < env_len; len++)
    {
        /*
         * When envp must be freed it is impossible to know if envp[i]
         * is environ pointer or a product of te_asprintf().
         * So do copy for safe free().
        */
        tmp[len] = strdup(environ[len]);
        if (tmp[len] == NULL)
        {
            ps_free_envp(tmp);
            return TE_RC(TE_TA_UNIX, TE_ENOMEM);
        }
    }

    SLIST_FOREACH(env, &ps->envs, links)
    {
        name_len = strlen(env->name);
        found = FALSE;
        for (i = 0; i < env_len; i++)
        {
            sub = strchr(tmp[i], '=');
            if (((sub - tmp[i]) == name_len) &&
                (memcmp(tmp[i], env->name, name_len) == 0))
            {
                free(tmp[i]);
                tmp[i] = NULL;
                found = TRUE;
                break;
            }
        }

        if (te_asprintf(&tmp[found ? i : len], "%s=%s",
            env->name, env->value) < 0)
        {
            ps_free_envp(tmp);
            return TE_RC(TE_TA_UNIX, TE_EFAIL);
        }

        len = found ? len : len + 1;
    }

    tmp[len] = NULL;

    *envp = tmp;

    return 0;
}

static te_errno
ps_start(struct ps_entry *ps)
{
    char **argv = NULL;
    char **envp = NULL;
    te_errno rc;

    rc = ps_get_argv(ps, &argv);
    if (rc != 0)
        return rc;

    rc = ps_get_envp(ps, &envp);
    if (rc != 0)
        return rc;

    ps->id = te_exec_child(ps->exe, argv, envp,
                             (uid_t)-1, NULL, NULL, NULL, NULL);

    ps_free_argv(ps, argv);
    ps_free_envp(envp);

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
    ps->long_opt_sep = FALSE;

    SLIST_INIT(&ps->args);
    SLIST_INIT(&ps->envs);
    SLIST_INIT(&ps->opts);

    SLIST_INSERT_HEAD(&processes, ps, links);

    return 0;
}

static te_errno
ps_long_opt_sep_get(unsigned int gid, const char *oid, char *value,
                    const char *ps_name)
{
    struct ps_entry *ps;

    UNUSED(gid);
    UNUSED(oid);

    ENTRY("%s", ps_name);

    ps = ps_find(ps_name);
    if (ps == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(value, RCF_MAX_VAL, "%s", ps->long_opt_sep ? "=" : "");

    return 0;
}

static te_errno
ps_long_opt_sep_set(unsigned int gid, const char *oid, const char *value,
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

    if (strcmp(value, "=") == 0)
        ps->long_opt_sep = TRUE;
    else if (value[0] == '\0')
        ps->long_opt_sep = FALSE;
    else
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    return 0;
}

static void
ps_arg_free(struct ps_arg_entry *arg)
{
    free(arg->value);
    free(arg);
}

static void
ps_env_free(struct ps_env_entry *env)
{
    free(env->name);
    free(env->value);
    free(env);
}

static void
ps_opt_free(struct ps_opt_entry *opt)
{
    free(opt->name);
    free(opt->value);
    free(opt);
}

static void
ps_free(struct ps_entry *ps)
{
    struct ps_arg_entry *arg;
    struct ps_arg_entry *arg_tmp;
    struct ps_env_entry *env;
    struct ps_env_entry *env_tmp;
    struct ps_opt_entry *opt;
    struct ps_opt_entry *opt_tmp;

    SLIST_FOREACH_SAFE(arg, &ps->args, links, arg_tmp)
    {
        SLIST_REMOVE(&ps->args, arg, ps_arg_entry, links);
        ps_arg_free(arg);
    }

    SLIST_FOREACH_SAFE(env, &ps->envs, links, env_tmp)
    {
        SLIST_REMOVE(&ps->envs, env, ps_env_entry, links);
        ps_env_free(env);
    }

    SLIST_FOREACH_SAFE(opt, &ps->opts, links, opt_tmp)
    {
        SLIST_REMOVE(&ps->opts, opt, ps_opt_entry, links);
        ps_opt_free(opt);
    }

    free(ps->name);
    free(ps->exe);
    free(ps);
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

    ps_free(ps);

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

    snprintf(value, RCF_MAX_VAL, "%s", te_str_empty_if_null(ps->exe));

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

static struct ps_env_entry *
ps_env_find(const struct ps_entry *ps, const char *env_name)
{
    struct ps_env_entry *p;

    if (ps == NULL)
        return NULL;

    SLIST_FOREACH(p, &ps->envs, links)
    {
        if (strcmp(env_name, p->name) == 0)
            return p;
    }

    return NULL;
}

static te_errno
ps_env_list(unsigned int gid, const char *oid, const char *sub_id, char **list,
            const char *ps_name)
{
    te_string result = TE_STRING_INIT;
    struct ps_entry *ps;
    struct ps_env_entry *env;
    te_bool first = TRUE;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    if ((ps = ps_find(ps_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_FOREACH(env, &ps->envs, links)
    {
        rc = te_string_append(&result, "%s%s", first ? "" : " ", env->name);
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
ps_env_get(unsigned int gid, const char *oid, char *value,
           const char *ps_name, const char *env_name)
{
    struct ps_entry *ps;
    struct ps_env_entry *env = NULL;

    UNUSED(gid);
    UNUSED(oid);

    if ((ps = ps_find(ps_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    env = ps_env_find(ps, env_name);
    if (env == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(value, RCF_MAX_VAL, "%s", env->value);

    return 0;
}

static te_errno
ps_env_add(unsigned int gid, const char *oid, const char *value,
           const char *ps_name, const char *env_name)
{
    struct ps_entry *ps;
    struct ps_env_entry *env;

    UNUSED(gid);
    UNUSED(oid);

    if ((ps = ps_find(ps_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (ps_env_find(ps, env_name) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    if (ps_is_running(ps))
        return TE_RC(TE_TA_UNIX, TE_ETXTBSY);

    env = TE_ALLOC(sizeof(*env));
    if (env == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    env->name = strdup(env_name);
    if (env->name == NULL)
    {
        ps_env_free(env);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    env->value = strdup(value);
    if (env->value == NULL)
    {
        ps_env_free(env);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    SLIST_INSERT_HEAD(&ps->envs, env, links);

    return 0;
}

static te_errno
ps_env_del(unsigned int gid, const char *oid,
           const char *ps_name, const char *env_name)
{
    struct ps_entry *ps;
    struct ps_env_entry *env;

    UNUSED(gid);
    UNUSED(oid);

    if ((ps = ps_find(ps_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (ps_is_running(ps))
        return TE_RC(TE_TA_UNIX, ETXTBSY);

    env = ps_env_find(ps, env_name);
    if (env == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_REMOVE(&ps->envs, env, ps_env_entry, links);

    ps_env_free(env);

    return 0;
}

static struct ps_opt_entry *
ps_opt_find(const struct ps_entry *ps, const char *opt_name)
{
    struct ps_opt_entry *p;

    if (ps == NULL)
        return NULL;

    SLIST_FOREACH(p, &ps->opts, links)
    {
        if (strcmp(opt_name, p->name) == 0)
            return p;
    }

    return NULL;
}

static te_errno
ps_opt_list(unsigned int gid, const char *oid, const char *sub_id,
            char **list, const char *ps_name)
{
    te_string result = TE_STRING_INIT;
    struct ps_entry *ps;
    struct ps_opt_entry *opt;
    te_bool first = TRUE;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    if ((ps = ps_find(ps_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_FOREACH(opt, &ps->opts, links)
    {
        rc = te_string_append(&result, "%s%s", first ? "" : " ", opt->name);
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
ps_opt_get(unsigned int gid, const char *oid, char *value,
           const char *ps_name, const char *opt_name)
{
    struct ps_entry *ps;
    struct ps_opt_entry *opt = NULL;

    UNUSED(gid);
    UNUSED(oid);

    if ((ps = ps_find(ps_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    opt = ps_opt_find(ps, opt_name);
    if (opt == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(value, RCF_MAX_VAL, "%s", opt->value);

    return 0;
}

static te_errno
ps_opt_add(unsigned int gid, const char *oid, const char *value,
           const char *ps_name, const char *opt_name)
{
    struct ps_entry *ps;
    struct ps_opt_entry *opt;

    UNUSED(gid);
    UNUSED(oid);

    if ((ps = ps_find(ps_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (ps_opt_find(ps, opt_name) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    if (ps_is_running(ps))
        return TE_RC(TE_TA_UNIX, TE_ETXTBSY);

    opt = TE_ALLOC(sizeof(*opt));
    if (opt == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    opt->name = strdup(opt_name);
    if (opt->name == NULL)
    {
        ps_opt_free(opt);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    opt->value = strdup(value);
    if (opt->value == NULL)
    {
        ps_opt_free(opt);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    opt->is_long = (strlen(opt->name) > 1) ? TRUE : FALSE;

    SLIST_INSERT_HEAD(&ps->opts, opt, links);

    return 0;
}

static te_errno
ps_opt_del(unsigned int gid, const char *oid,
           const char *ps_name, const char *opt_name)
{
    struct ps_entry *ps;
    struct ps_opt_entry *opt;

    UNUSED(gid);
    UNUSED(oid);

    if ((ps = ps_find(ps_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (ps_is_running(ps))
        return TE_RC(TE_TA_UNIX, ETXTBSY);

    opt = ps_opt_find(ps, opt_name);
    if (opt == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_REMOVE(&ps->opts, opt, ps_opt_entry, links);

    ps_opt_free(opt);

    return 0;
}

RCF_PCH_CFG_NODE_RW_COLLECTION(node_ps_arg, "arg", NULL, NULL,
                               ps_arg_get, NULL, ps_arg_add,
                               ps_arg_del, ps_arg_list, NULL);

RCF_PCH_CFG_NODE_RW_COLLECTION(node_ps_env, "env", NULL, &node_ps_arg,
                               ps_env_get, NULL, ps_env_add,
                               ps_env_del, ps_env_list, NULL);

RCF_PCH_CFG_NODE_RW_COLLECTION(node_ps_opt, "option", NULL, &node_ps_env,
                               ps_opt_get, NULL, ps_opt_add,
                               ps_opt_del, ps_opt_list, NULL);

RCF_PCH_CFG_NODE_RW(node_ps_exe, "exe", NULL, &node_ps_opt,
                    ps_exe_get, ps_exe_set);

RCF_PCH_CFG_NODE_RW(node_ps_status, "status", NULL, &node_ps_exe,
                    ps_status_get, ps_status_set);

RCF_PCH_CFG_NODE_RW(node_ps_long_opt_sep, "long_option_value_separator", NULL,
                    &node_ps_status, ps_long_opt_sep_get, ps_long_opt_sep_set);

RCF_PCH_CFG_NODE_COLLECTION(node_ps, "process", &node_ps_long_opt_sep, NULL,
                            ps_add, ps_del, ps_list, NULL);

te_errno
ta_unix_conf_ps_init(void)
{
    return rcf_pch_add_node("/agent", &node_ps);
}
