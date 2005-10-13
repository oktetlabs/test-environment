/** @file
 * @brief Test Environment: Raw log V1 routines.
 *
 * Implementation of "RLF version 1" specific functions.
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
 * @author Oleg N. Kravtsov  <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#include <stdio.h>
#include <obstack.h>

#include "log_format.h"
#include "rgt_common.h"
#include "io.h"
#include "memory.h"

/** Indeces of the error events */
enum e_error_msg_index {
    RLF_V1_RLM_VERSION           = 0,  /**< There is no log version field */
    RLF_V1_RLM_TIMESTAMP         = 1,  /**< Timestamp field is too short */
    RLF_V1_RLM_LOGLEVEL          = 2,  /**< Log Level field is too short */
    RLF_V1_RLM_LOG_ID            = 3,  /**< Log ID field is too short */
    RLF_V1_RLM_ENTITY_NAME       = 4,  /**< Entity name is too short */
    RLF_V1_RLM_USER_NAME         = 5,  /**< Entity name is too short */
    RLF_V1_RLM_FORMAT_STRING     = 6,  /**< Format string is out
                                            of message */
    RLF_V1_RLM_ARG_LEN           = 7,  /**< Agrument is out of message */
    RLF_V1_RLM_UNKNOWN_LOGLEVEL  = 8,  /**< Unknown log level value */
};

/**
  * This variable is used for trace incorrect raw log file. It contains 
  * offset of the message is being processed from the start of RLF.
  */
static long cur_msg_offset;

/** 
 * This variable contains error index that expresses possible error 
 * of the operation to be performed.
 */
static enum e_error_msg_index cur_error_index;

/** 
 * This is an array of error messages each corresponding
 * to an appropriate error index of enum "e_error_msg_index".
 * DON'T CHANGE THE ORDER OF THESE MESSAGES!
 */
static struct debug_msg {
    char *content;
} dbg_msgs[] = {
    {"*** Invalid log message version."},
    {"*** Log message timestamp is truncated."},
    {"*** Log message log level is truncated."},
    {"*** Log message test ID is truncated."},
    {"*** Entity name is truncated."},
    {"*** User name is truncated."},
    {"*** Log message format string is truncated."},
    {"*** Log message argument is truncated."},
    {"*** Value of log level is unkown."},
};

/** 
 * Set index of the error event that can be occured during the operation
 * to be performed or just to output an error message.
 */
#define LOG_FORMAT_DEBUG_SET(err_index) \
    do {                                \
        cur_error_index = (err_index);  \
    } while (0)

/** 
 * Output error message to the stderr according to 
 * the value of cur_error_index variable.
 */
#define PRINT_ERROR \
    do {                                                                  \
        fprintf(stderr, "Incorrect format of the log message started at " \
                "%ld offset from the beginning\nof the raw log file:\n"   \
                "%s\n",                                                   \
                cur_msg_offset, dbg_msgs[cur_error_index].content);       \
    } while (0)

/** 
 * Reads specified number of bytes from "_fd" file descriptor and 
 * generates an exception if not all bytes read.
 * @se It may never return control.
 */
#define READ(_fd, _buf, _len) \
    do {                                                                   \
        if (universal_read((_fd), (_buf), (_len), ctx->io_mode) != (_len)) \
        {                                                                  \
            /* Error: File is truncated */                                 \
            PRINT_ERROR;                                                   \
            THROW_EXCEPTION;                                               \
        }                                                                  \
    } while (0)

/* 
 * Macro to convert "next field length" field from network to
 * host byte order.
 */
#if SIZEOF_TE_LOG_NFL == 4
#define RGT_NFL_NTOH(val_) \
    do {                    \
        val_ = ntohl(val_); \
    } while (0)
#elif SIZEOF_TE_LOG_NFL == 2
#define RGT_NFL_NTOH(val_) \
    do {                    \
        val_ = ntohs(val_); \
    } while (0)
#elif SIZEOF_TE_LOG_NFL == 1
/* Do nothing in case of 1 byte */
#define NFL_NTOH(val_)
#else
#error SIZEOF_TE_LOG_NFL is expected to be 1, 2 or 4
#endif

/**
 * Extracts the next log message from a raw log file version 1.
 * The format of raw log file version 1 can be found in
 * OKT-HLD-0000095-TE_TS.
 *
 * @param  msg        Storage for log message to be extracted.
 * @param  fd         File descriptor of the raw log file.
 *
 * @return  Status of the operation.
 *
 * @retval  1   Message is successfuly read from Raw log file
 * @retval  0   There is no log messages left.
 *
 * @se
 *   If the structure of a log message doesn't comfim to the specification,
 *   this function never returns, but rather it throws an exception with 
 *   longjmp call.
 */
int
fetch_log_msg_v1(log_msg **msg, rgt_gen_ctx_t *ctx)
{
    te_log_nfl        nflen; /* Next field length */
    te_log_version    log_ver;
    te_log_ts_sec     ts_sec;
    te_log_ts_usec    ts_usec;
    te_log_level      log_level;
    te_log_id         log_id;
    FILE             *fd = ctx->rawlog_fd;

    char     *entity_name;
    char     *user_name;
    char     *fmt_str;
    msg_arg **arg;

    struct obstack *obstk;
    /* 
     * Get offset of the log message from the beginning of the RLF.
     * It is used in the case of an error occurs.
     */
    cur_msg_offset = ftell(fd);

    /* 
     * Read version of log message.
     * Just ignore (TODO: more useful processing below)
     */
    if (universal_read(fd, &log_ver, sizeof(log_ver), ctx->io_mode) == 0)
    {
        /*
         * There are no messages left (rgt operation mode is postponed)
         * Notify about that fact.
         */
        return 0;
    }
    LOG_FORMAT_DEBUG_SET(RLF_V1_RLM_VERSION);
    if (log_ver != TE_LOG_VERSION)
    {
        PRINT_ERROR;
        THROW_EXCEPTION;
    }
    
    /* START PROCESSING OF A NEW MESSAGE */
    *msg = alloc_log_msg();
    obstk = (*msg)->obstk;
    arg = &((*msg)->args);
    
    /* Read timestamp */
    LOG_FORMAT_DEBUG_SET(RLF_V1_RLM_TIMESTAMP);
    READ(fd, &ts_sec,  sizeof(ts_sec));
    READ(fd, &ts_usec, sizeof(ts_usec));

    /* Read log level */
    LOG_FORMAT_DEBUG_SET(RLF_V1_RLM_LOGLEVEL);
    READ(fd, &log_level, sizeof(log_level));
#if SIZEOF_TE_LOG_LEVEL == 2
    log_level = ntohs(log_level);
#elif SIZEOF_TE_LOG_LEVEL == 4
    log_level = ntohl(log_level);
#elif SIZEOF_TE_LOG_LEVEL == 1
    /* Do nothing */
#else
#error SIZEOF_TE_LOG_LEVEL is expected to be 1, 2, or 4
#endif

    /* Read log ID */
    LOG_FORMAT_DEBUG_SET(RLF_V1_RLM_LOG_ID);
    READ(fd, &log_id, sizeof(log_id));
#if SIZEOF_TE_LOG_ID == 4
    log_id = ntohl(log_id);
#elif SIZEOF_TE_LOG_ID == 2
    log_id = ntohs(log_id);
#elif SIZEOF_TE_LOG_ID == 1
    /* Do nothing */
#else
#error SIZEOF_TE_LOG_ID is expected to be 1, 2, or 4
#endif

    LOG_FORMAT_DEBUG_SET(RLF_V1_RLM_ENTITY_NAME);
    /* Read entity name length */
    READ(fd, &nflen, sizeof(nflen));
    RGT_NFL_NTOH(nflen);
    /* Allocate memory for entity name and read it */
    entity_name = (char *)obstack_alloc(obstk, nflen + 1);
    READ(fd, entity_name, nflen);
    entity_name[nflen] = '\0';

    LOG_FORMAT_DEBUG_SET(RLF_V1_RLM_USER_NAME);
    /* Read user name length */
    READ(fd, &nflen, sizeof(nflen));
    RGT_NFL_NTOH(nflen);
    /* Allocate memory for user name and read it */
    user_name = (char *)obstack_alloc(obstk, nflen + 1);
    READ(fd, user_name, nflen);
    user_name[nflen] = '\0';

    LOG_FORMAT_DEBUG_SET(RLF_V1_RLM_FORMAT_STRING);
    /* Read format string length */
    READ(fd, &nflen, sizeof(nflen));
    RGT_NFL_NTOH(nflen);
    /* Allocate memory for format string and read it */
    fmt_str = (char *)obstack_alloc(obstk, nflen + 1);
    READ(fd, fmt_str, nflen);
    fmt_str[nflen] = '\0';

    LOG_FORMAT_DEBUG_SET(RLF_V1_RLM_ARG_LEN);

    /* Process format string arguments */
    (*msg)->args_count = 0;
    
    /* Read the next argument length */
    READ(fd, &nflen, sizeof(nflen));
    RGT_NFL_NTOH(nflen);

    while (nflen != TE_LOG_RAW_EOR_LEN)
    {
        /* Get parameter */
        *arg = (msg_arg *)obstack_alloc(obstk, sizeof(msg_arg));
        (*arg)->len = nflen;

        /* 
         * Here we append parameter with '\0' character:
         * If parameter is a string it terminates string, if 
         * it is interger the '\0' character won't be taken into 
         * account (according to the len field).
         */
        (*arg)->val = (uint8_t *)obstack_alloc(obstk, nflen + 1);
        READ(fd, (*arg)->val, nflen);
        (*arg)->val[nflen] = '\0';

        arg = &((*arg)->next);
        (*msg)->args_count++;

        /* Read the next argument length */
        READ(fd, &nflen, sizeof(nflen));
        RGT_NFL_NTOH(nflen);
    }

    *arg = NULL;

    (*msg)->id = log_id;
    (*msg)->entity = entity_name;
    (*msg)->user   = user_name;
    (*msg)->timestamp[0] = ntohl(ts_sec);
    (*msg)->timestamp[1] = ntohl(ts_usec);
    (*msg)->fmt_str = fmt_str;
    (*msg)->cur_arg = (*msg)->args;
    (*msg)->txt_msg = NULL;


    switch (log_level)
    {
#define RGT_LL_CASE(lvl_) \
        case TE_LL_ ## lvl_:                         \
            (*msg)->level = RGT_LL_ ## lvl_ ## _STR; \
            break

        RGT_LL_CASE(ERROR);
        RGT_LL_CASE(WARN);
        RGT_LL_CASE(RING);
        RGT_LL_CASE(INFO);
        RGT_LL_CASE(VERB);
        RGT_LL_CASE(ENTRY_EXIT);

#undef RGT_LL_CASE
        default:
            /* Print error message but continue processing */
            LOG_FORMAT_DEBUG_SET(RLF_V1_RLM_UNKNOWN_LOGLEVEL);
            PRINT_ERROR;

            (*msg)->level = RGT_LL_UNKNOWN_STR;
    }

    obstk = NULL;

    return 1;
}
