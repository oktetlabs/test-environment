/** @file
 * @brief Generic Test API for packetdrill test tool
 *
 * Generic API to use packetdrill test tool.
 *
 * Copyright (C) 2018-2022 OKTET Labs. All rights reserved.
 *
 */
#define TE_LGR_USER     "TAPI packetdrill"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "tapi_rpc_stdio.h"
#include "tapi_rpc_signal.h"
#include "tapi_rpc_unistd.h"
#include "tapi_test_log.h"
#include "tapi_rpcsock_macros.h"
#include "tapi_mem.h"
#include "tapi_rpc_misc.h"
#include "tapi_test.h"
#include "te_str.h"
#include "tapi_packetdrill.h"
#include "te_sockaddr.h"
#include "te_vector.h"
#include "tapi_job.h"
#include "tapi_job_opt.h"
#include "tapi_job_factory_rpc.h"

/** Time to wait till data is ready to read from stdout */
#define TAPI_PACKETDRILL_TIMEOUT_MS 1000

/** Packetdrill specific macro to bind 'unsigned int' argument. */
#define TAPI_PACKETDRILL_OPT_UINT(_prefix, _field) \
    TAPI_JOB_OPT_UINT(_prefix, TRUE, NULL, tapi_packetdrill_opts, _field)

/** Packetdrill specific macro to bind 'char *' argument. */
#define TAPI_PACKETDRILL_OPT_STRING(_prefix, _field) \
    TAPI_JOB_OPT_STRING(_prefix, TRUE, tapi_packetdrill_opts, _field)

/** Packetdrill specific macro to bind 'struct sockaddr *' argument. */
#define TAPI_PACKETDRILL_OPT_SOCKADDR(_prefix, _field) \
    TAPI_JOB_OPT_SOCKADDR_PTR(_prefix, TRUE, tapi_packetdrill_opts, _field)

/* See description in tapi_packetdrill.h */
struct tapi_packetdrill_app {
    tapi_job_t         *job;        /**< Job instance. */
    tapi_job_channel_t *out_chs[2]; /**< Standart output channels. */
    tapi_job_factory_t *factory;    /**< Factory used for the app intance. */
    tapi_job_channel_t *syscall_f;  /**< Filter to catch syscall messages. */
    tapi_job_channel_t *warning_f;  /**< Filter to catch warnings. */
    tapi_job_channel_t *error_f;    /**< Filter to catch errors. */
    tapi_job_channel_t *assertion_f;/**< Filter to catch assertions.*/
    char                pd_script_path[PATH_MAX]; /**< Full script path on
                                                       agent side. */
    char                pd_script_name[PATH_MAX]; /**< Script name. */
    te_bool             is_client;  /**< Flag displaying whether the app is
                                         running in a client mode. */
};

/**
 * Convert IP version to string representation to use
 * in command line.
 *
 * @param ip_version    IP version.
 *
 * @return string containing IP version in acceptable for packetdrill
 *         format.
 */
static inline const char *
packetdrill_ipver2str(tapi_packetdrill_ip_version_t ip_version)
{
    switch (ip_version)
    {
        case TAPI_PACKETDRILL_IP_VERSION_4:
            return "ipv4";

        case TAPI_PACKETDRILL_IP_VERSION_4_MAPPED_6:
            return "ipv4-mapped-ipv6";

        case TAPI_PACKETDRILL_IP_VERSION_6:
            return "ipv6";

        default:
            return NULL;
    }
}

/**
 * Copy packetdrill test to agent
 *
 * @param opts           Packetdrill opts.
 * @param app            Application handle.
 */
static void
copy_test(tapi_packetdrill_opts *opts, tapi_packetdrill_app *app)
{
    te_string       dst_path = TE_STRING_INIT_STATIC(PATH_MAX);
    te_string       src_path = TE_STRING_INIT_STATIC(PATH_MAX);
    char           *agt_dir = NULL;
    const char     *ta = tapi_job_factory_ta(app->factory);

    CHECK_RC(cfg_get_instance_string_fmt(&agt_dir, "/agent:%s/dir:", ta));

    CHECK_RC(te_string_append(&src_path, "%s/%s", opts->src_test_dir,
                              opts->short_test_name));
    CHECK_RC(te_string_append(&dst_path, "%s/%s", agt_dir,
                              opts->short_test_name));
    if (access(src_path.ptr, F_OK) < 0)
    {
        TEST_FAIL("There is no such test: %s, errno=%r", opts->short_test_name,
                  errno);
    }
    else
    {
        CHECK_RC(rcf_ta_put_file(ta, 0, src_path.ptr, dst_path.ptr));
        TE_SPRINTF(app->pd_script_path, "%s", dst_path.ptr);
        TE_SPRINTF(app->pd_script_name, "%s", opts->short_test_name);
    }

    free(agt_dir);
    return;
}

/* See description in tapi_packetdrill.h */
void
tapi_packetdrill_app_destroy(tapi_packetdrill_app *app)
{
    te_errno rc;

    if (app == NULL)
        return;

    rc = tapi_job_destroy(app->job, -1);
    if (rc != 0)
        ERROR("Failed to destroy packetdrill application, errno=%r", rc);

    if (app->is_client)
    {
        rc = rcf_ta_del_file(tapi_job_factory_ta(app->factory), 0,
                             app->pd_script_path);
        if (rc != 0)
            ERROR("Failed to remove %s, errno=%r", app->pd_script_path, rc);
    }

    tapi_job_factory_destroy(app->factory);
    free(app);
}

/**
 * Build command line arguments to run client packetdrill app.
 *
 * @param[in]  path             Program location.
 * @param[in]  pd_script_path   Packetdrill script full path.
 * @param[in]  opts             Packetdrill custom option struct.
 * @param[out] argv             Vector with command line aarguments.
 *
 * @return Status code.
 */
static te_errno
build_client_argv(const char *path, const char *pd_script_path,
                  const tapi_packetdrill_opts *opts, te_vec *argv)
{
    const tapi_job_opt_bind opt_binds[] = TAPI_JOB_OPT_SET(
        TAPI_JOB_OPT_DUMMY((opts->prefix != NULL) ? path : NULL),
        TAPI_JOB_OPT_DUMMY("-v"),
        TAPI_JOB_OPT_DUMMY("--wire_client"),
        TAPI_JOB_OPT_DUMMY(pd_script_path),
        TAPI_PACKETDRILL_OPT_STRING("--ip_version=", ip_version_str),
        TAPI_PACKETDRILL_OPT_UINT("--wire_server_port=", wire_server_port),
        TAPI_PACKETDRILL_OPT_STRING("--wire_client_dev=", wire_device),
        TAPI_PACKETDRILL_OPT_STRING("--non_fatal=", non_fatal),
        TAPI_PACKETDRILL_OPT_UINT("--bind_port=", bind_port),
        TAPI_PACKETDRILL_OPT_UINT("--connect_port=", connect_port),
        TAPI_PACKETDRILL_OPT_SOCKADDR("--local_ip=", local_ip),
        TAPI_PACKETDRILL_OPT_SOCKADDR("--remote_ip=", remote_ip),
        TAPI_PACKETDRILL_OPT_SOCKADDR("--gateway_ip=", gateway_ip),
        TAPI_PACKETDRILL_OPT_SOCKADDR("--wire_server_ip=", wire_server_ip)
    );

    return tapi_job_opt_build_args(opts->prefix != NULL ? opts->prefix : path,
                                   opt_binds, opts, argv);
}

/**
 * Build command line arguments to run server packetdrill app.
 *
 * @param[in]  path     Program location.
 * @param[in]  opts     Packetdrill custom option struct.
 * @param[out] argv     Vector with command line aarguments.
 *
 * @return Status code.
 */
static te_errno
build_server_argv(const char *path, const tapi_packetdrill_opts *opts,
                  te_vec *argv)
{
    const tapi_job_opt_bind opt_binds[] = TAPI_JOB_OPT_SET(
        TAPI_JOB_OPT_DUMMY((opts->prefix != NULL) ? path : NULL),
        TAPI_JOB_OPT_DUMMY("-v"),
        TAPI_JOB_OPT_DUMMY("--wire_server"),
        TAPI_PACKETDRILL_OPT_STRING("--ip_version=", ip_version_str),
        TAPI_PACKETDRILL_OPT_UINT("--wire_server_port=", wire_server_port),
        TAPI_PACKETDRILL_OPT_STRING("--wire_server_dev=", wire_device)
    );

    return tapi_job_opt_build_args(opts->prefix != NULL ? opts->prefix : path,
                                   opt_binds, opts, argv);
}

/**
 * Build command line arguments to run packetdrill app.
 *
 * @param[in]  path     Program location.
 * @param[in]  app      Packetdrill application handle.
 * @param[in]  opts     Packetdrill custom option struct.
 * @param[out] argv     Vector with command line arguments.
 *
 * @return Status code.
 */
static te_errno
build_argv(const char *path, tapi_packetdrill_app *app,
           tapi_packetdrill_opts *opts, te_vec *argv)
{
    opts->ip_version_str = packetdrill_ipver2str(opts->ip_version);

    if (opts->is_client)
        return build_client_argv(path, app->pd_script_path, opts, argv);
    else
        return build_server_argv(path, opts, argv);
}

/* See description in tapi_packetdrill.h */
te_errno
tapi_packetdrill_app_start(tapi_packetdrill_app *app)
{
    return tapi_job_start(app->job);
}

/* See description in tapi_packetdrill.h */
void
tapi_packetdrill_print_logs(tapi_packetdrill_app *app)
{
    tapi_job_buffer_t   buf = TAPI_JOB_BUFFER_INIT;
    unsigned int        eos_count = 0;

    if (!app->is_client)
        return;

    /* 4 is a number of listening filters. */
    while (eos_count < 4)
    {
        tapi_job_simple_receive(TAPI_JOB_CHANNEL_SET(app->syscall_f,
                                                     app->warning_f,
                                                     app->error_f,
                                                     app->assertion_f),
                                TAPI_PACKETDRILL_TIMEOUT_MS, &buf);
        if (buf.data.len > 0)
        {
            if (buf.filter == app->syscall_f)
                TE_LOG_RING(app->pd_script_name, "%s", buf.data.ptr);
            else if (buf.filter == app->warning_f)
                TE_LOG_WARN(app->pd_script_name, "%s", buf.data.ptr);
            else
                ERROR_VERDICT(buf.data.ptr);
        }

        if (buf.eos)
            ++eos_count;
    }
}

/* See description in tapi_packetdrill.h */
te_errno
tapi_packetdrill_app_wait(tapi_packetdrill_app *app, int timeout_s)
{
    tapi_job_status_t   status = {0};
    te_errno            rc = 0;

    rc = tapi_job_wait(app->job, TE_SEC2MS(timeout_s), &status);
    if (rc != 0)
        return rc;

    if (status.type == TAPI_JOB_STATUS_SIGNALED)
    {
        WARN("Packetdrill app was terminated by a signal");
        return 0;
    }
    else if (status.type == TAPI_JOB_STATUS_UNKNOWN)
    {
        ERROR("Packetdrill app terminated by unknown reason");
        return TE_RC(TE_TAPI, TE_EFAIL);
    }
    else if (status.value != 0)
    {
        ERROR("Packetdrill app failed with return code %d", status.value);
        return TE_RC(TE_TAPI, TE_ESHCMD);
    }

    return 0;
}

/* See description in tapi_packetdrill.h */
te_errno
tapi_packetdrill_app_init(tapi_job_factory_t *factory,
                          tapi_packetdrill_opts *opts,
                          tapi_packetdrill_app **app)
{
    te_errno                rc = 0;
    tapi_job_simple_desc_t  job_descr;
    tapi_packetdrill_app   *handle = NULL;
    te_vec                  argv;

    if (factory == NULL || opts == NULL || app == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    handle = tapi_calloc(1, sizeof(*handle));
    handle->factory = factory;
    handle->is_client = opts->is_client;

    if (handle->is_client)
        copy_test(opts, handle);

    rc = build_argv("packetdrill", handle, opts, &argv);
    if (rc != 0)
    {
        ERROR("Failed to initialize packetdrill app options");
        free(handle);
        return rc;
    }

    job_descr.spawner = NULL;
    job_descr.program = TE_VEC_GET(char *, &argv, 0);
    job_descr.argv = (const char **)argv.data.ptr;
    job_descr.env = NULL;
    job_descr.job_loc = &handle->job;
    job_descr.stdin_loc = NULL;
    job_descr.stdout_loc = &handle->out_chs[0];
    job_descr.stderr_loc = &handle->out_chs[1];

    if (handle->is_client)
    {
        job_descr.filters = TAPI_JOB_SIMPLE_FILTERS(
                /*
                 * Filter to catch messages about syscalls.
                 * Example:
                 * socket syscall: 1544162535.818347
                 */
                {
                    .use_stdout  = TRUE,
                    .use_stderr  = FALSE,
                    .filter_name = NULL,
                    .readable    = TRUE,
                    .log_level   = 0,
                    .re          = ".*syscall.*",
                    .extract     = 0,
                    .filter_var  = &handle->syscall_f,
                },
                /*
                 * Filter to catch warning messages.
                 * Example (XX is script line number):
                 * - XX: warning handling packet: bad value outbound TCP option
                 */
                {
                    .use_stdout  = FALSE,
                    .use_stderr  = TRUE,
                    .filter_name = NULL,
                    .readable    = TRUE,
                    .log_level   = 0,
                    .re          = "\\d+: warning.*",
                    .extract     = 0,
                    .filter_var  = &handle->warning_f,
                },
                /*
                 * Filter to catch error messages (without 'warning' word).
                 * Examples (XX is script line number):
                 * - XX: error handling packet: ...
                 * - XX: runtime error in connect call: ...
                 * - XX: timing error: expected system call start ...
                 * Example which does not match (it goes to the "warning filter"):
                 * - XX: warning handling packet: timing error: ...
                 */
                {
                    .use_stdout  = FALSE,
                    .use_stderr  = TRUE,
                    .filter_name = NULL,
                    .readable    = TRUE,
                    .log_level   = 0,
                    .re          = "\\d+:(?!.*warning.*).*error.*",
                    .extract     = 0,
                    .filter_var  = &handle->error_f,
                },
                /* Filter to catch assertions in scripts or packetdrill code. */
                {
                    .use_stdout  = FALSE,
                    .use_stderr  = TRUE,
                    .filter_name = NULL,
                    .readable    = TRUE,
                    .log_level   = 0,
                    .re          = "[Aa]ssert.*",
                    .extract     = 0,
                    .filter_var  = &handle->assertion_f,
                },
                /* Filter used just for printing stderr stream as TE warnings. */
                {
                    .use_stdout  = FALSE,
                    .use_stderr  = TRUE,
                    .filter_name = "stderr_client",
                    .readable    = TRUE,
                    .log_level   = TE_LL_WARN,
                    .re          = NULL,
                    .extract     = 0,
                    .filter_var  = NULL,
                }
            );
    }
    else
    {
        job_descr.filters = TAPI_JOB_SIMPLE_FILTERS(
                /* Filter used just for printing stderr stream as TE warnings. */
                {
                    .use_stdout  = FALSE,
                    .use_stderr  = TRUE,
                    .filter_name = "stderr_server",
                    .readable    = TRUE,
                    .log_level   = TE_LL_WARN,
                    .re          = NULL,
                    .extract     = 0,
                    .filter_var  = NULL,
                }
            );
    }

    rc = tapi_job_simple_create(handle->factory, &job_descr);
    if (rc == 0)
    {
        *app = handle;
    }
    else
    {
        ERROR("Initialization of packetdrill app job context failed");
        free(handle);
    }

    te_vec_deep_free(&argv);

    return rc;
}
