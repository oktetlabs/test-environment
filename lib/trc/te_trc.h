/** @file
 * @brief Testing Results Comparator
 *
 * Definition of data types and API to use TRC.
 *
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TRC_H__
#define __TE_TRC_H__

#include "te_errno.h"
#include "te_queue.h"
#include "te_test_result.h"
#include "logic_expr.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Single test result with auxiliary information for TRC.
 */
typedef struct trc_exp_result_entry {
    TAILQ_ENTRY(trc_exp_result_entry)   links;  /**< List links */

    te_test_result  result; /**< Test result */

    /* Auxiliary information */
    char       *key;        /**< BugID-like information */
    char       *notes;      /**< Any kind of notes */
} trc_exp_result_entry;

/**
 * Expected test result.
 */
typedef struct trc_exp_result {
    LIST_ENTRY(trc_exp_result)  links;  /**< List links */

    char       *tags_str;   /**< String representation of tags logical
                                 expression */
    logic_expr *tags_expr;  /**< Tags logical expression */

    /** Results expected for such tags logical expression */
    TAILQ_HEAD(, trc_exp_result_entry)  results;

    /* Auxiliary information common for expected results */
    char       *key;        /**< BugID-like information */
    char       *notes;      /**< Any kind of notes */
} trc_exp_result;

/** List of expected results */
typedef LIST_HEAD(trc_exp_results, trc_exp_result) trc_exp_results;


/*
 * Testing Results Comparator database interface
 */

/* Forward declaration of the Testing Results Comparator instance */
struct te_trc;
/* Forward declaration of the TRC database instance */
struct te_trc_db;
typedef struct te_trc_db te_trc_db;

/**
 * Open TRC database.
 *
 * @param location      Location of the database
 * @param p_trc_db      Location for TRC database instance handle
 *
 * @return Status code.
 *
 * @sa trc_db_close
 */
extern te_errno trc_db_open(const char *location,
                            struct te_trc_db **p_trc_db);

/**
 * Close TRC database.
 *
 * @param trc_db        TRC database instance handle
 */
extern void trc_db_close(struct te_trc_db *trc_db);


/*
 * Testing Results Comparator instance.
 */

/**
 * Initialize TRC instance.
 *
 * @param p_trci        Location for TRC instance handle
 *
 * @return Status code.
 */
extern te_errno trc_init(struct te_trc **p_trci);

/**
 * Set TRC database.
 *
 * @param trci          TRC instance handle
 * @param db            TRC database instance handle
 *
 * @return Status code.
 */
extern te_errno trc_set_db(struct te_trc    *trci,
                           struct te_trc_db *trc_db);

#if 0
/**
 * Initialize TRC instance.
 *
 * @param trci          TRC instance handle
 * @param iut_id        IUT identification tags
 *
 * @return Status code.
 */
extern te_errno trc_add_iut_id(struct te_trc *trci, const trc_tags *iut_id);
#endif

/**
 * Free TRC instance.
 *
 * @param trci          TRC instance handle
 */
extern void trc_free(struct te_trc *trci);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TRC_H__ */
