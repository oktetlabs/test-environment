/** @file
 * @brief TE project. Logger subsystem.
 * 
 * Logger executable module. 
 *
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
 *
 * @author Igor B. Vasiliev <Igor.Vasiliev@oktetlabs.ru>
 *
 * $Id$
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_TIME_H
#include <time.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "te_raw_log.h"
#include "logger_int.h"
#include "logger_internal.h"


#define LGR_MAX_BUF 0x4000    /* FIXME */

#define FREAD(_fd, _buf, _len) \
    fread((_buf), sizeof(char), (_len), (_fd))
    
#define SET_SEC(_poll)  ((_poll)/1000000)
#define SET_MSEC(_poll) ((_poll)%1000000)


DEFINE_LGR_ENTITY("Logger");

/* TEN entities linked list */
te_inst *te_list = NULL;

/* TA single linked list */
ta_inst *ta_list = NULL;


/* Temporary raw log file */
static FILE *raw_file;

char te_log_dir[TE_LOG_FIELD_MAX];
static char te_log_tmp[TE_LOG_FIELD_MAX];


/**
 * Check if user incoming message should be processed.
 *
 * @param user_name  User name.
 * @param filters    Filters location
 *
 * @retval  LGR_INCLUDE  Message should be processed.
 * @retval  LGR_EXCLUDE  Message should not be processed.
 */
static int
lgr_message_filter(const char *user_name, re_fltr *filters)
{
    re_fltr    *tmp_fltr;
    regmatch_t  pmatch[1];
    int         lgr_type = LGR_INCLUDE;

    tmp_fltr = filters;
    while (tmp_fltr->next != tmp_fltr)
    {
        size_t str_len = strlen(user_name);

        if (regexec(tmp_fltr->preg, user_name, 1, pmatch, 0) == 0)
        {
           if ((str_len - (pmatch->rm_eo - pmatch->rm_so) == 0))
               lgr_type = tmp_fltr->type;
        }
        tmp_fltr = tmp_fltr->next;
    }
    return lgr_type;
}


/**
 * Register the log message in the raw log file.
 *
 * @param buf_mess  Log message location.
 * @param buf_len   Log message length.
 */
void
lgr_register_message(const void *buf_mess, size_t buf_len)
{
    /* MUTual  EXclusion device */
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

    pthread_mutex_lock(&mutex);
    if (fwrite(buf_mess, buf_len, 1, raw_file) != 1)
    {
        perror("fwrite() failure");
    } 
    fflush(raw_file);
    pthread_mutex_unlock(&mutex);
}

/**
 * This is an entry point of logger message server.
 * This server should be run as separate thread.
 * All log messages from TEN entities will be processed by
 * this routine.
 */
static void * 
te_handler(void)
{
    char                        err_buf[BUFSIZ];
    static struct ipc_server   *srv;
    struct ipc_server_client   *ipcsc_p;
    char                        buf_mess[LGR_MAX_BUF];
    size_t                      buf_len;


    /* Register TEN server */
    srv = ipc_register_server(LGR_SRV_NAME);
    if (srv == NULL)
    {
        strerror_r(errno, err_buf, sizeof(err_buf));
        ERROR("IPC server '%s' registration failed: %s\n",
              LGR_SRV_NAME, err_buf);
        return NULL;
    }
    INFO("IPC server '%s' registered\n", LGR_SRV_NAME);

    while (TRUE) /* Forever loop */
    {
        buf_len = LGR_MAX_BUF;
        ipcsc_p = NULL;
        if (ipc_receive_message(srv, buf_mess, &buf_len, &ipcsc_p) != 0)
        {
            ERROR("Message receiving failure\n");
            break;
        }
        
        if (strncmp((buf_mess + sizeof(te_log_nfl_t)), LGR_SHUTDOWN,
                    *(te_log_nfl_t *)buf_mess) == 0)
        {
            RING("Logger shutdown ...\n");
            break;
        }
        else 
        {
            uint32_t          log_flag = LGR_INCLUDE;
            struct te_inst   *te_el;
            char              tmp_name[TE_LOG_FIELD_MAX];
            uint32_t          len = buf_mess[0];

            memcpy(tmp_name, buf_mess, len);
            tmp_name[len] = 0;

            te_el = te_list;
            while (te_el != NULL)
            {
                if (!strcmp(te_el->entity, tmp_name))
                {
                    char *tmp_pnt;

                    len = TE_LOG_NFL_SZ + buf_mess[0] + TE_LOG_VERSION_SZ + 
                          TE_LOG_TIMESTAMP_SZ + TE_LOG_MSG_LEN_SZ;
                    tmp_pnt = buf_mess + len + 1;
                    len = buf_mess[len];
                    memcpy(tmp_name, tmp_pnt, len);
                    log_flag = lgr_message_filter(tmp_name, &te_el->filters);
                    break;
                }
                te_el = te_el->next;
            }           

            if (log_flag == LGR_INCLUDE)
                lgr_register_message(buf_mess, buf_len);
        }
    } /* end of forever loop */

    INFO("Close IPC server '%s'\n", LGR_SRV_NAME);
    if (ipc_close_server(srv) != 0)
    {
        strerror_r(errno, err_buf, sizeof(err_buf));
        ERROR("IPC server '%s' shutdown failed: %s\n",
              LGR_SRV_NAME, err_buf);
    }

    return NULL;
}

/**
 * This is an entry point of TA log message gatherer.
 * This routine periodically polls appropriate TA to get
 * TA local log. Besides, log is solicited if flush is requested.
 * 
 * @param  ta   Location of TA parameters.
 */
static void *
ta_handler(void *ta)
{
    struct timeval      tv, tv_msg;
    char                log_file[RCF_MAX_PATH];
    ta_inst            *inst = (ta_inst *)ta;
    FILE               *ta_file = NULL; 
    fd_set              rfds;     
    int                 fd_server;
    struct ipc_server  *srv;
    char                ta_srv[LGR_MAX_NAME] = LGR_SRV_FOR_TA_PREFIX;
    char                buf_mess[LGR_MAX_BUF] = { 0, };
    size_t              buf_len = sizeof(buf_mess);
    uint32_t            empty_flag  = 0; /* Set if file is empty */
    uint32_t            tstamp_flag = 0; /* Set if timestamp */
    uint32_t            flush_flag = 0;  /* Set if flush is detected */

    strcat(ta_srv, inst->agent); 
    srv = ipc_register_server(ta_srv);
    if (srv == NULL)
    {
        ERROR("Failed to register IPC server '%s'", ta_srv);
        return NULL;
    }
    fd_server = ipc_get_server_fd(srv);

    gettimeofday(&tv, NULL);
   
    while (1)
    {
        int rc;
        int polling;
        struct timeval cm;
        int msg_count = 0;
        struct stat log_file_stat;

#if 0
        static int get_log_count = 0;
#endif
        
        /* Watch stdin (fd 0) to see when it has input. */
        FD_ZERO(&rfds);
        FD_SET(fd_server, &rfds);

        polling = inst->polling *1000;
            

        if (flush_flag == 0)
        {
            struct timeval delay = {0,0};

            gettimeofday(&cm, NULL);

            /* calculate moment until we should wait before next get log */
            tv.tv_sec  += SET_SEC (polling);
            tv.tv_usec += SET_MSEC(polling); 

            if (tv.tv_usec >= 1000000)
            {
                tv.tv_usec -= 1000000;
                tv.tv_sec ++;
            }

            /* calculate delay of waiting we should wait before next get log */
            if (tv.tv_sec >= cm.tv_sec)
                delay.tv_sec  = tv.tv_sec  - cm.tv_sec;

            if (tv.tv_usec >= cm.tv_usec)
                delay.tv_usec = tv.tv_usec - cm.tv_usec;
            else if (delay.tv_sec > 0)
            {
                delay.tv_usec = (tv.tv_usec + 1000000) - cm.tv_usec;
                delay.tv_sec--;
            } 
            cm = delay; 
            select(FD_SETSIZE, &rfds, NULL, NULL, &delay);

            /* Get time for filtering incoming messages */
        }

        gettimeofday(&tv, NULL);
#if 0
        printf("Logger " 
        "-- GET LOG fl_flag %d; before request: %d, time: %u.%u; delay %u.%u\n", 
                flush_flag, get_log_count, tv.tv_sec, tv.tv_usec, cm.tv_sec, cm.tv_usec);
#endif
        *log_file = 0;
        if ((rc = rcf_ta_get_log(inst->agent, log_file)) != 0)
        {
            if (rc == ETAREBOOTED)
                continue;
            break; 
        }
#if 0
        {
            struct timeval ltv;
        gettimeofday(&ltv, NULL);
        printf("Logger " "-- GET LOG after request: %d, sec %u, usec %u\n", 
                get_log_count, ltv.tv_sec, ltv.tv_usec);
        }
        get_log_count++;
#endif
        rc = stat(log_file, &log_file_stat);
        if (rc < 0)
        {
            perror("log file stat failure");
            continue;
        }

        if (log_file_stat.st_size != 0)
        {
            if ((ta_file = fopen(log_file, "r")) == NULL)
                ERROR("TA:%s, fopen(%s) failure\n", inst->agent, log_file); 
        }

        if ((ta_file == NULL) || (log_file_stat.st_size == 0))
        {
            remove(log_file);
            if (flush_flag == 1)
            {
                flush_flag = 0;
                tstamp_flag = 1;
            }
            goto response_to_flush;
        }

        do {
            char               *p_buf = buf_mess;
            te_log_level_t      log_level;
            te_log_msg_len_t    len;
            char                tmp_name[TE_LOG_FIELD_MAX];
            uint32_t            log_flag = LGR_INCLUDE;
            uint32_t            sequence = 0;
            uint32_t            rc = 0;

            /* Add TA name and corresponding NFL to the message */
            msg_count++;
            *p_buf = strlen(inst->agent); 
            p_buf++;
            memcpy(p_buf, inst->agent, buf_mess[0]);
            p_buf += buf_mess[0];

            /* Get message sequence number */
            rc = FREAD(ta_file, (uint8_t *)&sequence, sizeof(uint32_t));
            if (rc != sizeof(uint32_t))
            {
                break;
            }

            sequence = ntohl(sequence);
            sequence -= inst->sequence;
            if (sequence > 1)
                WARN("Lost %d messages\n", sequence);
            inst->sequence += sequence;


            /* Read control fields value */
            len = TE_LOG_VERSION_SZ + TE_LOG_TIMESTAMP_SZ + \
                  TE_LOG_LEVEL_SZ + TE_LOG_MSG_LEN_SZ;
            if (FREAD(ta_file, p_buf, len) != len)
            {
                empty_flag = 1; /* File is empty or reading error */            
                break;
            }

            /* Get message timestamp value */
            assert(TE_LOG_TIMESTAMP_SZ == sizeof(uint32_t) * 2);
            memcpy(&tv_msg.tv_sec, p_buf + TE_LOG_VERSION_SZ,
                   sizeof(uint32_t));
            tv_msg.tv_sec = ntohl(tv_msg.tv_sec);
            memcpy(&tv_msg.tv_usec,
                   p_buf + TE_LOG_VERSION_SZ + sizeof(uint32_t),
                   sizeof(uint32_t));
            tv_msg.tv_usec = ntohl(tv_msg.tv_usec);

            /* Get message level value */
            memcpy(&log_level,
                   p_buf + TE_LOG_VERSION_SZ + TE_LOG_TIMESTAMP_SZ,
                   TE_LOG_LEVEL_SZ);
            log_level = htons(log_level);

            /* Get message length */
            p_buf += (len - TE_LOG_MSG_LEN_SZ);
            memcpy(&len, p_buf, TE_LOG_MSG_LEN_SZ);
#if (TE_LOG_MSG_LEN_SZ == 1)
            /* Do nothing */
#elif (TE_LOG_MSG_LEN_SZ == 2)
            len = ntohs(len);
#elif (TE_LOG_MSG_LEN_SZ == 4)
            len = ntohl(len);
#else
#error Such TE_LOG_MSG_LEN_SZ is not supported here.
#endif
            p_buf += TE_LOG_MSG_LEN_SZ;

            /* Copy the rest of the message to the buffer */
            if (FREAD(ta_file, p_buf, len) != len)
            {
                empty_flag = 1; /* File is empty or reading error */
                break;
            }

            /* Extract user_name value */
            memcpy(tmp_name, p_buf + 1, *p_buf);
            tmp_name[(int)(*p_buf)] = '\0';
            p_buf += len;

            log_flag = lgr_message_filter(tmp_name, &inst->filters);

            if (log_flag == LGR_INCLUDE)
            {
                if (flush_flag == 1)
                {
                    /* Check timestamp value */
                    if (tv_msg.tv_sec > tv.tv_sec)
                    {
#if 0
        printf("Logger " "GET LOG %d; msg tv: %u.%u, flush finish.\n", 
                get_log_count, tv_msg.tv_sec, tv_msg.tv_usec);
#endif
                        flush_flag = 0;
                        tstamp_flag = 1;
                    } 
                    else if (tv_msg.tv_sec == tv.tv_sec)
                    {
                        if (tv_msg.tv_usec > tv.tv_usec)
                        {
#if 0
        printf("Logger " "GET LOG %d; msg tv: %u.%u, flush finish.\n", 
                get_log_count, tv_msg.tv_sec, tv_msg.tv_usec);
#endif
                            flush_flag = 0;
                            tstamp_flag = 1;
                        }
                    }
                }
                lgr_register_message(buf_mess, p_buf - buf_mess);
            }
        } while (1);

#if 0
        printf("Logger " "GET LOG %d; got %d log messages from TA\n", 
                get_log_count, msg_count);
#endif

        if (feof(ta_file) == 0)
        {
            empty_flag = 0;
            perror("Logger file failure");
        }

        fclose(ta_file);
        remove(log_file);
        ta_file = NULL;

#if 0
        /* No messages are in the agent buffer by this time */
        if ((flush_flag == 1) && (empty_flag == 1))
        {
            ERROR("GET LOG %d; empty, flush finish", get_log_count) 
            flush_flag = 0;
            empty_flag = 0;
            tstamp_flag = 1;
        }
#endif

response_to_flush:
        if ((flush_flag == 0) && (tstamp_flag == 0))
        {
           if (FD_ISSET(fd_server, &rfds))
               flush_flag = 1;
        }

        if ((flush_flag == 0) && (tstamp_flag == 1))
        {
            struct ipc_server_client  *ipcsc_p = NULL;

            ipc_receive_message(srv, buf_mess, &buf_len, &ipcsc_p);
            ipc_send_answer(srv, ipcsc_p, buf_mess, strlen(buf_mess));
            tstamp_flag = 0;
        }
    }

    if (ipc_close_server(srv) != 0)
    {
        ERROR("Failed to close IPC server '%s': %X", ta_srv, errno);
    }
    rcf_api_cleanup();

    return NULL;
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
main(int argc, char *argv[])
{
    int         result = EXIT_FAILURE;
    char        err_buf[BUFSIZ];
    char       *ta_names = NULL;
    size_t      names_len;
    size_t      str_len = 0;    
    int         res = 0;
    int         scale = 0;
    pthread_t   te_thread;
    ta_inst    *ta_el;
    char       *te_tmp = NULL;


    /* Get environment variable value for temporary TE location. */
    te_tmp = getenv("TE_LOG_DIR");
    if (te_tmp == NULL)
    {
        fprintf(stderr, "TE_LOG_DIR is not defined\n");
        return EXIT_FAILURE;
    }

    /* Form full names for temporary log location and temporary log file */
    strcpy(te_log_dir, te_tmp);
    strcpy(te_log_tmp, te_log_dir);
    strcat(te_log_tmp, "/tmp_raw_log");

    /* Open raw log file for addition */
    raw_file = fopen(te_log_tmp, "ab");
    if (raw_file == NULL)
    {
        perror("fopen() failure");
        return EXIT_FAILURE;
    }
    /* Futher we must goto 'exit' in the case of failure */


    /* Log file must be processed before start of messages processing */
    INFO("Logger configuration file parsing\n");    
    /* Parse configuration file */
    if (argc < 1)
    {
        ERROR("No Logger configuration file passed\n");    
        goto exit;
    }
    if (configParser(argv[1]) != 0)
    {
        ERROR("Logger configuration file failure\n");
        goto exit;
    }

    
    /* Initialize IPC before any servers creation */
    if (ipc_init() != 0)
    {
        strerror_r(errno, err_buf, sizeof(err_buf));
        ERROR("IPC initialization failed: %s\n", err_buf);
        goto exit;
    }

    /* ASAP create separate thread for log message server */
    res = pthread_create(&te_thread, NULL, (void *)&te_handler, NULL);
    if (res != 0)
    {
        strerror_r(errno, err_buf, sizeof(err_buf));
        ERROR("Server: pthread_create() failed: %s\n", err_buf);
        goto exit;
    }
    /* Further we must goto 'join_te_srv' in the case of failure */


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

    } while (res == ETESMALLBUF);

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

        ta_el->filters.next = ta_el->filters.last = &ta_el->filters;

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

        LGR_FREE_FLTR_LIST(&ta_el->filters);
        free(ta_el);
    }
    while (te_list != NULL)
    {
        te_inst *te_el;

        te_el = te_list;
        te_list = te_el->next;
        LGR_FREE_FLTR_LIST(&te_el->filters);
        free(te_el);
    }

    rcf_api_cleanup();

    if (ipc_kill() != 0)
    {
        strerror_r(errno, err_buf, sizeof(err_buf));
        ERROR("IPC termination failed: %s\n", err_buf);
        result = EXIT_FAILURE;
    }
    
    RING("Shutdown is completed\n");

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
