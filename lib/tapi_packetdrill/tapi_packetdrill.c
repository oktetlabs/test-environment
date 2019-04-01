/** @file
 * @brief Generic Test API for packetdrill test tool
 *
 * Generic API to use packetdrill test tool.
 *
 * Copyright (C) 2018 OKTET Labs. All rights reserved.
 *
 * @author Denis Pryazhennikov <Denis.Pryazhennikov@oktetlabs.ru>
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

/* Time to wait till data is ready to read from stdout */
#define TAPI_PACKETDRILL_TIMEOUT_MS 1000

/* See description in tapi_packetdrill.h */
struct tapi_packetdrill_app {
    tapi_packetdrill_opts    opts;        /**< Tool's options */
    rcf_rpc_server          *rpcs;        /**< RPC server handle */
    tarpc_pid_t              pid;         /**< PID */
    int                      fd_stdout;   /**< File descriptor to read
                                               from stdout stream */
    int                      fd_stderr;   /**< File descriptor to read
                                               from stderr stream */
    char                    *cmd;         /**< Command line string to run
                                               the application */
    te_string                stdout;      /**< Buffer to save tool's
                                               stdout message */
    te_string                stderr;      /**< Buffer to save tool's
                                               stderr message */
};

/*
 * Close packetdrill application opened descriptors.
 *
 * @param app               Application context.
 */
static void
close_descriptors(tapi_packetdrill_app *app)
{
    if (app->rpcs == NULL)
        return;

    if (app->fd_stdout >= 0)
        RPC_CLOSE(app->rpcs, app->fd_stdout);
    if (app->fd_stderr >= 0)
        RPC_CLOSE(app->rpcs, app->fd_stderr);
}

/**
 * Initialize packetdrill application context.
 *
 * @param app           Packetdrill tool context
 * @param opts          Packetdrill tool options
 */
static void
init_opts(tapi_packetdrill_app *app, tapi_packetdrill_opts *opts)
{
    memcpy(&app->opts, opts, sizeof(app->opts));
    if (app->opts.non_fatal != NULL)
        app->opts.non_fatal = tapi_strdup(opts->non_fatal);

#define COPY_ADDR(_opt) \
    do {                                                        \
        if (opts->_opt != NULL)                                 \
            app->opts._opt = tapi_memdup(app->opts._opt,        \
                                         sizeof(*opts->_opt));  \
    } while (0)
    COPY_ADDR(local_ip);
    COPY_ADDR(remote_ip);
    COPY_ADDR(gateway_ip);
    COPY_ADDR(netmask_ip);
    COPY_ADDR(wire_server_ip);
#undef COPY_ADDR
}

/*
 * Uninitialize packetdrill application context.
 *
 * @param app           Packetdrill tool context.
 */
static void
app_fini(tapi_packetdrill_app *app)
{
    close_descriptors(app);
    free(app->cmd);
    free(app->opts.non_fatal);

#define FREE_ADDR(_opt) \
    do {                               \
        free((void *)app->opts._opt); \
    } while (0)
    FREE_ADDR(local_ip);
    FREE_ADDR(remote_ip);
    FREE_ADDR(gateway_ip);
    FREE_ADDR(netmask_ip);
    FREE_ADDR(wire_server_ip);
#undef FREE_ADDR

    te_string_free(&app->stdout);
    te_string_free(&app->stderr);
}

/*
 * Copy packetdrill test to agent
 *
 * @param opts           Packetdrill opts.
 * @param rpcs           RPC server handle.
 */
static void
copy_test(tapi_packetdrill_opts *opts, rcf_rpc_server *rpcs)
{
    te_string       dst_path = TE_STRING_INIT_STATIC(PATH_MAX);
    te_string       src_path = TE_STRING_INIT_STATIC(PATH_MAX);
    char           *agt_dir = NULL;
    te_string       full_test_name_str;
    cfg_val_type    val_type = CVT_STRING;

    CHECK_RC(cfg_get_instance_fmt(&val_type, &agt_dir, "/agent:%s/dir:",
                                  rpcs->ta));

    CHECK_RC(te_string_append(&src_path, "%s/%s", opts->src_test_dir,
                              opts->short_test_name));
    CHECK_RC(te_string_append(&dst_path, "%s%s", agt_dir,
                              opts->short_test_name));
    if (access(src_path.ptr, F_OK) < 0)
    {
        TEST_FAIL("There is no such test: %s, errno=%r", opts->short_test_name,
                  errno);
    }
    else
    {
        CHECK_RC(rcf_ta_put_file(rpcs->ta, 0, src_path.ptr, dst_path.ptr));
        te_string_assign_buf(&full_test_name_str, opts->full_test_name,
                             sizeof(opts->full_test_name));
        CHECK_RC(te_string_append(&full_test_name_str, "%s", dst_path.ptr));
    }

    free(agt_dir);
    return;
}

/* See description in tapi_packetdrill.h */
tapi_packetdrill_app *
tapi_packetdrill_app_init(tapi_packetdrill_opts *opts,
                          rcf_rpc_server *rpcs)
{
    tapi_packetdrill_app *app;

    if (rpcs == NULL)
        TEST_FAIL("Invalid RPC server in packetdrill app initialization");

    app = tapi_malloc(sizeof(*app));

    app->rpcs = rpcs;
    app->pid = -1;
    app->fd_stdout = -1;
    app->fd_stderr = -1;
    app->cmd = NULL;
    app->stdout = (te_string)TE_STRING_INIT;
    app->stderr = (te_string)TE_STRING_INIT;

    if (opts->is_client)
        copy_test(opts, rpcs);

    init_opts(app, opts);

    return app;
}

/* See description in tapi_packetdrill.h */
void
tapi_packetdrill_app_destroy(tapi_packetdrill_app *app)
{
    te_errno rc;

    if (app == NULL)
        return;

    tapi_packetdrill_app_stop(app);

    if (app->opts.is_client)
    {
        rc = rcf_ta_del_file(app->rpcs->ta, 0, app->opts.full_test_name);
        if (rc != 0)
            ERROR("Failed to remove %s, errno=%r", rc);
    }
    app_fini(app);
    free(app);
}

/*
 * Set string option in packetdrill tool format.
 *
 * @param cmd        Buffer contains a command to add option to.
 * @param opt_value  String to set.
 * @param opt_name   Packetdrill options name.
 */
static void
set_opt_str(te_string *cmd, const char *opt_value, char *opt_name)
{
    if (opt_value != NULL)
        CHECK_RC(te_string_append(cmd, " --%s=%s", opt_name, opt_value));
}

/*
 * Set integer option in packetdrill tool format.
 *
 * @param cmd        Buffer contains a command to add option to.
 * @param opt_value  Integer value to set.
 * @param opt_name   Packetdrill options name.
 */
static void
set_opt_int(te_string *cmd, int opt_value, const char *opt_name)
{
    if (opt_value >= 0)
        CHECK_RC(te_string_append(cmd, " --%s=%d", opt_name, opt_value));
}

/*
 * Set ip version option in packetdrill tool format.
 *
 * @param cmd         Buffer contains a command to add option to.
 * @param ip_version  Type of IP version
 */
static void
set_ip_version(te_string *cmd, tapi_packetdrill_ip_version_t ip_version)
{
    char *ip_version_str;

    switch (ip_version)
    {
        case TAPI_PACKETDRILL_IP_VERSION_4:
            ip_version_str = "ipv4";
            break;
        case TAPI_PACKETDRILL_IP_VERSION_4_MAPPED_6:
            ip_version_str = "ipv4-mapped-ipv6";
            break;
        case TAPI_PACKETDRILL_IP_VERSION_6:
            ip_version_str = "ipv6";
            break;
        case TAPI_PACKETDRILL_IP_UNKNOWN:
            return;
        default:
            ERROR("Wrong IP version parameter specification");
            return;
    }

    CHECK_RC(te_string_append(cmd, " --ip_version=%s", ip_version_str));
}

/*
 * Set IP address option in packetdrill tool format.
 *
 * @param cmd        Buffer contains a command to add option to.
 * @param opt_value  Address to set.
 * @param opt_name   Packetdrill options name.
 */
static void
set_opt_addr(te_string *cmd, const struct sockaddr *opt_value, char *opt_name)
{
    const char *addr_str;

    if (opt_value == NULL)
        return;

    addr_str = te_sockaddr_get_ipstr(opt_value);

    CHECK_RC(te_string_append(cmd, " --%s=%s", opt_name, addr_str));
}

/*
 * Build command string to run client packetdrill app.
 *
 * @param cmd        Buffer to put built command to.
 * @param opts       Packetdrill tool client opts.
 */
static void
build_client_cmd(te_string *cmd, const tapi_packetdrill_opts *opts)
{
    CHECK_RC(te_string_append(cmd, "packetdrill -v --wire_client %s",
                              opts->full_test_name));
    set_ip_version(cmd, opts->ip_version);
    set_opt_int(cmd, opts->wire_server_port, "wire_server_port");
    set_opt_str(cmd, opts->wire_device, "wire_client_dev");
    set_opt_str(cmd, opts->non_fatal, "non_fatal");
    set_opt_int(cmd, opts->bind_port, "bind_port");
    set_opt_int(cmd, opts->connect_port, "connect_port");
    set_opt_addr(cmd, opts->local_ip, "local_ip");
    set_opt_addr(cmd, opts->remote_ip, "remote_ip");
    set_opt_addr(cmd, opts->gateway_ip, "gateway_ip");
    set_opt_addr(cmd, opts->netmask_ip, "netmask_ip");
    set_opt_addr(cmd, opts->wire_server_ip, "wire_server_ip");
    set_opt_int(cmd, opts->speed, "speed");
    set_opt_int(cmd, opts->mtu, "mtu");
    set_opt_int(cmd, opts->tolerance_usecs, "tolerance_usecs");
    set_opt_int(cmd, opts->tcp_ts_tick_usecs, "tcp_ts_tick_usecs");
    CHECK_RC(te_string_append(cmd, " 2>&1"));
}

/*
 * Build command string to run server packetdrill app.
 *
 * @param cmd        Buffer to put built command to.
 * @param opts       Packetdrill tool server opts.
 */
static void
build_server_cmd(te_string *cmd, const tapi_packetdrill_opts *opts)
{
    CHECK_RC(te_string_append(cmd, "packetdrill -v --wire_server"));
    set_ip_version(cmd, opts->ip_version);
    set_opt_int(cmd, opts->wire_server_port, "wire_server_port");
    set_opt_str(cmd, opts->wire_device, "wire_server_dev");
    CHECK_RC(te_string_append(cmd, " 2>&1"));
}

/*
 * Build command string to run packetdrill app.
 *
 * @param cmd        Buffer to put built command to.
 * @param opts       Packetdrill tool opts.
 */
static void
build_cmd(te_string *cmd, const tapi_packetdrill_opts *opts)
{
    if (opts->is_client)
        build_client_cmd(cmd, opts);
    else
        build_server_cmd(cmd, opts);

}

/* See description in tapi_packetdrill.h */
te_errno
tapi_packetdrill_app_start(tapi_packetdrill_app *app)
{
    tarpc_pid_t pid;
    int         stdout = -1;
    int         stderr = -1;
    te_string   cmd = TE_STRING_INIT;

    build_cmd(&cmd, (const tapi_packetdrill_opts*)&app->opts);

    RING("Run \"%s\"", cmd.ptr);
    pid = rpc_te_shell_cmd(app->rpcs, cmd.ptr, -1, NULL, &stdout, &stderr);

    close_descriptors(app);
    app->pid = pid;
    app->fd_stdout = stdout;
    app->fd_stderr = stderr;
    free(app->cmd);
    app->cmd = cmd.ptr;

    return 0;
}

/* See description in tapi_packetdrill.h */
te_errno
tapi_packetdrill_app_stop(tapi_packetdrill_app *app)
{
    if (app->pid >= 0)
    {
        rpc_ta_kill_death(app->rpcs, app->pid);
        app->pid = -1;
    }

    return 0;
}

/**
 * Map packetdrill log line to native TE log style.
 *
 * @param str           String with one line of packetdrill log.
 * @param test_name     Packetdrill test name.
 */
static void
parse_log_str(char *str, const char *test_name)
{
    char *tmp_ptr = NULL;

    /* Example: socket syscall: 1544162535.818347 */
    tmp_ptr = strstr(str, "syscall:");
    if (tmp_ptr != NULL)
    {
        *(tmp_ptr - 1) = '\0';
        TE_LOG_RING(test_name, "%s() -> 0", str);
        return;
    }

    /*
     * Example:tests/linux/close/close-so-linger-onoff-1-linger-0-rst.pkt:14:
     * warning handling packet: bad value outbound TCP option 3
     */
    tmp_ptr = strstr(str, "warning");
    if (tmp_ptr != NULL)
    {
        TE_LOG_WARN(test_name, "%s", str);
        return;
    }

    /*
     * Example: tmp_mss-getsockopt-tcp_maxseg-client.pkt:7:
     * runtime error in connect call: Expected result 0 but
     * got -1 with errno 111 (Connection refused)
     */
    tmp_ptr = strstr(str, "error");
    if (tmp_ptr != NULL)
    {
        /*
         * Any packetdrill test errors contain the name of the test,
         * the rest of errors is displayed as INFO()
         */
        tmp_ptr = strstr(str, test_name);
        if (tmp_ptr != NULL)
        {
            tmp_ptr += strlen(test_name) + 1;
            TEST_VERDICT("%s", tmp_ptr);;
        }
    }

    TE_LOG_INFO(test_name, "%s", str);
}

/**
 * Parse packetdrill logs line-by-line to make it looks like native TE logs.
 *
 * @param packetdrill_logs   Packetdrill output.
 * @param test_name          Packetdrill test name.
 */
static void
parse_logs(te_string *packetdrill_logs, const char *test_name)
{
    char *tmp_str = NULL;
    char *line = NULL;
    char *rest = NULL;

    tmp_str = tapi_strdup(packetdrill_logs->ptr);
    rest = tmp_str;

    while ((line = strtok_r(rest, "\n", &rest)) != NULL)
        parse_log_str(line, test_name);

    free(tmp_str);
}

/* See description in tapi_packetdrill.h */
te_errno
tapi_packetdrill_print_logs(tapi_packetdrill_app *app)
{
    /* stderr is redirected to stdout */
    rpc_read_fd2te_string(app->rpcs, app->fd_stdout,
                          TAPI_PACKETDRILL_TIMEOUT_MS, 0,
                          &app->stdout);

    if (app->stdout.ptr != NULL && app->stdout.len != 0)
        parse_logs(&app->stdout, app->opts.short_test_name);

    return 0;
}

/**
 * Wait while application finishes its work.
 *
 * @param app           Application context.
 * @param timeout_s       Time to wait for application results.
 *
 * @return Status code.
 * @retval 0            No errors.
 * @retval TE_ESHCMD    Application returns non-zero exit code.
 * @retval TE_EFAIL     Critical error, application should be stopped.
 */
te_errno
tapi_packetdrill_app_wait(tapi_packetdrill_app *app, int timeout_s)
{
    rpc_wait_status stat;
    tarpc_pid_t     pid;
    unsigned        num_attempts;
    const unsigned  delay_ms = 1000;
    unsigned        i;

    num_attempts = TE_SEC2MS(timeout_s) / delay_ms;

    /* Ensure that will be at least one iteration */
    if (num_attempts == 0)
        num_attempts = 1;

    for (i = 0; i < num_attempts; i++)
    {
        RPC_AWAIT_ERROR(app->rpcs);
        pid = rpc_waitpid(app->rpcs, app->pid, &stat, RPC_WNOHANG);
        if (app->pid == pid)
        {
            break;
        }
        else if (pid != 0)
        {
            ERROR("Failed to wait for pid %d: %r", pid, RPC_ERRNO(app->rpcs));
            return RPC_ERRNO(app->rpcs);
        }
        if (i < num_attempts - 1)
            te_msleep(delay_ms);
    }

    if (pid != app->pid)
    {
        ERROR("Failed to wait for finishing packetdrill work");
        return TE_RC(TE_TAPI, TE_EFAIL);
    }
    app->pid = -1;

    return 0;
}
