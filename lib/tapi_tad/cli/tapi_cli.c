/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Proteos, Tester API for CLI CSAP.
 *
 * Implementation of Tester API for CLI CSAP.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "TAPI CLI"

#include "te_config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdio.h>
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif

#include <unistd.h>

#include "rcf_api.h"
#include "ndn_cli.h"
#include "logger_api.h"
#include "conf_api.h"

#include "tapi_cli.h"


#define TAPI_CLI_CSAP_STR_MAXLEN            512
#define TAPI_CLI_CSAP_INIT_FILENAME_MAXLEN  128


/** CLI CSAP type names */
const char * const tapi_cli_csap_type_name[] =
    {"serial", "telnet", "ssh", "sh"};

/** Default command prompt on redhat is '[...]$ ' */
const char * const tapi_cli_redhat_cprompt_dflt = "\\]\\$\\ ";

/** Default command prompt on debian is '...$ ' */
const char * const tapi_cli_debian_cprompt_dflt = "\\$\\ ";

/** Default login prompt for serial console is '[L|l]ogin: ' */
const char * const tapi_cli_serial_lprompt_dflt = "ogin: ";

/** Default password prompt for serial console is '[P|p]assword: ' */
const char * const tapi_cli_serial_pprompt_dflt = "assword: ";

/** Default login prompt for telnet console is 'Login: ' */
const char * const tapi_cli_telnet_lprompt_dflt = "ogin: ";

/** Default password prompt for telnet console is '[P|p]assword: ' */
const char * const tapi_cli_telnet_pprompt_dflt = "assword: ";

/** There is no default login prompt for ssh console */
const char * const tapi_cli_ssh_lprompt_dflt = NULL;

/** Default password prompt for ssh console is '[P|p]assword: ' */
const char * const tapi_cli_ssh_pprompt_dflt = "assword: ";

/** Default login prompt for shell console is '[L|l]ogin: ' */
const char * const tapi_cli_shell_lprompt_dflt = "ogin: ";

/** Default password prompt for shell console is '[P|p]assword: ' */
const char * const tapi_cli_shell_pprompt_dflt = "assword: ";


/**
 * Add prompts parameters to CLI CSAP initialisation string
 *
 * @param buf                   Buffer that contains CLI CSAP
 *                              initialisation string
 * @param buf_size              Buffer size
 * @param command_prompt_type   Expected command prompt type
 *                              (plain or regular expression)
 * @param command_prompt        Expected command prompt
 *                              (when commands may be sent)
 * @param login_prompt_type     Expected login prompt type
 *                              (plain or regular expression)
 * @param login_prompt          Expected login prompt
 *                              (when login may be sent)
 * @oparam login_name           Login name to use on matching with
 *                              login prompt
 * @param passwd_prompt_type    Expected password prompt type
 *                              (plain or regular expression)
 * @param passwd_prompt         Expected password prompt
 *                              (when password may be sent)
 * @param passwd                Password to use on matching with
 *                              password prompt
 *
 * @return length of added parameters string.
 */
static int
tapi_cli_csap_add_prompts(char *buf, int buf_size,
                          tapi_cli_prompt_t cmd_prompt_type,
                          const char *cmd_prompt,
                          tapi_cli_prompt_t login_prompt_type,
                          const char *login_prompt,
                          const char *login_name,
                          tapi_cli_prompt_t passwd_prompt_type,
                          const char *passwd_prompt,
                          const char *passwd)
{
    int len = 0;

    if (cmd_prompt != NULL)
        len += snprintf(buf + len, buf_size - len,
                        ", command-prompt %s : \"%s\"",
                        (cmd_prompt_type == TAPI_CLI_PROMPT_TYPE_REG_EXP) ?
                        "script" : "plain",
                        cmd_prompt);

    if (login_prompt != NULL)
        len += snprintf(buf + len, buf_size - len,
                        ", login-prompt %s : \"%s\"",
                        (login_prompt_type == TAPI_CLI_PROMPT_TYPE_REG_EXP) ?
                        "script" : "plain",
                        login_prompt);

    if (login_name != NULL)
        len += snprintf(buf + len, buf_size - len,
                        ", user plain : \"%s\"", login_name);

    if (passwd_prompt != NULL)
        len += snprintf(buf + len, buf_size - len,
                        ", password-prompt %s : \"%s\"",
                        (passwd_prompt_type == TAPI_CLI_PROMPT_TYPE_REG_EXP) ?
                        "script" : "plain",
                        passwd_prompt);

    if (passwd != NULL)
        len += snprintf(buf + len, buf_size - len,
                        ", password plain : \"%s\"", passwd);

    return len;
}


/**
 * Create common CLI CSAP on local device (using millicom).
 *
 * @param ta_name               Test Agent name
 * @param sid                   RCF session
 * @param device                Local device name
 * @param command_prompt_type   Expected command prompt type
 *                              (plain or regular expression)
 * @param command_prompt        Expected command prompt
 *                              (when commands may be sent)
 * @param login_prompt_type     Expected login prompt type
 *                              (plain or regular expression)
 * @param login_prompt          Expected login prompt
 *                              (when login name may be sent)
 * @param login_name            Login name to be sent if login
 *                              prompt is detected
 * @param password_prompt_type  Expected password prompt type
 *                              (plain or regular expression)
 * @param password_prompt       Expected password prompt
 *                              (when password may be sent)
 * @param password              Password to be sent if password
 *                              prompt is detected
 * @param cli_csap              Identifier of created CSAP (OUT)
 *
 * @return 0 on success, otherwise standard or common TE error code.
 */
int
tapi_cli_csap_local_create(const char *ta_name, int sid,
                           const char *device,
                           tapi_cli_prompt_t command_prompt_type,
                           const char *command_prompt,
                           tapi_cli_prompt_t login_prompt_type,
                           const char *login_prompt,
                           const char *login_name,
                           tapi_cli_prompt_t password_prompt_type,
                           const char *password_prompt,
                           const char *password,
                           csap_handle_t *cli_csap)
{
    int   rc = 0;
    int   len = 0;
    char *buf = (char *)malloc(TAPI_CLI_CSAP_STR_MAXLEN);
    int   buf_size = TAPI_CLI_CSAP_STR_MAXLEN;
    int   type = TAPI_CLI_CSAP_TYPE_SERIAL;

    if (buf == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);

    len += snprintf(buf + len, buf_size - len,
                    "{ layers { cli : { conn-type %d,"
                    "          conn-params serial : "
                    "{ device plain : \"%s\" }",
                    type, device);

    len += tapi_cli_csap_add_prompts(buf + len, buf_size - len,
                                     command_prompt_type, command_prompt,
                                     login_prompt_type,
                                     login_prompt, login_name,
                                     password_prompt_type,
                                     password_prompt, password);

    len += snprintf(buf + len, buf_size - len, " } } }");

    rc = tapi_cli_csap_create (ta_name, sid, buf, cli_csap);

    free(buf);

    return rc;
}

/**
 * Create common CLI CSAP on remote connection (telnet or ssh).
 *
 * @param ta_name               Test Agent name
 * @param sid                   RCF session
 * @param type                  Remote connection type (telnet or ssh)
 * @param host                  Remote host name
 * @param port                  Remote port number
 * @param command_prompt_type   Expected command prompt type
 *                              (plain or regular expression)
 * @param command_prompt        Expected command prompt
 *                              (when commands may be sent)
 * @param login_prompt_type     Expected login prompt type
 *                              (plain or regular expression)
 * @param login_prompt          Expected login prompt
 *                              (when login name may be sent)
 * @param login_name            Login name to be sent if login
 *                              prompt is detected
 * @param password_prompt_type  Expected password prompt type
 *                              (plain or regular expression)
 * @param password_prompt       Expected password prompt
 *                              (when password may be sent)
 * @param password              Password to be sent if password
 *                              prompt is detected
 * @param cli_csap              Identifier of created CSAP (OUT)
 *
 * @return 0 on success, otherwise standard or common TE error code.
 */
int
tapi_cli_csap_remote_create(const char *ta_name, int sid,
                            int type, const char *host, int port,
                            tapi_cli_prompt_t command_prompt_type,
                            const char *command_prompt,
                            tapi_cli_prompt_t login_prompt_type,
                            const char *login_prompt,
                            const char *login_name,
                            tapi_cli_prompt_t password_prompt_type,
                            const char *password_prompt,
                            const char *password,
                            csap_handle_t *cli_csap)
{
    int   rc = 0;
    int   len = 0;
    char *buf = (char *)malloc(TAPI_CLI_CSAP_STR_MAXLEN);
    int   buf_size = TAPI_CLI_CSAP_STR_MAXLEN;

    if (buf == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);

    len += snprintf(buf + len, buf_size - len,
                    "{ layers { cli : { conn-type %d,"
                    "          conn-params telnet : { host plain : \"%s\","
                    "                                 port plain : %d }",
                    type, host, port);

    len += tapi_cli_csap_add_prompts(buf + len, buf_size - len,
                                     command_prompt_type, command_prompt,
                                     login_prompt_type,
                                     login_prompt, login_name,
                                     password_prompt_type,
                                     password_prompt, password);

    len += snprintf(buf + len, buf_size - len, " } } }");

    rc = tapi_cli_csap_create (ta_name, sid, buf, cli_csap);

    free(buf);

    return rc;
}


/**
 * Create common CLI CSAP using shell.
 *
 * @param ta_name               Test Agent name
 * @param sid                   RCF session
 * @param shell_args            Arguments to shell interpreter
 * @param command_prompt_type   Expected command prompt type
 *                              (plain or regular expression)
 * @param command_prompt        Expected command prompt
 *                              (when commands may be sent)
 * @param login_prompt_type     Expected login prompt type
 *                              (plain or regular expression)
 * @param login_prompt          Expected login prompt
 *                              (when login name may be sent)
 * @param login_name            Login name to be sent if login
 *                              prompt is detected
 * @param password_prompt_type  Expected password prompt type
 *                              (plain or regular expression)
 * @param password_prompt       Expected password prompt
 *                              (when password may be sent)
 * @param password              Password to be sent if password
 *                              prompt is detected
 * @param cli_csap              Identifier of created CSAP (OUT)
 *
 * @return 0 on success, otherwise standard or common TE error code.
 */
int
tapi_cli_csap_shell_create(const char *ta_name, int sid,
                           const char *shell_args,
                           tapi_cli_prompt_t command_prompt_type,
                           const char *command_prompt,
                           tapi_cli_prompt_t login_prompt_type,
                           const char *login_prompt,
                           const char *login_name,
                           tapi_cli_prompt_t password_prompt_type,
                           const char *password_prompt,
                           const char *password,
                           csap_handle_t *cli_csap)
{
    int   rc = 0;
    int   len = 0;
    char *buf = (char *)malloc(TAPI_CLI_CSAP_STR_MAXLEN);
    int   buf_size = TAPI_CLI_CSAP_STR_MAXLEN;
    int   type = TAPI_CLI_CSAP_TYPE_SHELL;

    if (buf == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);

    len += snprintf(buf + len, buf_size - len,
                    "{ layers { cli : { conn-type %d,"
                    "          conn-params shell : { args plain : \"%s\" }",
                    type, shell_args);

    len += tapi_cli_csap_add_prompts(buf + len, buf_size - len,
                                     command_prompt_type, command_prompt,
                                     login_prompt_type,
                                     login_prompt, login_name,
                                     password_prompt_type,
                                     password_prompt, password);

    len += snprintf(buf + len, buf_size - len, " } } }");

    rc = tapi_cli_csap_create (ta_name, sid, buf, cli_csap);

    free(buf);

    return rc;
}


/**
 * Create common CLI CSAP.
 *
 * @param ta_name       Test Agent name;
 * @param sid           RCF session;
 * @param buf           CLI CSAP initialisation string.
 * @param cli_csap      Identifier of created CSAP (OUT).
 *
 * @return zero on success, otherwise standard or common TE error code.
 */
int
tapi_cli_csap_create(const char *ta_name, int sid,
                     const char *buf, csap_handle_t *cli_csap)
{
    int   rc;
    char  tmp_name[] = "/tmp/te_cli_csap_create.XXXXXX";
    FILE *f;

    if ((ta_name == NULL) || (buf == NULL) || (cli_csap == NULL))
        return TE_RC(TE_TAPI, TE_EINVAL);

    if ((rc = te_make_tmp_file(tmp_name)) != 0)
        return TE_RC(TE_TAPI, rc);

    if ((f = fopen(tmp_name, "w+")) == NULL)
    {
        ERROR("fopen(%s, \"w+\") failed with errno %d", tmp_name, errno);
        return TE_OS_RC(TE_TAPI, errno);
    }

    fprintf(f, "%s", buf);
    fclose(f);

    rc = rcf_ta_csap_create(ta_name, sid, "cli", tmp_name, cli_csap);
    if (rc != 0)
    {
        ERROR("rcf_ta_csap_create() failed(%r) on TA %s:%d file %s",
              rc, ta_name, sid, tmp_name);
    }
    else if ((rc = cfg_synchronize_fmt(TRUE, "/agent:%s/csap:*",
                                       ta_name)) != 0)
    {
        ERROR("%s(): cfg_synchronize_fmt(/agent:%s/csap:*) failed: %r",
              __FUNCTION__, ta_name, rc);
    }

    unlink(tmp_name);

    return rc;
}


static int
tapi_internal_write_cmd_to_file(char *tmp_name, const char *command,
                                tapi_cli_prompt_t cmd_prompt_type,
                                const char *cmd_prompt,
                                tapi_cli_prompt_t passwd_prompt_type,
                                const char *passwd_prompt)
{
    char *buf;
    int   buf_size = TAPI_CLI_CSAP_STR_MAXLEN;
    int   len = 0;
    int   rc;
    FILE *f;

    if (command == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    if ((rc = te_make_tmp_file(tmp_name)) != 0)
        return TE_RC(TE_TAPI, rc);

    buf = (char *)malloc(buf_size);
    if (buf == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);

    VERB("%s() file: %s\n", __FUNCTION__, tmp_name);

    if ((f = fopen(tmp_name, "w+")) == NULL)
    {
        ERROR("fopen(%s, \"w+\" failed with errno %d", tmp_name, errno);
        free(buf);
        return TE_OS_RC(TE_TAPI, errno);
    }

    len += snprintf(buf + len, buf_size - len,
                    "{ pdus { cli : { message plain : \"%s\"", command);

    len += tapi_cli_csap_add_prompts(buf + len, buf_size - len,
                                     cmd_prompt_type, cmd_prompt,
                                     TAPI_CLI_PROMPT_TYPE_PLAIN, NULL, NULL,
                                     passwd_prompt_type, passwd_prompt,
                                     NULL);

    len += snprintf(buf + len, buf_size - len, "} } }");

    fprintf(f, "%s", buf);
    free(buf);

    fclose(f);

    return 0;
}

/**
 * Send specified command to the CSAP's CLI session.
 *
 * @param ta_name       Test Agent name;
 * @param sid           RCF session identifier;
 * @param cli_csap      CSAP handle;
 * @param command       Command to send;
 * @param blk_mode      RCF Blocking mode;
 *
 * @return zero on success, otherwise standard or common TE error code.
 */
static int
tapi_internal_cli_send(const char *ta_name, int sid, csap_handle_t cli_csap,
                       const char *command, rcf_call_mode_t blk_mode)
{
    int  rc = 0;
    char tmp_name[] = "/tmp/te_cli_trsend.XXXXXX";

    if (ta_name == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    if ((rc = tapi_internal_write_cmd_to_file(tmp_name, command,
                                              TAPI_CLI_PROMPT_TYPE_PLAIN, NULL,
                                              TAPI_CLI_PROMPT_TYPE_PLAIN, NULL)) != 0)
    {
        ERROR("Failed to create send pattern for CLI session");
        return rc;
    }

    rc = rcf_ta_trsend_start(ta_name, sid, cli_csap, tmp_name, blk_mode);
    if (rc != 0)
    {
        ERROR("rcf_ta_trsend_start() failed(%r) on TA %s:%d CSAP %d "
              "file %s", rc, ta_name, sid, cli_csap, tmp_name);
    }

    unlink(tmp_name);

    return rc;
}


/**
 * Handler that is used as a callback routine for processing incoming
 * messages
 *
 * @param msg_fname     File name with ASN.1 representation of the received
 *                      packet
 * @param msg_p         Pointer to the placeholder of the CLI message
 *
 * @se It allocates a memory for CLI message
 */
void
tapi_cli_msg_handler(const char *msg_fname, void *user_param)
{
    int                       rc;
    int                       s_parsed;
    asn_value *               cli_response;
#if 1
    const asn_value *         cli_msg;
#endif
    size_t                    msg_len;
    char                     *msg = NULL;
    char                    **msg_p = user_param;

    VERB("%s(): msg_fname=%s, user_param=%x",
         __FUNCTION__, msg_fname, user_param);
    rc = asn_parse_dvalue_in_file(msg_fname, ndn_raw_packet,
                                  &cli_response, &s_parsed);
    if (rc != 0)
    {
        ERROR("Failed to parse ASN.1 text file to ASN.1 value: "
              "rc=%r", rc);
        return;
    }

#if 1
    rc = asn_get_descendent(cli_response, (asn_value **)&cli_msg,
                            "pdus.0.#cli");
    if (rc != 0)
    {
        ERROR("Failed to get 'pdus' from CLI response: %r", rc);
        return;
    }

    msg_len = asn_get_length(cli_msg, "message.#plain");
    if (msg_len <= 0)
    {
        ERROR("Cannot get message.#plain field from CLI response");
        return;
    }

    VERB("Try to get CLI message of %d bytes", msg_len);

    msg = (char *)malloc(msg_len + 1);
    if (msg == NULL)
    {
        ERROR("Cannot allocate enough memory for CLI response");
        return;
    }

    msg[msg_len] = '\0';

    rc = asn_read_value_field(cli_msg, msg,
                              &msg_len, "message.#plain");
    if (rc != 0)
    {
        ERROR("Failed to get message body from CLI response rc=%r", rc);
        free(msg);
        return;
    }
#else
    msg_len = asn_get_length(cli_response, "payload");
    if (msg_len <= 0)
    {
        ERROR("Invalid CLI response payload length");
        return;
    }

    msg = (char *)malloc(msg_len + 1);
    if (msg == NULL)
    {
        ERROR("Cannot allocate enough memory for CLI response");
        return;
    }

    msg[msg_len] = '\0';

    rc = asn_read_value_field(cli_response, msg, &msg_len, "payload");
#endif

    VERB("Received msg : %s", msg);

    *msg_p = msg;
}


/**
 * Send specified command to the CSAP's CLI session and receive response.
 *
 * @param ta_name       Test Agent name;
 * @param sid           RCF session identifier;
 * @param cli_csap      CSAP handle;
 * @param command       Command to send;
 * @param msg           Returned CLI response to command (memory for the
 *                      response is allocated inside this routine);
 * @param timeout       CLI response timeout in seconds;
 *
 * @return zero on success, otherwise standard or common TE error code.
 */
static int
tapi_internal_cli_send_recv(const char *ta_name, int sid,
                            csap_handle_t cli_csap, const char *command,
                            char **msg, unsigned int timeout,
                            tapi_cli_prompt_t cmd_prompt_type,
                            const char *cmd_prompt,
                            tapi_cli_prompt_t passwd_prompt_type,
                            const char *passwd_prompt)
{
    int  rc = 0;
    char tmp_fname[] = "/tmp/te_cli_tr_sendrecv.XXXXXX";
    int  err; /* useless parameter for rcf_ta_trsend_recv() */

    *msg = NULL;

    if (ta_name == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    VERB("%s() started", __FUNCTION__);

    if ((rc = tapi_internal_write_cmd_to_file(tmp_fname, command,
                                              cmd_prompt_type, cmd_prompt,
                                              passwd_prompt_type, passwd_prompt)) != 0)
    {
        ERROR("Failed to create send pattern for CLI session");
        return rc;
    }

    rc = rcf_ta_trsend_recv(ta_name, sid, cli_csap, tmp_fname,
                            tapi_cli_msg_handler, (void *)msg,
                            timeout * 1000, &err);
    if (rc != 0)
    {
        ERROR("rcf_ta_trsend_start() failed(%r) on TA %s:%d CSAP %d "
              "file %s", rc, ta_name, sid, cli_csap, tmp_fname);
    }

#if 0
    VERB("%s() response is : %s", __FUNCTION__, *msg);
#endif

    unlink(tmp_fname);

    VERB("%s() finished", __FUNCTION__);

    return rc;
}

/**
 * Send specified command to the CSAP's CLI session.
 *
 * @param ta_name       Test Agent name;
 * @param sid           RCF session identifier;
 * @param cli_csap      CSAP handle;
 * @param command       Command to send;
 *
 * @return zero on success, otherwise standard or common TE error code.
 */
int
tapi_cli_send(const char *ta_name, int sid,
              csap_handle_t cli_csap,
              const char *command)
{
    return tapi_internal_cli_send(ta_name, sid, cli_csap, command,
                                  RCF_MODE_BLOCKING);
}

/**
 * Send specified command to the CSAP's CLI session and receive response.
 *
 * @param ta_name             Test Agent name;
 * @param sid                 RCF session identifier;
 * @param cli_csap            CSAP handle;
 * @param command             Command to send;
 * @param msg                 Returned CLI response to command (memory for the
 *                            response is allocated inside this routine);
 * @param timeout             CLI response timeout in seconds;
 * @param cmd_prompt_type     Type of command prompt;
 * @param cmd_prompt          Command prompt to use for command run;
 * @param passwd_prompt_type  Type of password prompt;
 * @param passwd_prompt       Password prompt value (NULL to use default);
 *
 * @return zero on success, otherwise standard or common TE error code.
 */
int
tapi_cli_send_recv_with_prompts(const char *ta_name, int sid,
                                csap_handle_t cli_csap, const char *command,
                                char **msg, unsigned int timeout,
                                tapi_cli_prompt_t cmd_prompt_type,
                                const char *cmd_prompt,
                                tapi_cli_prompt_t passwd_prompt_type,
                                const char *passwd_prompt)
{
    return tapi_internal_cli_send_recv(ta_name, sid, cli_csap,
                                       command, msg, timeout,
                                       cmd_prompt_type, cmd_prompt,
                                       passwd_prompt_type, passwd_prompt);
}

