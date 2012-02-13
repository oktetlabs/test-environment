/** @file
 * @brief TE project. Logger subsystem.
 *
 * Logger executable module.
 *
 *
 * Copyright (C) 2003-2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
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
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Igor B. Vasiliev <Igor.Vasiliev@oktetlabs.ru>
 * @author Edward Makarov <Edward.Makarov@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_TIME_H
#include <time.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_SIGNAL_H
#include <signal.h>
#endif
#if HAVE_POPT_H
#include <popt.h>
#else
#error popt library (development version) is required for Logger
#endif

#include "te_raw_log.h"
#include "logger_int.h"
#include "logger_internal.h"
#include "logger_ten.h"

#define LGR_TA_MAX_BUF      0x4000 /* FIXME */

/** Initial (minimum) Logger message buffer size */
#define LGR_MSG_BUF_MIN     0x100

#define FREAD(_fd, _buf, _len) \
    fread((_buf), sizeof(uint8_t), (_len), (_fd))

#define SET_MSEC(_poll) ((_poll) % 1000000)


DEFINE_LGR_ENTITY("Logger");

/* TA single linked list */
ta_inst *ta_list = NULL;

snif_polling_sets_t snifp_sets;

/* Path to the directory for logs */
const char *te_log_dir = NULL;

/* Raw log file */
static FILE *raw_file = NULL;

/** Logger PID */
static pid_t    pid;

static unsigned int         lgr_flags = 0;
/** @name Logger global context flags */
#define LOGGER_FOREGROUND   0x01    /**< Run Logger in foreground */
#define LOGGER_NO_RCF       0x02    /**< Run Logger without interaction
                                         with RCF */
#define LOGGER_CHECK        0x04    /**< Check messages before store in
                                         raw log file */
#define LOGGER_SHUTDOWN     0x10    /**< Logger is shuting down */
/*@}*/

static const char          *cfg_file = NULL;
static struct ipc_server   *logger_ten_srv = NULL;

/**
 * Get NFL from buffer in TE raw log format.
 *
 * @param buf   Pointer to the buffer
 *
 * @return NFL value in host byte order
 */
static inline te_log_nfl
te_log_raw_get_nfl(const void *buf)
{
    te_log_nfl  nfl;
    
    memcpy(&nfl, buf, sizeof(nfl));
#if HAVE_ASSERT_H
    assert(sizeof(nfl) == 2);
#endif
    return ntohs(nfl);
}

/**
 * Check raw log message format.
 *
 * @param msg       Log message location
 * @param len       Log message length
 *
 * @return Is log message valid?
 */
static te_bool
lgr_message_valid(const void *msg, size_t len)
{
    const uint8_t *p = msg;

    UNUSED(p);
    UNUSED(len);

    return TRUE;
}

/**
 * Register the log message in the raw log file.
 *
 * @param buf       Log message location
 * @param len       Log message length
 */
void
lgr_register_message(const void *buf, size_t len)
{
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

    if ((lgr_flags & LOGGER_CHECK) && !lgr_message_valid(buf, len))
        return;

    pthread_mutex_lock(&mutex);
    if (fwrite(buf, len, 1, raw_file) != 1)
        perror("fwrite() failure");
    if (fflush(raw_file) != 0)
        perror("fflush(raw_file) failed");
    pthread_mutex_unlock(&mutex);
}

static pthread_mutex_t add_remove_mutex = PTHREAD_MUTEX_INITIALIZER;

static void
add_inst(ta_inst *inst)
{
    pthread_mutex_lock(&add_remove_mutex);
    inst->next = ta_list;
    ta_list = inst;
    pthread_mutex_unlock(&add_remove_mutex);
}

static void
remove_inst(ta_inst *inst)
{
    ta_inst **a;

    pthread_mutex_lock(&add_remove_mutex);
    for (a = &ta_list; *a != NULL; a = &(*a)->next)
    {
        if (*a == inst)
        {
            *a = inst->next;
            break;
        }
    }
    pthread_mutex_unlock(&add_remove_mutex);
}

/** Forward declaration */
static void * ta_handler(void *ta);

/**
 * This is an entry point of logger message server.
 * This server should be run as separate thread.
 * All log messages from TEN entities will be processed by
 * this routine.
 */
static void *
te_handler(void)
{
    struct ipc_server          *srv = logger_ten_srv;
    struct ipc_server_client   *ipcsc_p;
    size_t                      buf_len;
    void                       *buf;
    size_t                      len;
    te_errno                    rc;
    pthread_t                   sniffer_mark_thread;
    char                        err_buf[BUFSIZ];
    char                       *arg_str;

    buf_len = LGR_MSG_BUF_MIN;
    buf = malloc(buf_len);
    if (buf == NULL)
    {
        ERROR("%s(): malloc() failed", __FUNCTION__);
        return NULL;
    }

    while (TRUE) /* Forever loop */
    {
        len = buf_len;
        ipcsc_p = NULL;
        rc = ipc_receive_message(srv, buf, &len, &ipcsc_p);
        if (rc != 0)
        {
            size_t received;
            size_t total;

            if (rc != TE_RC(TE_IPC, TE_ESMALLBUF))
            {
                ERROR("Message receiving failure: %r", rc);
                break;
            }
            
            received = buf_len;
            total = buf_len + len;
            /* Increase buffer length until message does not fit in it */
            do {
                /* Double size of the buffer */
                buf_len = buf_len << 1;
            } while (buf_len < total);
            
            /* Reallocate the buffer */
            buf = realloc(buf, buf_len);
            if (buf == NULL)
            {
                ERROR("%s(): realloc(p, %u) failed",
                      __FUNCTION__, buf_len);
                break;
            }

            /* Receive the rest of the message */
            len = buf_len - received;
            rc = ipc_receive_message(srv, (uint8_t *)buf + received,
                                     &len, &ipcsc_p);
            if (rc != 0)
            {
                ERROR("Failed to receive the rest of the message "
                      "from client, rest=%u: %r", len, rc);
                break;
            }
            if (received + len != total)
            {
                ERROR("Invalid length of the rest of the message "
                      "in comparison with declared first: total=%u, "
                      "first=%u, rest=%u", total, received, len);
                break;
            }
            /* 'len' should contain full length of the message */
            len = total;
        }

        {
            char const *msg = (void *)((char *)buf + sizeof(te_log_nfl));

            unsigned int const ml = te_log_raw_get_nfl(buf);
            unsigned int const pl = strlen(LGR_SRV_FOR_TA_PREFIX);
            unsigned int       data_len;

            if (ml + sizeof(te_log_nfl) == len &&
                strncmp(msg, LGR_SHUTDOWN, ml) == 0)
            {
                RING("Logger shutdown ...\n");
                lgr_flags |= LOGGER_SHUTDOWN;
                (void)kill(pid, SIGUSR1);
                break;
            }
            /* Check whether logger TA handler invocation is needed */
            else if (ml + sizeof(te_log_nfl) == len &&
                     ml > pl && ml - pl < RCF_MAX_NAME &&
                     strncmp(msg, LGR_SRV_FOR_TA_PREFIX, pl) == 0)
            {
                ta_inst *inst = calloc(1, sizeof(*inst));

                if (inst == NULL)
                {
                    ERROR("No Memory");
                    continue;
                }

                inst->next = ta_list;
                inst->thread_run = FALSE;
                memcpy(inst->agent, msg + pl, ml - pl);

                RING("Logger '%s' TA handler is being added", inst->agent);

                if ((rc = rcf_ta_name2type(inst->agent, inst->type)) != 0)
                {
                    ERROR("Cannot interact with RCF: %r", rc);
                    free(inst);
                    continue;
                }

                if (pthread_create(&inst->thread, NULL,
                                   (void *)&ta_handler,
                                   (void *)inst) != 0)
                {
                    ERROR("Logger(client): pthread_create() failed: %s",
                          strerror(errno));
                    free(inst);
                    continue;
                }

                inst->thread_run = TRUE;
                add_inst(inst);

                RING("Logger '%s' TA handler has been added", inst->agent);
                INFO("Logger configuration file parsing\n");

                if (configParser(cfg_file) != 0)
                    WARN("Logger configuration file failure\n");
            }
            /* Check whether insert sniffer mark invocation is needed */
            else if (ml + sizeof(te_log_nfl) == len &&
                     ml >= strlen(LGR_SRV_SNIFFER_MARK) &&
                     strncmp(msg, LGR_SRV_SNIFFER_MARK,
                             strlen(LGR_SRV_SNIFFER_MARK)) == 0)
            {
                data_len = ml - strlen(LGR_SRV_SNIFFER_MARK) + 1;
                arg_str = malloc(data_len);
                snprintf(arg_str, data_len, "%s",
                         msg + strlen(LGR_SRV_SNIFFER_MARK));
                arg_str[data_len - 1] = '\0';
                /* Create separate thread for sniffer mark processing */
                rc = pthread_create(&sniffer_mark_thread, NULL,
                                    (void *)&sniffer_mark_handler,
                                    arg_str);
                if (rc != 0)
                {
                    strerror_r(errno, err_buf, sizeof(err_buf));
                    ERROR("Sniffer: pthread_create() failed: %s\n",
                          err_buf);
                }
            }
            else
            {
                lgr_register_message(buf, len);
            }
        }
    } /* end of forever loop */

    free(buf);

    return NULL;
}


/**
 * Reply to flush operation requester when it is done.
 *
 * @param srv       Logger IPC server for TA
 *
 * @return Status code.
 */
static int
ta_flush_done(struct ipc_server *srv)
{
    struct ipc_server_client   *ipcsc_p = NULL;
    uint8_t                     buf[strlen(LGR_FLUSH) + 1];
    size_t                      len = sizeof(buf);
    int                         rc;

    rc = ipc_receive_message(srv, buf, &len, &ipcsc_p);
    if (rc != 0)
    {
        ERROR("FATAL ERROR: Failed to read flush request: %r", rc);
        return rc;
    }
    rc = ipc_send_answer(srv, ipcsc_p, buf, len);
    if (rc != 0)
    {
        ERROR("FATAL ERROR: Failed to send answer to flush "
              "request: %r", rc);
        return rc;
    }

    return 0;
}


/**
 * This is an entry point of TA log message gatherer.
 * This routine periodically polls appropriate TA to get
 * TA local log. Besides, log is solicited if flush is requested.
 *
 * @param  ta   Location of TA parameters.
 *
 * @return NULL
 */
static void *
ta_handler(void *ta)
{
    /* TA and IPC server data and related variables */
    ta_inst            *inst = (ta_inst *)ta;
    char                srv_name[LGR_MAX_NAME];
    struct ipc_server  *srv = NULL;
    int                 fd_server;
    fd_set              rfds;
    int                 rc;
    pthread_t           sniffer_thread;
    char                err_buf[BUFSIZ];

    /* Polling variables */
    unsigned long int   polling;
    struct timeval      poll_ts;    /**< The last poll time stamp */
    struct timeval      now;        /**< Current time */

    /* Flush variavles */
    te_bool             do_flush = FALSE;
    te_bool             flush_done = FALSE;
    unsigned int        flush_msg_max = 0;
    struct timeval      flush_ts;   /**< Time stamp when flush has been
                                         started */
    te_log_ts_sec       msg_ts_sec;
    te_log_ts_usec      msg_ts_usec;

    /* Log file processing variables */
    char                log_file[RCF_MAX_PATH];
    struct stat         log_file_stat;
    size_t              ta_name_len = strlen(inst->agent);
    FILE               *ta_file;
    uint8_t             buf[LGR_TA_MAX_BUF];


    /* Register IPC Server for the TA */
    sprintf(srv_name, "%s%s", LGR_SRV_FOR_TA_PREFIX, inst->agent);
    rc = ipc_register_server(srv_name, LOGGER_IPC, &srv);
    if (rc != 0)
    {
        ERROR("Failed to register IPC server '%s': %r", srv_name, rc);
        return NULL;
    }
    assert(srv != NULL);
    fd_server = ipc_get_server_fd(srv);

    /* Do not allow to poll in flood mode */
    if (inst->polling == 0)
        inst->polling = LGR_TA_POLL_DEF;

    /* Recalculate polling timeout in microseconds */
    polling = TE_MS2US(inst->polling);

    /* It not so important to poll at start up */
    gettimeofday(&poll_ts, NULL);

    /* Create separate thread for sniffers log message processing */
    rc = pthread_create(&sniffer_thread, NULL, (void *)&sniffers_handler, 
                        inst->agent);
    if (rc != 0)
    {
        strerror_r(errno, err_buf, sizeof(err_buf));
        ERROR("Sniffer: pthread_create() failed: %s\n", err_buf);
    }

    while (1)
    {
        /* If flush operation is done, reply to requester */
        if (flush_done)
        {
            flush_done = FALSE;
            if (ta_flush_done(srv) != 0)
                break;
        }

        /* 
         * If we are not flushing, wait for polling timeout or
         * flush request
         */
        if (!do_flush)
        {
            struct timeval delay = { 0, 0 };

            gettimeofday(&now, NULL);

            /* Calculate moment until we should wait before next get log */
            poll_ts.tv_sec  += TE_US2SEC(polling);
            poll_ts.tv_usec += SET_MSEC(polling);
            if (poll_ts.tv_usec >= 1000000)
            {
                poll_ts.tv_usec -= 1000000;
                poll_ts.tv_sec++;
            }

            /*
             * Calculate delay of waiting we should wait
             * before next get log.
             */
            if (poll_ts.tv_sec > now.tv_sec)
                delay.tv_sec = poll_ts.tv_sec - now.tv_sec;

            if (poll_ts.tv_usec >= now.tv_usec)
            {
                delay.tv_usec = poll_ts.tv_usec - now.tv_usec;
            }
            else if (delay.tv_sec > 0) 
            {
                delay.tv_sec--;
                delay.tv_usec = (poll_ts.tv_usec + 1000000) - now.tv_usec;
            }

            FD_ZERO(&rfds);
            FD_SET(fd_server, &rfds);

            rc = select(fd_server + 1, &rfds, NULL, NULL, &delay);
            if (rc < 0)
            {
                break;
            }
            else if (rc > 0)
            {
                if (FD_ISSET(fd_server, &rfds))
                {
                    /* Go into the logs flush mode */
                    do_flush = TRUE;
                    flush_msg_max = LGR_FLUSH_TA_MSG_MAX;
                    gettimeofday(&flush_ts, NULL);
                }
                else
                {
                    ERROR("FATAL ERROR: TA %s: select() returns %d, "
                          "but server fd is not readable",
                          inst->agent, rc);
                    break;
                }
            }

            /* Get time for filtering incoming messages */
        }

        /* Make time stamp when we poll TA */
        gettimeofday(&poll_ts, NULL);

        *log_file = '\0';
        if ((rc = rcf_ta_get_log(inst->agent, log_file)) != 0)
        {
            /* Any error interrupts flush operation */
            if (do_flush)
            {
                do_flush = FALSE;
                flush_done = TRUE;
            }
            if (/* No log messages */
                (rc == TE_RC(TE_RCF_PCH, TE_ENOENT)) ||
                /* RCF request to TA is timed out */
                (rc == TE_RC(TE_RCF, TE_ETIMEDOUT)) ||
                /* TA is being rebooted, or has been rebooted */
                (rc == TE_RC(TE_RCF, TE_ETAREBOOTED)) ||
                /* TA has dies, but may be revivified later by RCF */
                (rc == TE_RC(TE_RCF, TE_ETADEAD)))
            {
                continue;
            }
            else
            {
                /* The rest of errors are considered as fatal */
                break;
            }
        }

        rc = stat(log_file, &log_file_stat);
        if (rc < 0)
        {
            ERROR("FATAL ERROR: TA %s: log file '%s' stat() failure: "
                  "errno=%d", inst->agent, log_file, errno);
            break;
        }
        else if (log_file_stat.st_size == 0)
        {
            /* File is empty */
            ERROR("TA %s: log file '%s' is empty", inst->agent, log_file);

            if (remove(log_file) != 0)
            {
                ERROR("Failed to delete log file '%s': errno=%d",
                      log_file, errno);
                /* Continue */
            }
            if (do_flush)
            {
                do_flush = FALSE;
                flush_done = TRUE;
            }
            continue;
        }

        ta_file = fopen(log_file, "r");
        if (ta_file == NULL)
        {
            ERROR("FATAL ERROR: TA %s: fopen(%s) failure: errno=%d",
                  inst->agent, log_file, errno);
            break;
        }

        do { /* messages reading loop */

            uint8_t    *p_buf = buf;
            uint32_t    sequence;
            int         lost;
            size_t      len;

            /* Get message sequence number */
            if (FREAD(ta_file, (uint8_t *)&sequence, sizeof(uint32_t)) !=
                    sizeof(uint32_t))
            {
                break;
            }
            sequence = ntohl(sequence);
            lost = sequence - inst->sequence - 1;
            if (lost > 0)
                WARN("TA %s: Lost %d messages", inst->agent, lost);
            inst->sequence = sequence;

            /* Read control fields value */
            len = TE_LOG_MSG_COMMON_HDR_SZ;
            if (FREAD(ta_file, p_buf, len) != len)
            {
                break;
            }

            /* Get message timestamp value */
            if (do_flush)
            {
                /* Message is started */
                flush_msg_max--;

                memcpy(&msg_ts_sec,
                       p_buf + sizeof(te_log_version),
                       sizeof(te_log_ts_sec));
                msg_ts_sec = ntohl(msg_ts_sec);
                memcpy(&msg_ts_usec,
                       p_buf + sizeof(te_log_version) +
                       sizeof(te_log_ts_sec), sizeof(te_log_ts_usec));
                msg_ts_usec = ntohl(msg_ts_usec);

                /* Check timestamp value */
                if ((msg_ts_sec > (te_log_ts_sec)flush_ts.tv_sec) ||
                    ((msg_ts_sec == (te_log_ts_sec)flush_ts.tv_sec) &&
                     (msg_ts_usec > (te_log_ts_usec)flush_ts.tv_usec)) ||
                    (flush_msg_max == 0))
                {
                    do_flush = FALSE;
                    flush_done = TRUE;
                    if (flush_msg_max == 0)
                    {
                        WARN("TA %s: Flush operation was interrupted",
                             inst->agent);
                    }
                }
            }
            p_buf += TE_LOG_MSG_COMMON_HDR_SZ;

            /*
             * Add log ID equal to TE_LOG_ID_UNDEFINED,
             * as we log from Engine application - "Logger" itself.
             */
#if SIZEOF_TE_LOG_ID == 4
            LGR_32_TO_NET(TE_LOG_ID_UNDEFINED, p_buf);
#else
#error Unsupported sizeof(te_log_id)
#endif
            p_buf += sizeof(te_log_id);

            /* Add TA name and corresponding NFL to the message */
            LGR_NFL_PUT(ta_name_len, p_buf);
            memcpy(p_buf, inst->agent, ta_name_len);
            p_buf += ta_name_len;

            /* Read the first NFL after header */
            if (FREAD(ta_file, p_buf, sizeof(te_log_nfl)) !=
                    sizeof(te_log_nfl))
            {
                break;
            }
            len = te_log_raw_get_nfl(p_buf);
            p_buf += sizeof(te_log_nfl);

            while (len != TE_LOG_RAW_EOR_LEN)
            {
                /* Read the field in accordance with NFL */
                if (len > 0 && FREAD(ta_file, p_buf, len) != len)
                {
                    break;
                }
                p_buf += len;

                /* Read the next NFL */
                if (FREAD(ta_file, p_buf, sizeof(te_log_nfl)) !=
                        sizeof(te_log_nfl))
                {
                    break;
                }
                len = te_log_raw_get_nfl(p_buf);
                p_buf += sizeof(te_log_nfl);
            };
            if (len != TE_LOG_RAW_EOR_LEN)
                break;

            lgr_register_message(buf, p_buf - buf);

        } while (TRUE);

        if (feof(ta_file) == 0)
        {
            ERROR("TA %s: Invalid file '%s' with logs",
                  inst->agent, log_file);
            /* Continue */
        }

        if (fclose(ta_file) != 0)
        {
            ERROR("TA %s: fclose() of '%s' failed: errno=%d",
                  inst->agent, log_file, errno);
            /* Continue */
        }
        if (remove(log_file) != 0)
        {
            ERROR("TA %s: Failed to delete file '%s': errno=%d",
                  inst->agent, log_file, errno);
            /* Continue */
        }

    } /* end of forever loop */

    remove_inst(inst);
    free(inst);

    if (do_flush || flush_done)
    {
        (void)ta_flush_done(srv);
    }

    rc = ipc_close_server(srv);
    if (rc != 0)
    {
        ERROR("Failed to close IPC server '%s': %r", srv_name, rc);
    }
    else
    {
        RING("IPC Server '%s' closed", srv_name);
    }

    return NULL;
}

/**
 * Process command line options and parameters specified in argv.
 * The procedure contains "Option table" that should be updated 
 * if some new options are going to be added.
 *
 * @param argc  Number of elements in array "argv".
 * @param argv  Array of strings that represents all command line arguments.
 *
 * @return EXIT_SUCCESS or EXIT_FAILURE
 */
static int
process_cmd_line_opts(int argc, const char **argv)
{
    poptContext  optCon;
    int          rc;

    /* Option Table */
    struct poptOption options_table[] = {
        { "foreground", 'f',
          POPT_ARG_NONE | POPT_BIT_SET, &lgr_flags, LOGGER_FOREGROUND,
          "Run Logger in foreground (usefull for Logger debugging).",
          NULL },

        { "no-rcf", '\0',
          POPT_ARG_NONE | POPT_BIT_SET, &lgr_flags, LOGGER_NO_RCF,
          "Run Logger without interaction with RCF, "
          "i.e. polling of Test Agents (usefull for Logger debugging).",
          NULL },

        { "check", 'c',
          POPT_ARG_NONE | POPT_BIT_SET, &lgr_flags, LOGGER_CHECK,
          "Check log messages received from users before store in raw "
          "log file.",
          NULL },

        POPT_AUTOHELP
        POPT_TABLEEND
    };
    
    /* Process command line options */
    optCon = poptGetContext(NULL, argc, argv, options_table, 0);
  
    poptSetOtherOptionHelp(optCon, "[OPTIONS] <cfg-file>");

    while ((rc = poptGetNextOpt(optCon)) >= 0)
    {
        switch (rc)
        {
            default:
                fprintf(stderr, "Unexpected option number %d", rc);
                poptFreeContext(optCon);
                return EXIT_FAILURE;
        }
    }

    if (rc < -1)
    {
        /* An error occurred during option processing */
        fprintf(stderr, "%s: %s\n",
                poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
                poptStrerror(rc));
        poptFreeContext(optCon);
        return EXIT_FAILURE;
    }

    /* Get Logger configuration file name (optional) */
    cfg_file = poptGetArg(optCon);

    if ((cfg_file != NULL) && (poptGetArg(optCon) != NULL))
    {
        poptFreeContext(optCon);
        return EXIT_FAILURE;
    }

    poptFreeContext(optCon);

    return EXIT_SUCCESS;
}

/**
 * Start initialization of capture logs polling variables.
 */
static void
sniffer_polling_sets_start_init(void)
{
    memset(snifp_sets.dir, 0, RCF_MAX_PATH);
    memset(snifp_sets.name, 0, RCF_MAX_PATH);
    snifp_sets.osize    = 0;
    snifp_sets.sn_space = 0;
    snifp_sets.fsize    = 0;
    snifp_sets.rotation = 0;
    snifp_sets.period   = 0;
    snifp_sets.ofill    = ROTATION;
    snifp_sets.errors   = FALSE;

    sniffers_init();
}

/**
 * Initialization of capture logs polling variables by dispatcher cli.
 */
static void
sniffer_polling_sets_cli_init(void)
{
    char *tmp;

    /* Get environment variable value for capture logs directory. */
    tmp = getenv("TE_SNIFF_LOG_DIR");
    if (tmp != NULL)
        strncpy(snifp_sets.dir, tmp, RCF_MAX_PATH);
    if (strlen(snifp_sets.dir) == 0)
    {
        tmp = getenv("TE_SNIFF_DEF_LOG_DIR");
        if (tmp != NULL)
            strncpy(snifp_sets.dir, tmp, RCF_MAX_PATH);
        else
        {
            snifp_sets.errors = TRUE;
            return;
        }
    }

    /* Cleanup old capture logs */
    sniffers_logs_cleanup(snifp_sets.dir);

    tmp = getenv("TE_SNIFF_LOG_NAME");
    if (tmp != NULL)
        strncpy(snifp_sets.name, tmp, RCF_MAX_PATH);

    tmp = getenv("TE_SNIFF_LOG_OSIZE");
    if (tmp != NULL)
        snifp_sets.osize = (unsigned)atoi(tmp) << 20;

    tmp = getenv("TE_SNIFF_LOG_SPACE");
    if (tmp != NULL)
        snifp_sets.sn_space = (unsigned)atoi(tmp) << 20;

    tmp = getenv("TE_SNIFF_LOG_FSIZE");
    if (tmp != NULL)
        snifp_sets.fsize = (unsigned)atoi(tmp) << 20;

    tmp = getenv("TE_SNIFF_LOG_OFILL");
    if (tmp != NULL)
        snifp_sets.ofill = atoi(tmp) == 0 ? ROTATION : TAIL_DROP;

    tmp = getenv("TE_SNIFF_LOG_PER");
    if (tmp != NULL)
        snifp_sets.period = (unsigned)atoi(tmp);
}

/**
 * This is an entry point of Logger process running on TEN side.
 *
 * @param argc  Argument count.
 * @param argv  Argument vector.
 *
 * @return Operation status.
 *
 * @retval EXIT_SUCCESS     Success.
 * @retval EXIT_FAILURE     Failure.
 */
int
main(int argc, const char *argv[])
{
    int         result = EXIT_FAILURE;
    char        err_buf[BUFSIZ];
    const char *pid_fn = getenv("TE_LOGGER_PID_FILE");
    FILE       *pid_f = NULL;
    sigset_t    sigs;
    char       *ta_names = NULL;
    size_t      names_len;
    size_t      str_len = 0;
    int         res = 0;
    int         scale = 0;
    pthread_t   te_thread;
    ta_inst    *ta_el;
    char       *te_log_raw = NULL;

    if (process_cmd_line_opts(argc, argv) != EXIT_SUCCESS)
    {
        fprintf(stderr, "Command line options processing failure\n");
        return EXIT_FAILURE;
    }

    /* Get environment variable value for logs. */
    te_log_dir = getenv("TE_LOG_DIR");
    if (te_log_dir == NULL)
    {
        fprintf(stderr, "TE_LOG_DIR is not defined\n");
        return EXIT_FAILURE;
    }
    /* Get environment variable value with raw log file location. */
    te_log_raw = getenv("TE_LOG_RAW");
    if (te_log_raw == NULL)
    {
        fprintf(stderr, "TE_LOG_RAW is not defined\n");
        return EXIT_FAILURE;
    }
    /* Open raw log file for addition */
    raw_file = fopen(te_log_raw, "ab");
    if (raw_file == NULL)
    {
        perror("fopen() failure");
        return EXIT_FAILURE;
    }
    /* Futher we must goto 'exit' in the case of failure */

    /* Initialize IPC before any servers creation */
    if (ipc_init() != 0)
    {
        strerror_r(errno, err_buf, sizeof(err_buf));
        ERROR("IPC initialization failed: %s\n", err_buf);
        goto exit;
    }

    /* Register TEN server */
    res = ipc_register_server(LGR_SRV_NAME, LOGGER_IPC, &logger_ten_srv);
    if (res != 0)
    {
        ERROR("IPC server '%s' registration failed: %r",
              LGR_SRV_NAME, res);
        goto exit;
    }
    assert(logger_ten_srv != NULL);
    INFO("IPC server '%s' registered\n", LGR_SRV_NAME);

    /* Open PID-file for writing */
    if (pid_fn != NULL && (pid_f = fopen(pid_fn, "w")) == NULL)
    {
        res = TE_OS_RC(TE_LOGGER, errno);
        ERROR("Failed to open PID-file '%s' for writing: error=%r",
              pid_fn, res);
        goto exit;
    }
    
    if (pid_f != NULL && (sigemptyset(&sigs) != 0 ||
                          sigaddset(&sigs, SIGUSR1) != 0 ||
                          sigprocmask(SIG_BLOCK, &sigs, NULL) != 0))
    {
        ERROR("Failed to prepare blocking signals set");
        goto exit;
    }

    /* 
     * Go to background, if foreground mode is not requested.
     * No threads should be created before become a daemon.
     */
    if ((~lgr_flags & LOGGER_FOREGROUND) && (daemon(TRUE, TRUE) != 0))
    {
        ERROR("daemon() failed");
        goto exit;
    }

    /* Store my PID in global variable */
    pid = getpid();

    /* ASAP create separate thread for log message server */
    res = pthread_create(&te_thread, NULL, (void *)&te_handler, NULL);
    if (res != 0)
    {
        strerror_r(errno, err_buf, sizeof(err_buf));
        ERROR("Server: pthread_create() failed: %s\n", err_buf);
        goto exit;
    }
    /* Further we must goto 'join_te_srv' in the case of failure */

    /* Write own PID to the file */
    if (pid_f != NULL)
    {
        (void)fprintf(pid_f, "%d", (int)pid);
        if (fclose(pid_f) != 0)
        {
            strerror_r(errno, err_buf, sizeof(err_buf));
            ERROR("Failed to close PID-file: %s", err_buf);
            /* Try to continue */
        }
        pid_f = NULL;

        /* Wait for SIGUSR1 to be sent by Dispatcher */
        if (sigwaitinfo(&sigs, NULL) < 0)
        {
            res = TE_OS_RC(TE_LOGGER, errno);
            ERROR("sigwaitinfo() failed: error=%r", res);
            goto join_te_srv;
        }

        if (lgr_flags & LOGGER_SHUTDOWN)
        {
            WARN("Logger is shut down without polling of TAs");
            goto join_te_srv;
        }
    }

    if (~lgr_flags & LOGGER_NO_RCF)
    {
        INFO("Request RCF about list of active TA\n");
        /* Get list of active TA */
        do {
            ++scale;
            names_len = LGR_TANAMES_LEN * scale;
            ta_names = (char *)realloc(ta_names, names_len * sizeof(char));
            if (ta_names == NULL)
            {
                ERROR("Memory allocation failure");
                goto join_te_srv;
            }
            memset(ta_names, 0, names_len * sizeof(char));
            res = rcf_get_ta_list(ta_names, &names_len);

        } while (TE_RC_GET_ERROR(res) == TE_ESMALLBUF);

        if (res != 0)
        {
            ERROR("Failed to get list of active TA from RCF\n");
            /* Continue processing with empty list of Test Agents */
            ta_names[0] = '\0';
            names_len = 0;
        }

        /* Create single linked list of active TA */
        while (names_len != str_len)
        {
            char       *aux_str;
            size_t      tmp_len;

            ta_el = (struct ta_inst *)malloc(sizeof(struct ta_inst));
            memset(ta_el, 0, sizeof(struct ta_inst));
            ta_el->thread_run = FALSE;
            aux_str = ta_names + str_len;
            tmp_len = strlen(aux_str) + 1;
            memcpy(ta_el->agent, aux_str, tmp_len);
            str_len += tmp_len;

            res = rcf_ta_name2type(ta_el->agent, ta_el->type);
            if (res != 0)
            {
                ERROR("Cannot interact with RCF\n");
                free(ta_names);
                goto join_te_srv;
            }

            ta_el->next = ta_list;
            ta_list = ta_el;
        }
        free(ta_names);
    }

    sniffer_polling_sets_start_init();

    /* 
     * FIXME:
     * Log file must be processed before start of messages
     * processing 
     */
    /* Parse log file when list of TAs is known */
    INFO("Logger configuration file parsing\n");
    if (configParser(cfg_file) != 0)
    {
        ERROR("Logger configuration file failure\n");
        goto join_te_srv;
    }

    sniffer_polling_sets_cli_init();

    INFO("TA handlers creation\n");
    /* Create threads according to active TA list */
    ta_el = ta_list;
    while (ta_el != NULL)
    {
        res = pthread_create(&ta_el->thread, NULL,
                             (void *)&ta_handler, (void *)ta_el);
        if (res != 0)
        {
            strerror_r(errno, err_buf, sizeof(err_buf));
            ERROR("Logger(client): pthread_create() failed: %s\n", err_buf);
            goto join_te_srv;
        }
        ta_el->thread_run = TRUE;
        ta_el = ta_el->next;
    }

    result = EXIT_SUCCESS;

join_te_srv:
    INFO("Joining Logger TEN server\n");
    if (pthread_join(te_thread, NULL) != 0)
    {
        strerror_r(errno, err_buf, sizeof(err_buf));
        ERROR("pthread_join() failed: %s", err_buf);
        result = EXIT_FAILURE;
    }

exit:
    /* Release all memory on shutdown */
    pthread_mutex_lock(&add_remove_mutex);
    {
        ta_inst **a = &ta_list;

        while ((ta_el = *a) != NULL)
        {
            if (ta_el->thread_run)
            {
                /* Keep current 'ta_el' intact */
                a = &ta_el->next;
            }
            else
            {
                /* Remove 'ta_el' from the list and free it */
                *a = ta_el->next;
                free(ta_el);
            }
        }
    }
    pthread_mutex_unlock(&add_remove_mutex);

#if 0
    while (ta_list != NULL)
    {
        ta_el = ta_list;
        ta_list = ta_el->next;

#if 0
        if (ta_el->thread_run &&
            pthread_join(ta_el->thread, NULL) != 0)
        {
            strerror_r(errno, err_buf, sizeof(err_buf));
            ERROR("pthread_join() failed: %s\n", err_buf);
            result = EXIT_FAILURE;
        }
#endif

        free(ta_el);
    }
#endif

    if ((pid_f != NULL) && (fclose(pid_f) != 0))
    {
        strerror_r(errno, err_buf, sizeof(err_buf));
        ERROR("Failed to close PID-file: %s", err_buf);
        result = EXIT_FAILURE;
    }

    INFO("Close IPC server '%s'\n", LGR_SRV_NAME);
    if (ipc_close_server(logger_ten_srv) != 0)
    {
        strerror_r(errno, err_buf, sizeof(err_buf));
        ERROR("IPC server '%s' shutdown failed: %s",
              LGR_SRV_NAME, err_buf);
        result = EXIT_FAILURE;
    }

    if (ipc_kill() != 0)
    {
        strerror_r(errno, err_buf, sizeof(err_buf));
        ERROR("IPC termination failed: %s", err_buf);
        result = EXIT_FAILURE;
    }

    RING("Shutdown is completed");

    if (fflush(raw_file) != 0)
    {
        perror("fflush() failed");
        result = EXIT_FAILURE;
    }
    if (fclose(raw_file) != 0)
    {
        perror("fclose() failed");
        result = EXIT_FAILURE;
    }

    return result;
}
