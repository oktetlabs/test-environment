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

#include <stdio.h>
#include <obstack.h>

#include "log_format.h"
#include "rgt_common.h"
#include "io.h"
#include "memory.h"

/** Indeces of the error events */
enum e_error_msg_index {
    RLF_V1_RLM_ENTITY_NAME       = 0, /**< Entity name is too short */
    RLF_V1_RLM_VERSION           = 1, /**< There is no log version field */
    RLF_V1_RLM_TIMESTAMP         = 2, /**< Timestamp field is too short */
    RLF_V1_RLM_LOGLEVEL          = 3, /**< Log Level field is too short */
    RLF_V1_RLM_REST_LEN          = 4, /**< Message length field is too short */
    RLF_V1_RLM_REST_LEN_VALUE    = 5, /**< Message length field is incorrect */
    RLF_V1_RLM_WHOLE             = 6, /**< Message is truncated */
    RLF_V1_RLM_USER_NAME_LEN     = 7, /**< User name is out of message */
    RLF_V1_RLM_FORMAT_STRING_LEN = 8, /**< Format string is out of message */
    RLF_V1_RLM_ARG_LEN           = 9, /**< Agrument is out of message */
    RLF_V1_RLM_UNKNOWN_LOGLEVEL  = 10, /**< Unknown log level value */
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
 * This is an array of error messages each corresponding to an appropriate error 
 * index of enum "e_error_msg_index".
 * DON'T CHANGE THE ORDER OF THESE MESSAGES!
 */
static struct debug_msg {
    char *content;
} dbg_msgs[] = {
    {"*** Entity name is truncated."},
    {"*** Log message version is truncated."},
    {"*** Log message timestamp is truncated."},
    {"*** Log message log level is truncated."},
    {"*** Log message length field is truncated."},
    {"*** Value of the log message length is incorrect. "
     "It should be more than two."},
    {"*** Log message payload is too short according to log message "
     "length field."},
    {"*** Value of the log message length or user name "
     "length is incorrect: \n"
     "*** User name length is out of the log message."},
    {"*** Value of the log message length or format string "
     "length is incorrect: \n"
     "*** Format string length is out of the log message."},
    {"*** Value of the log message length or argument length is incorrect: \n"
     "*** Argument length is out of the log message."},
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
    do {                                                                      \
        if (universal_read((_fd), (_buf), (_len), rgt_rmode) != (_len))       \
        {                                                                     \
            /* Error: File is truncated */                                    \
            PRINT_ERROR;                                                      \
            THROW_EXCEPTION;                                                  \
        }                                                                     \
    } while (0)


/**
 * Extracts the next log message from a raw log file version 1.
 * The format of raw log file version 1 can be found in OKT-HLD-0000095-TE_TS.
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
fetch_log_msg_v1(log_msg **msg, FILE *fd)
{
    uint8_t   nflen; /* Next field length */
    uint32_t  timestamp[2];
    uint16_t  log_level;
    uint16_t  msg_len;
    uint8_t  *msg_content;

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

    /* Read length of entity name */
    if (universal_read(fd, &nflen, 1, rgt_rmode) == 0)
    {
        /*
         * There are no messages left (rgt operation mode is postponed)
         * Notify about that fact.
         */
        return 0;
    }

    *msg = alloc_log_msg();
    obstk = (*msg)->obstk;

    arg = &((*msg)->args);

    /* START PROCESSING OF A NEW MESSAGE */
    
    /* Process Entity Name: Note that it doesn't end with '\0' */
    entity_name = (char *)obstack_alloc(obstk, nflen + 1);
    LOG_FORMAT_DEBUG_SET(RLF_V1_RLM_ENTITY_NAME);
    READ(fd, entity_name, nflen);
    entity_name[nflen] = '\0';

    /* 
     * Read version of log message.
     * Just ignore (TODO: more useful processing)
     */
    LOG_FORMAT_DEBUG_SET(RLF_V1_RLM_VERSION);
    READ(fd, &nflen, 1);
    
    /* Read timestamp */
    LOG_FORMAT_DEBUG_SET(RLF_V1_RLM_TIMESTAMP);
    READ(fd, timestamp, 8);

    /* Read log level */
    LOG_FORMAT_DEBUG_SET(RLF_V1_RLM_LOGLEVEL);
    READ(fd, &log_level, 2);
    log_level = ntohs(log_level);
    
    /* Read log msg len: It is placed in Network byte order */
    LOG_FORMAT_DEBUG_SET(RLF_V1_RLM_REST_LEN);
    READ(fd, &msg_len, 2);
    msg_len = ntohs(msg_len);

    /* 
     * 1 bytes for "User name" field length and 
     * 1 byte for "Format string" field length.
     */
    if (msg_len < 2)
    {
        /* Log message should be at least two bytes */
        LOG_FORMAT_DEBUG_SET(RLF_V1_RLM_REST_LEN_VALUE);
        PRINT_ERROR;
        THROW_EXCEPTION;
    }

    /* 
     * Allocate memory for the whole message 
     * TODO: Allocate memory through malloc or get from some pool.
     *       Because we will allocate this memory twise.
     */
    msg_content = (uint8_t *)obstack_alloc(obstk, msg_len);
    LOG_FORMAT_DEBUG_SET(RLF_V1_RLM_WHOLE);
    READ(fd, msg_content, msg_len);
    nflen = *(msg_content++);

    if (msg_len < nflen + 1)
    {
        /* User name length is out of the log message */
        LOG_FORMAT_DEBUG_SET(RLF_V1_RLM_USER_NAME_LEN);
        PRINT_ERROR;
        THROW_EXCEPTION;
    }

    user_name = (char *)obstack_copy0(obstk, msg_content, nflen);
    msg_content += nflen;
    msg_len -= (nflen + 2);
    nflen = *(msg_content++);

    /*
     * Now msg_len is a number of bytes started from format string 
     * to the rest of the message.
     */

    if (msg_len < nflen)
    {
        /* Format string length is out of the log message */
        LOG_FORMAT_DEBUG_SET(RLF_V1_RLM_FORMAT_STRING_LEN);
        PRINT_ERROR;
        THROW_EXCEPTION;
    }

    fmt_str = (char *)obstack_copy0(obstk, msg_content, nflen);
    msg_content += nflen;
    msg_len -= nflen;
    
    /* Process format string arguments */
    (*msg)->args_count = 0;
    while (msg_len-- > 0)
    {
        nflen = *(msg_content++);

        if (msg_len < nflen)
        {
            /* Argument length field is out of the log message length */
            LOG_FORMAT_DEBUG_SET(RLF_V1_RLM_ARG_LEN);
            PRINT_ERROR;
            THROW_EXCEPTION;
        }

        /* Get parameter */
        *arg = (msg_arg *)obstack_alloc(obstk, sizeof(msg_arg));
        (*arg)->len = nflen;

        /* 
         * Here we append parameter with '\0' character:
         * If parameter is a string it terminates string, if 
         * it is interger the '\0' character won't be taken into 
         * account (according to the len field).
         */
        (*arg)->val = (uint8_t *)obstack_copy0(obstk, msg_content, nflen);

        arg = &((*arg)->next);
        msg_len -= nflen;
        msg_content += nflen;
        (*msg)->args_count++;
    }

    *arg = NULL;

    (*msg)->entity = entity_name;
    (*msg)->user   = user_name;
    (*msg)->timestamp[0] = ntohl(timestamp[0]);
    (*msg)->timestamp[1] = ntohl(timestamp[1]);
    (*msg)->fmt_str = fmt_str;
    (*msg)->cur_arg = (*msg)->args;
    (*msg)->txt_msg = NULL; 


#ifdef HAVE_LOGGER_DEFS_H
    switch (log_level)
    {
#define LGLVL_CASE(lvl_) \
        case lvl_ ## _LVL:                          \
            (*msg)->level = LGLVL_ ## lvl_ ## _STR; \
            break

        LGLVL_CASE(ERROR);
        LGLVL_CASE(WARNING);
        LGLVL_CASE(RING);
        LGLVL_CASE(INFORMATION);
        LGLVL_CASE(VERBOSE);
        LGLVL_CASE(ENTRY_EXIT);

#undef LGLVL_CASE
        default:
            /* Print error message but continue processing */
            LOG_FORMAT_DEBUG_SET(RLF_V1_RLM_UNKNOWN_LOGLEVEL);
            PRINT_ERROR;

            (*msg)->level = LGLVL_UNKNOWN_STR;
    }
#else
    (*msg)->level = LGLVL_EMPTY_STR;
#endif

    obstk = NULL;

    return 1;
}
