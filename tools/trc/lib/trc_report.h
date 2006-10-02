/** @file
 * @brief Testing Results Comparator: report tool
 *
 * Definition of TRC report tool types and related routines.
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TRC_REPORT_H__
#define __TE_TRC_REPORT_H__

#include "te_defs.h"
#include "te_queue.h"
#include "te_errno.h"
#include "tq_string.h"
#include "te_trc.h"
#include "trc_db.h"

#ifdef __cplusplus
extern "C" {
#endif


/** Testing results comparator statistics */
typedef struct trc_report_stats {
    unsigned int    pass_exp;       /**< Passed as expected */
    unsigned int    pass_une;       /**< Passed unexpectedly */
    unsigned int    fail_exp;       /**< Failed as expected */
    unsigned int    fail_une;       /**< Failed unexpectedly */
    unsigned int    aborted;        /**< No usefull result */
    unsigned int    new_run;        /**< Run iterations with unknown
                                         expected result */
    unsigned int    not_run;        /**< Not run iterations */
    unsigned int    skip_exp;       /**< Skipped as expected */
    unsigned int    skip_une;       /**< Skipped unexpectedly */
    unsigned int    new_not_run;    /**< Not run iterations with unknown
                                         expected result */
} trc_report_stats;

/** Number of run test iterations */
#define TRC_STATS_RUN(s) \
    ((s)->pass_exp + (s)->pass_une + (s)->fail_exp + (s)->fail_une + \
     (s)->aborted + (s)->new_run)

/** Number of test iterations with specified result */
#define TRC_STATS_SPEC(s) \
    (TRC_STATS_RUN(s) + (s)->skip_exp + (s)->skip_une)

/** Number of test iterations with unexpected results */
#define TRC_STATS_UNEXP(s) \
    ((s)->pass_une + (s)->fail_une + (s)->skip_une + (s)->aborted + \
     (s)->new_run + (s)->not_run + (s)->new_not_run)

/** Number of test iterations which have not been run in fact */
#define TRC_STATS_NOT_RUN(s) \
    ((s)->not_run + (s)->skip_exp + (s)->skip_une + (s)->new_not_run)


/** TRC report tool options */
enum trc_report_flags {
    /* HTML report options */
    TRC_REPORT_NO_TOTAL_STATS   = 0x01,  /**< Hide grand total
                                              statistics */
    TRC_REPORT_NO_PACKAGES_ONLY = 0x02,  /**< Hide packages only
                                              statistics */
    TRC_REPORT_NO_SCRIPTS       = 0x04,  /**< Hide scripts */
    TRC_REPORT_STATS_ONLY       = 0x08,  /**< Show statistics only */
    TRC_REPORT_NO_UNSPEC        = 0x10,  /**< Hide entries with no
                                              obtained result */
    TRC_REPORT_NO_SKIPPED       = 0x20,  /**< Hide skipped iterations */
    TRC_REPORT_NO_EXP_PASSED    = 0x40,  /**< Hide passed as expected
                                              iterations */
    TRC_REPORT_NO_EXPECTED      = 0x80,  /**< Hide all expected
                                              iterations */
    TRC_REPORT_NO_STATS_NOT_RUN = 0x100, /**< Hide entries with
                                              unexpected not run
                                              statistic */

    /* DB processing options */
    TRC_REPORT_UPDATE_DB        = 0x200, /**< Update TRC database */
    TRC_REPORT_IGNORE_LOG_TAGS  = 0x400, /**< Ignore TRC tags extracted
                                              from the log */
};

/** TRC report context */
typedef struct trc_report_ctx {
    unsigned int        flags;  /**< Report options */
    te_trc_db          *db;     /**< TRC database handle */
    tqh_strings         tags;   /**< TRC tags specified by user */
    trc_report_stats    stats;  /**< Grand total statistics */
} trc_report_ctx;

/**
 * Initialize TRC report tool context.
 *
 * @param ctx           Context to be initialized
 */
extern void trc_report_init_ctx(trc_report_ctx *ctx);

/**
 * Process TE log file with obtained testing results.
 *
 * @param ctx           TRC report context
 * @param log           Name of the file with TE log in XML format
 *
 * @return Status code.
 */
extern te_errno trc_report_process_log(trc_report_ctx *ctx,
                                       const char     *log);

/**
 * Output TRC report in HTML format.
 *
 * @param ctx           TRC report context
 * @param filename      Name of the file for HTML report
 * @param header        File with header to be added in HTML report
 * @param flags         Report options
 *
 * @return Status code.
 */
extern te_errno trc_report_to_html(trc_report_ctx *ctx,
                                   const char     *filename,
                                   FILE           *header,
                                   unsigned int    flags);

/**
 * Add one statistics to another.
 *
 * @param stats     Total statistics
 * @param add       Statistics to add
 */
extern void trc_report_stats_add(trc_report_stats       *stats,
                                 const trc_report_stats *add);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TRC_REPORT_H__ */
