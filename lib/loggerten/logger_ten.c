/** @file
 * @brief Logger subsystem API - TEN side
 * 
 * TEN side Logger library. 
 *
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS
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
 *
 * @author Igor B. Vasiliev <Igor.Vasiliev@oktetlabs.ru>
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include "te_defs.h"
#include "te_stdint.h"
#include "ipc_client.h"
#include "logger_ten.h"

#define LGR_USER    "LOGGER TEN"
#include "logger_api.h"


/** The name of server process providing logging facilities */
#define LGR_SRV_NAME    "LOGGER"

/** Maximum logger message length */
#define LGR_MAX_BUF     3851 

/** Maximum logger message arguments */
#define LGR_MAX_ARGS    16


#ifndef HAVE_PTHREAD_H
#define pthread_self()  0
#endif

/*   Raw log file format. Max length of the raw log message 
 *   is a sum of:
 *      NFL(Entity_name) - 1 byte
 *      Entity_name - 255 bytes max
 *      Log_ver - 1 byte
 *      Timestamp - 8 byte
 *      Level - 2 bytes
 *      Log_message_length - 2 bytes
 *      NFL(User_name) - 1 byte
 *      User_name - 255 bytes max
 *      NFL(of form_str) - 1 byte
 *      Log_data_format_str - 255 bytes max
 *      NFL(arg1) - 1 byte
 *      arg1 - 255 bytes max
 *         ...
 *      NFL(arg12) - 1 byte
 *      arg12 - 255 bytes max
 *
 *      TOTAL = 3853 bytes
 */

/* 
 * Number of bytes is not added to the Log_message_length: 
 * NFL(Entity_name) - 1 byte;
 * Log_ver - 1 byte;
 * Timestamp - 8 byte;
 * Level - 2 bytes;
 * Log_message_length - 2 bytes.
 */
#define LGR_UNACCOUNTED_LEN   14

/**
 * This macro fills in NFL(Next_Field_Length) field and corresponding 
 * string field as following:
 *     NFL (1 byte) - actual next string length without trailing null;
 *     Argument field (1 .. 255 bytes) - logged string.
 * NOTE: if logged string pointer equal NULL the "(NULL)" string will be
 *       written in the raw log file.
 *
 * @param _log_str      logged string location
 * @param _len          (OUT) string length
 */
#define LGR_FILL_STR(_log_str, _len) \
    do {                                        \
        const char *tmp_str = null_str;         \
                                                \
        if ((_log_str) != NULL)                 \
            tmp_str = (_log_str);               \
        (_len) = strlen(tmp_str);               \
        if ((_len) > LGR_FIELD_MAX)             \
            (_len) = LGR_FIELD_MAX;             \
        *mess_buf = (_len);                     \
        ++mess_buf;                             \
        memcpy(mess_buf, tmp_str, (_len));      \
        mess_buf += (_len);                     \
     } while (0);
    
static const char *null_str = "(NULL)";
static int   one_time_check = 0;
static char *te_tmp = NULL;
static char  te_log_path[LGR_FIELD_MAX];
static char  te_log_tmp[LGR_FIELD_MAX];


#ifdef HAVE_PTHREAD_H
#if 0 /* FIXME */
static pthread_once_t   once_control = PTHREAD_ONCE_INIT;
static pthread_key_t    key;
#endif
static pthread_mutex_t  lgr_lock = PTHREAD_MUTEX_INITIALIZER;
#endif


/* WORK AROUND against RedHat bug */
#ifdef HAVE_PTHREAD_H

#define LGR_MAX_THREADS     1024

/** Array to work around pthread get/setspecific broken functionality */
static struct {
    pthread_t          tid;
    struct ipc_client *handle;
} handles[LGR_MAX_THREADS];

/**
 * Find thread IPC handle or initialize a new one.
 *
 * @param create    Create handle if not found or not?
 *
 * @return IPC handle or NULL if IPC library returned an error
 */
static struct ipc_client *
get_client_handle(te_bool create)
{
    pthread_t mine = pthread_self();
    int       i;

    pthread_mutex_lock(&lgr_lock);
    for (i = 0; i < LGR_MAX_THREADS; i++)
    {
         if (handles[i].tid == mine)
         {
             pthread_mutex_unlock(&lgr_lock);
             return handles[i].handle;
         }
    }
    if (!create)
    {
        pthread_mutex_unlock(&lgr_lock);
        return NULL;
    }

    for (i = 0; i < LGR_MAX_THREADS; i++)
    {
        if (handles[i].tid == 0)
        {
            char name[LGR_FIELD_MAX];

            handles[i].tid = mine;
            snprintf(name, LGR_FIELD_MAX, "lgr_client_%u_%u",
                     (unsigned int)getpid(), (unsigned int)mine);
            if ((handles[i].handle = ipc_init_client(name)) == NULL)
            {
                pthread_mutex_unlock(&lgr_lock);
                fprintf(stderr, "ipc_init_client() failed\n");
                return NULL;
            }
            pthread_mutex_unlock(&lgr_lock);
            return handles[i].handle;
        }
    }
    pthread_mutex_unlock(&lgr_lock);
    fprintf(stderr, "too many threads\n");
    return NULL;
}

/**
 * Frep IPC client handle.
 */
static void
free_client_handle(void)
{
    pthread_t mine = pthread_self();
    int       i;

    pthread_mutex_lock(&lgr_lock);
    for (i = 0; i < LGR_MAX_THREADS; i++)
    {
         if (handles[i].tid == mine)
         {
             handles[i].tid = 0;
             handles[i].handle = NULL;
             break;
         }
    }
    pthread_mutex_unlock(&lgr_lock);
}
#else
/**
 * Find thread IPC handle or initialize a new one.
 * 
 * @param create    Create handle if not found or not?
 *
 * @return IPC handle or NULL if IPC library returned an error
 */
static struct ipc_client *
get_client_handle(te_bool create)
{
#ifdef HAVE_PTHREAD_H
    struct ipc_client *handle;
#else
    static struct ipc_client *handle;
#endif
    static int                init = 0;


    if (!init)
    {
#ifdef HAVE_PTHREAD_H
        if (pthread_key_create(&key, NULL) != 0)
            return NULL;
        init = 1;
    }
    handle = (struct ipc_client *)pthread_getspecific(key);
    if ((handle == NULL) && create)
    {
#endif
        char name[LGR_FIELD_MAX];

        sprintf(name, "lgr_client_%u_%u", (unsigned int)getpid(),
                (unsigned int)pthread_self());
        if ((handle = ipc_init_client(name)) == NULL)
            return NULL;
#ifdef HAVE_PTHREAD_H
        if (pthread_setspecific(key, (void *)handle) != 0)
        {
            LOG_DEBUG("Logger TEN", "pthread_setspecific() failed\n");
            fprintf(stderr, "Logger TEN: pthread_setspecific() failed\n");
        }
#else
        init = 1;
#endif
    }
    return handle;
}
#endif


/* See description in logger_ten.h */
void
log_client_close(void)
{
    int                 res;
    struct ipc_client  *ipcc = get_client_handle(FALSE);

    if (ipcc == NULL)
        return;
        
    res = ipc_close_client(ipcc);
    if (res != 0)
        fprintf(stderr, "log_client_close(): ipc_close_client failed\n");

    free_client_handle();
}


/* See description in logger_api.h */
/** 
 * Create message and register it in the raw log file.
 * @param  level          Log level valued to be passed to raw log file
 * @param  entity_name    Entity name whose user generates this message;
 * @param  user_name      Arbitrary "user name";
 * @param  form_str       Raw log format string. This string should contain
 *                        conversion specifiers if some arguments following;
 * @param  ...            Arguments passed into the function according to 
 *                        raw log format string description.
 *
 * @bugs   Should be processed test result value for %te specifier?
 */
void
log_message(uint16_t level, const char *entity_name, 
            const char *user_name, const char *form_str, ...)
{
    va_list     ap;
    struct      timeval tv;
    uint32_t    tvsec;
    uint32_t    tvusec;    
    const char *p_fs;
    int         narg = 0;
    int8_t      raw_mess[LGR_MAX_BUF];
    int8_t     *mess_buf = raw_mess;
    uint16_t    tmp_length = 0;    
    uint16_t   *mess_length_field;
    uint16_t    mess_length;

    struct ipc_client *log_client;

    static char *skip_flags, *skip_width;

    skip_flags = "#-+ 0";
    skip_width = "*0123456789";

    memset(raw_mess, 0, LGR_MAX_BUF);
    
    if (one_time_check == 0)
    {   
         /* Get environment variable value for temporary TE location. */    
         te_tmp = getenv("TE_LOG_DIR");
         if (te_tmp == NULL)
         {
             fprintf(stderr, "TE_LOG_DIR is not defined\n");
             return;
         }
         /* Form full names for temporary log location and temporary log file */
         strcpy(te_log_path, te_tmp);
         strcpy(te_log_tmp, te_log_path);
         strcat(te_log_tmp, "/tmp_raw_log");         
         one_time_check = 1;
    }

    log_client = get_client_handle(TRUE);
        
    if (log_client == NULL)
        return;
        
    /* Fill in Entity_name field and corresponding Next_filed_length one */
    LGR_FILL_STR(entity_name, tmp_length);

    /* Fill in Log_ver and timestamp fields */
    *mess_buf = LGR_LOG_VERSION;
    ++mess_buf;
    
    gettimeofday(&tv, NULL);
    LGR_32_TO_NET(tv.tv_sec, &tvsec);
    LGR_32_TO_NET(tv.tv_usec, &tvusec);
    *((uint32_t *)mess_buf) = tvsec;
    mess_buf += sizeof(uint32_t);
    *((uint32_t *)mess_buf) = tvusec;    
    mess_buf += sizeof(uint32_t);

    /* Fill in Log level to be passed in raw log */
    *((uint16_t *)mess_buf) = htons(level);
    mess_buf += sizeof(uint16_t);
    
    /* Fix Log_message_length field and initiate one */
    mess_length_field = (unsigned short *)mess_buf;
    *mess_length_field = 0;
    ++mess_buf; ++mess_buf;
    
    /* Fill in User_name field and corresponding Next_filed_length one */
    LGR_FILL_STR(user_name, tmp_length);
    *mess_length_field = *mess_length_field + LGR_NFL_FLD + tmp_length;

    /* 
     * Fill in Log_data_format_string field and corresponding 
     * Next_filed_length one 
     */    
    LGR_FILL_STR(form_str, tmp_length);
    *mess_length_field = *mess_length_field + LGR_NFL_FLD + tmp_length;
    
    va_start(ap, form_str);
    for (p_fs = form_str; *p_fs; p_fs++)
    {
        if (*p_fs != '%')
            continue;

        if (*++p_fs == '%')
            continue;

        /* skip the flags field  */
        for (; index(skip_flags, *p_fs); ++p_fs);

        /* skip to possible '.', get following precision */
        for (; index(skip_width, *p_fs); ++p_fs);
        if (*p_fs == '.')
            ++p_fs;

        /* skip to conversion char */
        for (; index(skip_width, *p_fs); ++p_fs);

    
        if ((++narg) > LGR_MAX_ARGS)
        {
            log_message(0x2, "LOGGER", "WARNING", "Too many arguments\n");
            return;
        }

        switch (*p_fs)
        {           
            case 'd':
            case 'i':
            case 'o':
            case 'x':
            case 'X':
            case 'u':
            case 'p':
            case 'r':   /* TE-specific specifier for error codes */
            {
                int32_t val;

                *mess_buf = (unsigned char)sizeof(int);
                ++mess_buf;
                val = va_arg(ap, int);
                LGR_32_TO_NET(val, mess_buf);
                mess_buf += sizeof(int);
                *mess_length_field = *mess_length_field +
                                     LGR_NFL_FLD + sizeof(int);
                break;
            }                     
        
            case 'c':
                *mess_buf = (unsigned char)sizeof(char);
                ++mess_buf;
                *mess_buf = (int8_t)va_arg(ap, int);
                ++mess_buf;
                *mess_length_field = *mess_length_field + 
                                     LGR_NFL_FLD + sizeof(char);
                break;     
               
            case 's':
            {
                char *tmp = va_arg(ap, char *);

                LGR_FILL_STR(tmp, tmp_length);
                *mess_length_field = *mess_length_field + 
                                     LGR_NFL_FLD + tmp_length;
                break;
            }
                
            case 't': /* %tm, %tb, %te, %tf */
                if (*++p_fs != 0 )
                {
                    if (*p_fs == 'm')
                    {
                        /* 
                         * Args order should be:
                         *  - start address of dumped memory;  
                         *  - size of memory piece;
                         */
                        char *dump = (char *)va_arg(ap, char *);
    
                        tmp_length = (uint16_t)va_arg(ap, int);
                        if (tmp_length > LGR_FIELD_MAX)
                            tmp_length = LGR_FIELD_MAX;
                            
                        *mess_buf = tmp_length;
                        ++mess_buf;

                        memcpy(mess_buf, dump, tmp_length);
                        *mess_length_field = *mess_length_field + 
                                             LGR_NFL_FLD + tmp_length;
                        break;
                    }
                    else if (*p_fs == 'b' || *p_fs == 'e') 
                    {    /* ??? test result for 'e' case */
                        char *tmp = va_arg(ap, char *);
                        LGR_FILL_STR(tmp, tmp_length);
                        *mess_length_field = *mess_length_field + 
                                             LGR_NFL_FLD + tmp_length;
                        break;
                    }
                    else if (*p_fs == 'f')
                    {
                        char new_path[LGR_FIELD_MAX] = "";
                        char templ[LGR_FILE_MAX] = "XXXXXX";
                        char *tmp = va_arg(ap, char *);
                        int  fd;

                        fd =  mkstemp(templ);
                        if (fd < 0)
                        {
                            log_message(0x2, "LOGGER", "WARNING",
                                        "mkstemp() failure.\n");
                            return;
                        }

                        if (close(fd) != 0)
                        {
                            log_message(0x2, "LOGGER", "WARNING",
                                        "close() failure.\n");
                            return;
                        }

                        strcat(new_path, te_log_path);
                        strcat(new_path, templ);
                        if (rename(tmp, new_path) != 0)
                        {
                            log_message(0x2, "LOGGER", "WARNING",
                                        "File %s copying error\n", tmp);
                            return;
                        }

                        LGR_FILL_STR(templ, tmp_length);
                        *mess_length_field = *mess_length_field +
                                             LGR_NFL_FLD + tmp_length;
                        break;
                    }
                }
    
                log_message(0x2, "LOGGER", "WARNING", 
                            "Illegal specifier - t%c\n", *p_fs);
                return;

            default:
                log_message(0x2, "LOGGER", "WARNING", 
                            "Illegal specifier - %c\n", *p_fs);
                return;
        }
    }    
    va_end(ap);
    
    mess_length = *mess_length_field;
    *mess_length_field = htons(mess_length);
    
    /* Send message to the Logger process by means of IPC */
    /* raw_mess[0] - Entity name length */
    ipc_send_message(log_client, LGR_SRV_NAME, raw_mess, 
                     mess_length + raw_mess[0] + LGR_UNACCOUNTED_LEN);
}


/* See description in logger_ten.h */
void 
log_flush_ten(const char *ta_name)
{
    static struct ipc_client *log_client;
    char               ta_srv[LGR_FIELD_MAX] = "LOGGER-";
    char               mess[LGR_FIELD_MAX] = "LOGGER-FLUSH";
    char               answer[LGR_FIELD_MAX];
    uint32_t           answer_len = LGR_FIELD_MAX;

    strcat(ta_srv, ta_name);
    log_client = ipc_init_client("LOGGER_FLUSH_CLIENT");
    if (log_client != NULL)
    {
        ipc_send_message_with_answer(log_client, ta_srv, mess, strlen(mess),
                                     answer, &answer_len);
        ipc_close_client(log_client);
    }
}
