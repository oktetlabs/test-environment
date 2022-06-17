/** @file
 * @brief Test API for OpenSSH
 *
 * Test API to control OpenSSH tools and utilities.
 *
 * Copyright (C) 2022 OKTET Labs. All rights reserved.
 *
 * @author Pavel Liulchak <Pavel.Liulchak@oktetlabs.ru>
 */
#include <stddef.h>

#define TE_LGR_USER "TAPI SSH"

#include "tapi_job_opt.h"
#include "tapi_job.h"
#include "tapi_job_factory_rpc.h"
#include "te_alloc.h"
#include "te_str.h"
#include "tapi_file.h"
#include "rcf_rpc.h"
#include "tapi_ssh.h"

/** The termination timeout of a job */
#define TAPI_SSH_TERM_TIMEOUT_MS 500

#define TAPI_SSH_DEFAULT_PATH "/usr/bin/ssh"
#define TAPI_SSH_SSHD_DEFAULT_PATH "/usr/sbin/sshd"

#define TAPI_SSH_DEFAULT_PORT 22

const tapi_ssh_client_opt tapi_ssh_client_opt_default_opt = {
    .path = TAPI_SSH_DEFAULT_PATH,
    .gateway_ports = FALSE,
    .local_port_forwarding = NULL,
    .forbid_remote_commands_execution = FALSE,
    .remote_port_forwarding = NULL,
    .identity_file = NULL,
    .login_name = NULL,
    .strict_host_key_checking = TAPI_SSH_STRICT_HOST_KEY_CHECKING_NO,
    .user_known_hosts_file = NULL,
    .port = TAPI_SSH_DEFAULT_PORT,
    .destination = NULL,
    .command = NULL,
};

const tapi_ssh_server_opt tapi_ssh_server_opt_default_opt = {
    .path = TAPI_SSH_SSHD_DEFAULT_PATH,
    .host_key_file = NULL,
    .authorized_keys_file = NULL,
    .permit_root_login = TAPI_SSH_PERMIT_ROOT_LOGIN_YES,
    .pid_file = NULL,
    .pub_key_authentication = TRUE,
    .strict_modes = FALSE,
    .port = TAPI_SSH_DEFAULT_PORT,
};

static const te_enum_map tapi_ssh_option_yes_no_mapping[] = {
    {.name = "yes", .value = TRUE},
    {.name = "no", .value = FALSE},
    TE_ENUM_MAP_END
};

static const te_enum_map tapi_ssh_permit_root_login_mapping[] = {
    {.name = "yes", .value = TAPI_SSH_PERMIT_ROOT_LOGIN_YES},
    {.name = "no", .value = TAPI_SSH_PERMIT_ROOT_LOGIN_NO},
    {
        .name = "forced-commands-only",
        .value = TAPI_SSH_PERMIT_ROOT_LOGIN_FORCED_COMMANDS_ONLY
    },
    {
        .name = "prohibit-password",
        .value = TAPI_SSH_PERMIT_ROOT_LOGIN_PROHIBIT_PASSWORD
    },
    TE_ENUM_MAP_END
};

static const te_enum_map tapi_ssh_strict_host_key_checking_mapping[] = {
    {.name = "yes", .value = TAPI_SSH_STRICT_HOST_KEY_CHECKING_YES},
    {.name = "no", .value = TAPI_SSH_STRICT_HOST_KEY_CHECKING_NO},
    {
        .name = "accept-new",
        .value = TAPI_SSH_STRICT_HOST_KEY_CHECKING_ACCEPT_NEW
    },
    TE_ENUM_MAP_END
};

/* See description in tapi_ssh.h */
te_errno
tapi_ssh_create_client(tapi_job_factory_t *factory,
                       const tapi_ssh_client_opt *opt,
                       tapi_ssh_t **client_app)
{
    tapi_ssh_t        *client;
    te_vec            args = TE_VEC_INIT(char *);
    te_errno          rc = 0;
    const char        *path = opt->path;

    const tapi_job_opt_bind client_binds[] = TAPI_JOB_OPT_SET(
        TAPI_JOB_OPT_STRING("-i", FALSE, tapi_ssh_client_opt,
                            identity_file),
        TAPI_JOB_OPT_STRING("-l", FALSE, tapi_ssh_client_opt,
                            login_name),
        TAPI_JOB_OPT_ENUM("-o StrictHostKeyChecking=", TRUE, tapi_ssh_client_opt,
                          strict_host_key_checking,
                          tapi_ssh_strict_host_key_checking_mapping),
        TAPI_JOB_OPT_STRING("-o UserKnownHostsFile=", TRUE, tapi_ssh_client_opt,
                            user_known_hosts_file),
        TAPI_JOB_OPT_BOOL("-g", tapi_ssh_client_opt,
                          gateway_ports),
        TAPI_JOB_OPT_BOOL("-N", tapi_ssh_client_opt,
                          forbid_remote_commands_execution),
        TAPI_JOB_OPT_STRING("-L", FALSE, tapi_ssh_client_opt,
                            local_port_forwarding),
        TAPI_JOB_OPT_STRING("-R", FALSE, tapi_ssh_client_opt,
                            remote_port_forwarding),
        TAPI_JOB_OPT_UINT("-p", FALSE, NULL, tapi_ssh_client_opt, port),
        TAPI_JOB_OPT_STRING(NULL, FALSE, tapi_ssh_client_opt, destination),
        TAPI_JOB_OPT_STRING(NULL, FALSE, tapi_ssh_client_opt, command)
    );

    if (opt->port == TAPI_SSH_DEFAULT_PORT)
        WARN("SSH client is connecting to default port: %u", opt->port);

    client = TE_ALLOC(sizeof(*client));

    rc = tapi_job_opt_build_args(path, client_binds, opt, &args);
    if (rc != 0)
        goto out;

    rc = tapi_job_simple_create(factory,
                                &(tapi_job_simple_desc_t){
                                    .program = path,
                                    .argv = (const char **)args.data.ptr,
                                    .job_loc = &client->job,
                                    .stdout_loc = &client->out_chs[0],
                                    .stderr_loc = &client->out_chs[1],
                                    .filters = TAPI_JOB_SIMPLE_FILTERS(
                                        {
                                         .use_stdout = TRUE,
                                         .log_level = TE_LL_RING,
                                         .readable = FALSE,
                                         .filter_name = "out",
                                        },
                                        {
                                         .use_stderr = TRUE,
                                         .log_level = TE_LL_ERROR,
                                         .readable = FALSE,
                                         .filter_name = "err",
                                        }
                                    )
                                });

    if (rc != 0)
        goto out;

    *client_app = client;

out:
    te_vec_deep_free(&args);
    if (rc != 0)
        free(client);

    return rc;
}

/* See description in tapi_ssh.h */
te_errno
tapi_ssh_create_server(tapi_job_factory_t *factory,
                       const tapi_ssh_server_opt *opt,
                       tapi_ssh_t **server_app)
{
    tapi_ssh_t        *server;
    te_vec            args = TE_VEC_INIT(char *);
    te_errno          rc = 0;
    const char        *path = opt->path;

    const tapi_job_opt_bind server_binds[] = TAPI_JOB_OPT_SET(
        TAPI_JOB_OPT_STRING("-h", FALSE, tapi_ssh_server_opt,
                            host_key_file),
        TAPI_JOB_OPT_STRING("-o AuthorizedKeysFile=", TRUE, tapi_ssh_server_opt,
                            authorized_keys_file),
        TAPI_JOB_OPT_ENUM("-o PermitRootLogin=", TRUE, tapi_ssh_server_opt,
                          permit_root_login, tapi_ssh_permit_root_login_mapping),
        TAPI_JOB_OPT_STRING("-o PidFile=", TRUE, tapi_ssh_server_opt,
                            pid_file),
        TAPI_JOB_OPT_ENUM_BOOL("-o PubkeyAuthentication=", TRUE, tapi_ssh_server_opt,
                               pub_key_authentication, tapi_ssh_option_yes_no_mapping),
        TAPI_JOB_OPT_ENUM_BOOL("-o StrictModes=", TRUE, tapi_ssh_server_opt,
                               strict_modes, tapi_ssh_option_yes_no_mapping),
        TAPI_JOB_OPT_UINT("-p", FALSE, NULL, tapi_ssh_server_opt, port),
        TAPI_JOB_OPT_DUMMY("-D")
    );

    if (opt->port == TAPI_SSH_DEFAULT_PORT)
        WARN("SSHD is listening on default port: %u", opt->port);

    server = TE_ALLOC(sizeof(*server));

    rc = tapi_job_opt_build_args(path, server_binds, opt, &args);
    if (rc != 0)
        goto out;

    rc = tapi_job_simple_create(factory,
                                &(tapi_job_simple_desc_t){
                                    .program = path,
                                    .argv = (const char **)args.data.ptr,
                                    .job_loc = &server->job,
                                    .stdout_loc = &server->out_chs[0],
                                    .stderr_loc = &server->out_chs[1],
                                    .filters = TAPI_JOB_SIMPLE_FILTERS(
                                        {
                                         .use_stdout = TRUE,
                                         .log_level = TE_LL_RING,
                                         .readable = FALSE,
                                         .filter_name = "out",
                                        },
                                        {
                                         .use_stderr = TRUE,
                                         .log_level = TE_LL_ERROR,
                                         .readable = FALSE,
                                         .filter_name = "err",
                                        }
                                    )
                                });

    if (rc != 0)
        goto out;

    *server_app = server;

out:
    te_vec_deep_free(&args);
    if (rc != 0)
        free(server);

    return rc;
}

/* See description in tapi_ssh.h */
te_errno
tapi_ssh_start_app(tapi_ssh_t *app)
{
    return tapi_job_start(app->job);
}

/* See description in tapi_ssh.h */
te_errno
tapi_ssh_wait_app(tapi_ssh_t *app, int timeout_ms)
{
    tapi_job_status_t status;
    te_errno          rc;

    rc = tapi_job_wait(app->job, timeout_ms, &status);
    if (rc != 0)
        return rc;

    if (status.type == TAPI_JOB_STATUS_UNKNOWN ||
        (status.type == TAPI_JOB_STATUS_EXITED && status.value != 0))
    {
        return TE_RC(TE_TAPI, TE_EFAIL);
    }
    return 0;
}

/* See description in tapi_ssh.h */
te_errno
tapi_ssh_kill_app(tapi_ssh_t *app, int signo)
{
    te_errno rc;

    rc = tapi_job_kill(app->job, signo);
    if (rc != 0 && TE_RC_GET_ERROR(rc) != TE_ESRCH)
        ERROR("Failed to kill app");

    return rc;
}

/* See description in tapi_ssh.h */
te_errno
tapi_ssh_destroy_app(tapi_ssh_t *app)
{
    te_errno rc = 0;

    if (app == NULL)
        return 0;

    rc = tapi_job_destroy(app->job, TAPI_SSH_TERM_TIMEOUT_MS);
    free(app);

    return rc;
}

/* See description in tapi_ssh.h */
te_errno
tapi_ssh_client_wrapper_add(tapi_ssh_t *app, const char *tool,
                            const char **argv,
                            tapi_job_wrapper_priority_t priority,
                            tapi_job_wrapper_t **wrap)
{
    return tapi_job_wrapper_add(app->job, tool, argv, priority, wrap);
}