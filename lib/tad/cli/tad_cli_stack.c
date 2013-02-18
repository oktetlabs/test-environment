/** @file
 * @brief TAD CLI
 *
 * Traffic Application Domain Command Handler.
 * CLI CSAP stack-related callbacks.
 *
 * Copyright (C) 2003-2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
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
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAD CLI"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_ASSERT_H
#include <assert.h>
#else
#define assert(x) (x)
#endif
#include "tad_cli_impl.h"
#include "logger_api.h"
#include "logfork.h"


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
    (((sync_res_) == SYNC_RES_OK) ? 0 :                     \
     ((sync_res_) == SYNC_RES_FAILED) ? TE_EREMOTEIO :      \
     ((sync_res_) == SYNC_RES_ABORTED) ? TE_ECONNABORTED :  \
     ((sync_res_) == SYNC_RES_INT_ERROR) ? TE_EFAULT :      \
     ((sync_res_) == SYNC_RES_TIMEOUT) ? TE_ETIMEDOUT :     \
         (assert(0), 0))


/* Forward declarations */
static int cli_expect_main(cli_csap_specific_data_p spec_data);


#if 1
/* 
 * FIXME:
 * The following extern declarations are copied from unix_internal.h
 * to avoid build-time warnings. It will not work with non-unix TA's.
 * Good solution is required.
 */
/**
 * waitpid() analogue, with the same parameters/return value.
 * Only WNOHANG option is supported for now.
 * Process groups are not supported for now.
 */
extern pid_t ta_waitpid(pid_t pid, int *status, int options);

/**
 * Kill a child process.
 *
 * @param pid PID of the child to be killed
 *
 * @retval 0 child was exited or killed successfully
 * @retval -1 there is no such child.
 */
extern int ta_kill_death(pid_t pid);
#endif


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
 * Define Linux kernel version.
 * This information is necessary for cli_session_alive() function,
 * that checks if child process is still alive.
 * In 2.4 kernels linux threads work differently from most recent 
 * kernels. Each thread has its own PID and so waitpid() function 
 * can't be used in the folowing situation:
 * 1. create CSAP in main thread, which forks to get a child process 
 *    call it "expect" process (let's keep its PID as CHLDPID).
 * 2. On processing send_recv operation, TAD core creates a separate 
 *    thread and in case of 2.4 kernel it has PID different from main
 *    thread's PID.
 * 3. Calling waitpid() in this new thread for CHLDPID. As "expect"
 *    process's parent is main, we get -1 telling us that it is 
 *    not our child.
 * 
 * @param spec_data  CSAP-specific data pointer
 *
 * @se If we failed to define linux kernel version we set it to 2.4
 * as in the worst case.
 */
static void
define_kernel_version(cli_csap_specific_data_p spec_data)
{
    FILE *fd;
    char  buf[128];
    char *ptr;
    int   major_ver = 0;
    int   minor_ver = 0;
    
    spec_data->kernel_like_2_4 = TRUE;

    if ((fd = fopen("/proc/version", "r")) == NULL)
        return;

    if (fgets(buf, sizeof(buf), fd) != NULL &&
        (ptr = strstr(buf, "version ")) != NULL &&
        (ptr += strlen("version ")) != NULL &&
        sscanf(ptr, "%d.%d.", &major_ver, &minor_ver) == 2)
    {
        if ((major_ver >= 2 && minor_ver > 4) || major_ver >= 3)
            spec_data->kernel_like_2_4 = FALSE;
    }

    fclose(fd);
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
    
    while (TRUE)
    {
        FD_ZERO(&read_set);
        FD_SET(spec_data->sync_pipe, &read_set);

        /* Wait for sync mark indefinitely */
        rc = select(spec_data->sync_pipe + 1,
                    &read_set, NULL, NULL, NULL);
        if (rc == -1 && errno == EINTR)
            continue;

        break;
    }

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
 * @param tv         Time to wait for notification or data byte
 * @param data       Single byte read from Expect side (OUT)
 *
 * @return Status code.
 * @retval 0            SYN_RES_OK notification or data byte came
 * @retval TE_ENOENT    No notification or data byte came in @a tv time
 * @retval other        errno mapped with MAP_SYN_RES2ERRNO() macro in
 *                      case of message arriving.
 *
 * @se If it captures an error from Expect side, it logs error message
 * and return correspnding errno value as its return value.
 */
static te_errno
parent_read_byte(cli_csap_specific_data_p spec_data,
                 struct timeval *tv, char *data)
{
    fd_set      read_set;
    int         max_descr;
    te_errno    rc;
    int         ret;
    
    max_descr = spec_data->sync_pipe > spec_data->data_sock ?
        spec_data->sync_pipe : spec_data->data_sock;

    while (TRUE)
    {
        FD_ZERO(&read_set);
        FD_SET(spec_data->sync_pipe, &read_set);
        FD_SET(spec_data->data_sock, &read_set);

        ret = select(max_descr + 1, &read_set, NULL, NULL, tv);
        if (ret == -1 && errno == EINTR)
            continue;

        if (ret < 0)
        {
            rc = te_rc_os2te(errno);
            ERROR("Reading single character from Expect side "
                  "fails on select(), errno = %r", rc);
            return rc;
        }
        
        if (ret == 0)
        {
            /* There is nothing in data socket and in sync pipe */
            return TE_ETIMEDOUT;
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
            if ((ret = read(spec_data->data_sock, data, 1)) != 1)
            {
                rc = te_rc_os2te(errno);
                ERROR("Reading single character from Expect side "
                      "fails on read(), ret=%d, errno=%r", ret, rc);
                return (ret == 0) ? TE_ECONNABORTED : rc;
            }
        }
        else
        {
            ERROR("select() returns non-zero value, but there is no "
                  "readable descriptor");
            return TE_EFAULT;
        }

        return 0;
    }
    
    /* We never reach this code */
    return 0;
}

/**
 * The function processes sync_pipe to see if pending reply has been sent
 * from Expect side.
 * The function should be called when user runs a new command and 
 * there has been no reply got for the previous one. In that case
 * we should make sure that the previous command finished.
 *
 * @param spec_data  Pointer to CSAP-specific data
 *
 * @se If the previous command has finished, function clears 
 * CLI_CSAP_STATUS_REPLY_WAITING bit from "status" of the CSAP.
 */
static te_errno
process_sync_pipe(cli_csap_specific_data_p spec_data)
{
    struct timeval tv = { 0, 0 };
    char           data;
    te_errno       rc;

    while ((rc = parent_read_byte(spec_data, &tv, &data)) != 0)
    {
        if (rc == TE_ETIMEDOUT)
        {
            /* Try once again */
            continue;
        }
        else if (rc == TE_ENOENT)
        {
            /* 
             * Expect side still can't provide reply 
             * to the command, so think of the CSAP as still busy.
             */
            return TE_EBUSY;
        }
        else
            return rc;
    }
    
    /*
     * We've got OK notification from Expect side, 
     * so reset CLI_CSAP_STATUS_REPLY_WAITING bit.
     */
    spec_data->status &= ~CLI_CSAP_STATUS_REPLY_WAITING;

    return 0;
}

/**
 * Read reply from Expect side.
 *
 * @param spec_data      Pointer to CSAP-specific data
 * @param cmd_buf_len    The length of the command we capturing reply from.
 *                       It is necessary to strip that number of starting 
 *                       bytes from the reply because they just echoed.
 * @param reply_buf      Buffer for reply message, can be NULL (OUT)
 * @param reply_buf_len  Length of reply buffer
 * @param tv             Time to wait for reply
 *
 * @return 0 on timeout detection, -errno on error,
 *         > 0 - the number of bytes in @a reply_buf
 */
static int
parent_read_reply(cli_csap_specific_data_p spec_data,
                  size_t cmd_buf_len,
                  char *reply_buf, size_t reply_buf_len,
                  struct timeval *tv)
{
    te_errno    rc;
    char        data;
    te_bool     echo_stripped = FALSE;
    size_t      bytes_read = 0;
    size_t      echo_count = 0;

    /* Wait for CLI response */
    do {
        rc = parent_read_byte(spec_data, tv, &data);
        if (rc == TE_ETIMEDOUT)
        {
            /*
             * Keep in mind that we should get reply before running 
             * the next command.
             */
            spec_data->status |= CLI_CSAP_STATUS_REPLY_WAITING;
            VERB("%s(): Timeout", __FUNCTION__);
            return 0;
        }
        else if (rc != 0)
            return -TE_RC(TE_TAD_CSAP, rc);

        /* Remove echo characters (command + \r + \n) */
        if (echo_count < cmd_buf_len)
        {
            echo_count++;
            continue;
        }
        else if (!echo_stripped)
        {
            if (data == '\n' || data == '\r')
                continue;

            echo_stripped = TRUE;
        }

        if (reply_buf == NULL)
        {
            /* Just count bytes */
            bytes_read++;
            continue;
        }

        if ((bytes_read < reply_buf_len) && (data != '\0'))
        {
            reply_buf[bytes_read] = data;
            bytes_read++;
        }
    } while (data != '\0');

    if (reply_buf == NULL)
    {
        return bytes_read;
    }
    
    if (bytes_read < reply_buf_len)
        reply_buf[bytes_read] = '\0';

    if (bytes_read == 0)
    {
        assert(reply_buf_len >= 2);
        reply_buf[0] = '\n';
        reply_buf[1] = '\0';
        bytes_read = 1;
    }

    return bytes_read;
}

/**
 * Check whether CLI session is still alive.
 *
 * @param spec_data  CSAP-specific data
 *
 * @return TRUE if CLI session is running, and FALSE otherwise.
 */ 
static te_bool
cli_session_alive(cli_csap_specific_data_p spec_data)
{
    pid_t pid;
    int   status;

    if (spec_data->expect_pid == 0)
        return FALSE;

    if (spec_data->kernel_like_2_4)
    {
        /*
         * We are working with 2.4 kernel, so we can't define 
         * status of main child, as we are a thread created from main.
         * Always return TRUE, if child crashed, that will be defined 
         * as the result of write failure, or read returning 0.
         */
        return TRUE;
    }

    pid = ta_waitpid(spec_data->expect_pid, &status, WNOHANG);
    if (pid < 0)
    {
        ERROR("waitpid(%d) failed, errno = %d",
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
        assert(pid == spec_data->expect_pid);
        VERB("The child with PID %d is finished", spec_data->expect_pid);
        return FALSE;
    }
    
    assert(0);
    return FALSE;
}


/**
 * Read the STRING type value from the CSAP description (in ASN.1 notation).
 *
 * @param csap_spec    CSAP description (in ASN.1 notation).
 * @param asn_name     Name of the field to read.
 * @param str_value    Pointer to returned string.
 *
 * @return 0 on success or -1 if not found.
 *
 * @note Function modifies @p str_value only on success.
 */ 
static int
cli_get_asn_string_value(const asn_value *csap_spec,
                         const char *asn_name,
                         char **str_value)
{
    int rc = 0;
    size_t tmp_len = asn_get_length(csap_spec, asn_name);

    if (tmp_len > 0)
    {
        char *val;

        /* allocate memory for the string */
        val = (char *) malloc(tmp_len + 1);
        if (val == NULL)
            return TE_ENOMEM;

        rc = asn_read_value_field(csap_spec, val,
                                  &tmp_len, asn_name);
        if (rc != 0)
        {
            free(val);
            rc = TE_EINVAL;
        }
        else
        {
            val[tmp_len] = '\0';
            *str_value = val;
        }
    }
    else
    {
        rc = TE_EINVAL;
    }

    return rc;
}

/**
 * Get the value of prompt parameter that can be either fixed value
 * or regular expression.
 *
 * @param cli_container  ASN container where to get the value from
 * @param param_name     Name of prompt parameter whose value to get
 * @param prompt         Expect match structure
 *
 * @return Status of the operation
 */
int
cli_container_get_prompt_param(const asn_value *cli_container, const char *param_name, cli_csap_prompt *prompt)
{
    /* Pairs of ASN to Expect value mappings */
    struct param_type {
        const char    *asn_type; /**< ASN value type */
        enum exp_type  exp_type; /**< Expect value type */
    } types_asn2exp[] = {
        { "plain", exp_exact },
        { "script", exp_regexp },
    };
    unsigned int  i;
    char         *str_val;
    char          cmd_buf[32];
    int           rc = TE_EINVAL;

    for (i = 0; i < sizeof(types_asn2exp) / sizeof(types_asn2exp[0]); i++)
    {
        snprintf(cmd_buf, sizeof(cmd_buf), "%s.#%s",
                 param_name, types_asn2exp[i].asn_type);

        rc = cli_get_asn_string_value(cli_container, cmd_buf, &str_val);
        if (rc == 0)
        {
            prompt->len = snprintf(prompt->val, sizeof(prompt->val), "%s", str_val);
            prompt->type = types_asn2exp[i].exp_type;
            free(str_val);
            return rc;
        }
    }
    return rc;
}

/**
 * Update CLI prompt values based on the information kept in
 * ASN container (CSAP SPEC or Data PDU).
 *
 * @param cli_container  ASN container where to get data from
 * @param cli_prompts    Storage of CLI prompt values (to update
 *                       according to information kept in @p cli_container)
 *
 * @return N/A
 */
void
cli_container_get_prompt_params(const asn_value *cli_container,
                                cli_csap_prompts_t *cli_prompts)
{
    /* Get command-prompt value */
    cli_container_get_prompt_param(cli_container, "command-prompt", &cli_prompts->cmd);
    /* Get login-prompt value */
    cli_container_get_prompt_param(cli_container, "login-prompt", &cli_prompts->login);
    /* Get password-prompt value */
    cli_container_get_prompt_param(cli_container, "password-prompt", &cli_prompts->passwd);
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
        /*
         * spec_data->prompts[i].pattern points to
         * a buffer in spec_data->cur_prompts or
         * spec_data->init_prompts, i.e. it is incorrect
         * to free them.
         */

        free(spec_data->prompts[i].re);
    }

    free(spec_data);   
}


/* See description tad_cli_impl.h */
te_errno
tad_cli_read_cb(csap_p csap, unsigned int timeout,
                tad_pkt *pkt, size_t *pkt_len)
{
    cli_csap_specific_data_p spec_data;

    tad_pkt_seg    *seg;
    int             my_timeout = timeout;
    int             timeout_rate;
    te_errno        rc;
    struct timeval  tv;

    spec_data = csap_get_rw_data(csap); 

    VERB("%s() Called with CSAP %d", __FUNCTION__, csap->id);
    assert(spec_data->io >= 0);

    if (!cli_session_alive(spec_data))
    {
        ERROR("CLI session is not running");
        return TE_RC(TE_TAD_CSAP, TE_EIO);
    }

#if 0
    if ((spec_data->status & CLI_CSAP_STATUS_REPLY_WAITING) == 0)
    {
        VERB("Nothing to read!");
        /*
         * Nothing to read, we've sent reply for the previous command
         * and now you should first run some command to get something
         * from the CSAP. Behave like we returned by timeout.
         * @todo May be sleep(timeout) should be executed here?
         */
        return TE_RC(TE_TAD_CSAP, TE_ETIMEDOUT);
    }
#endif

    TE_US2TV(timeout, &tv);

    /* Try to wait for command reply during timeout */
    timeout_rate = timeout / 10;
    while (TRUE)
    {
        if ((rc = process_sync_pipe(spec_data)) != 0)
        {
            if (rc == TE_EBUSY && my_timeout != 0)
            {
                /* Sleep for a while */
                usleep(timeout_rate);
                if ((my_timeout -= timeout_rate) < 0)
                    my_timeout = 0;
                continue;
            }
            
            return TE_RC(TE_TAD_CSAP, rc);
        }
        else
            break;
    }

    seg = tad_pkt_first_seg(pkt);
    if (seg == NULL)
    {
        seg = tad_pkt_alloc_seg(NULL, 0x1000, NULL);
        if (seg == NULL)
            return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
        tad_pkt_append_seg(pkt, seg);
    }
    else if (seg->data_ptr == NULL)
    {
        void *mem = malloc(0x1000);

        if (mem == NULL)
            return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
        tad_pkt_put_seg_data(pkt, seg, mem, 0x1000, tad_pkt_seg_data_free);
    }
    
    /*
     * We've got reply ready notification (OK notification),
     * so read out the reply.
     */
    rc = parent_read_reply(spec_data, spec_data->last_cmd_len,
                           seg->data_ptr, seg->data_len, &tv);
    if (rc < 0)
    {
        rc = -rc;
    }
    else
    {
        *pkt_len = rc;
        VERB("%s(): read data length %u", __FUNCTION__,
             (unsigned)*pkt_len);
        rc = 0;
    }

    return rc;
}


/* See description tad_cli_impl.h */
te_errno
tad_cli_write_cb(csap_p csap, const tad_pkt *pkt)
{
#if 1
    const void *buf;
    size_t      buf_len;

    if (pkt == NULL || tad_pkt_seg_num(pkt) != 1)
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);
    buf     = tad_pkt_first_seg(pkt)->data_ptr;
    buf_len = tad_pkt_first_seg(pkt)->data_len;
#endif
    cli_csap_specific_data_p spec_data;

    int    timeout;
    size_t bytes_written;
    int    ret, ret2;


    VERB("%s() Called with CSAP %d", __FUNCTION__, csap->id);

    spec_data = csap_get_rw_data(csap); 

    assert(spec_data->io >= 0);

    if (!cli_session_alive(spec_data))
    {
        ERROR("CLI session is not running");
        return -1;
    }

    if ((spec_data->status & CLI_CSAP_STATUS_REPLY_WAITING) != 0)
    {
        /*
         * We haven't got a reply for the previous command,
         * see if it has finished by now.
         */
        if ((ret = process_sync_pipe(spec_data)) != 0)
            return TE_RC(TE_TAD_CSAP, TE_EIO);
    }

    timeout = csap->stop_latency_timeout;

    /**
     * Send the following information to Expect aware process:
     * - timeout to wait for reply;
     * - prompt values to use in reply processing;
     * - command to run.
     */
    ret = write(spec_data->data_sock, &timeout, sizeof(timeout));
    ret2 = write(spec_data->data_sock, &spec_data->cur_prompts, sizeof(spec_data->cur_prompts));
    bytes_written = write(spec_data->data_sock, buf, buf_len);
    if (ret != (int)sizeof(timeout) || ret2 != (int)sizeof(spec_data->cur_prompts) ||
        bytes_written != buf_len)
    {
        ERROR("%s(): Cannot write '%s' command to Expect side, "
              "rc = %d, errno = %d",
              __FUNCTION__, buf, bytes_written, errno);
        return TE_RC(TE_TAD_CSAP, TE_EIO);
    }

    spec_data->last_cmd_len = buf_len;

    return 0;
}


/* See description tad_cli_impl.h */
te_errno
tad_cli_write_read_cb(csap_p csap, unsigned int timeout,
                      const tad_pkt *w_pkt,
                      tad_pkt *r_pkt, size_t *r_pkt_len)
{
#if 1
    const void *w_buf;
    size_t      w_buf_len;

    if (w_pkt == NULL || tad_pkt_seg_num(w_pkt) != 1)
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);
    w_buf     = tad_pkt_first_seg(w_pkt)->data_ptr;
    w_buf_len = tad_pkt_first_seg(w_pkt)->data_len;
#endif
    cli_csap_specific_data_p spec_data;

    size_t bytes_written;
    int    ret, ret2;
    te_errno    rc;

    struct timeval tv = { timeout / 1000000, timeout % 1000000 };

    VERB("%s() Called with CSAP %d", __FUNCTION__, csap->id);

    if (csap == NULL)
        return -1;
    
    spec_data = csap_get_rw_data(csap); 

    assert(spec_data->io >= 0);

    if (!cli_session_alive(spec_data))
    {
        ERROR("CLI session is not running");
        return -1;
    }

    if ((spec_data->status & CLI_CSAP_STATUS_REPLY_WAITING) != 0)
    {
        /*
         * We haven't got a reply for the previous command,
         * see if it has finished by now.
         */
        VERB("A reply for the previous command hasn't been got yet, "
             "so read out sync_pipe to see if now it's waiting us");
        if ((ret = process_sync_pipe(spec_data)) != 0)
        {
            VERB("Not yet ...");
            return TE_RC(TE_TAD_CSAP, TE_EIO);
        }
        VERB("Yes we've just read out reply notification!\n"
             "We are ready to run next command.");

        /* Read out pending reply */
        ret = parent_read_reply(spec_data, spec_data->last_cmd_len, 
                               NULL, 0, &tv);
        if (ret < 0)
        {
            rc = TE_RC(TE_TAD_CSAP, -ret);
        }
        else
        {
            rc = 0;
        }

        if (rc != 0)
            return rc;
    }

    VERB("Send command '%s' to Expect side", w_buf);

    ret = write(spec_data->data_sock, &timeout, sizeof(timeout));
    ret2 = write(spec_data->data_sock, &spec_data->cur_prompts, sizeof(spec_data->cur_prompts));
    bytes_written = write(spec_data->data_sock, w_buf, w_buf_len);
    if (ret != (int)sizeof(timeout) || ret2 != (int)sizeof(spec_data->cur_prompts) ||
        bytes_written != w_buf_len)
    {
        ERROR("%s(): Cannot write '%s' command to Expect side, "
              "rc = %d, errno = %d",
              __FUNCTION__, w_buf, bytes_written, errno);
        return TE_RC(TE_TAD_CSAP, TE_EIO);
    }

    spec_data->last_cmd_len = w_buf_len;

    /* Wait for CLI response */
    ret = parent_read_reply(spec_data, w_buf_len,
                            r_pkt->segs.cqh_first->data_ptr,
                            r_pkt->segs.cqh_first->data_len, &tv);

    if (ret < 0)
    {
        rc = TE_RC(TE_TAD_CSAP, -ret);
        VERB("Reading reply from Expect side finishes with %r return code",
             rc);
    }
    else
    {
        *r_pkt_len = ret;
        rc = 0;
    }

    return rc;
}

/**
 * Convert prompt information from generic to Expect specific format.
 *
 * @param cur_prompts  values of prompts in application format
 * @param exp_prompts  values of prompts in Expect format
 *
 * @return N/A
 */
void
prepare_exp_prompts(cli_csap_prompts_t *cur_prompts, struct exp_case *exp_prompts)
{
    if (cur_prompts->cmd.len != 0)
    {
        exp_prompts->pattern = cur_prompts->cmd.val;
        exp_prompts->type = cur_prompts->cmd.type;
        exp_prompts->value = CLI_COMMAND_PROMPT;
        exp_prompts++;
    }

    if (cur_prompts->login.len != 0)
    {
        exp_prompts->pattern = cur_prompts->login.val;
        exp_prompts->type = cur_prompts->login.type;
        exp_prompts->value = CLI_LOGIN_PROMPT;
        exp_prompts++;
    }
    
    if (cur_prompts->passwd.len != 0)
    {
        exp_prompts->pattern = cur_prompts->passwd.val;
        exp_prompts->type = cur_prompts->passwd.type;
        exp_prompts->value = CLI_PASSWORD_PROMPT;
        exp_prompts++;
    }

    exp_prompts->type = exp_end;
}


/* See description tad_cli_impl.h */
te_errno
tad_cli_rw_init_cb(csap_p csap)
{
    int rc;
    size_t tmp_len;
    int sv[2];
    int pipe_descrs[2];

    cli_csap_prompts_t         *init_prompts;
    cli_csap_specific_data_p    cli_spec_data; /**< CLI CSAP specific data    */
    asn_value                  *cli_csap_spec; /**< ASN value with csap init
                                                    parameters */

    setvbuf(stdout, NULL, _IONBF, 0);
    
    VERB("%s() entered", __FUNCTION__);

    cli_csap_spec = csap->layers[csap_get_rw_layer(csap)].nds;

    cli_spec_data = calloc(1, sizeof(*cli_spec_data));
    if (cli_spec_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    /* Initialize pipe descriptors to undefined value */
    cli_spec_data->data_sock = cli_spec_data->sync_pipe = -1;

    define_kernel_version(cli_spec_data);
    if (cli_spec_data->kernel_like_2_4)
    {
        WARN("You are working with 2.4 kernel so we assume that you "
             "do not have NPTL Thread Library on your system.");
    }
    
    VERB("We are working with %s kernel",
         cli_spec_data->kernel_like_2_4 ? "2.4": "not 2.4");

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
            rc = TE_EINVAL;
            goto error;
    }

    cli_spec_data->program = cli_programs[cli_spec_data->conn_type];


    /* Get initial values of prompt parameters from CSAP SPEC */
    init_prompts = &cli_spec_data->init_prompts;
    cli_container_get_prompt_params(cli_csap_spec, init_prompts);

    /* Check that command-prompt value is specified (it is mandatory) */
    if (init_prompts->cmd.len == 0)
    {
        ERROR("Cannot get command prompt value");
        goto error;
    }
    VERB("command-prompt: %s", init_prompts->cmd.val);

    /* login-prompt value is optional */
    if (init_prompts->login.len != 0)
        VERB("login-prompt = %s", init_prompts->login.val);

    /* password-prompt value is optional */
    if (init_prompts->passwd.len != 0)
        VERB("password-prompt = %s", init_prompts->passwd.val);

    /*
     * Configure expect prompts based on values specified in CSAP init.
     * In order to be able to accept user commands Expect aware process
     * should get COMMAND PROMPT. As a part of getting COMMAND PROMPT
     * action the output can contain requests to enter login and password.
     */
    prepare_exp_prompts(init_prompts, &cli_spec_data->prompts[0]);


    /* Get user value (optional) */
    if ((rc = cli_get_asn_string_value(cli_csap_spec,
                  "user.#plain", &cli_spec_data->user)) != 0)
    {
        if (init_prompts->login.len != 0)
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
        if (init_prompts->passwd.len != 0)
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

    if (socketpair(PF_UNIX, SOCK_STREAM, 0, sv) != 0)
    {
        rc = te_rc_os2te(errno);
        ERROR("Cannot create a pair of sockets, errno %d", rc);
        goto error;
    }
    
    if (pipe(pipe_descrs) != 0)
    {
        rc = te_rc_os2te(errno);
        ERROR("Cannot create pipe, errno %d", rc);
        close(sv[0]);
        close(sv[1]);
        goto error;
    }

    csap_set_rw_data(csap, cli_spec_data);

    cli_spec_data->read_timeout = CLI_CSAP_DEFAULT_TIMEOUT;

    if ((cli_spec_data->expect_pid = fork()) == -1)
    {
        rc = te_rc_os2te(errno);
        ERROR("fork failed, errno %d", rc);
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
            tad_cli_rw_destroy_cb(csap);
            return TE_RC(TE_TAD_CSAP, MAP_SYN_RES2ERRNO(sync_res));
        }

        VERB("Child has just been initialised");

        /* As we do not call waitpid() at destroy time, we should print pid
         * of CSAP in the log to find if sigchild handler will tell us of 
         * any problems with exit status. */
        RING("CLI CSAP with pid %d was initialized", 
             cli_spec_data->expect_pid);
    }

    return 0;

error:
    free_cli_csap_data(cli_spec_data);

    return TE_RC(TE_TAD_CSAP, rc);
}


/* See description tad_cli_impl.h */
te_errno
tad_cli_rw_destroy_cb(csap_p csap)
{
    cli_csap_specific_data_p spec_data = csap_get_rw_data(csap);

    VERB("%s() started, CSAP %d", __FUNCTION__, csap->id);

    if (spec_data == NULL)
    {
        ERROR("%s(): Invalid pointer to specific data", __FUNCTION__);
        return -1;
    }

    if (spec_data->expect_pid > 0)
    {
        VERB("kill CLI session, pid=%d", spec_data->expect_pid);
        ta_kill_death(spec_data->expect_pid);
        spec_data->expect_pid = 0;
    }
    
    VERB("%s(): try to free CLI CSAP specific data", __FUNCTION__);
    free_cli_csap_data(spec_data);

    csap_set_rw_data(csap, NULL);
   
    return 0;
}

/**************************************************************************
 *
 * Routines executed in the Expect process context
 *
 **************************************************************************/

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
        VERB("> %s -l %s", spec_data->program, spec_data->device);
        sleep(4);
        spec_data->io = exp_spawnl(spec_data->program, spec_data->program,
                                   "-l", spec_data->device, NULL);
        sleep(4);

        /* Send '\r' to CLI session to get first command prompt */
        if (spec_data->io >= 0 && write(spec_data->io, "\r", 1) != 1)
        {
            ERROR("Failed to send initial CTR-R character to Expect, "
                  "errno = %d", errno);
        }
        sleep(2);
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
                 spec_data->program, user_param, spec_data->host, port_param);
            spec_data->io = exp_spawnl(spec_data->program, spec_data->program,
                                       user_param, spec_data->host, port_param, NULL);

            INFO("returned %d\n", spec_data->io);
        }
        else
        {
            INFO("> %s %s %s\n",
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
            VERB("> %s %s %s",
                 spec_data->program, shell_args_param, spec_data->shell_args);
            spec_data->io = exp_spawnl(spec_data->program, spec_data->program,
                                       shell_args_param, spec_data->shell_args,
                                       NULL);
        }
        else
        {
            VERB("> %s", spec_data->program);
            spec_data->io = exp_spawnl(spec_data->program, spec_data->program,
                                       NULL);
        }
    }

    if (spec_data->io < 0)
    {
        ERROR("exp_spawnl failed with errno=%d", errno);
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
        return TE_EINVAL;
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
        ta_kill_death(spec_data->session_pid);
        spec_data->session_pid = 0;
    }
    close(spec_data->io);

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
    VERB("%s(): Called with sync_val %s",
         __FUNCTION__, sync_res_h2str(sync_val));

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

    {
        struct exp_case *exp_prompt = &spec_data->prompts[0];

        VERB("Start waiting for:\n");
        while (exp_prompt->type != exp_end)
        {
            VERB("'%s', '%s', val %d\n",
                 (exp_prompt->type == exp_glob) ? "exp_glob" :
                 ((exp_prompt->type == exp_exact) ? "exp_exact" : "exp_regexp"),
                 exp_prompt->pattern,
                 exp_prompt->value);

            exp_prompt++;
        }
    }

    res = exp_expectv(spec_data->io, spec_data->prompts);

    switch (res)
    {
        case CLI_COMMAND_PROMPT:
            break;

        case CLI_LOGIN_PROMPT:
            VERB("Got login prompt");
            if (write_string_to_expect(spec_data, spec_data->user) != 0)
            {
                cli_expect_finalize(spec_data, SYNC_RES_FAILED);
            }
            break;

        case CLI_PASSWORD_PROMPT:
            VERB("Got password prompt");
            if (write_string_to_expect(spec_data, spec_data->password) != 0)
            {
                cli_expect_finalize(spec_data, SYNC_RES_FAILED);
            }
            break;

        case EXP_EOF:
            VERB("EOF detected");
            cli_expect_finalize(spec_data, SYNC_RES_ABORTED);
            break;
        
        case EXP_TIMEOUT:
            break;

        default:
            ERROR("Unexpected result got from exp_expectv(): %d", res);
            cli_expect_finalize(spec_data, SYNC_RES_FAILED);
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
    static char cmd_buf[1024];
    int         i;

    int     timeout;
    int     rc;
    char    data;
    int     reply_len;
    te_bool timeout_notif_sent = FALSE;

    logfork_register_user("CLI CSAP CHILD");

    /* Run program under expect control */
    if (cli_session_open(spec_data) != 0)
    {
        cli_expect_finalize(spec_data, SYNC_RES_FAILED);
    }

    INFO("CLI session is opened");

    /*
     * Capture command prompt to be sure that CSAP is ready to
     * run commands.
     */
    do {
        rc = cli_expect_wait_for_prompt(spec_data);
        if (rc == EXP_TIMEOUT)
        {
            ERROR("Timeout waiting for command prompt on startup");
            cli_expect_finalize(spec_data, SYNC_RES_FAILED);
        }
    } while (rc != CLI_COMMAND_PROMPT);

    /* Tell CSAP Engine that expect session is ready */
    child_send_sync(spec_data, SYNC_RES_OK);
    
    VERB("CLI session is synchronized with CSAP Engine");

    /* Wait for command from CSAP Engine */
    while (TRUE)
    {
        VERB("Start waiting for a command from CSAP Engine");

        /* First read timeout value */
        rc = read(spec_data->data_sock, &timeout, sizeof(timeout));
        if (rc != (int)sizeof(timeout))
        {
            ERROR("Cannot read timeout value from CSAP Engine, "
                  "rc = %d, errno = %d", rc, errno);
            cli_expect_finalize(spec_data, SYNC_RES_FAILED);
        }
        rc = read(spec_data->data_sock, &spec_data->cur_prompts, sizeof(spec_data->cur_prompts));
        if (rc != (int)sizeof(spec_data->cur_prompts))
        {
            ERROR("Cannot read current prompts from CSAP Engine, "
                  "rc = %d, errno = %d", rc, errno);
            cli_expect_finalize(spec_data, SYNC_RES_FAILED);
        }

        /*
         * Prepare a set of prompts Expect should be waiting for
         * based on the values passed as a part of command control
         * information.
         */
        prepare_exp_prompts(&spec_data->cur_prompts, &spec_data->prompts[0]);

        VERB("Start command mark, timeout %d", timeout);

        /* Update Expect timeout value */
        exp_timeout = timeout;

        i = 0;

        /* Read out the command to execute */
        while ((rc = read(spec_data->data_sock, &data, 1)) == 1 &&
               data != '\0')
        {
            cmd_buf[i++] = data;

            if ((rc = write(spec_data->io, &data, 1)) != 1)
            {
                ERROR("Cannot send '%c' character to expect, "
                      "rc = %d, errno = %d", data, rc, errno);
                cli_expect_finalize(spec_data, SYNC_RES_FAILED);
            }
        }

        if (rc != 1)
        {
            ERROR("Error occurred during reading command from "
                  "CSAP Engine side, rc = %d, errno = %d", rc, errno);
            cli_expect_finalize(spec_data, SYNC_RES_FAILED);
        }

        cmd_buf[i] = data;
        INFO("We are about to run '%s' command", cmd_buf);

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
        VERB("Start waiting prompt");

        do {
            rc = cli_expect_wait_for_prompt(spec_data);
            if (rc == EXP_TIMEOUT)
            {
                VERB("Timeout waiting for prompt");
                timeout_notif_sent = TRUE;
                child_send_sync(spec_data, SYNC_RES_TIMEOUT);
            }
        } while (rc != CLI_COMMAND_PROMPT);

        VERB("We've got prompt");

        if (timeout_notif_sent)
        {
            /*
             * We've got stuck wating for command prompt, but
             * now everything is OK, and we are ready to run 
             * next commands. Send notification about that.
             */
            VERB("Notify CSAP Engine that eventually we've got prompt");
            child_send_sync(spec_data, SYNC_RES_OK);

            timeout_notif_sent = FALSE;
        }

        VERB("Send reply for the command");

        /* Transfer CLI session output to the CSAP Engine */
        reply_len = (exp_match - exp_buffer);

        /*
         * Output the reply byte by byte removing ESCape sequences.
         * We need to remove ESC sequences because CSAP Engine removes
         * echoed characters not taking into account any ESC sequences,
         * i.e. if we do not remove them, we will return some garbage
         * to the user.
         * When we have equipment that sends a lot of different ESC
         * sequences, we will have to rewrite this code to support all
         * of them, but for now we are happy with excluding only the
         * particular ESC/CSI sequence "CSI J".
         */

#define ESC_SEQ_START 0x1b /* Start ESC sequence character */
#define ESC_CSI_CHAR  '[' /* Control Sequence Introducer (ESC + '[') */

        i = 0;
        while (i < reply_len)
        {
            if (exp_buffer[i] == ESC_SEQ_START)
            {
                /* ESCape Sequence starts */
                if ((i + 1) >= reply_len ||
                    exp_buffer[i + 1] != ESC_CSI_CHAR)
                {
                    ERROR("Broken or unsupported ESC sequence");
                    /* Flush all the data without parsing */
                    write(spec_data->data_sock, exp_buffer + i, reply_len - i);
                    break;
                }
                /*
                 * Right now we support only "CSI J" sequence that
                 * means "Erase from cursor to end of the screen".
                 */
                if ((i + 2) >= reply_len ||
                    exp_buffer[i + 2] != 'J')
                {
                    ERROR("Unsupported ESC sequence");
                    /* Flush all the data without parsing */
                    write(spec_data->data_sock, exp_buffer + i, reply_len - i);
                    break;
                }
                i += 3;
                continue;
            }
            rc = write(spec_data->data_sock, &exp_buffer[i], 1);
            if (rc != 1)
            {
                ERROR("Failed to send command reply to CSAP Engine, "
                      "rc = %d, errno = %d", rc, errno);
                cli_expect_finalize(spec_data, SYNC_RES_FAILED);
            }
            i++;
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
