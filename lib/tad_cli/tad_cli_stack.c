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
 * Author: Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define LGR_USER "TAD CLI STACK"

#include "tad_cli_impl.h"

#define CLI_COMMAND_PROMPT  0
#define CLI_LOGIN_PROMPT    1
#define CLI_PASSWORD_PROMPT 2

#define CLI_PROGRAM_NAME_SIZE           40
#define CLI_SESSION_PARAM_LENGTH_MAX    40

#define EXP_DEBUG
#undef EXP_DEBUG
#define TALOGDEBUG

static char cli_programs[][CLI_PROGRAM_NAME_SIZE] = {
    "millicom",
    "telnet",
    "ssh" };

/**
 * Read the STRING type value from the CSAP description (in ASN.1 notation).
 *
 * @param csap_spec    CSAP description (in ASN.1 notation).
 * @param asn_name     Name of the field to read.
 * @param str_value    Pointer to returned string.
 *
 * @return 0 on success or -1 if not found. 
 */ 
int cli_get_asn_string_value(asn_value_p csap_spec,
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
int cli_get_asn_integer_value(asn_value_p csap_spec,
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
 * Routines executed in the Expect process context
 *
 **************************************************************************/


/**
 * Open new CLI session (spawn CLI program).
 *
 * @param spec_data     csap special data structure.
 *
 * @return 0 on success or -1 if failed. 
 */ 
int cli_session_open(cli_csap_specific_data_p spec_data)
{
    FILE *dbg;
    FILE *log;

    /** -p<port> - port parameter */
    char port_param[CLI_SESSION_PARAM_LENGTH_MAX]; 

    /** -l<use> - user parameter */
    char user_param[CLI_SESSION_PARAM_LENGTH_MAX]; 

    printf("%s()\n", __FUNCTION__);

    spec_data->dbg_file = fopen("logfile.txt", "a+");
    if (spec_data->dbg_file == NULL)
    {
        printf("cannot open debug file");
        return -EINVAL;
    }

    fprintf(spec_data->dbg_file, "%s()\n", __FUNCTION__);
    fprintf(spec_data->dbg_file, "logfile fileno is %d\n", fileno(spec_data->dbg_file));
    fflush(spec_data->dbg_file);

    dbg = fopen("exp_debug.txt", "a+");
    if (dbg == NULL)
    {
        fprintf(spec_data->dbg_file, "cannot open debug file");
        fclose(spec_data->dbg_file);
        return -EINVAL;
    }
    log = fopen("exp_log.txt", "a+");
    if (dbg == NULL)
    {
        fprintf(spec_data->dbg_file, "cannot open log file");
        fclose(spec_data->dbg_file);
        return -EINVAL;
    }

    fprintf(spec_data->dbg_file, "exp_debugfile fileno is %d\n", fileno(dbg));
    fprintf(spec_data->dbg_file, "exp_logfile fileno is %d\n", fileno(log));

    fflush(spec_data->dbg_file);

#ifdef EXP_DEBUG
    exp_console = 1;
    exp_is_debugging = 1;
    exp_loguser = 1;
    exp_logfile_all = 1;
    
    exp_debugfile = dbg;
    exp_logfile = log;
#endif

#ifdef EXP_DEBUG    
    fprintf(exp_debugfile, "Debug started\n");
    fflush(exp_debugfile);

    fprintf(exp_logfile, "Log started\n");
    fflush(exp_logfile);
#endif

    if (spec_data->conn_type == CLI_CONN_TYPE_SERIAL)
    {
        fprintf(spec_data->dbg_file, "> %s %s\n",
                spec_data->program, spec_data->device);
        fflush(spec_data->dbg_file);
        spec_data->io = exp_spawnl(spec_data->program, spec_data->program,
                                   spec_data->device, NULL);
    }
    else
    {
        sprintf(port_param, "-p%d", spec_data->port);
        if (spec_data->user != NULL)
        {
            sprintf(user_param, "-l%s", spec_data->user);
            fprintf(spec_data->dbg_file, "> %s %s %s %s\n",
                    spec_data->program, spec_data->host, port_param, user_param);
            fflush(spec_data->dbg_file);
            fflush(exp_debugfile);
            fflush(exp_logfile);
            spec_data->io = exp_spawnl(spec_data->program, spec_data->program,
                                       spec_data->host, port_param, user_param, NULL);
            fflush(exp_debugfile);
            fflush(exp_logfile);

            fprintf(spec_data->dbg_file, "returned %d\n", spec_data->io);
            fflush(spec_data->dbg_file);
        }
        else
        {
            fprintf(spec_data->dbg_file, "> %s %s %s\n",
                    spec_data->program, spec_data->host, port_param);
            fflush(spec_data->dbg_file);
            spec_data->io = exp_spawnl(spec_data->program, spec_data->program,
                                       spec_data->host, port_param, NULL);
        }
    }

    fprintf(spec_data->dbg_file, "exp_spawnl() finished, fd=%d\n", spec_data->io);
    fflush(spec_data->dbg_file);
    
    if (spec_data->io == -1)
    {
        fprintf(spec_data->dbg_file, "exp_spawnl() failed\n");
        fclose(spec_data->dbg_file);
        return -EINVAL;
    }
    else {
        fprintf(spec_data->dbg_file, "exp_spawnl() sucessfull, fd=%d\n", spec_data->io);
    }

    spec_data->fp = fdopen(spec_data->io, "r+");
    if (spec_data->fp == NULL)
    {
        fprintf(spec_data->dbg_file, "fdopen(%d) failed\n", spec_data->io);
        fclose(spec_data->dbg_file);
        return -EINVAL;
    }

    spec_data->session_pid = exp_pid;
    
    fprintf(spec_data->dbg_file, "ExpectPID=%d, fd=%d\n", exp_pid, spec_data->io);
    printf("ExpectPID=%d, fd=%d\n", exp_pid, spec_data->io);
    fflush(spec_data->dbg_file);

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
int cli_session_close(cli_csap_specific_data_p spec_data)
{
    printf("%s()\n", __FUNCTION__);
    fprintf(spec_data->dbg_file, "%s()\n", __FUNCTION__);
    fflush(spec_data->dbg_file);

    /* terminate CLI session */
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

    fprintf(spec_data->dbg_file, "closing session\n\n");
    fclose(spec_data->dbg_file);

    return 0;
}

/**
 * Terminate current Expect process and corresponding CLI session.
 *
 * @param spec_data     csap special data structure.
 *
 * @return 0 on success or -1 if failed. 
 */ 
void
cli_expect_finalize(cli_csap_specific_data_p spec_data)
{
    printf("%s()\n", __FUNCTION__);

    /* terminate current CLI session */
    cli_session_close(spec_data);

    /* make sure that pipes are closed */
    close(spec_data->sync_p2c[0]);
    close(spec_data->sync_c2p[1]);

    /* terminate current Expect process */
    _exit(0);
}

int
cli_expect_wait_for_prompt(cli_csap_specific_data_p spec_data)
{
    int res;

    printf("%s()\n", __FUNCTION__);

    res = exp_expectv(spec_data->io, spec_data->prompts);

    switch (res)
    {
        case CLI_COMMAND_PROMPT:
#if 0
            if (write(spec_data->io, "\r", 1) != 1)
            {
                /* something wrong with CLI session */
                printf("something wrong with CLI session\n");
                cli_expect_finalize(spec_data);
            }
#endif
            break;

        case CLI_LOGIN_PROMPT:
            if (write(spec_data->io, spec_data->user,
                      strlen(spec_data->user)) != (int) strlen(spec_data->user))
            {
                /* something wrong with CLI session */
                printf("something wrong with CLI session\n");
                cli_expect_finalize(spec_data);
            }
            if (write(spec_data->io, "\r", 1) != 1)
            {
                /* something wrong with CLI session */
                printf("something wrong with CLI session\n");
                cli_expect_finalize(spec_data);
            }
            break;

        case CLI_PASSWORD_PROMPT:
            printf("Write password\n");
            if (write(spec_data->io, spec_data->password,
                      strlen(spec_data->password)) !=
                (int) strlen(spec_data->password))
            {
                /* something wrong with CLI session */
                printf("something wrong with CLI session\n");
                cli_expect_finalize(spec_data);
            }
            printf("Write \\r\n");
            if (write(spec_data->io, "\r", 1) != 1)
            {
                /* something wrong with CLI session */
                printf("something wrong with CLI session\n");
                cli_expect_finalize(spec_data);
            }
            printf("Password-prompt processed\n");
            break;

        case EXP_EOF:
            printf("EOF detected\n");
            cli_expect_finalize(spec_data);
            break;
        
        case EXP_TIMEOUT:
            printf("Expect timeout\n");
            break;

        default:
            res = -1;
            break;
    }
    
    return res;
}

int
cli_expect_main(cli_csap_specific_data_p spec_data)
{
    int rc;
    char data;
    char *pdata;
    fd_set read_set;

    printf("%s()\n", __FUNCTION__);

    if (cli_session_open(spec_data) != 0)
    {
        cli_expect_finalize(spec_data);
    }

    printf("cli_session_open() returns successful\n");
    
    /* Tell the CLI CSAP layer that expect session is ready */
    printf("Tell the CLI CSAP layer that expect session is ready\n");
    data = '\0';
    if (write(spec_data->sync_c2p[1], pdata, 1) != 1)
    {
        printf("write() failed on sync_c2p pipe\n");
        cli_expect_finalize(spec_data);
    }

    for (;;)
    {
        FD_ZERO(&read_set);
        FD_SET(spec_data->sync_p2c[0], &read_set);

        /* wait indefinitely */
        printf("wait indefinitely\n");
        rc = select(spec_data->sync_p2c[0] + 1, &read_set,
                    NULL, NULL, NULL);
        if (rc == 0)
        {
            /* an error occured on sync pipe or a signal has been delivered */
            printf("an error occured on sync pipe or a signal has been delivered\n");
            cli_expect_finalize(spec_data);
        }
    
        do {
            rc = cli_expect_wait_for_prompt(spec_data);
            if (rc == EXP_EOF)
            {
                /* something wrong with CLI session */
                printf("cli_expect_wait_for_prompt() returned EXP_EOF\n");
                cli_expect_finalize(spec_data);
            } else if (rc == EXP_FULLBUFFER) {
                printf("cli_expect_wait_for_prompt() returned EXP_FULLBUFFER\n");
            }
            
        } while (rc != CLI_COMMAND_PROMPT);

        printf("Transmit message: \'");
        
        do {
            if (read(spec_data->sync_p2c[0], &data, 1) != 1)
            {
                /* an error occured on sync_p2c pipe */
                printf("an error occured on sync_p2c pipe\n");
                cli_expect_finalize(spec_data);
            }
	    
            printf("%c", data);
            
            if (data != 0)
            {
                if (write(spec_data->io, &data, 1) != 1)
                {
                    /* something wrong with CLI session */
                    printf("something wrong with CLI session\n");
                    cli_expect_finalize(spec_data);
                }
            }
        } while (data != 0);

        printf("\'\n");

        /* send '\r' to CLI session to finish the command sequence */
        printf("send '\\r' to CLI session to finish the command sequence\n");
        if (write(spec_data->io, "\r", 1) != 1)
        {
            /* something wrong with CLI session pipe */
            printf("something wrong with CLI session pipe\n");
            cli_expect_finalize(spec_data);
        }

        do {
            rc = cli_expect_wait_for_prompt(spec_data);
            if (rc < 0)
            {
                /* something wrong with CLI session */
                printf("something wrong with CLI session\n");
                cli_expect_finalize(spec_data);
            }
        } while (rc != CLI_COMMAND_PROMPT);

        /* transfer CLI session output to the CSAP layer */
        printf("transfer CLI session output to the CSAP layer\n");
        printf("Receive CLI session output: \'");
        for (pdata = exp_buffer; pdata < exp_match; pdata++)
        {
            if (write(spec_data->sync_c2p[1], pdata, 1) != 1)
            {
                /* something wrong with sync_c2p pipe */
                printf("something wrong with sync_c2p pipe\n");
                cli_expect_finalize(spec_data);
            }
            printf("%c", *pdata);
        }
        printf("\'\n");

        /* send '\r' to CLI session to generate another prompt */
        printf("send '\\r' to CLI session to generate another prompt\n");
        if (write(spec_data->io, "\r", 1) != 1)
        {
            /* something wrong with CLI session pipe */
            printf("something wrong with CLI session pipe\n");
            cli_expect_finalize(spec_data);
        }

        /* finish transfer */
        printf("finish transfer\n");
        data = '\0';
        if (write(spec_data->sync_c2p[1], &data, 1) != 1)
        {
            /* something wrong with sync_c2p pipe */
            printf("something wrong with sync_c2p pipe\n");
            cli_expect_finalize(spec_data);
        }
    }
    
    return 0;
}

/**************************************************************************
 *
 * Routines executed in the CSAP layer process context
 *
 **************************************************************************/


/**
 * Find number of CLI layer in CSAP stack.
 *
 * @param csap_descr    CSAP description structure.
 * @param layer_name    Name of the layer to find.
 *
 * @return number of layer (start from zero) or -1 if not found. 
 */ 
int 
find_csap_layer(csap_p csap_descr, char *layer_name)
{
    int i; 
    for (i = 0; i < csap_descr->depth; i++)
        if (strcmp(csap_descr->proto[i], layer_name) == 0)
            return i;
    return -1;
}


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
    
    if (spec_data->sync_p2c[1] >= 0)
        close(spec_data->sync_p2c[1]);

    if (spec_data->sync_c2p[0] >= 0)
        close(spec_data->sync_c2p[0]);

    if (spec_data->device != NULL)
       free(spec_data->device);

    if (spec_data->host != NULL)
       free(spec_data->host);   

    if (spec_data->user != NULL)
       free(spec_data->user);   

    if (spec_data->password != NULL)
       free(spec_data->password);   

    for (i = 0; i < CLI_MAX_PROMPTS; i++)
    {
        if (spec_data->prompts[i].pattern != NULL)
            free(spec_data->prompts[i].pattern);

        if (spec_data->prompts[i].re != NULL)
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
cli_read_cb (csap_p csap_descr, int timeout, char *buf, int buf_len)
{
    UNUSED(csap_descr);
    UNUSED(timeout);
    UNUSED(buf);
    UNUSED(buf_len);

    return EOPNOTSUPP;
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
cli_write_cb (csap_p csap_descr, char *buf, int buf_len)
{
    cli_csap_specific_data_p    spec_data;
    int                         layer;    

    fd_set                      read_set;
    char                        data = -1;
    int                         bytes_written = 0;
    int                         rc = 0;
    
    if (csap_descr == NULL)
    {
        return -1;
    }
    
    layer = csap_descr->read_write_layer;
    
    spec_data = (cli_csap_specific_data_p) csap_descr->layer_data[layer]; 

#ifdef TALOGDEBUG
    printf("Writing data to CLI session: %d", spec_data->io);
#endif        

    if(spec_data->io < 0)
    {
        return -1;
    }

    bytes_written = write(spec_data->sync_p2c[1], buf, buf_len);
    if (bytes_written != buf_len)
    {
        /* something wrong with sync_p2c pipe */
        cli_single_destroy_cb(csap_descr->id, layer);
        return -1;
    }
    
    FD_ZERO(&read_set);
    FD_SET(spec_data->sync_c2p[0], &read_set);
    
    /* wait forever */
    do {
        rc = select(spec_data->sync_c2p[0] + 1, &read_set, NULL, NULL, NULL);
    } while ((rc == -1) && (errno == EINTR));

    if (rc <= 0)
    {
        /* something wrong with sync_c2p pipe */
        cli_single_destroy_cb(csap_descr->id, layer);
        return -1;
    }

    do {
        if (read(spec_data->sync_c2p[0], &data, 1) != 1)
        {
            /* something wrong with sync_c2p pipe */
            cli_single_destroy_cb(csap_descr->id, layer);
            return -1;
        }
    } while (data != 0);

    return bytes_written;
}

/**
 * Callback for write data to media of cliernet CSAP and read
 *  data from media just after write, to get answer to sent request. 
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
cli_write_read_cb (csap_p csap_descr, int timeout,
                   char *w_buf, int w_buf_len,
                   char *r_buf, int r_buf_len)
{
    cli_csap_specific_data_p    spec_data;
    int                         layer;    

    fd_set                      read_set;
    char                        data = -1;
    int                         bytes_written = 0;
    int                         bytes_read = 0;
    int                         rc = 0;

    /* XXX: timeout should be used */
    UNUSED(timeout);

    printf("In function %s(%d)\n", __FUNCTION__, csap_descr->id);
    
    if (csap_descr == NULL)
    {
        return -1;
    }
    
    layer = csap_descr->read_write_layer;
    
    spec_data = (cli_csap_specific_data_p) csap_descr->layer_data[layer]; 

    {
        pid_t pid;
        int   status;
        pid = waitpid(spec_data->expect_pid, &status, WNOHANG);
        if (pid < 0)
        {
            printf("waitpid(%d) error\n", spec_data->expect_pid);
        } else
        if (pid == 0)
        {
            printf("the child pid=%d is still alive\n", spec_data->expect_pid);
        } else {
            printf("the child pid=%d is finished\n", spec_data->expect_pid);
        }
    }

#ifdef TALOGDEBUG
    printf("Writing %s (%d) bytes to CLI session %d\n",
           w_buf, w_buf_len, spec_data->sync_p2c[1]);
#endif        

    if(spec_data->io < 0)
    {
        printf("Not session opened\n");
        return -1;
    }

    bytes_written = write(spec_data->sync_p2c[1], w_buf, w_buf_len);

    {
        pid_t pid;
        int   status;
        pid = waitpid(spec_data->expect_pid, &status, WNOHANG);
        if (pid < 0)
        {
            printf("waitpid(%d) error\n", spec_data->expect_pid);
        } else
        if (pid == 0)
        {
            printf("the child pid=%d is still alive\n", spec_data->expect_pid);
        } else {
            printf("the child pid=%d is finished\n", spec_data->expect_pid);
        }
    }

    if (bytes_written != w_buf_len)
    {
        /* something wrong with sync_p2c pipe */
        printf("something wrong with sync_p2c pipe, %d bytes written\n", bytes_written);
        cli_single_destroy_cb(csap_descr->id, layer);
        return -1;
    }
    
    FD_ZERO(&read_set);
    FD_SET(spec_data->sync_c2p[0], &read_set);
    
    /* wait forever */
    printf("wait for data from CLI session\n");
    do {
        rc = select(spec_data->sync_c2p[0] + 1, &read_set, NULL, NULL, NULL);
    } while ((rc == -1) && (errno == EINTR));

    if (rc <= 0)
    {
        /* something wrong with sync_c2p pipe */
        cli_single_destroy_cb(csap_descr->id, layer);
        return -1;
    }

    do {
        if (read(spec_data->sync_c2p[0], &data, 1) != 1)
        {
            /* something wrong with sync_c2p pipe */
            cli_single_destroy_cb(csap_descr->id, layer);
            return -1;
        }
        
        if ((bytes_read < r_buf_len) && (data != 0))
        {
            r_buf[bytes_read] = data;
            bytes_read++;
        }
    } while (data != 0);
    
    if (bytes_read < r_buf_len)
        r_buf[bytes_read] = '\0';
    
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
cli_single_init_cb (int csap_id, const asn_value_p csap_nds, int layer)
{
    int      rc;
    int      tmp_len;

    struct exp_case *prompt;    /**< prompts the Expect process waits
                                     from the CLI session                     */

    csap_p   csap_descr;        /**< csap description                         */

    cli_csap_specific_data_p    cli_spec_data; /**< CLI CSAP specific data    */
    asn_value_p                 cli_csap_spec; /**< ASN value with csap init
                                                    parameters */
    
    printf("%s()\n", __FUNCTION__);

    if (csap_nds == NULL)
        return ETEWRONGPTR;

    if ((csap_descr = csap_find (csap_id)) == NULL)
        return ETADCSAPNOTEX;

    cli_csap_spec = asn_read_indexed(csap_nds, layer, "");

    cli_spec_data = malloc (sizeof(cli_csap_specific_data_t));
    if (cli_spec_data == NULL)
        return ENOMEM;

    memset(cli_spec_data, 0, (sizeof(cli_csap_specific_data_t)));
    

    /* get conn-type value (mandatory) */
    tmp_len = sizeof(cli_spec_data->conn_type);
    rc = asn_read_value_field(cli_csap_spec,
                              &cli_spec_data->conn_type,
                              &tmp_len,
                              "conn-type");
    if (rc != 0)
    {
        goto error;
    }

    printf("  conn-type=%d (using %s)\n", cli_spec_data->conn_type,
           cli_programs[cli_spec_data->conn_type]);

    switch (cli_spec_data->conn_type)
    {
        case CLI_CONN_TYPE_SERIAL:
            printf("  getting device name...\n");

            /* get conn-params.serial.device value (mandatory) */
            if ((rc =
                cli_get_asn_string_value(cli_csap_spec,
                                         "conn-params.#serial.device.#plain",
                                         &cli_spec_data->device)) != 0)
            {
                goto error;
            }
            printf("  device=%s\n", cli_spec_data->device);
            break;
            
        case CLI_CONN_TYPE_TELNET:
        case CLI_CONN_TYPE_SSH:
            printf("  getting host name...\n");

            /* get conn-params.telnet.host value (mandatory) */
            if ((rc =
                cli_get_asn_string_value(cli_csap_spec,
                                         "conn-params.#telnet.host.#plain",
                                         &cli_spec_data->host)) != 0)
            {
                goto error;
            }
            printf("  host=%s\n", cli_spec_data->host);

            printf("  getting host port...\n");
            /* get conn-params.telnet.port value (mandatory) */

            tmp_len = sizeof(cli_spec_data->port);
            rc = asn_read_value_field(cli_csap_spec,
                                      &cli_spec_data->port,
                                      &tmp_len,
                                      "conn-params.#telnet.port.#plain");
            if (rc != 0)
#if 0
            if ((rc =
                cli_get_asn_integer_value(cli_csap_spec,
                                          "conn-params.#telnet.port.#plain",
                                          &cli_spec_data->port)) != 0)
#endif
            {
                goto error;
            }
            printf("  port=%d\n", cli_spec_data->port);

            break;

        default:
            rc = EINVAL;
            goto error;
    }

    cli_spec_data->program = cli_programs[cli_spec_data->conn_type];

    cli_spec_data->prompts_status = 0;

    prompt = &cli_spec_data->prompts[0];

    /* get command-prompt value (mandatory) */
    if ((rc =
        cli_get_asn_string_value(cli_csap_spec,
                                 "command-prompt.#plain",
                                 &prompt->pattern)) == 0)
    {
        printf("  command-prompt=%s\n", prompt->pattern);
        cli_spec_data->prompts_status |= CLI_PROMPT_STATUS_COMMAND;
        prompt->type = exp_glob;
        prompt->value = CLI_COMMAND_PROMPT;
        prompt++;
    }
    else
    {
        if ((rc =
            cli_get_asn_string_value(cli_csap_spec,
                                     "command-prompt.#script",
                                     &prompt->pattern)) == 0)
        {
            printf("  command-prompt=%s\n", prompt->pattern);
            cli_spec_data->prompts_status |= CLI_PROMPT_STATUS_COMMAND;
            prompt->type = exp_regexp;
            prompt->value = CLI_COMMAND_PROMPT;
            prompt++;
        }
        else
        {
            goto error;
        }
    }

    /* get login-prompt value (optional) */
    if ((rc =
        cli_get_asn_string_value(cli_csap_spec,
                                 "login-prompt.#plain",
                                 &prompt->pattern)) == 0)
    {
        printf("  login-prompt=%s\n", prompt->pattern);
        cli_spec_data->prompts_status |= CLI_PROMPT_STATUS_LOGIN;
        prompt->type = exp_glob;
        prompt->value = CLI_LOGIN_PROMPT;
        prompt++;
    }
    else
    {
        if ((rc =
            cli_get_asn_string_value(cli_csap_spec,
                                     "login-prompt.#script",
                                     &prompt->pattern)) == 0)
        {
            printf("  login-prompt=%s\n", prompt->pattern);
            cli_spec_data->prompts_status |= CLI_PROMPT_STATUS_LOGIN;
            prompt->type = exp_regexp;
            prompt->value = CLI_LOGIN_PROMPT;
            prompt++;
        }
    }

    /* get password-prompt value (optional) */
    if ((rc =
        cli_get_asn_string_value(cli_csap_spec,
                                 "password-prompt.#plain",
                                 &prompt->pattern)) == 0)
    {
        printf("  password-prompt=%s\n", prompt->pattern);
        cli_spec_data->prompts_status |= CLI_PROMPT_STATUS_PASSWORD;
        prompt->type = exp_glob;
        prompt->value = CLI_PASSWORD_PROMPT;
        prompt++;
    }
    else
    {
        if ((rc =
            cli_get_asn_string_value(cli_csap_spec,
                                     "password-prompt.#script",
                                     &prompt->pattern)) == 0)
        {
            printf("  password-prompt=%s\n", prompt->pattern);
            cli_spec_data->prompts_status |= CLI_PROMPT_STATUS_PASSWORD;
            prompt->type = exp_regexp;
            prompt->value = CLI_PASSWORD_PROMPT;
            prompt++;
        }
    }
    
    prompt->type = exp_end;

    /* get user value (optional) */
    if ((rc =
        cli_get_asn_string_value(cli_csap_spec,
                                 "user.#plain",
                                 &cli_spec_data->user)) != 0)
    {
        if ((cli_spec_data->prompts_status & CLI_PROMPT_STATUS_LOGIN) != 0)
        {
            goto error;
        }
    }
    else
    {
        printf("  user=%s\n", cli_spec_data->user);
    }

    /* get user value (optional) */
    if ((rc =
        cli_get_asn_string_value(cli_csap_spec,
                                 "password.#plain",
                                 &cli_spec_data->password)) != 0)
    {
        if ((cli_spec_data->prompts_status & CLI_PROMPT_STATUS_PASSWORD) != 0)
        {
            goto error;
        }
    }
    else
    {
        printf("  password=%s\n", cli_spec_data->password);
    }

    /* default read timeout */
    cli_spec_data->read_timeout = CLI_CSAP_DEFAULT_TIMEOUT; 

    csap_descr->layer_data[layer] = cli_spec_data;

    csap_descr->read_cb           = cli_read_cb;
    csap_descr->write_cb          = cli_write_cb;
    csap_descr->write_read_cb     = cli_write_read_cb;
    csap_descr->read_write_layer  = layer; 
    csap_descr->timeout           = 500000;

    if (pipe(cli_spec_data->sync_p2c) == -1)
        goto error;

    if (pipe(cli_spec_data->sync_c2p) == -1)
        goto error;

    printf("prepare to fork()\n");

    if ((cli_spec_data->expect_pid = fork()) == -1)
    {
        goto error;
    }
    
    if (cli_spec_data->expect_pid == 0)
    {
        /* child */
        close(cli_spec_data->sync_p2c[1]);
        close(cli_spec_data->sync_c2p[0]);

        printf("child process started, send=%d, recv=%d\n",
               cli_spec_data->sync_c2p[1], cli_spec_data->sync_p2c[0]);

        cli_expect_main(cli_spec_data);
    }
    else
    {
        /* parent */
        char data;
        fd_set read_set;
        
        close(cli_spec_data->sync_p2c[0]);
        close(cli_spec_data->sync_c2p[1]);
        printf("parent process continues, child_pid=%d, send=%d, recv=%d\n",
               cli_spec_data->expect_pid, cli_spec_data->sync_p2c[1], cli_spec_data->sync_c2p[0]);

        /* Wait for child initialisation finished */

        FD_ZERO(&read_set);
        FD_SET(cli_spec_data->sync_c2p[0], &read_set);

        /* wait indefinitely */
        printf("wait indefinitely\n");
        rc = select(cli_spec_data->sync_c2p[0] + 1, &read_set,
                    NULL, NULL, NULL);
        if (rc == 0)
        {
            printf("select() failed on sync_c2p pipe or a signal has been delivered\n");
            cli_single_destroy_cb(csap_descr->id, layer);
            goto error;
        }

        printf("read() sync byte from sync_c2p pipe\n");
        if (read(cli_spec_data->sync_c2p[0], &data, 1) != 1)
        {
            printf("read() failed on sync_c2p pipe\n");
            cli_single_destroy_cb(csap_descr->id, layer);
            goto error;
        }
        
        printf("Child is initalised...\n");
    }
    
    return 0;

error:
    free_cli_csap_data(cli_spec_data);

    return rc;
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
cli_single_destroy_cb (int csap_id, int layer)
{
    csap_p csap_descr = csap_find(csap_id);

    cli_csap_specific_data_p spec_data = 
        (cli_csap_specific_data_p) csap_descr->layer_data[layer];

    if (spec_data->expect_pid > 0)
    {
        kill(spec_data->expect_pid, SIGKILL);
    }
    
    waitpid(spec_data->expect_pid, NULL, 0);
    
    free_cli_csap_data(spec_data);

    csap_descr->layer_data[layer] = NULL;
   
    return 0;
}


