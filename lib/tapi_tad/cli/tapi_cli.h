/** @file
 * @brief Proteos, TAPI CLI.
 *
 * Declarations of API for TAPI CLI.
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS in the
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

#ifndef __TE_TAPI_CLI_H__
#define __TE_TAPI_CLI_H__

#include <stdio.h>
#if HAVE_ASSERT_H
#include <assert.h>
#endif

#include "te_stdint.h"
#include "te_defs.h"
#include "te_errno.h"
#include "tad_common.h"
#include "asn_usr.h"
#include "ndn_cli.h"

/** Default ssh port is 22 */
#define TAPI_CLI_SSH_PORT_DFLT      22

/** Default telnet port is 23 */
#define TAPI_CLI_TELNET_PORT_DFLT   23

/** CLI CSAP type definition */
typedef enum {
    TAPI_CLI_CSAP_TYPE_SERIAL = 0, /**< Serial connection */
    TAPI_CLI_CSAP_TYPE_TELNET = 1, /**< Telnet connection */
    TAPI_CLI_CSAP_TYPE_SSH    = 2, /**< SSH connection */
    TAPI_CLI_CSAP_TYPE_SHELL  = 3, /**< Shell console */
} tapi_cli_csap_type;

/** TAPI CLI prompt type definition */
typedef enum {
    TAPI_CLI_PROMPT_TYPE_PLAIN = 0,     /**< Plain prompt */
    TAPI_CLI_PROMPT_TYPE_REG_EXP = 1,   /**< Regular expression prompt */
} tapi_cli_prompt_t;

/** CLI CSAP type names */
extern const char * const tapi_cli_csap_type_name[];

/** Default command prompt on redhat is '[...]$ ' */
extern const char * const tapi_cli_redhat_cprompt_dflt;

/** Default command prompt on debian is '...$ ' */
extern const char * const tapi_cli_debian_cprompt_dflt;

/** Default login prompt for serial console is '[L|l]ogin: ' */
extern const char * const tapi_cli_serial_lprompt_dflt;

/** Default password prompt for serial console is '[P|p]assword: ' */
extern const char * const tapi_cli_serial_pprompt_dflt;

/** Default login prompt for telnet console is 'Login: ' */
extern const char * const tapi_cli_telnet_lprompt_dflt;

/** Default password prompt for telnet console is '[P|p]assword: ' */
extern const char * const tapi_cli_telnet_pprompt_dflt;

/** There is no default login prompt for ssh console */
extern const char * const tapi_cli_ssh_lprompt_dflt;

/** Default password prompt for ssh console is '[P|p]assword: ' */
extern const char * const tapi_cli_ssh_pprompt_dflt;

/** Default login prompt for shell console is '[L|l]ogin: ' */
extern const char * const tapi_cli_shell_lprompt_dflt;

/** Default password prompt for shell console is '[P|p]assword: ' */
extern const char * const tapi_cli_shell_pprompt_dflt;

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
extern int tapi_cli_csap_local_create(const char *ta_name, int sid,
                                      const char *device,
                                      tapi_cli_prompt_t command_prompt_type,
                                      const char *command_prompt,
                                      tapi_cli_prompt_t login_prompt_type,
                                      const char *login_prompt,
                                      const char *login_name,
                                      tapi_cli_prompt_t password_prompt_type,
                                      const char *password_prompt,
                                      const char *password,
                                      csap_handle_t *cli_csap);


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
extern int tapi_cli_csap_remote_create(const char *ta_name, int sid,
                                       int type, const char *host, int port,
                                       tapi_cli_prompt_t command_prompt_type,
                                       const char *command_prompt,
                                       tapi_cli_prompt_t login_prompt_type,
                                       const char *login_prompt,
                                       const char *login_name,
                                       tapi_cli_prompt_t password_prompt_type,
                                       const char *password_prompt,
                                       const char *password,
                                       csap_handle_t *cli_csap);

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
extern int tapi_cli_csap_shell_create(const char *ta_name, int sid,
                                      const char *shell_args,
                                      tapi_cli_prompt_t command_prompt_type,
                                      const char *command_prompt,
                                      tapi_cli_prompt_t login_prompt_type,
                                      const char *login_prompt,
                                      const char *login_name,
                                      tapi_cli_prompt_t password_prompt_type,
                                      const char *password_prompt,
                                      const char *password,
                                      csap_handle_t *cli_csap);


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
extern int tapi_cli_csap_create(const char *ta_name, int sid,
                                const char *buf, csap_handle_t *cli_csap);


/**
 * Sends CLI command according specified template from the CSAP.
 * This function is blocking, returns after all commands are sent and
 * CSAP operation finished.
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF session identifier
 * @param cli_csap      CSAP handle
 * @param command       Command string to be sent to CLI session
 *
 * @return zero on success, otherwise standard or common TE error code.
 */
extern int tapi_cli_send(const char *ta_name, int sid,
                         csap_handle_t cli_csap,
                         const char *command);

/**
 * Send specified command to the CSAP's CLI session and receive response.
 * This function is blocking, returns after all commands are sent and
 * response is received or timeout ocured.
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
extern int tapi_cli_send_recv(const char *ta_name, int sid,
                              csap_handle_t cli_csap, const char *command,
                              char **msg, unsigned int timeout);


/**
 * Macro around tapi_cli_csap_local_create()
 *
 * @param  ta_name    Test agent name
 * @param  sid        RCF session id
 * @param  device     local device name
 * @param  user       Login name to be sent if login prompt is detected 
 * @param  pwd        Password to be sent if password prompt is detected
 * @param  cprompt    Expected command prompt (when commands may be sent)
 * @param  cli_csap   Identifier of created CSAP
 *
 */
#define TAPI_CLI_CSAP_CREATE_SERIAL(ta_name, sid, device, user, pwd,           \
                                    cprompt, cli_csap)                         \
    do {                                                                       \
        CHECK_RC(                                                              \
            tapi_cli_csap_local_create(ta_name, sid, device,                   \
                                       TAPI_CLI_PROMPT_TYPE_PLAIN, cprompt,    \
                                       TAPI_CLI_PROMPT_TYPE_PLAIN,             \
                                       tapi_cli_serial_lprompt_dflt, user,     \
                                       TAPI_CLI_PROMPT_TYPE_PLAIN,             \
                                       tapi_cli_serial_pprompt_dflt, pwd,      \
                                       cli_csap)                               \
        );                                                                     \
    } while (0)


/**
 * Macro around tapi_cli_csap_remote_create() when connection type is 
 * @c TAPI_CLI_CSAP_TYPE_TELNET
 *
 * @param  ta_name    Test agent name
 * @param  sid        RCF session id
 * @param  host       Remote host name
 * @param  user       Login name to be sent if login prompt is detected 
 * @param  pwd        Password to be sent if password prompt is detected
 * @param  cprompt    Expected command prompt (when commands may be sent)
 * @param  cli_csap   Identifier of created @c CSAP
 *
 */
#define TAPI_CLI_CSAP_CREATE_TELNET(ta_name, sid, host, user, pwd,             \
                                    cprompt, cli_csap)                         \
    do {                                                                       \
        CHECK_RC(                                                              \
            tapi_cli_csap_remote_create(ta_name, sid,                          \
                                        TAPI_CLI_CSAP_TYPE_TELNET,             \
                                        host, TAPI_CLI_TELNET_PORT_DFLT,       \
                                        TAPI_CLI_PROMPT_TYPE_PLAIN, cprompt,   \
                                        TAPI_CLI_PROMPT_TYPE_PLAIN,            \
                                        tapi_cli_telnet_lprompt_dflt, user,    \
                                        TAPI_CLI_PROMPT_TYPE_PLAIN,            \
                                        tapi_cli_telnet_pprompt_dflt, pwd,     \
                                        cli_csap)                              \
        );                                                                     \
    } while (0)

/**
 * Macro around tapi_cli_csap_remote_create() when connection type is 
 * @c TAPI_CLI_CSAP_TYPE_SSH
 *
 * @param  ta_name    Test agent name
 * @param  sid        RCF session id
 * @param  host       Remote host name
 * @param  user       Login name to be sent if login prompt is detected 
 * @param  pwd        Password to be sent if password prompt is detected
 * @param  cprompt    Expected command prompt (when commands may be sent)
 * @param  cli_csap   Identifier of created @c CSAP
 *
 */
#define TAPI_CLI_CSAP_CREATE_SSH(ta_name, sid, host, user, pwd,                \
                                 cprompt, cli_csap)                            \
    do {                                                                       \
        CHECK_RC(                                                              \
            tapi_cli_csap_remote_create(ta_name, sid, TAPI_CLI_CSAP_TYPE_SSH,  \
                                        host, TAPI_CLI_SSH_PORT_DFLT,          \
                                        TAPI_CLI_PROMPT_TYPE_PLAIN, cprompt,   \
                                        TAPI_CLI_PROMPT_TYPE_PLAIN, NULL, user,\
                                        TAPI_CLI_PROMPT_TYPE_PLAIN,            \
                                        tapi_cli_ssh_pprompt_dflt, pwd,        \
                                        cli_csap)                              \
        );                                                                     \
    } while (0)

/**
 * Macro around tapi_cli_csap_shell_create() when connection type is 
 * @c TAPI_CLI_CSAP_TYPE_SHELL
 *
 * @param  ta_name    Test agent name
 * @param  sid        RCF session id
 * @param  shell_args Shell arguments
 * @param  user       Login name to be sent if login prompt is detected 
 * @param  pwd        Password to be sent if password prompt is detected
 * @param  cprompt    Expected command prompt (when commands may be sent)
 * @param  cli_csap   Identifier of created @c CSAP
 *
 */
#define TAPI_CLI_CSAP_CREATE_SHELL(ta_name, sid, shell_args, cprompt,          \
                                   lprompt, user, pprompt, pwd, cli_csap)      \
    do {                                                                       \
        CHECK_RC(                                                              \
            tapi_cli_csap_shell_create(ta_name, sid, shell_args,               \
                                       TAPI_CLI_PROMPT_TYPE_PLAIN, cprompt,    \
                                       TAPI_CLI_PROMPT_TYPE_PLAIN,             \
                                       lprompt, user,                          \
                                       TAPI_CLI_PROMPT_TYPE_PLAIN,             \
                                       pprompt, pwd, cli_csap)                 \
        );                                                                     \
    } while (0)

#endif /* __TE_TAPI_CLI_H__ */
