/** @file 
 * @brief Test Environment: RGT common declarations.
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

#ifndef __TE_RGT_COMMON_H__
#define __TE_RGT_COMMON_H__
#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if HAVE_INTTYPES_H
# include <inttypes.h>
#else
# if HAVE_STDINT_H
#  include <stdint.h>
# endif
#endif

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif

#if HAVE_STRING_H
#include <string.h>
#else
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#endif

#ifdef HAVE_ASSERT_H
#include <assert.h>
#else
#define assert(_x)
#endif

#include <sys/types.h>
#include <errno.h>
#include <setjmp.h>
#include <stdio.h>

/* For byte order conversions */
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#else

# define RGT_UINT16_SWAP(val) \
    ((uint16_t) ((uint16_t) ((uint16_t) (val) >> 8) | \
                 (uint16_t) ((uint16_t) (val) << 8)))

# define RGT_UINT32_SWAP(val) \
    ((uint32_t) ((((uint32_t) (val) & (uint32_t) 0x000000ffU) << 24) | \
                 (((uint32_t) (val) & (uint32_t) 0x0000ff00U) <<  8) | \
                 (((uint32_t) (val) & (uint32_t) 0x00ff0000U) >>  8) | \
                 (((uint32_t) (val) & (uint32_t) 0xff000000U) >> 24)))

# ifdef WORDS_BIGENDIAN
  /* Native format Big Endian */
#   define ntohl(x) (x)
#   define ntohs(x) (x)
# else
  /* Native format Little Endian */
#   define ntohl(x) RGT_UINT32_SWAP(x)
#   define ntohs(x) RGT_UINT16_SWAP(x)
# endif
#endif /* HAVE_NETINET_IN_H */

#include "io.h"

#include "te_defs.h"

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef UINT32_MAX
#define UINT32_MAX (4294967295U)
#endif

/** 
 * Compares two timestamp values and returns the following values depending
 * on ts1 and ts2 values:
 *  if ts1 < ts2 it returns -1
 *  if ts1 > ts2 it returns  1
 *  if ts1 = ts2 it returns  0
 */
#define TIMESTAMP_CMP(ts1, ts2) \
        ((ts1[0] < ts2[0]) ? -1 :              \
             ((ts1[0] > ts2[0]) ? 1 :          \
                 (ts1[1] < ts2[1]) ? -1 :      \
                 (ts1[1] > ts2[1]) ? 1 : 0))

#ifndef ESUCCESS
#define ESUCCESS 0
#endif

/*************************************************************************/
/*                  RGT-specific definitions                             */
/*************************************************************************/

/** The stack context of the main procedure */
extern jmp_buf rgt_mainjmp;

/* Generates an exception from any point of RGT */
#define THROW_EXCEPTION \
    do {                     \
        longjmp(rgt_mainjmp, 1); \
    } while (0)


#define TRACE(str)               fprintf(stderr, "%s", str)
#define FMT_TRACE(fmt, _args...) fprintf(stderr, fmt "\n", _args + 0)

/** RGT operation mode constants */
typedef enum rgt_op_mode {
    RGT_OP_MODE_LIVE      = 0, /**< Live operation mode */
    RGT_OP_MODE_POSTPONED = 1, /**< Postponed operation mode */
    RGT_OP_MODE_DEFAULT   = RGT_OP_MODE_POSTPONED /**< Default operation
                                                       mode */
} rgt_op_mode_t;

/* Two modes of operation in string representation */
#define RGT_OP_MODE_LIVE_STR      "live"
#define RGT_OP_MODE_POSTPONED_STR "postponed"

/* Define default mode of operation */
#define RGT_OP_MODE_DEFAULT_STR RGT_OP_MODE_POSTPONED_STR


struct log_msg;
struct rgt_gen_ctx;

/**
 * Type of function that is used for extracting log messages from
 * a raw log file. Such function is responsible for only raw-level
 * parsing and it doesn't generates the complete log string, so that
 * it shouldn't fill "txt_msg" field of "log_msg" structure.
 */
typedef int (* f_fetch_log_msg)(struct log_msg **msg,
                                struct rgt_gen_ctx *ctx);

/** 
 * Structure that keeps generic data used in processing raw log file.
 */
typedef struct rgt_gen_ctx {
    const char    *rawlog_fname; /**< Raw log file name */
    FILE          *rawlog_fd; /**< Raw log file pointer */
    long           rawlog_size; /**< Size of Raw log file,
                                     has sense only in postponed mode */
    const char    *out_fname; /**< Output file name */
    FILE          *out_fd; /**< Output file pointer */

    const char    *fltr_fname; /**< TCL filter file name */

    rgt_op_mode_t  op_mode; /**< Rgt operation mode */
    const char    *op_mode_str; /**< Rgt operation mode in string 
                                     representation */

    /**
     * Operation mode (live or postponed) influences on desirable 
     * read behaviour that can be blocking or nonblocking.
     * This field keeps current rgt reading mode.
     */
    rgt_io_mode_t  io_mode;

    /**
     * Pointer to a function that should be used for 
     * extracting of log messages from a raw log file.
     * This field is set to an appropriate function according to
     * RLF version determined from the first byte of the RLF.
     */
    f_fetch_log_msg fetch_log_msg;
        
    te_bool         proc_cntrl_msg; /**< Whether Rgt should process control 
                                         messages or not */
                                        
    te_bool         verb; /**< Whether to use verbose output or not */
} rgt_gen_ctx_t;


extern rgt_gen_ctx_t rgt_ctx;

/** The structure keeps statistic on processing raw log file */
struct rgt_statistics {
    uint32_t n_logs;        /**< Total number of log messages */
    uint32_t n_test_pass;   /**< Total number of passed tests */
    uint32_t n_test_fail;   /**< Total number of failed tests */
    uint32_t n_pkg_pass;    /**< Total number of passed packages */
    uint32_t n_pkg_fail;    /**< Total number of failed packages */
    uint32_t n_sess_pass;   /**< Total number of passed sessions */
    uint32_t n_sess_fail;   /**< Total number of failed sessions */
    uint32_t log_start[2];  /**< The timestamp value of the oldest
                                 message */
    uint32_t log_end[2];    /**< The timestamp value of the most
                                 recent message */
    uint16_t n_files;       /**< Total number of files logged */
    uint16_t n_mem;         /**< Total number of memory dumps logged */
    
    /**
     * The following fields are counted only when RGT works 
     * in the mode with detailed statistics.
     */
    /**
     * @todo Number of logs from a particulat entity/user name 
     * May be it should be implemented with Hash tables.
     */
};

/** 
 * Structure that represents argument in its raw representation
 * There must be some more information given to determine which type
 * of data it consists of. (This information can be obtained form
 * format string)
 */
typedef struct msg_arg {
    struct msg_arg *next; /**< Pointer to the next argument */
    uint8_t        *val;  /**< Pointer to raw argument content 
                               (numbers are keeped in network byte order) */
    int             len;  /**< Number of bytes allocated for the argument */
} msg_arg;


/** Structure that keeps log message in an universal format */
typedef struct log_msg {
    struct obstack *obstk;    /**< Internal field: 
                                   Obstack for the message */

    char       *entity;       /**< Entity name of the message */
    char       *user;         /**< User name of the message */
    uint32_t    timestamp[2]; /**< Timestamp value */
    const char *level;        /**< Log level */
    char       *fmt_str;      /**< Raw format string */
    msg_arg    *args;         /**< List of arguments for format string */
    msg_arg    *cur_arg;      /**< Internal field: 
                                   used by get_next_arg function */
    int         args_count;   /**< Total number of the arguments */

    char       *txt_msg;
} log_msg;

#ifdef __cplusplus
}
#endif

#endif /* __TE_RGT_COMMON_H__ */

