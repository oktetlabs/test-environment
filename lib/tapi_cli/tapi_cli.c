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
 * $Id: $
 */

#include "te_config.h"

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
#include "tapi_cli.h"

#define TE_LGR_USER     "TAPI CLI"
#include "logger_api.h"

#define TAPI_CLI_CSAP_STR_MAXLEN            512
#define TAPI_CLI_CSAP_INIT_FILENAME_MAXLEN  128

#undef TAPI_DEBUG
#define TAPI_DEBUG  1


/** CLI CSAP type names */
const char * const tapi_cli_csap_type_name[] = {"serial", "telnet", "ssh"};

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


/**
 * Add prompts parameters to CLI CSAP initialisation string
 *
 * @param buf               Buffer that contains CLI CSAP initialisation string
 * @param buf_size          Buffer size
 * @param command_prompt    Expected command prompt (when commands may be sent)
 * @param login_prompt      Expected login prompt (when login name may be sent)
 * @param login_name        Login name to be sent if login prompt is detected
 * @param password_prompt   Expected password prompt (when password may be sent)
 * @param password          Password to be sent if password prompt is detected
 *
 * @return length of added parameters string.
 */
static int
tapi_cli_csap_add_prompts(char *buf, int buf_size,
                          const char *command_prompt,
                          const char *login_prompt,
                          const char *login_name,
                          const char *password_prompt,
                          const char *password)
{
    int len = 0;
    
    if (command_prompt != NULL)
        len += snprintf(buf + len, buf_size - len,
                        ", command-prompt plain : \"%s\"", command_prompt);

    if (login_prompt != NULL)
        len += snprintf(buf + len, buf_size - len,
                        ", login-prompt plain : \"%s\"", login_prompt);

    if (login_name != NULL)
        len += snprintf(buf + len, buf_size - len,
                        ", user plain : \"%s\"", login_name);

    if (password_prompt != NULL)
        len += snprintf(buf + len, buf_size - len,
                        ", password-prompt plain : \"%s\"", password_prompt);

    if (password != NULL)
        len += snprintf(buf + len, buf_size - len,
                        ", password plain : \"%s\"", password);

    return len;
}


/**
 * Create common CLI CSAP on local device (using millicom).
 *
 * @param ta_name           Test Agent name
 * @param sid               RCF session
 * @param device            Local device name
 * @param command_prompt    Expected command prompt (when commands may be sent)
 * @param login_prompt      Expected login prompt (when login name may be sent)
 * @param login_name        Login name to be sent if login prompt is detected
 * @param password_prompt   Expected password prompt (when password may be sent)
 * @param password          Password to be sent if password prompt is detected
 * @param cli_csap          Identifier of created CSAP (OUT)
 *
 * @return 0 on success, otherwise standard or common TE error code.
 */
extern int
tapi_cli_csap_local_create(const char *ta_name, int sid,
                    const char *device,
                    const char *command_prompt,
                    const char *login_prompt,
                    const char *login_name,
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
                                     command_prompt, login_prompt, login_name,
                                     password_prompt, password);

    len += snprintf(buf + len, buf_size - len, " } }");

    rc = tapi_cli_csap_create (ta_name, sid, buf, cli_csap);
    
    free(buf);
    
    return rc;
}

/**
 * Create common CLI CSAP on remote connection (telnet or ssh).
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF session
 * @param type          Remote connection type (telnet or ssh)
 * @param host          Remote host name
 * @param port          Remote port number
 * @param command_prompt    Expected command prompt (when commands may be sent)
 * @param login_prompt      Expected login prompt (when login name may be sent)
 * @param login_name        Login name to be sent if login prompt is detected
 * @param password_prompt   Expected password prompt (when password may be sent)
 * @param password          Password to be sent if password prompt is detected
 * @param cli_csap      Identifier of created CSAP (OUT)
 *
 * @return 0 on success, otherwise standard or common TE error code.
 */
extern int
tapi_cli_csap_remote_create(const char *ta_name, int sid,
                            int type, const char *host, int port,
                            const char *command_prompt,
                            const char *login_prompt,
                            const char *login_name,
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
                                     command_prompt, login_prompt, login_name,
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
    int     rc;
    char   *fname;
    FILE   *f;

    if ((ta_name == NULL) || (buf == NULL) || (cli_csap == NULL))
        return TE_RC(TE_TAPI, EINVAL);

    VERB("%s(): %s\n", __FUNCTION__, buf);

    fname = (char *)malloc(TAPI_CLI_CSAP_INIT_FILENAME_MAXLEN);
    if (fname == NULL)
        return TE_RC(TE_TAPI, ENOMEM);

    strcpy(fname, "/tmp/te_cli_csap_create.XXXXXX");
    mkstemp(fname);

    VERB("file: %s\n", fname);

    f = fopen(fname, "w+");
    if (f == NULL)
    {
        ERROR("fopen() of %s failed(%d)", fname, errno);

        free(fname);

        return TE_RC(TE_TAPI, errno); /* return system errno */
    }

    fprintf(f, "%s", buf);

    fclose(f);

    rc = rcf_ta_csap_create(ta_name, sid, "cli", fname, cli_csap);
    if (rc != 0)
    {
        ERROR("rcf_ta_csap_create() failed(0x%x) on TA %s:%d file %s",
              rc, ta_name, sid, fname);
    }

#if !(TAPI_DEBUG)
    unlink(fname);
#endif

    free(fname);

    return rc;
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
    int   rc = 0;
    char *fname;
    FILE *f;

    if ( (ta_name == NULL) || (command == NULL) )
        return TE_RC(TE_TAPI, EINVAL);

    fname = (char *)malloc(TAPI_CLI_CSAP_INIT_FILENAME_MAXLEN);
    if (fname == NULL)
        return TE_RC(TE_TAPI, ENOMEM);

    strcpy(fname, "/tmp/te_cli_trsend.XXXXXX");
    mkstemp(fname);

    VERB("file: %s\n", fname);

    f = fopen(fname, "w+");
    if (f == NULL)
    {
        ERROR("fopen() of %s failed(%d)", fname, errno);

        free(fname);

        return TE_RC(TE_TAPI, errno); /* return system errno */
    }

    fprintf(f, "{ pdus { cli : { message plain : \"%s\" } } }", command);

    fclose(f);

    rc = rcf_ta_trsend_start(ta_name, sid, cli_csap, fname, blk_mode);
    if (rc != 0)
    {
        ERROR("rcf_ta_trsend_start() failed(0x%x) on TA %s:%d CSAP %d "
              "file %s", rc, ta_name, sid, cli_csap, fname);
    }

#if !(TAPI_DEBUG)
    unlink(fname);
#endif

    free(fname);

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

