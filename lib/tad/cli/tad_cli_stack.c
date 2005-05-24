/** @file
 * @brief Test Environment: 
 *
 * Traffic Application Domain Command Handler
 * CLI CSAP, stack-related callbacks.
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAD CLI STACK"

#include "te_config.h"

#if HAVE_ASSERT_H
#include <assert.h>
#else
#define assert(x) (x)
#endif
#include "tad_cli_impl.h"

#define CLI_COMMAND_PROMPT  0
#define CLI_LOGIN_PROMPT    1
#define CLI_PASSWORD_PROMPT 2

#define CLI_PROGRAM_NAME_SIZE           40
#define CLI_SESSION_PARAM_LENGTH_MAX    40

#define CLI_REMOVE_ECHO     1

#define EXP_DEBUG
#undef EXP_DEBUG

static char cli_programs[][CLI_PROGRAM_NAME_SIZE] = {
    "cu",
    "telnet",
    "ssh",
    "sh"};

/** Synchronization result between Expect and CSAP Engine processes */
typedef enum cli_sync_res {
    SYNC_RES_OK, /**< Expect side is ready to run the following command */
    SYNC_RES_FAILED, /**< Expect side encountered unexpected error */
    SYNC_RES_ABORTED, /**< Programm run under control of Expect side 
                           unexpectedly terminates */
    SYNC_RES_TIMEOUT, /**< Timeout waiting for prompt */
    SYNC_RES_INT_ERROR, /**< Synchronization fails due to an error 
                             encountered on CSAP Engine side */
} cli_sync_res_t;

/** Mapping of cli_sync_res_t values into errno */
#define MAP_SYN_RES2ERRNO(sync_res_) \
    (((sync_res_) == SYNC_RES_OK) ? 0 :                 \
     ((sync_res_) == SYNC_RES_FAILED) ? EREMOTEIO :     \
     ((sync_res_) == SYNC_RES_ABORTED) ? ECONNABORTED : \
     ((sync_res_) == SYNC_RES_INT_ERROR) ? EFAULT :     \
     ((sync_res_) == SYNC_RES_TIMEOUT) ? ETIMEDOUT :    \
         (assert(0), 0))

/* Forward declarations */
static int cli_expect_main(cli_csap_specific_data_p spec_data);


/**
 * Convert CLI conection type into string representation.
 *
 * @param conn_type  Connection type
 *
 * @return Connection type in string representation.
 */
static const char *
csap_conn_type_h2str(cli_conn_type_t conn_type)
{
    switch (conn_type)
    {
#define CONN_TYPE_H2STR(conn_type_) \
        case CLI_CONN_TYPE_ ## conn_type_: return #conn_type_

        CONN_TYPE_H2STR(SERIAL);
        CONN_TYPE_H2STR(TELNET);
        CONN_TYPE_H2STR(SSH);
        CONN_TYPE_H2STR(SHELL);

#undef CONN_TYPE_H2STR

        default:
            return "UNKNOWN";
    }

    return "";
}

static const char *
sync_res_h2str(cli_sync_res_t sync_res)
{
    switch (sync_res)
    {
#define SYNC_RES_H2STR(sync_res_) \
        case SYNC_RES_ ## sync_res_: return #sync_res_

        SYNC_RES_H2STR(OK);
        SYNC_RES_H2STR(FAILED);
        SYNC_RES_H2STR(ABORTED);
        SYNC_RES_H2STR(TIMEOUT);
        SYNC_RES_H2STR(INT_ERROR);

#undef SYNC_RES_H2STR

        default:
            return "UNKNOWN";
    }

    return "";
}


/**
 * Reads synchronization data mark from Expect side.
 *
 * @param spec_data  CSAP-specific data pointer
 *
 * @return Synchronization data mark value.
 */
static cli_sync_res_t
parent_wait_sync(cli_csap_specific_data_p spec_data)
{
    cli_sync_res_t sync_val;
    fd_set         read_set;
    int            rc;
    
    FD_ZERO(&read_set);
    FD_SET(spec_data->sync_pipe, &read_set);

    /* Wait for sync mark indefinitely */
    rc = select(spec_data->sync_pipe + 1, &read_set, NULL, NULL, NULL);

    if (rc != 1)
    {
        ERROR("Synchronization with Expect side fails on select(), "
              "errno = %d", errno);
        return SYNC_RES_INT_ERROR;
    }

    rc = read(spec_data->sync_pipe, &sync_val, sizeof(sync_val));
    if (rc != (ssize_t)sizeof(sync_val))
    {
        ERROR("Synchronization with Expect side fails on read(), "
              "rc = %d, errno = %d", rc, errno);
        return SYNC_RES_INT_ERROR;
    }

    return sync_val;
}

/**
 * Read one byte from Expect side.
 *
 * @param spec_data  CSAP-specific data pointer
 * @param data       Single byte read from Expect side (OUT)
 *
 * @return Status of the operation.
 *
 * @se If it captures an error from Expect side, it logs error message
 * and return correspnding errno value as its return value.
 */
static int 
parent_read_byte(cli_csap_specific_data_p spec_data, char *data)
{
    fd_set read_set;
    int    max_descr;
    int    rc;
    
    max_descr = spec_data->sync_pipe > spec_data->data_sock ?
        spec_data->sync_pipe : spec_data->data_sock;

    while (TRUE)
    {
        FD_ZERO(&read_set);
        FD_SET(spec_data->sync_pipe, &read_set);
        FD_SET(spec_data->data_sock, &read_set);

        rc = select(max_descr + 1, &read_set, NULL, NULL, NULL);
        if (rc == -1 && errno == EINTR)
            continue;

        if (rc < 0)
        {
            ERROR("Reading single character from Expect side "
                  "fails on select(), errno = %X", errno);
            return errno;
        }
        
        if (FD_ISSET(spec_data->sync_pipe, &read_set))
        {
            cli_sync_res_t sync_val;

            /* 
             * @todo Probably we need to read out everything
             * from 'data_sock'.
             */

            /* Got sync mark from Expect side */
            sync_val = parent_wait_sync(spec_data);
            WARN("Get %s sync mark from Expect side",
                 sync_res_h2str(sync_val));
            return MAP_SYN_RES2ERRNO(sync_val);
        }
        else if (FD_ISSET(spec_data->data_sock, &read_set))
        {
            if ((rc = read(spec_data->data_sock, data, 1)) != 1)
            {
                ERROR("Reading single character from Expect side "
                      "fails on read(), rc = %d, errno = %X", rc, errno);
                return (rc == 0) ? ECONNABORTED : errno;
            }
        }
        else
        {
            ERROR("select() returns non-zero value, but there is no "
                  "readable descriptor");
            return EFAULT;
        }

        return 0;
    }
    
    /* We never reach this code */
    return 0;
}

/**
 * Read the STRING type value from the CSAP description (in ASN.1 notation).
 *
 * @param csap_spec    CSAP description (in ASN.1 notation).
 * @param asn_name     Name of the field to read.
 * @param str_value    Pointer to returned string.
 *
 * @return 0 on success or -1 if not found. 
 */ 
int cli_get_asn_string_value(asn_value * csap_spec,
                             const char *asn_name,
                             char **str_value)
{
    int rc = 0;
    int tmp_len = asn_get_length(csap_spec, asn_name);
    if (tmp_len > 0)
    {
        /* allocate memory for the string */
        *str_value = (char *) malloc(tmp_len + 1);
        if (*str_value == NULL)
            return ENOMEM;

        rc = asn_read_value_field(csap_spec, *str_value,
                                  &tmp_len, asn_name);
        if (rc != 0)
        {
            free(*str_value);
            *str_value = NULL;
            rc = EINVAL;
        }
        else
        {
            (*str_value)[tmp_len] = '\0';
        }
    }
    else
    {
        rc = EINVAL;
    }

    return rc;
}

/**
 * Read the INTEGER type value from the CSAP description (in ASN.1 notation).
 *
 * @param csap_spec    CSAP description (in ASN.1 notation).
 * @param asn_name     Name of the field to read.
 * @param str_value    Pointer to returned integer.
 *
 * @return 0 on success or -1 if not found.
 */ 
int cli_get_asn_integer_value(asn_value * csap_spec,
                              const char *asn_name,
                              int *int_value)
{
    int rc = 0;
    int tmp_len = sizeof(int);

    rc = asn_read_value_field(csap_spec, int_value, &tmp_len, asn_name);
    if (rc != 0)
    {
        rc = EINVAL;
    }

    return rc;
}


/**************************************************************************
 *
 * Routines executed in the CSAP layer process context
 *
 **************************************************************************/

/**
 * Free all memory allocated by CLI CSAP specific data
 *
 * @param cli_csap_specific_data_p poiner to structure
 *
 */ 
void 
free_cli_csap_data(cli_csap_specific_data_p spec_data)
{
    int i;

    VERB("%s() started", __FUNCTION__);

    if (spec_data->data_sock >= 0)
        close(spec_data->data_sock);

    if (spec_data->sync_pipe >= 0)
        close(spec_data->sync_pipe);

    free(spec_data->device);
    free(spec_data->host);
    free(spec_data->shell_args);
    free(spec_data->user);
    free(spec_data->password);

    for (i = 0; i < CLI_MAX_PROMPTS; i++)
    {
        free(spec_data->prompts[i].pattern);
        free(spec_data->prompts[i].re);
    }

    free(spec_data);   
}


/**
 * Callback for read data from media of CLI CSAP. 
 *
 * @param csap_id       identifier of CSAP.
 * @param timeout       timeout of waiting for data in microseconds.
 * @param buf           buffer for read data.
 * @param buf_len       length of available buffer.
 *
 * @return 
 *      quantity of read octets, or -1 if error occured, 0 if timeout expired. 
 */ 
int 
cli_read_cb(csap_p csap_descr, int timeout, char *buf, size_t buf_len)
{
    UNUSED(csap_descr);
    UNUSED(timeout);
    UNUSED(buf);
    UNUSED(buf_len);

    return -EOPNOTSUPP;
}

/**
 * Check whether CLI session is still alive.
 *
 * @param spec_data  CSAP-specific data
 *
 * @return TRUE if CLI session is running, and FALSE otherwise.
 */ 
te_bool
cli_session_alive(cli_csap_specific_data_p spec_data)
{
    pid_t pid;
    int   status;

    pid = waitpid(spec_data->expect_pid, &status, WNOHANG);
    if (pid < 0)
    {
        ERROR("waitpid(%d) failed, errno = %X",
              spec_data->expect_pid, errno);
        return FALSE;
    }
    else if (pid == 0)
    {
        VERB("The child with PID %d is still alive", spec_data->expect_pid);
        return TRUE;
    }
    else
    {
        VERB("The child with PID %d is finished", spec_data->expect_pid);
        return FALSE;
    }
    
    assert(0);
    return FALSE;
}

/**
 * Callback for write data to media of CLI CSAP. 
 *
 * @param csap_id       identifier of CSAP.
 * @param buf           buffer with data to be written.
 * @param buf_len       length of data in buffer.
 *
 * @return 
 *      quantity of written octets, or -1 if error occured. 
 */ 
int 
cli_write_cb(csap_p csap_descr, char *buf, size_t buf_len)
{
    cli_csap_specific_data_p    spec_data;
    int     layer;    

    char    data = -1;
    size_t  bytes_written = 0;
    int     rc = 0;
    
    if (csap_descr == NULL)
    {
        return -1;
    }

    VERB("%s() started", __FUNCTION__);
    
    layer = csap_descr->read_write_layer;
    
    spec_data = (cli_csap_specific_data_p)
        csap_descr->layers[layer].specific_data; 

    assert(spec_data->io >= 0);

    if (!cli_session_alive(spec_data))
    {
        ERROR("CLI session is not running");
        return -TE_RC(TE_TAD_CSAP, ENOTCONN);
    }

    bytes_written = write(spec_data->data_sock, buf, buf_len);
    if (bytes_written != buf_len)
    {
        /* something wrong with sync_p2c pipe */
        ERROR("%s(): write to sync_p2c[1] failed, rc=%d",
              __FUNCTION__, bytes_written);
        return -TE_RC(TE_TAD_CSAP, EFAULT);
    }
    
    /* Wait for CLI response */
    do {
        rc = parent_read_byte(spec_data, &data);
        if (rc == ETIMEDOUT)
        {
            WARN("%s(): We can wait forever on timeout", __FUNCTION__);
            continue;
        }
        else if (rc != 0)
            return -1;
    } while (data != '\0');

    VERB("%s() finished", __FUNCTION__);

    return bytes_written;
}

/**
 * Callback for write data to media of CLI CSAP and read
 * data from media just after write, to get answer to sent request. 
 *
 * @param csap_id       identifier of CSAP.
 * @param timeout       timeout of waiting for data.
 * @param w_buf         buffer with data to be written.
 * @param w_buf_len     length of data in w_buf.
 * @param r_buf         buffer for data to be read.
 * @param r_buf_len     available length r_buf.
 *
 * @return 
 *      quantity of read octets, or -1 if error occured, 0 if timeout expired. 
 */ 
int 
cli_write_read_cb(csap_p csap_descr, int timeout,
                  char *w_buf, size_t w_buf_len,
                  char *r_buf, size_t r_buf_len)
{
    cli_csap_specific_data_p    spec_data;

    int    layer;    
    char   data = -1;
    size_t bytes_written = 0;
    size_t bytes_read = 0;
    int    rc = 0;

#if CLI_REMOVE_ECHO
    char   *echo_p = w_buf;
    size_t  echo_count = 0;
#endif

    /* XXX: timeout should be used */
    UNUSED(timeout);

    if (csap_descr == NULL)
    {
        return -1;
    }
    
    layer = csap_descr->read_write_layer;
    
    spec_data = (cli_csap_specific_data_p)
        csap_descr->layers[layer].specific_data; 

    assert(spec_data->io >= 0);

    if (!cli_session_alive(spec_data))
    {
        ERROR("CLI session is not running");
        return -TE_RC(TE_TAD_CSAP, ENOTCONN);
    }

    bytes_written = write(spec_data->data_sock, w_buf, w_buf_len);
    if (bytes_written != w_buf_len)
    {
        ERROR("%s(): Cannot write '%s' command to Expect side, "
              "rc = %d, errno = %X",
              __FUNCTION__, w_buf, bytes_written, errno);
        return -TE_RC(TE_TAD_CSAP, EFAULT);
    }

    /* Wait for CLI response */
    do {
        rc = parent_read_byte(spec_data, &data);
        if (rc == ETIMEDOUT)
            return 0;
        else if (rc != 0)
            return -1;

#if CLI_REMOVE_ECHO
        if ((echo_count++) < w_buf_len + 2)
        {
            if (*(echo_p++) == data)
                continue;

            if ((data == '\r') || (data == '\n'))
                continue;
        }
#endif

        if ((bytes_read < r_buf_len) && (data != '\0'))
        {
            r_buf[bytes_read] = data;
            bytes_read++;
        }
    } while (data != '\0');
    
    if (bytes_read < r_buf_len)
        r_buf[bytes_read] = '\0';
        
    if (bytes_read == 0)
    {
        r_buf[0] = '\n';
        r_buf[1] = '\0';
        bytes_read = 1;
    }

    return bytes_read;
}

/**
 * Callback for init CLI CSAP layer  if single in stack.
 *
 * @param csap_id       identifier of CSAP.
 * @param csap_nds      asn_value with CSAP init parameters
 * @param layer         numeric index of layer in CSAP type to be processed. 
 *                      Layers are counted from zero, from up to down.
 *
 * @return zero on success or error code.
 */ 
int 
cli_single_init_cb(int csap_id, const asn_value *csap_nds, int layer)
{
    int rc;
    int tmp_len;
    int sv[2];
    int pipe_descrs[2];

    struct exp_case *prompt;    /**< prompts the Expect process waits
                                     from the CLI session                     */

    csap_p   csap_descr;        /**< csap description                         */

    cli_csap_specific_data_p    cli_spec_data; /**< CLI CSAP specific data    */
    asn_value_p                 cli_csap_spec; /**< ASN value with csap init
                                                    parameters */

    setvbuf(stdout, NULL, _IONBF, 0);
    
    VERB("%s() entered", __FUNCTION__);

    if (csap_nds == NULL)
        return ETEWRONGPTR;

    if ((csap_descr = csap_find(csap_id)) == NULL)
        return ETADCSAPNOTEX;

    cli_csap_spec = asn_read_indexed(csap_nds, layer, "");

    cli_spec_data = calloc(1, sizeof(cli_csap_specific_data_t));
    if (cli_spec_data == NULL)
        return ENOMEM;

    /* Initialize pipe descriptors to undefined value */
    cli_spec_data->data_sock = cli_spec_data->sync_pipe = -1;

    /* Get conn-type value (mandatory) */
    tmp_len = sizeof(cli_spec_data->conn_type);
    rc = asn_read_value_field(cli_csap_spec, &cli_spec_data->conn_type,
                              &tmp_len, "conn-type");
    if (rc != 0)
    {
        ERROR("Cannot get '%s' value from CSAP parameters", "conn-type");
        goto error;
    }

    VERB("Conn-type %s using %s command",
         csap_conn_type_h2str(cli_spec_data->conn_type),
         cli_spec_data->conn_type <
             sizeof(cli_programs) / sizeof(cli_programs[0]) ?
             cli_programs[cli_spec_data->conn_type] : "INVALID");

    switch (cli_spec_data->conn_type)
    {
        case CLI_CONN_TYPE_SERIAL:
            VERB("Getting device name...");

            /* Get conn-params.serial.device value (mandatory) */
            if ((rc = cli_get_asn_string_value(cli_csap_spec,
                          "conn-params.#serial.device.#plain",
                          &cli_spec_data->device)) != 0)
            {
                ERROR("Cannot get device name for %s CSAP",
                      csap_conn_type_h2str(cli_spec_data->conn_type));
                goto error;
            }
            VERB("Device = %s", cli_spec_data->device);
            break;
            
        case CLI_CONN_TYPE_TELNET:
        case CLI_CONN_TYPE_SSH:
            VERB("Getting host name...");

            /* Get conn-params.telnet.host value (mandatory) */
            if ((rc = cli_get_asn_string_value(cli_csap_spec,
                         "conn-params.#telnet.host.#plain",
                         &cli_spec_data->host)) != 0)
            {
                ERROR("Cannot get host name for %s CSAP",
                      csap_conn_type_h2str(cli_spec_data->conn_type));
                goto error;
            }
            VERB("host = %s", cli_spec_data->host);

            VERB("Getting host port...");

            /* Get conn-params.telnet.port value (mandatory) */
            tmp_len = sizeof(cli_spec_data->port);
            rc = asn_read_value_field(cli_csap_spec,
                                      &cli_spec_data->port,
                                      &tmp_len,
                                      "conn-params.#telnet.port.#plain");
            if (rc != 0)
            {
                ERROR("Cannot get host port for %s CSAP",
                      csap_conn_type_h2str(cli_spec_data->conn_type));
                goto error;
            }
            VERB("port = %d", cli_spec_data->port);
            break;

        case CLI_CONN_TYPE_SHELL:
            VERB("Getting shell command args...");

            /* Get conn-params.shell.args value */
            if ((rc = cli_get_asn_string_value(cli_csap_spec,
                          "conn-params.#shell.args.#plain",
                          &cli_spec_data->shell_args)) != 0)
            {
                cli_spec_data->shell_args = NULL;
            }
            VERB("shell args = %s", cli_spec_data->shell_args != NULL ? 
                 cli_spec_data->shell_args : "<Empty>");
            break;

        default:
            ERROR("Unknown CLI connection type specified %d",
                  cli_spec_data->conn_type);
            rc = EINVAL;
            goto error;
    }

    cli_spec_data->program = cli_programs[cli_spec_data->conn_type];
    cli_spec_data->prompts_status = 0;

    prompt = &cli_spec_data->prompts[0];

    /* Get command-prompt value (mandatory) */
    if ((rc = cli_get_asn_string_value(cli_csap_spec,
                  "command-prompt.#plain", &prompt->pattern)) == 0)
    {
        cli_spec_data->prompts_status |= CLI_PROMPT_STATUS_COMMAND;
        prompt->type = exp_glob;
    }
    else if ((rc = cli_get_asn_string_value(cli_csap_spec,
                       "command-prompt.#script", &prompt->pattern)) == 0)
    {
        cli_spec_data->prompts_status |= CLI_PROMPT_STATUS_COMMAND;
        prompt->type = exp_regexp;
    }
    else
    {
        ERROR("Cannot get command prompt value");
        goto error;
    }
    VERB("command-prompt: %s", prompt->pattern);
    prompt->value = CLI_COMMAND_PROMPT;
    prompt++;

    /* Get login-prompt value (optional) */
    if ((rc = cli_get_asn_string_value(cli_csap_spec,
                  "login-prompt.#plain", &prompt->pattern)) == 0)
    {
        cli_spec_data->prompts_status |= CLI_PROMPT_STATUS_LOGIN;
        prompt->type = exp_glob;
    }
    else if ((rc = cli_get_asn_string_value(cli_csap_spec,
                       "login-prompt.#script", &prompt->pattern)) == 0)
    {
        cli_spec_data->prompts_status |= CLI_PROMPT_STATUS_LOGIN;
        prompt->type = exp_regexp;
    }

    if (prompt->pattern != NULL)
    {
        VERB("login-prompt = %s", prompt->pattern);
        prompt->value = CLI_LOGIN_PROMPT;
        prompt++;
    }

    /* Get password-prompt value (optional) */
    if ((rc = cli_get_asn_string_value(cli_csap_spec,
                  "password-prompt.#plain", &prompt->pattern)) == 0)
    {
        cli_spec_data->prompts_status |= CLI_PROMPT_STATUS_PASSWORD;
        prompt->type = exp_glob;
    }
    else if ((rc = cli_get_asn_string_value(cli_csap_spec,
                       "password-prompt.#script", &prompt->pattern)) == 0)
    {
        VERB("  password-prompt=%s\n", prompt->pattern);
        cli_spec_data->prompts_status |= CLI_PROMPT_STATUS_PASSWORD;
        prompt->type = exp_regexp;
    }

    if (prompt->pattern != NULL)
    {
        VERB("password-prompt = %s", prompt->pattern);
        prompt->value = CLI_PASSWORD_PROMPT;
        prompt++;
    }

    prompt->type = exp_end;

    /* Get user value (optional) */
    if ((rc = cli_get_asn_string_value(cli_csap_spec,
                  "user.#plain", &cli_spec_data->user)) != 0)
    {
        if ((cli_spec_data->prompts_status & CLI_PROMPT_STATUS_LOGIN) != 0)
        {
            ERROR("Cannot find '%s' value although login prompt "
                  "specified", "user name");
            goto error;
        }
    }
    else
    {
        VERB("user = %s", cli_spec_data->user);
    }

    /* Get password value (optional) */
    if ((rc = cli_get_asn_string_value(cli_csap_spec,
                  "password.#plain", &cli_spec_data->password)) != 0)
    {
        if ((cli_spec_data->prompts_status & CLI_PROMPT_STATUS_PASSWORD) != 0)
        {
            ERROR("Cannot find 'password' value although "
                  "password prompt specified");
            goto error;
        }
    }
    else
    {
        VERB("password = %s", cli_spec_data->password);
    }

    if (socketpair(PF_LOCAL, SOCK_STREAM, 0, sv) != 0)
    {
        rc = errno;
        ERROR("Cannot create a pair of sockets, errno %X", errno);
        goto error;
    }
    
    if (pipe(pipe_descrs) != 0)
    {
        rc = errno;
        ERROR("Cannot create pipe, errno %X", errno);
        close(sv[0]);
        close(sv[1]);
        goto error;
    }

    /* Default read timeout */
    cli_spec_data->read_timeout = CLI_CSAP_DEFAULT_TIMEOUT; 

    csap_descr->layers[layer].specific_data = cli_spec_data;

    csap_descr->read_cb           = cli_read_cb;
    csap_descr->write_cb          = cli_write_cb;
    csap_descr->write_read_cb     = cli_write_read_cb;
    csap_descr->read_write_layer  = layer; 
    csap_descr->timeout           = 500000;

    if ((cli_spec_data->expect_pid = fork()) == -1)
    {
        rc = errno;
        ERROR("fork failed, errno %X", errno);
        close(sv[0]);
        close(sv[1]);
        close(pipe_descrs[0]);
        close(pipe_descrs[1]);
        goto error;
    }

    if (cli_spec_data->expect_pid == 0)
    {
        /* Child */

        setvbuf(stdout, NULL, _IONBF, 0);

        cli_spec_data->data_sock = sv[0];
        close(sv[1]);
        cli_spec_data->sync_pipe = pipe_descrs[1];
        close(pipe_descrs[0]);

        cli_expect_main(cli_spec_data);
    }
    else
    {
        /* Parent */
        cli_sync_res_t sync_res;

        setvbuf(stdout, NULL, _IONBF, 0);
        
        cli_spec_data->data_sock = sv[1];
        close(sv[0]);
        cli_spec_data->sync_pipe = pipe_descrs[0];
        close(pipe_descrs[1]);

        VERB("Parent process continues, child_pid = %d",
             cli_spec_data->expect_pid);

        /* Wait for child initialisation finished */
        if ((sync_res = parent_wait_sync(cli_spec_data)) != SYNC_RES_OK)
        {
            cli_single_destroy_cb(csap_descr->id, layer);
            return TE_RC(TE_TAD_CSAP, MAP_SYN_RES2ERRNO(sync_res));
        }

        VERB("Child has just been initialised");
    }

    return 0;

error:
    free_cli_csap_data(cli_spec_data);

    return TE_RC(TE_TAD_CSAP, rc);
}

/**
 * Callback for destroy CLI CSAP layer  if single in stack.
 *      This callback should free all undeground media resources used by 
 *      this layer and all memory used for layer-specific data and pointed 
 *      in respective structure in 'layer-data' in CSAP instance struct. 
 *
 * @param csap_id       identifier of CSAP.
 * @param csap_nds      asn_value with CSAP init parameters
 * @param layer         numeric index of layer in CSAP type to be processed. 
 *                      Layers are counted from zero, from up to down.
 *
 * @return zero on success or error code.
 */ 
int 
cli_single_destroy_cb(int csap_id, int layer)
{
    csap_p csap_descr = csap_find(csap_id);

    cli_csap_specific_data_p spec_data = 
        (cli_csap_specific_data_p)csap_descr->layers[layer].specific_data;

    VERB("%s() started, spec_data=%x", __FUNCTION__, spec_data);

    if (spec_data == NULL)
    {
        ERROR("%s(): Invalid pointer to specific data", __FUNCTION__);
        return -1;
    }

    if (spec_data->expect_pid > 0)
    {
        INFO("kill CLI session, pid=%d", spec_data->expect_pid);
        kill(spec_data->expect_pid, SIGKILL);
    }
    
    VERB("waitpid for CLI session, pid=%d", spec_data->expect_pid);
    waitpid(spec_data->expect_pid, NULL, 0);
    
    VERB("%s(): try to free CLI CSAP specific data", __FUNCTION__);
    free_cli_csap_data(spec_data);

    csap_descr->layers[layer].specific_data = NULL;
   
    return 0;
}

/**************************************************************************
 *
 * Routines executed in the Expect process context
 *
 **************************************************************************/

#include "ta_logfork.h"

static void
child_send_sync(cli_csap_specific_data_p spec_data, 
                cli_sync_res_t sync_val)
{
    int rc;
    
    rc = write(spec_data->sync_pipe, &sync_val, sizeof(sync_val));
    if (rc != (ssize_t)sizeof(sync_val))
    {
        ERROR("Failed to send synchronization mark from Expect side, "
              "rc = %d, errno = %d", rc, errno);
    }
}

/**
 * Sends specified string to expect process.
 *
 * @param spec_data  CSAP-specific data
 * @param str        String to be sent
 *
 * @return 0 on success, errno on failure.
 */
static int
write_string_to_expect(cli_csap_specific_data_p spec_data, const char *str)
{
    int rc;

    if ((rc = write(spec_data->io, str, strlen(str))) != (ssize_t)strlen(str))
    {
        ERROR("Failed to send '%s' string to CSAP Engine, "
              "rc = %d, errno = %d", rc, errno);
        return EFAULT;
    }
    if ((rc = write(spec_data->io, "\r", 1)) != 1)
    {
        ERROR("Failed to send trailing CTR-R character to CSAP Engine, "
              "rc = %d, errno = %d", rc, errno);
        return EFAULT;
    }

    return 0;
}

/**
 * Open a new CLI session (spawn CLI program).
 *
 * @param spec_data  Csap-specific data structure
 *
 * @return 0 on success, otherwise error code
 */ 
static int
cli_session_open(cli_csap_specific_data_p spec_data)
{
    FILE *dbg;
    FILE *log;

    dbg = fopen("/tmp/exp_debug.txt", "a+");
    if (dbg == NULL)
    {
        ERROR("Cannot open %s file for appending", "/tmp/exp_debug.txt");
        return errno;
    }
    setvbuf(dbg, NULL, _IONBF, 0);

    log = fopen("/tmp/exp_log.txt", "a+");
    if (log == NULL)
    {
        ERROR("Cannot open %s file for appending", "/tmp/exp_log.txt");
        return errno;
    }
    setvbuf(log, NULL, _IONBF, 0);

#ifdef EXP_DEBUG
    exp_console = 0;
    exp_is_debugging = 0;
    exp_loguser = 0;
    exp_logfile_all = 1;
    
    exp_debugfile = dbg;
    exp_logfile = log;
#endif

    if (spec_data->conn_type == CLI_CONN_TYPE_SERIAL)
    {
        INFO("> %s %s", spec_data->program, spec_data->device);
        spec_data->io = exp_spawnl(spec_data->program, spec_data->program,
                                   "-l", spec_data->device, NULL);
        sleep(2);

        /* Send '\r' to CLI session to get first command prompt */
        if (spec_data->io >= 0 && write(spec_data->io, "\r", 1) != 1)
        {
            ERROR("Failed to send initial CTR-R character to Expect, "
                  "errno = %d", errno);
        }
    }
    else if ((spec_data->conn_type == CLI_CONN_TYPE_TELNET) ||
             (spec_data->conn_type == CLI_CONN_TYPE_SSH))
    {
        /** -p<port> - port parameter */
        char port_param[CLI_SESSION_PARAM_LENGTH_MAX]; 

        /** -l<user> - user parameter */
        char user_param[CLI_SESSION_PARAM_LENGTH_MAX]; 

        if (spec_data->conn_type == CLI_CONN_TYPE_TELNET)
            sprintf(port_param, "%d", spec_data->port);
        else if (spec_data->conn_type == CLI_CONN_TYPE_SSH)
            sprintf(port_param, "-p%d", spec_data->port);
        else
            port_param[0] = '\0';
            
        if (spec_data->user != NULL)
        {
            sprintf(user_param, "-l%s", spec_data->user);
            INFO("> %s %s %s %s",
                 spec_data->program, spec_data->host, port_param, user_param);
            spec_data->io = exp_spawnl(spec_data->program, spec_data->program,
                                       spec_data->host, port_param, user_param, NULL);

            fprintf(spec_data->dbg_file, "returned %d\n", spec_data->io);
        }
        else
        {
            fprintf(spec_data->dbg_file, "> %s %s %s\n",
                    spec_data->program, spec_data->host, port_param);
            spec_data->io = exp_spawnl(spec_data->program, spec_data->program,
                                       spec_data->host, port_param, NULL);
        }
    }
    else if (spec_data->conn_type == CLI_CONN_TYPE_SHELL)
    {
        /** -c<args> - execute commands after this parameter */
        char shell_args_param[CLI_SESSION_PARAM_LENGTH_MAX]; 

        if (spec_data->shell_args != NULL)
        {
            sprintf(shell_args_param, "-c");
            spec_data->io = exp_spawnl(spec_data->program, spec_data->program,
                                       shell_args_param, spec_data->shell_args,
                                       NULL);
        }
        else
        {
            spec_data->io = exp_spawnl(spec_data->program, spec_data->program,
                                       NULL);
        }
    }

    if (spec_data->io < 0)
    {
        ERROR("exp_spawnl failed with errno=%X", errno);
        return EFAULT;
    }
    else
    {
        VERB("exp_spawnl() sucessfull, fd=%d", spec_data->io);
    }

    spec_data->fp = fdopen(spec_data->io, "r+");
    if (spec_data->fp == NULL)
    {
        ERROR("fdopen(%d) failed", spec_data->io);
        return EINVAL;
    }

    spec_data->session_pid = exp_pid;
    
    VERB("ExpectPID=%d, fd=%d", exp_pid, spec_data->io);

    exp_timeout = spec_data->read_timeout;

    return 0;
}

/**
 * Terminate the CLI session corresponding to current Expect process.
 *
 * @param spec_data     csap special data structure.
 *
 * @return 0 on success or -1 if failed. 
 */ 
static void
cli_session_close(cli_csap_specific_data_p spec_data)
{
    VERB("%s() called", __FUNCTION__);

    /* Terminate CLI session */
    if (spec_data->session_pid != 0)
    {
        kill(spec_data->session_pid, SIGKILL);
    }
    close(spec_data->io);

    waitpid(spec_data->session_pid, NULL, 0);

#ifdef EXP_DEBUG
    fclose(exp_debugfile);
    fclose(exp_logfile);
#endif
}

/**
 * Terminate current Expect process and corresponding CLI session.
 *
 * @param spec_data     csap special data structure.
 *
 * @return 0 on success or -1 if failed. 
 */ 
static void
cli_expect_finalize(cli_csap_specific_data_p spec_data,
                    cli_sync_res_t sync_val)
{
    child_send_sync(spec_data, sync_val);

    /* Terminate current CLI session */
    cli_session_close(spec_data);

    close(spec_data->data_sock);
    close(spec_data->sync_pipe);

    /* Terminate current Expect process */
    _exit(0);
}

/**
 * Function waits for prompt. In case it gets login (or password)
 * prompt, it sends login (or password) value to the program.
 * In case of EOF detected it terminates CLI session and exits calling
 * cli_expect_finalize() function.
 *
 * @param spec_data  CSAP-specific data pointer
 *
 * @return Result of exp_expectv() function.
 */
static int
cli_expect_wait_for_prompt(cli_csap_specific_data_p spec_data)
{
    int res;

    if (spec_data == NULL)
    {
        ERROR("%s(): Invalid pointer to specific data", __FUNCTION__);
        return -1;
    }

    res = exp_expectv(spec_data->io, spec_data->prompts);

    switch (res)
    {
        case CLI_COMMAND_PROMPT:
            break;

        case CLI_LOGIN_PROMPT:
            if (write_string_to_expect(spec_data, spec_data->user) != 0)
            {
                cli_expect_finalize(spec_data, SYNC_RES_FAILED);
            }
            break;

        case CLI_PASSWORD_PROMPT:
            if (write_string_to_expect(spec_data, spec_data->password) != 0)
            {
                cli_expect_finalize(spec_data, SYNC_RES_FAILED);
            }
            break;

        case EXP_EOF:
            INFO("EOF detected");
            cli_expect_finalize(spec_data, SYNC_RES_ABORTED);
            break;
        
        case EXP_TIMEOUT:
            break;

        default:
            WARN("Unknown result got from exp_expectv(): %d", res);
            break;
    }

    return res;
}

/**
 * Main function for Expect side:
 * - creates CLI session running a program under expect control;
 * - sends synchronization mark to CSAP Engine (fail or success);
 * - enters into infinite loop waiting for commands from CSAP Engine;
 * - as a command is read out and exectured, it sends back the result,
 *   and again enters into infinite loop;
 */
static int
cli_expect_main(cli_csap_specific_data_p spec_data)
{
    int    rc;
    char   data;
    int    reply_len;

    logfork_register_user("CLI CSAP CHILD");

    /* Run program under expect control */
    if (cli_session_open(spec_data) != 0)
    {
        cli_expect_finalize(spec_data, SYNC_RES_FAILED);
    }

    /*
     * Capture command prompt to be sure that CSAP is ready to
     * run commands.
     */
    do {
        rc = cli_expect_wait_for_prompt(spec_data);
        if (rc == EXP_TIMEOUT)
        {
            ERROR("Timeout waiting for command prompt");
            cli_expect_finalize(spec_data, SYNC_RES_FAILED);
        }
    } while (rc != CLI_COMMAND_PROMPT);

    /* Tell CSAP Engine that expect session is ready */
    child_send_sync(spec_data, SYNC_RES_OK);

    /* Wait for command from CSAP Engine */
    for (;;)
    {
        /* Read out the command to execute */
        while ((rc = read(spec_data->data_sock, &data, 1)) == 1 &&
               data != '\0')
        {
            if ((rc = write(spec_data->io, &data, 1)) != 1)
            {
                ERROR("Cannot send '%c' character to expect, "
                      "rc = %d, errno = %X", data, rc, errno);
                cli_expect_finalize(spec_data, SYNC_RES_FAILED);
            }
        }

        if (rc != 1)
        {
            ERROR("Error occurred during reading command from "
                  "CSAP Engine side, rc = %d, errno = %X", rc, errno);
            cli_expect_finalize(spec_data, SYNC_RES_FAILED);
        }

        /* Send '\r' to CLI session to finish the command sequence */
        if (write(spec_data->io, "\r", 1) != 1)
        {
            ERROR("Failed to send trailing CTR-R character to Expect, "
                  "rc = %d, errno = %d", rc, errno);
        }

        /* 
         * Start waiting for command prompt to 
         * send back the command result.
         */
        do {
            rc = cli_expect_wait_for_prompt(spec_data);
            if (rc == EXP_TIMEOUT)
            {
                child_send_sync(spec_data, SYNC_RES_TIMEOUT);
            }
        } while (rc != CLI_COMMAND_PROMPT);

        /* Transfer CLI session output to the CSAP Engine */
        reply_len = (exp_match - exp_buffer);
        rc = write(spec_data->data_sock, exp_buffer, reply_len);
        if (rc != reply_len)
        {
            ERROR("Failed to send command reply to CSAP Engine, "
                  "rc = %d, errno = %d", rc, errno);
            cli_expect_finalize(spec_data, SYNC_RES_FAILED);
        }

        /* Send trailing '\0' character to finish transfer */
        if ((rc = write(spec_data->data_sock, "\0", 1)) != 1)
        {
            ERROR("Failed to send string termination character to "
                  "CSAP Engine, rc = %d, errno = %d", rc, errno);
        }
    }
    
    return 0;
}
