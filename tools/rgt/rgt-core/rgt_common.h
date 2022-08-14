/** @file
 * @brief Test Environment: RGT common declarations.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_RGT_COMMON_H__
#define __TE_RGT_COMMON_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "te_config.h"

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

#include <glib.h>

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
#include "te_raw_log.h"

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

/**
 * Performs the following operation:
 * res_ts = ts2 - ts1;
 */
#define TIMESTAMP_SUB(res_ts, ts2, ts1) \
     (res_ts[0] = ts2[0] - ts1[0] + ((ts1[1] > ts2[1]) ? -1 : 0), \
      res_ts[1] = (1000000 - ts1[1] + ts2[1]) % 1000000)

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
    RGT_OP_MODE_INDEX     = 2, /**< Index operation mode */
    RGT_OP_MODE_JUNIT     = 3, /**< JUnit operation mode */
    RGT_OP_MODE_DEFAULT   = RGT_OP_MODE_POSTPONED /**< Default operation
                                                       mode */
} rgt_op_mode_t;

/* Modes of operation in string representation */
#define RGT_OP_MODE_LIVE_STR      "live"
#define RGT_OP_MODE_POSTPONED_STR "postponed"
#define RGT_OP_MODE_INDEX_STR     "index"
#define RGT_OP_MODE_JUNIT_STR     "junit"

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
    off_t          rawlog_size; /**< Size of Raw log file,
                                     has sense only in postponed mode */
    off_t          rawlog_fpos; /**< Position in raw log file on
                                     reading the current message */
    const char    *out_fname; /**< Output file name */
    FILE          *out_fd; /**< Output file pointer */

    const char    *fltr_fname; /**< XML filter file name */

    char          *tmp_dir; /**< Temporary directory used for offloading
                                 of message pointers into files */

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

    te_bool         proc_incomplete; /**< Whether Rgt should process
                                          incomplete log reports as normal
                                          or give error message */

    te_bool         ignore_unknown_id;  /**< Ignore messages with unknown
                                             log node ID */

    te_bool         verb; /**< Whether to use verbose output or not */
    int             current_nest_lvl;  /**< Current nesting level */
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

/** Message flag values */
#define RGT_MSG_FLG_NORMAL   0x1 /**< An ordinary message */
#define RGT_MSG_FLG_VERDICT  0x2 /**< A message is verdict */
#define RGT_MSG_FLG_ARTIFACT 0x4 /**< A message is artifact */

/** Structure that keeps log message in an universal format */
typedef struct log_msg {
    struct obstack *obstk;      /**< Internal field:
                                     Obstack for the message */

    unsigned      id;           /**< ID of the log message, which currently
                                     defines which test is logged this
                                     message */
    uint32_t      flags;        /**< Message flags */
    char         *entity;       /**< Entity name of the message */
    char         *user;         /**< User name of the message */
    uint32_t      timestamp[2]; /**< Timestamp value */
    te_log_level  level;        /**< Log level */
    const char   *level_str;    /**< Log level in string format */
    char         *fmt_str;      /**< Raw format string */
    msg_arg      *args;         /**< List of arguments for format string */
    msg_arg      *cur_arg;      /**< Internal field:
                                     used by get_next_arg function */
    int           args_count;   /**< Total number of the arguments */

    char         *txt_msg;      /**< Processed fmt_str + args */
    int           nest_lvl;     /**< Nesting level */
} log_msg;

/**
 * Structure that is stored in the tree of log nodes instead of
 * log_msg structure for each regular log message. It is used to
 * reduce memory consumption: full log message is loaded to memory
 * from file only when we need to process it, and memory is released
 * as soon as we end with it.
 */
typedef struct log_msg_ptr {
  off_t       offset;         /**< At which offset in raw log file
                                   we can find message referenced
                                   by this structure */
  uint32_t    timestamp[2];   /**< Timestamp of referenced log
                                   message */
} log_msg_ptr;

/**
 * Structure storing a queue of regular log message pointers.
 */
typedef struct msg_queue {
    GQueue   *queue;            /**< Queue of message pointers
                                     stored in memory */
    GList    *cache;            /**< A slot in queue after which
                                     the next message pointer
                                     could be added with high probability */

    te_bool   offloaded;        /**< Whether some message pointers
                                     are offloaded to a file */
    uint32_t  offload_ts[2];    /**< Timestamp of the most recent
                                     message pointer offloaded to
                                     a file*/
} msg_queue;

/**
 * Iterate over message pouinters queue, starting with entries offloaded
 * to a file.
 *
 * @param q           Queue of message pointers
 * @param cb          Callback to be called for each queue entry
 * @param user_data   Pointer to be passed to callback
 */
extern void msg_queue_foreach(msg_queue *q, GFunc cb, void *user_data);

/**
 * Check whether message pointers queue is empty.
 *
 * @param q     Queue of message pointers
 *
 * @return TRUE if queue is empty, FALSE otherwise.
 */
extern te_bool msg_queue_is_empty(msg_queue *q);

#ifdef __cplusplus
}
#endif

#endif /* __TE_RGT_COMMON_H__ */

