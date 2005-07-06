/** @file
 * @brief Proteos, Tester API for CLI CSAP.
 *
 * Implementation of Tester API for CLI CSAP.
 *
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 *
 * $Id$
 */

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


#define TE_LGR_USER     "TAPI CLI"
#include "logger_api.h"

#include "tapi_cli.h"

#define TAPI_CLI_CSAP_STR_MAXLEN            512
#define TAPI_CLI_CSAP_INIT_FILENAME_MAXLEN  128

#undef TAPI_DEBUG
#define TAPI_DEBUG  0

#if 0
#define CLI_DEBUG(args...) \
    do {                                        \
        fprintf(stdout, "\nTAPI_CLI " args);    \
        INFO("TAPI_CLI " args);                 \
    } while (0)

#undef ERROR
#define ERROR(args...) CLI_DEBUG("ERROR: " args)

#undef RING
#define RING(args...) CLI_DEBUG("RING: " args)

#undef WARN
#define WARN(args...) CLI_DEBUG("WARN: " args)

#undef VERB
#define VERB(args...) CLI_DEBUG("VERB: " args)
#endif

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
 *                              (when login name may be sent)
 * @param login_name            Login name to be sent if login
 *                              prompt is detected
 * @param password_prompt_type  Expected password prompt type
 *                              (plain or regular expression)
 * @param password_prompt       Expected password prompt
 *                              (when password may be sent)
 * @param password              Password to be sent if password
 *                              prompt is detected
 *
 * @return length of added parameters string.
 */
static int
tapi_cli_csap_add_prompts(char *buf, int buf_size,
                          tapi_cli_prompt_t command_prompt_type,
                          const char *command_prompt,
                          tapi_cli_prompt_t login_prompt_type,
                          const char *login_prompt,
                          const char *login_name,
                          tapi_cli_prompt_t password_prompt_type,
                          const char *password_prompt,
                          const char *password)
{
    int len = 0;
    
    if (command_prompt != NULL)
        len += snprintf(buf + len, buf_size - len,
                        ", command-prompt %s : \"%s\"",
                        (command_prompt_type == TAPI_CLI_PROMPT_TYPE_REG_EXP) ?
                        "script" : "plain",
                        command_prompt);

    if (login_prompt != NULL)
        len += snprintf(buf + len, buf_size - len,
                        ", login-prompt %s : \"%s\"",
                        (login_prompt_type == TAPI_CLI_PROMPT_TYPE_REG_EXP) ?
                        "script" : "plain",
                        login_prompt);

    if (login_name != NULL)
        len += snprintf(buf + len, buf_size - len,
                        ", user plain : \"%s\"", login_name);

    if (password_prompt != NULL)
        len += snprintf(buf + len, buf_size - len,
                        ", password-prompt %s : \"%s\"",
                        (password_prompt_type == TAPI_CLI_PROMPT_TYPE_REG_EXP) ?
                        "script" : "plain",
                        password_prompt);

    if (password != NULL)
        len += snprintf(buf + len, buf_size - len,
                        ", password plain : \"%s\"", password);

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
        return TE_RC(TE_TAPI, ENOMEM);

    len += snprintf(buf + len, buf_size - len,
                    "{ cli : { conn-type %d,"
                    "          conn-params serial : { device plain : \"%s\" }",
                    type, device);

    len += tapi_cli_csap_add_prompts(buf + len, buf_size - len,
                                     command_prompt_type, command_prompt,
                                     login_prompt_type,
                                     login_prompt, login_name,
                                     password_prompt_type,
                                     password_prompt, password);

    len += snprintf(buf + len, buf_size - len, " } }");

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
        return TE_RC(TE_TAPI, ENOMEM);

    len += snprintf(buf + len, buf_size - len,
                    "{ cli : { conn-type %d,"
                    "          conn-params telnet : { host plain : \"%s\","
                    "                                 port plain : %d }",
                    type, host, port);

    len += tapi_cli_csap_add_prompts(buf + len, buf_size - len,
                                     command_prompt_type, command_prompt,
                                     login_prompt_type,
                                     login_prompt, login_name,
                                     password_prompt_type,
                                     password_prompt, password);

    len += snprintf(buf + len, buf_size - len, " } }");

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
        return TE_RC(TE_TAPI, ENOMEM);

    len += snprintf(buf + len, buf_size - len,
                    "{ cli : { conn-type %d,"
                    "          conn-params shell : { args plain : \"%s\" }",
                    type, shell_args);

    len += tapi_cli_csap_add_prompts(buf + len, buf_size - len,
                                     command_prompt_type, command_prompt,
                                     login_prompt_type,
                                     login_prompt, login_name,
                                     password_prompt_type,
                                     password_prompt, password);

    len += snprintf(buf + len, buf_size - len, " } }");

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
        return TE_RC(TE_TAPI, EINVAL);

    if ((rc = te_make_tmp_file(tmp_name)) != 0)
        return TE_RC(TE_TAPI, rc);

    if ((f = fopen(tmp_name, "w+")) == NULL)
    {
        ERROR("fopen(%s, \"w+\") failed with errno %d", tmp_name, errno);
        return TE_RC(TE_TAPI, errno);
    }

    fprintf(f, "%s", buf);
    fclose(f);

    rc = rcf_ta_csap_create(ta_name, sid, "cli", tmp_name, cli_csap);
    if (rc != 0)
    {
        ERROR("rcf_ta_csap_create() failed(0x%x) on TA %s:%d file %s",
              rc, ta_name, sid, tmp_name);
    }

#if !(TAPI_DEBUG)
    unlink(tmp_name);
#endif

    return rc;
}


static int
tapi_internal_write_cmd_to_file(char *tmp_name, const char *command)
{
    int   rc;
    FILE *f;

    if (command == NULL)
        return TE_RC(TE_TAPI, EINVAL);

    if ((rc = te_make_tmp_file(tmp_name)) != 0)
        return TE_RC(TE_TAPI, rc);

    VERB("%s() file: %s\n", __FUNCTION__, tmp_name);

    if ((f = fopen(tmp_name, "w+")) == NULL)
    {
        ERROR("fopen(%s, \"w+\" failed with errno %d", tmp_name, errno);
        return TE_RC(TE_TAPI, errno);
    }

    fprintf(f, "{ pdus { cli : { message plain : \"%s\" } } }", command);
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
        return TE_RC(TE_TAPI, EINVAL);

    if ((rc = tapi_internal_write_cmd_to_file(tmp_name, command)) != 0)
    {
        ERROR("Failed to create send pattern for CLI session");
        return rc;
    }

    rc = rcf_ta_trsend_start(ta_name, sid, cli_csap, tmp_name, blk_mode);
    if (rc != 0)
    {
        ERROR("rcf_ta_trsend_start() failed(0x%x) on TA %s:%d CSAP %d "
              "file %s", rc, ta_name, sid, cli_csap, tmp_name);
    }

#if !(TAPI_DEBUG)
    unlink(tmp_name);
#endif

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
tapi_cli_msg_handler(char *msg_fname, void *user_param)
{
    int                       rc;
    int                       s_parsed;
    asn_value_p               cli_response;
#if 0
    asn_value_p               cli_msg;
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
              "rc=0x%x", rc);
        return;
    }

#if 0
    cli_msg = asn_read_indexed(cli_response, 0, "pdus");
    if (cli_msg == NULL)
    {
        ERROR("Failed to get 'pdus' from CLI response");
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
        ERROR("Failed to get message body from CLI response rc=0x%x", rc);
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
                            char **msg, unsigned int timeout)
{
    int  rc = 0;
    char tmp_fname[] = "/tmp/te_cli_tr_sendrecv.XXXXXX";
    int  err; /* useless parameter for rcf_ta_trsend_recv() */

    *msg = NULL;

    if (ta_name == NULL)
        return TE_RC(TE_TAPI, EINVAL);

    VERB("%s() started", __FUNCTION__);

    if ((rc = tapi_internal_write_cmd_to_file(tmp_fname, command)) != 0)
    {
        ERROR("Failed to create send pattern for CLI session");
        return rc;
    }

    rc = rcf_ta_trsend_recv(ta_name, sid, cli_csap, tmp_fname,
                            tapi_cli_msg_handler, (void *)msg,
                            timeout * 1000, &err);
    if (rc != 0)
    {
        ERROR("rcf_ta_trsend_start() failed(0x%x) on TA %s:%d CSAP %d "
              "file %s", rc, ta_name, sid, cli_csap, tmp_fname);
    }

#if 0
    VERB("%s() response is : %s", __FUNCTION__, *msg);
#endif

#if !(TAPI_DEBUG)
    unlink(tmp_fname);
#endif

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
int
tapi_cli_send_recv(const char *ta_name, int sid,
                   csap_handle_t cli_csap, const char *command,
                   char **msg, unsigned int timeout)
{
    return tapi_internal_cli_send_recv(ta_name, sid, cli_csap,
                                       command, msg, timeout);
}

