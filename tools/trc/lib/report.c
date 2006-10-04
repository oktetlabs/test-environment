/** @file
 * @brief Testing Results Comparator: report tool
 *
 * Auxiluary routines for tool which generates TRC report by obtained
 * result.
 *
 *
 * Copyright (C) 2005-2006 Test Environment authors (see file AUTHORS
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
 *
 * $Id$
 */

#include "te_config.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_STRING_H
#include <string.h>
#endif

#include "te_queue.h"
#include "trc_report.h"


/* See the description in trc_report.h */
void
trc_report_init_ctx(trc_report_ctx *ctx)
{
    memset(ctx, 0, sizeof(ctx));
    TAILQ_INIT(&ctx->tags);
}

/* See the description in trc_report.h */
void
trc_report_stats_add(trc_report_stats *stats, const trc_report_stats *add)
{
    stats->pass_exp += add->pass_exp;
    stats->pass_une += add->pass_une;
    stats->fail_exp += add->fail_exp;
    stats->fail_une += add->fail_une;
    stats->aborted += add->aborted;
    stats->new_run += add->new_run;

    stats->not_run += add->not_run;
    stats->skip_exp += add->skip_exp;
    stats->skip_une += add->skip_une;
    stats->new_not_run += add->new_not_run;
}

/* See the description in trc_report.h */
void
trc_report_free_test_iter_data(trc_report_test_iter_data *data)
{
    trc_report_test_iter_entry *p;

    while ((p = data->runs.tqh_first) != NULL)
    {
        TAILQ_REMOVE(&data->runs, p, links);
        te_test_result_free_verdicts(&p->result);
        free(p);
    }
    free(data);
}
