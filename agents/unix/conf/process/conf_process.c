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
#include <signal.h>

#include "ta_job.h"
#include "logger_api.h"
#include "te_defs.h"
#include "te_queue.h"
#include "te_alloc.h"
#include "te_string.h"
#include "te_shell_cmd.h"
#include "te_str.h"
#include "te_sigmap.h"
#include "rcf_pch.h"
#include "agentlib.h"
#include "conf_common.h"
#include "conf_process.h"

/*
 * This value is taken from ta_kill_death() which was previously used
 * in ps_stop()
 */
/** Default timeout of process graceful termination */
#define PS_TERM_TIMEOUT_MS 1000

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

struct ps_ta_job {
    unsigned int  id;
    te_bool       created;
    /*
     * TA job subsystem is used to control actual processes on TA.
     * Everytime we change parameters of a process (e.g. arguments, exe, etc.),
     * new TA job should be created instead of the old one because TA jobs
     * parameters cannot be changed.
     * This flag is used to check whether or not TA job should be renewed.
     */
    te_bool       reconfigure_required;
};

struct ps_entry {
    SLIST_ENTRY(ps_entry)   links;
    te_bool                 enabled;
    char                   *name;
    char                   *exe;
    ps_arg_list_t           args;
    unsigned int            argc;
    ps_env_list_t           envs;
    ps_opt_list_t           opts;
    te_bool                 long_opt_sep;
    unsigned int            autorestart;
    unsigned int            time_until_check;
    te_bool                 autorestart_failed;
    struct ps_ta_job        ta_job;
};

static SLIST_HEAD(, ps_entry) processes = SLIST_HEAD_INITIALIZER(processes);

static pthread_t autorestart_thread;
/**
 * Mutex required for correct work of autorestart subsystem.
 * It protects a status of each process and the processes list.
 */
static pthread_mutex_t autorestart_lock = PTHREAD_MUTEX_INITIALIZER;

static ta_job_manager_t *manager = NULL;

static int
ps_arg_compare(const void *a, const void *b)
{
    const struct ps_arg_entry **arg_a = (const struct ps_arg_entry **)a;
    const struct ps_arg_entry **arg_b = (const struct ps_arg_entry **)b;

    return (*arg_a)->order - (*arg_b)->order;
}

/**
 * Whether the option consists of one or two arguments (i.e. there should be
 * space between option name and value).
 */
static te_bool
ps_opt_is_one_arg(struct ps_entry *ps, struct ps_opt_entry *opt)
{
    return (opt->value[0] == '\0') || (opt->is_long && ps->long_opt_sep);
}

static void
ps_free_argv(char **argv)
{
    te_str_free_array(argv);
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
        len += ps_opt_is_one_arg(ps, opt) ? 1 : 2;
    }

    tmp = TE_ALLOC((ps->argc + len + 2) * sizeof(char *));
    if (tmp == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    tmp[0] = strdup(ps->exe);
    if (tmp[0] == NULL)
    {
        free(tmp);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    i = 1;
    SLIST_FOREACH(opt, &ps->opts, links)
    {
        if (ps_opt_is_one_arg(ps, opt))
        {
            if (opt->value[0] == '\0')
            {
                rc = te_asprintf(&tmp[i++], "-%s%s",
                                 opt->is_long ? "-" : "", opt->name);
            }
            else
            {
                rc = te_asprintf(&tmp[i++], "--%s=%s", opt->name, opt->value);
            }
        }
        else
        {
            rc = te_asprintf(&tmp[i++], "-%s%s",
                             opt->is_long ? "-" : "", opt->name);
            if (rc > 0)
                rc = te_asprintf(&tmp[i++], "%s", opt->value);
        }

        if (rc < 0)
        {
            rc = errno;
            ps_free_argv(tmp);
            return TE_OS_RC(TE_TA_UNIX, rc);
        }
    }

    args = TE_ALLOC(ps->argc * sizeof(*args));
    if (args == NULL)
    {
        ps_free_argv(tmp);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    i = 0;
    SLIST_FOREACH(arg, &ps->args, links)
    {
        args[i] = arg;
        i++;
    }

    qsort(args, ps->argc, sizeof(*args), ps_arg_compare);

    for (i = 1; i < ps->argc + 1; i++)
    {
        tmp[i + len] = strdup(args[i - 1]->value);
        if (tmp[i + len] == NULL)
        {
            ps_free_argv(tmp);
            free(args);
            return TE_RC(TE_TA_UNIX, TE_ENOMEM);
        }
    }

    tmp[ps->argc + len + 1] = NULL;

    *argv = tmp;

    free(args);

    return 0;
}

static void
ps_free_envp(char **envp)
{
    te_str_free_array(envp);
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
generate_filter_names(struct ps_entry *ps, char **stdout_filter_name,
                      char **stderr_filter_name)
{
    char *stdout_name;
    char *stderr_name;

    stdout_name = te_string_fmt("%s stdout", ps->name);
    if (stdout_name == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    stderr_name = te_string_fmt("%s stderr", ps->name);
    if (stderr_name == NULL)
    {
        free(stdout_name);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    *stdout_filter_name = stdout_name;
    *stderr_filter_name = stderr_name;

    return 0;
}

static te_errno
ps_enable_stdout_and_stderr_logging(struct ps_entry *ps)
{
    te_errno rc;
    struct ps_ta_job *ta_job = &ps->ta_job;
    unsigned int channel_ids[2]; /* stdout and stderr */
    char *stdout_filter_name = NULL;
    char *stderr_filter_name = NULL;
    unsigned int stdout_filter_id;

    rc = ta_job_allocate_channels(manager, ta_job->id, FALSE, 2, channel_ids);
    if (rc != 0)
    {
        ERROR("Failed to allocate output channels for TA job corresponding "
              "to process '%s', error: %r", ps->name, rc);
        return rc;
    }

    rc = generate_filter_names(ps, &stdout_filter_name, &stderr_filter_name);
    if (rc != 0)
    {
        ERROR("Failed to generate names for filters to be attached to TA job "
              "corresponding to process '%s', error: %r", ps->name, rc);
        goto out;
    }

    /* Attach stdout filter */
    rc = ta_job_attach_filter(manager, stdout_filter_name, 1, &channel_ids[0],
                              FALSE, TE_LL_RING, &stdout_filter_id);
    if (rc != 0)
    {
        ERROR("Failed to attach stdout filter for TA job corresponding to "
              "process '%s', error: %r", ps->name, rc);
        goto out;
    }

    /* Attach stderr filter */
    rc = ta_job_attach_filter(manager, stderr_filter_name, 1, &channel_ids[1],
                              FALSE, TE_LL_WARN, NULL);
    if (rc != 0)
    {
        ERROR("Failed to attach stderr filter for TA job corresponding to "
              "process '%s', error: %r", ps->name, rc);

        (void)ta_job_filter_remove_channels(manager, stdout_filter_id, 1,
                                            &channel_ids[0]);
        goto out;
    }

out:
    if (rc != 0)
        ta_job_deallocate_channels(manager, 2, channel_ids);

    free(stdout_filter_name);
    free(stderr_filter_name);

    return rc;
}

static te_errno
ps_ta_job_reconfigure(struct ps_entry *ps)
{
    te_errno rc;
    char **argv = NULL;
    char **envp = NULL;
    struct ps_ta_job *ta_job = &ps->ta_job;

    if (ta_job->created)
    {
        rc = ta_job_destroy(manager, ta_job->id, -1);
        if (rc != 0)
        {
            ERROR("Failed to destroy TA job corresponding to process '%s', "
                  "error: %r", ps->name, rc);
            return rc;
        }

        ta_job->created = FALSE;
    }

    rc = ps_get_argv(ps, &argv);
    if (rc != 0)
        return rc;

    rc = ps_get_envp(ps, &envp);
    if (rc != 0)
    {
        ps_free_argv(argv);
        return rc;
    }

    rc = ta_job_create(manager, NULL, ps->exe, argv, envp, &ta_job->id);
    if (rc != 0)
    {
        te_str_free_array(argv);
        te_str_free_array(envp);

        ERROR("Failed to create TA job corresponding to the process '%s', "
              "error: %r", ps->name, rc);

        return rc;
    }

    ta_job->created = TRUE;

    return ps_enable_stdout_and_stderr_logging(ps);
}

static te_errno
ps_start(struct ps_entry *ps)
{
    te_errno rc;

    if (ps->ta_job.reconfigure_required)
    {
        rc = ps_ta_job_reconfigure(ps);
        if (rc != 0)
            return rc;

        ps->ta_job.reconfigure_required = FALSE;
    }

    rc = ta_job_start(manager, ps->ta_job.id);
    if (rc != 0)
    {
        ERROR("Failed to start TA job corresponding to the process '%s', "
              "error: %r", ps->name, rc);
    }

    return rc;
}

static te_errno
ps_stop(struct ps_entry *ps)
{
    return ta_job_stop(manager, ps->ta_job.id, SIGTERM, PS_TERM_TIMEOUT_MS);
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

static void
ps_ta_job_init(struct ps_ta_job *ta_job)
{
    ta_job->created = FALSE;
    /*
     * We should create a new TA job for the process because for now
     * it does not exist
     */
    ta_job->reconfigure_required = TRUE;
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

    ps->enabled = FALSE;
    ps->argc = 0;
    ps->long_opt_sep = FALSE;
    ps->autorestart = 0;
    ps->time_until_check = 0;
    ps->autorestart_failed = FALSE;

    ps_ta_job_init(&ps->ta_job);

    SLIST_INIT(&ps->args);
    SLIST_INIT(&ps->envs);
    SLIST_INIT(&ps->opts);

    pthread_mutex_lock(&autorestart_lock);
    SLIST_INSERT_HEAD(&processes, ps, links);
    pthread_mutex_unlock(&autorestart_lock);

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

    if (ps->enabled)
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    if (strcmp(value, "=") == 0)
        ps->long_opt_sep = TRUE;
    else if (value[0] == '\0')
        ps->long_opt_sep = FALSE;
    else
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    ps->ta_job.reconfigure_required = TRUE;

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
    te_errno rc = 0;

    UNUSED(gid);
    UNUSED(oid);

    ENTRY("%s", ps_name);

    ps = ps_find(ps_name);
    if (ps == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    pthread_mutex_lock(&autorestart_lock);

    if (ps->ta_job.created)
    {
        rc = ta_job_destroy(manager, ps->ta_job.id, -1);
        if (rc != 0)
        {
            ERROR("Failed to destroy TA job corresponding to process '%s', "
                  "error: %r", ps->name, rc);
        }

        ps->ta_job.created = FALSE;
    }

    SLIST_REMOVE(&processes, ps, ps_entry, links);

    pthread_mutex_unlock(&autorestart_lock);

    ps_free(ps);

    return rc;
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
    te_errno rc;
    struct ps_entry *ps;

    UNUSED(gid);
    UNUSED(oid);

    ENTRY("%s", ps_name);

    ps = ps_find(ps_name);
    if (ps == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (ps->enabled)
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    rc = string_replace(&ps->exe, value);
    if (rc == 0)
        ps->ta_job.reconfigure_required = TRUE;

    return rc;
}

static te_errno
ps_status_check(struct ps_entry *ps, te_bool *is_running)
{
    te_errno rc = ta_job_wait(manager, ps->ta_job.id, 0, NULL);

    switch (rc)
    {
        case 0:
        case TE_ECHILD:
            *is_running = FALSE;
            rc = 0;
            break;
        case TE_EINPROGRESS:
            *is_running = TRUE;
            rc = 0;
            break;
        default:
            WARN("Failed to check if TA job corresponding to process '%s' "
                 "is running, ta_job_wait() exited with error %r.\n"
                 "Considering that the job is not running.",
                 ps->name, rc);
            *is_running = FALSE;
            break;
    }

    return rc;
}

static te_errno
ps_status_get(unsigned int gid, const char *oid, char *value,
              const char *ps_name)
{
    te_errno rc = 0;
    struct ps_entry *ps;

    UNUSED(gid);
    UNUSED(oid);

    ENTRY("%s", ps_name);

    ps = ps_find(ps_name);
    if (ps == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    pthread_mutex_lock(&autorestart_lock);

    if (ps->autorestart == 0 && ps->enabled)
        rc = ps_status_check(ps, &ps->enabled);
    else if (ps->autorestart_failed)
        ps->enabled = FALSE;

    pthread_mutex_unlock(&autorestart_lock);

    snprintf(value, RCF_MAX_VAL, "%d", ps->enabled);

    return rc;
}

static te_errno
ps_status_set(unsigned int gid, const char *oid, const char *value,
              const char *ps_name)
{
    struct ps_entry *ps;
    te_bool enable = !!atoi(value);
    te_errno rc = 0;

    UNUSED(gid);
    UNUSED(oid);

    ENTRY("%s", ps_name);

    ps = ps_find(ps_name);
    if (ps == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (enable == ps->enabled)
    {
        ERROR("Trying to set status value which is already set");
        return TE_RC(TE_TA_UNIX, TE_EALREADY);
    }

    pthread_mutex_lock(&autorestart_lock);

    if (enable)
    {
        rc = ps_start(ps);
        if (rc == 0 && ps->autorestart != 0)
        {
            ps->autorestart_failed = FALSE;
            ps->time_until_check = ps->autorestart;
        }
    }
    else
    {
        rc = ps_stop(ps);
    }

    /*
     * If we failed to stop an autorestart process, we should
     * set ps->enable to false anyway for autorestart subsystem
     * not to try restarting the process over and over.
     */
    if (rc == 0 || (!enable && ps->autorestart != 0))
        ps->enabled = enable;

    pthread_mutex_unlock(&autorestart_lock);

    return rc;
}

static te_errno
ps_autorestart_get(unsigned int gid, const char *oid, char *value,
                   const char *ps_name)
{
    struct ps_entry *ps;

    UNUSED(gid);
    UNUSED(oid);

    ENTRY("%s", ps_name);

    ps = ps_find(ps_name);
    if (ps == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(value, RCF_MAX_VAL, "%d", ps->autorestart);

    return 0;
}

static te_errno
ps_autorestart_set(unsigned int gid, const char *oid, const char *value,
                   const char *ps_name)
{
    struct ps_entry *ps;

    UNUSED(gid);
    UNUSED(oid);

    ENTRY("%s", ps_name);

    ps = ps_find(ps_name);
    if (ps == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (ps->enabled)
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    return te_strtoui(value, 10, &ps->autorestart);
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

    if (ps->enabled)
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

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
    ps->ta_job.reconfigure_required = TRUE;

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

    if (ps->enabled)
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    rc = te_strtoui(arg_name, 10, &order);
    if (rc != 0)
        return rc;

    arg = ps_arg_find(ps, order);
    if (arg == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_REMOVE(&ps->args, arg, ps_arg_entry, links);
    ps->argc--;
    ps->ta_job.reconfigure_required = TRUE;

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

    if (ps->enabled)
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

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
    ps->ta_job.reconfigure_required = TRUE;

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

    if (ps->enabled)
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    env = ps_env_find(ps, env_name);
    if (env == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_REMOVE(&ps->envs, env, ps_env_entry, links);
    ps->ta_job.reconfigure_required = TRUE;

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

    if (ps->enabled)
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

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
    ps->ta_job.reconfigure_required = TRUE;

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

    if (ps->enabled)
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    opt = ps_opt_find(ps, opt_name);
    if (opt == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_REMOVE(&ps->opts, opt, ps_opt_entry, links);
    ps->ta_job.reconfigure_required = TRUE;

    ps_opt_free(opt);

    return 0;
}

/**
 * Sleep for (approximately) the specified number of seconds ignoring signals.
 *
 * If sleep is interrupted by signal, it returns the number of seconds left
 * to sleep. This way the precision is lost. nanosleep, however, returns
 * the time left to sleep in case of receiving a signal with higher precision,
 * so it can be called again with that time.
 */
static te_errno
signal_protected_sleep(unsigned int seconds)
{
    struct timespec req = { .tv_sec = seconds };
    int ret;

    do {
        ret = nanosleep(&req, &req);
    } while (ret == -1 && errno == EINTR);

    return ret == 0 ? 0 : TE_OS_RC(TE_TA_UNIX, errno);
}

/**
 * Wake up once per second, check for each process if it has to be (re)started,
 * and (re)start it if needed.
 */
static void *
autorestart_loop(void *arg)
{
    struct ps_entry *ps;
    te_errno rc;

    UNUSED(arg);

    while (1)
    {
        /*
         * Simple sleep should not be used here since a restarted process may
         * terminate and send SIGCHLD during the sleep and intrrupt it.
         */
        rc = signal_protected_sleep(1);
        if (rc != 0)
        {
            WARN("nanosleep failed (%r), autorestart timings may be shifted",
                 rc);
        }

        pthread_mutex_lock(&autorestart_lock);

        SLIST_FOREACH(ps, &processes, links)
        {
            /* It's time to check whether we should restart the process */
            if (ps->enabled && ps->autorestart != 0 &&
                !ps->autorestart_failed && --ps->time_until_check == 0)
            {
                te_bool is_running;

                /* Try to restart the process even if the check failed */
                ps_status_check(ps, &is_running);
                if (!is_running)
                {
                    rc = ps_start(ps);
                    if (rc != 0)
                    {
                        ps->autorestart_failed = TRUE;
                        ERROR("Failed to (re)start process '%s', error: %r",
                              ps->name, rc);
                    }
                    else
                    {
                        INFO("Process '%s' has been (re)started", ps->name);
                    }
                }

                ps->time_until_check = ps->autorestart;
            }
        }

        pthread_mutex_unlock(&autorestart_lock);
    }

    return NULL;
}

static te_errno
ps_kill_common_get(unsigned int gid, const char *oid, char *value,
                   const char *ps_name)
{
    struct ps_entry *ps;

    UNUSED(gid);
    UNUSED(oid);

    ENTRY("%s", ps_name);

    ps = ps_find(ps_name);
    if (ps == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    /* We do not care about the value we get since it does not mean anything */
    value[0] = '\0';

    return 0;
}

static te_errno
ps_kill_common_set(unsigned int gid, const char *oid, const char *value,
                   const char *ps_name, te_bool killpg)
{
    struct ps_entry *ps;
    int signo;

    UNUSED(gid);
    UNUSED(oid);

    ENTRY("%s", ps_name);

    ps = ps_find(ps_name);
    if (ps == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    signo = map_name_to_signo(value);
    if (signo == -1)
    {
        ERROR("Failed to send signal to %sprocess '%s', invalid signal "
              "name '%s'", (killpg ? "group of " : ""), ps->name, value);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (killpg)
        return ta_job_killpg(manager, ps->ta_job.id, signo);
    else
        return ta_job_kill(manager, ps->ta_job.id, signo);
}

static te_errno
ps_kill_self_set(unsigned int gid, const char *oid, const char *value,
                 const char *ps_name)
{
    return ps_kill_common_set(gid, oid, value, ps_name, FALSE);
}

static te_errno
ps_kill_group_set(unsigned int gid, const char *oid, const char *value,
                  const char *ps_name)
{
    return ps_kill_common_set(gid, oid, value, ps_name, TRUE);
}

static te_errno
subst_process(te_string *value, const char *subst,
              const char *replaced_value)
{
    te_errno rc = 0;
    te_substring_t iter = TE_SUBSTRING_INIT(value);

    if (strcmp_start(replaced_value, value->ptr) == 0)
    {
        iter.len = strlen(replaced_value);
        rc = te_substring_replace(&iter, subst);
    }

    if (rc != 0)
        ERROR("Failed to make a substitution in '%s': %r", value, rc);

    return rc;
}

static const rcf_pch_cfg_substitution subst[] = RCF_PCH_CFG_SUBST_SET(
    { "*", "/agent/dir", subst_process }
);

RCF_PCH_CFG_NODE_RW_COLLECTION_WITH_SUBST(node_ps_arg, "arg", NULL, NULL,
                                          ps_arg_get, NULL, ps_arg_add,
                                          ps_arg_del, ps_arg_list,
                                          NULL, subst);

RCF_PCH_CFG_NODE_RW_COLLECTION(node_ps_env, "env", NULL, &node_ps_arg,
                               ps_env_get, NULL, ps_env_add,
                               ps_env_del, ps_env_list, NULL);

RCF_PCH_CFG_NODE_RW_COLLECTION(node_ps_opt, "option", NULL, &node_ps_env,
                               ps_opt_get, NULL, ps_opt_add,
                               ps_opt_del, ps_opt_list, NULL);

RCF_PCH_CFG_NODE_RW_WITH_SUBST(node_ps_exe, "exe", NULL, &node_ps_opt,
                               ps_exe_get, ps_exe_set, subst);

RCF_PCH_CFG_NODE_RW(node_ps_status, "status", NULL, &node_ps_exe,
                    ps_status_get, ps_status_set);

RCF_PCH_CFG_NODE_RW(node_ps_long_opt_sep, "long_option_value_separator", NULL,
                    &node_ps_status, ps_long_opt_sep_get, ps_long_opt_sep_set);

RCF_PCH_CFG_NODE_RW(node_ps_autorestart, "autorestart", NULL,
                    &node_ps_long_opt_sep, ps_autorestart_get,
                    ps_autorestart_set);

RCF_PCH_CFG_NODE_RW(node_ps_kill_self, "self", NULL, NULL,
                    ps_kill_common_get, ps_kill_self_set);

RCF_PCH_CFG_NODE_RW(node_ps_kill_group, "group", NULL, &node_ps_kill_self,
                    ps_kill_common_get, ps_kill_group_set);

RCF_PCH_CFG_NODE_NA(node_ps_kill, "kill", &node_ps_kill_group,
                    &node_ps_autorestart);

RCF_PCH_CFG_NODE_COLLECTION(node_ps, "process", &node_ps_kill, NULL,
                            ps_add, ps_del, ps_list, NULL);

te_errno
ta_unix_conf_ps_init(void)
{
    te_errno rc;
    int ret;

    rc = ta_job_manager_init(&manager);
    if (rc != 0)
    {
        ERROR("Failed to initialize TA job manager, error: %r", rc);
        return rc;
    }

    ret = pthread_create(&autorestart_thread, NULL, autorestart_loop, NULL);
    if (ret != 0)
        return TE_OS_RC(TE_TA_UNIX, ret);

    return rcf_pch_add_node("/agent", &node_ps);
}
