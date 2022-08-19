/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief OpenSSH tools and utilities TAPI
 *
 * @defgroup tapi_ssh OpenSSH tools and utilities TAPI (tapi_ssh)
 * @ingroup te_ts_tapi
 * @{
 *
 * TAPI to handle OpenSSH tools and utilities.
 *
 * Copyright (C) 2022-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAPI_SSH_H__
#define __TE_TAPI_SSH_H__

#include "tapi_job.h"
#include "te_enum.h"
#include "rcf_rpc.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Timeout to wait for completion of entity app */
#define TAPI_SSH_APP_WAIT_TIME_MS 3000

/** Number of tapi_job output channels */
#define TAPI_SSH_NB_CHANNELS (1 /* stdout */ + 1 /* stderr */)

typedef struct tapi_ssh {
    tapi_job_t *job;                                   /**< TAPI job handle */
    tapi_job_channel_t *out_chs[TAPI_SSH_NB_CHANNELS]; /**< Output channels handles */
    tapi_job_channel_t *filter;                        /**< Output filter */
} tapi_ssh_t;

/**
 *  Representation of possible values for PermitRootLogin
 *  sshd option
 */
typedef enum tapi_ssh_permit_root_login {
    TAPI_SSH_PERMIT_ROOT_LOGIN_YES,
    TAPI_SSH_PERMIT_ROOT_LOGIN_NO,
    TAPI_SSH_PERMIT_ROOT_LOGIN_FORCED_COMMANDS_ONLY,
    TAPI_SSH_PERMIT_ROOT_LOGIN_PROHIBIT_PASSWORD,
} tapi_ssh_permit_root_login_t;

/**
 *  Representation of possible values for StrictHostKeyChecking
 *  ssh option
 */
typedef enum tapi_ssh_strict_host_key_checking {
    TAPI_SSH_STRICT_HOST_KEY_CHECKING_YES,
    TAPI_SSH_STRICT_HOST_KEY_CHECKING_NO,
    TAPI_SSH_STRICT_HOST_KEY_CHECKING_ACCEPT_NEW,
} tapi_ssh_strict_host_key_checking_t;

/** OpenSSH client specific options */
typedef struct tapi_ssh_client_opt {
    /** Path to ssh binary */
    char *path;

    /**
     * Allow remote hosts to connect to
     * local forwarded ports.
     *
     * ssh -g
     */
    te_bool gateway_ports;

    /**
     * String representation of
     * connection to the given TCP port
     * or Unix socket on the local (client)
     * host has to be forwarded to the given
     * host and port, or Unix socket, on the
     * remote side.
     *
     * ssh -L <local_port_forwarding>
     */
    char *local_port_forwarding;

    /**
     * Do not execute a remote command.
     * @note This is useful for just forwarding ports.
     *
     * ssh -N
     */
    te_bool forbid_remote_commands_execution;

    /**
     * File with identity (private key) for public key
     * authentication
     *
     * ssh -i <identity_file>
     */
    char *identity_file;

    /**
     * The user to log in on the remote machine
     *
     * ssh -l <login_name>
     */
    char *login_name;

    /**
     * Sets up extra confirmation during connection
     * to unknown host
     *
     * @note If StrictHostKeyChecking=yes and
     * host to connect is unknown, the user is
     * required to explicitly confirm connection to
     * the host by answering the question:
     * Are you sure you want to continue connecting
     * (yes/no/[fingerprint])?"
     *
     * ssh -o StrictHostKeyChecking=<strict_host_key_checking>
     * May be @c yes , @c no , @c accept-new
     */
    tapi_ssh_strict_host_key_checking_t strict_host_key_checking;

    /**
     * File to store known hosts keys
     *
     * ssh -o UserKnownHostsFile=<user_known_hosts_file>
     */
    char *user_known_hosts_file;

    /**
     * Remote host port to connect
     *
     *  ssh -p <port>
     */
    unsigned int port;

    /**
     * String representation of
     * connection to the given TCP port
     * or Unix socket on the remote (server)
     * host has to be forwarded to the local side.
     *
     * ssh -R <remote_port_forwarding>
     */
    char *remote_port_forwarding;

    /** Server to connect to */
    char *destination;

    /** Command to be executed on the server */
    char *command;
} tapi_ssh_client_opt;

/** Default ssh options initializer */
extern const tapi_ssh_client_opt tapi_ssh_client_opt_default_opt;

/** OpenSSH server specific options */
typedef struct tapi_ssh_server_opt {
    /** Path to sshd binary */
    char *path;

    /**
     * File to read a host key from
     *
     * sshd -h <host_key_file>
     */
    char *host_key_file;

    /**
     * File with public keys for users authentication
     *
     * sshd -o AuthorizedKeysFile=<authorized_keys_file>
     */
    char *authorized_keys_file;

    /**
     * Specifies whether root login is allowed
     *
     * sshd -o PermitRootLogin=<permit_root_login>
     * May be @c yes , @c no , @c forced-commands-only or @c prohibit-password
     */
    tapi_ssh_permit_root_login_t permit_root_login;
    /**
     * File to store SSHD process ID
     *
     * sshd -o PidFile=<pid_file>
     */

    char *pid_file;
    /**
     * Specifies whether public key authentication is allowed
     *
     * sshd -o PubkeyAuthentication=<pub_key_authentication>
     * May be @c yes or @c no
     */

    te_bool pub_key_authentication;
    /**
     * Sets checking ownership of user's files and home directory
     * before allowing the login.
     *
     * @note For instance authorized_keys file may reside in directory
     * that is owned not only by user. To allow sshd consider logged
     * users pubkeys from such authorized_keys file, we should set the option
     * value on @c no
     *
     * sshd -o StrictModes=<strict_modes>
     * May be @c yes or @c no
     */

    te_bool strict_modes;

    /**
     * Port to listen on
     *
     * sshd -p <port>
     */
    unsigned int port;
} tapi_ssh_server_opt;

/** Default sshd options initializer */
extern const tapi_ssh_server_opt tapi_ssh_server_opt_default_opt;

/**
 * Create ssh client app.
 *
 * @param[in]  factory      Client job factory.
 * @param[in]  opt          Command line options.
 * @param[in]  client_app   Client app handle.
 *
 * @return Status code.
 */
extern te_errno tapi_ssh_create_client(tapi_job_factory_t *factory,
                                       const tapi_ssh_client_opt *opt,
                                       tapi_ssh_t **client_app);

/**
 * Create sshd server app.
 *
 * @param[in]  factory      Server job factory.
 * @param[in]  opt          Command line options.
 * @param[in]  server_app   Server app (sshd) handle.
 *
 * @return Status code.
 */
extern te_errno tapi_ssh_create_server(tapi_job_factory_t *factory,
                                       const tapi_ssh_server_opt *opt,
                                       tapi_ssh_t **server_app);

/**
 * Start entity app.
 *
 * @param[in]  app          Entity app handle.
 * @note @p app entity may be @c ssh or @c sshd.
 *
 * @return Status code.
 */
extern te_errno tapi_ssh_start_app(tapi_ssh_t *app);

/**
 * Wait for completion of entity app.
 *
 * @param[in]  app          Entity app handle.
 * @param[in]  timeout_ms   Wait timeout in milliseconds
 *                          (negative means tapi_job_get_timeout()).
 * @note @p app entity may be @c ssh or @c sshd.
 *
 * @return Status code.
 */
extern te_errno tapi_ssh_wait_app(tapi_ssh_t *app, int timeout_ms);

/**
 * Send a signal to entity app.
 *
 * @param[in]  app          Entity app handle.
 * @param[in]  signo        Signal to send to client.
 * @note @p app entity may be @c ssh or @c sshd.
 *
 * @return Status code.
 */
extern te_errno tapi_ssh_kill_app(tapi_ssh_t *app, int signo);

/**
 * Destroy entity app.
 *
 * @param[in]  app          Entity app handle.
 * @note @p app entity may be @c ssh or @c sshd.
 *
 * @return Status code.
 */
extern te_errno tapi_ssh_destroy_app(tapi_ssh_t *app);

/**
 * Add a wrapper tool/script to OpenSSH.
 *
 * @param[in]  app      SSH client app handle.
 * @param[in]  tool     Path to the wrapper tool.
 * @param[in]  argv     Wrapper arguments (last item should be @c NULL ).
 * @param[in]  priority Wrapper priority.
 * @param[out] wrap     Wrapper instance handle.
 *
 * @return Status code.
 */
extern te_errno tapi_ssh_client_wrapper_add(tapi_ssh_t *app, const char *tool,
                                            const char **argv,
                                            tapi_job_wrapper_priority_t priority,
                                            tapi_job_wrapper_t **wrap);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_TAPI_SSH_H__ */

/**@} <!-- END tapi_ssh --> */
