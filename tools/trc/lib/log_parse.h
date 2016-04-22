/** @file
 * @brief Testing Results Comparator: tools
 *
 * Definition of XML log parsing common types and routines.
 *
 *
 * Copyright (C) 2005-2006 Test Environment authors (see file AUTHORS
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
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_LOG_PARSE_H__
#define __TE_LOG_PARSE_H_

#include "te_config.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "te_errno.h"
#include "te_alloc.h"
#include "te_queue.h"
#include "te_test_result.h"
#include "te_trc.h"
#include "te_string.h"
#include "trc_tags.h"
#include "trc_db.h"

#include "trc_report.h"

#ifdef __cplusplus
extern "C" {
#endif

/** TRC XML log parsing options */
enum trc_log_parse_flags {
    TRC_LOG_PARSE_IGNORE_LOG_TAGS
                               = (1LLU << 0),  /**< Ignore TRC tags
                                                    extracted
                                                    from the log */
};

typedef enum trc_log_parse_app_id {
    TRC_LOG_PARSE_UNKNOWN = 0,
    TRC_LOG_PARSE_APP_UPDATE,
} trc_log_parse_app_id;


/** State of the TE log parser from TRC point of view */
typedef enum trc_log_parse_state {
    TRC_LOG_PARSE_INIT,      /**< Initial state */
    TRC_LOG_PARSE_ROOT,      /**< Root state */
    TRC_LOG_PARSE_TAGS,      /**< Inside log message with TRC
                                         tags list */
    TRC_LOG_PARSE_TEST,      /**< Inside 'test', 'pkg' or
                                         'session' element */
    TRC_LOG_PARSE_META,      /**< Inside 'meta' element */
    TRC_LOG_PARSE_OBJECTIVE, /**< Inside 'objective' element */
    TRC_LOG_PARSE_VERDICTS,  /**< Inside 'verdicts' element */
    TRC_LOG_PARSE_VERDICT,   /**< Inside 'verdict' element */
    TRC_LOG_PARSE_PARAMS,    /**< Inside 'params' element */
    TRC_LOG_PARSE_LOGS,      /**< Inside 'logs' element */
    TRC_LOG_PARSE_SKIP,      /**< Skip entire contents */
} trc_log_parse_state;

/** TRC report TE log parser context. */
typedef struct trc_log_parse_ctx {

    te_errno              rc;         /**< Status of processing */

    trc_log_parse_app_id  app_id;     /**< Application id */
    void                 *app_ctx;    /*<< Application-specific context */

    uint32_t              flags;      /**< Processing flags */

    te_trc_db            *db;         /**< TRC database handle */
    unsigned int          db_uid;     /**< TRC database user ID */
    const char           *log;        /**< Name of the file with log */
    tqh_strings          *tags;       /**< List of tags */
    te_trc_db_walker   *db_walker;  /**< TRC database walker */
    char               *run_name;   /**< Name of the tests run the
                                         current log belongs to */

    trc_log_parse_state         state;  /**< Log parse state */
    trc_test_type               type;   /**< Type of the test */

    char   *str;    /**< Accumulated string (used when single string is
                         reported by few trc_log_parse_characters()
                         calls because of entities in it) */

    unsigned int                skip_depth; /**< Skip depth */
    trc_log_parse_state         skip_state; /**< State to return */

    trc_report_test_iter_data  *iter_data;  /**< Current test iteration
                                                 data */

    unsigned int        stack_size; /**< Size of the stack in elements */
    te_bool            *stack_info; /**< Stack */
    unsigned int        stack_pos;  /**< Current position in the stack */
} trc_log_parse_ctx;

extern te_errno trc_log_parse_process_log(trc_log_parse_ctx *ctx);

#define CONST_CHAR2XML  (const xmlChar *)
#define XML2CHAR(p)     ((char *)p)

/**
 * Convert string representation of test status to enumeration member.
 *
 * @param str           String representation of test status
 * @param status        Location for converted test status
 *
 * @return Status code.
 */
extern te_errno te_test_str2status(const char *str, te_test_status *status);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_LOG_PARSE_H__ */
