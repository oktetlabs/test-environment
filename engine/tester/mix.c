/** @file
 * @brief Tester Subsystem
 *
 * Implementation of mixing test iterations before run.
 *
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
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

#define TE_LGR_USER     "Mix"

#include "te_config.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "te_errno.h"

#include "tester_conf.h"
#include "tester_run.h"


#if 0
typedef struct tester_mix_data {
} tester_mix_data;


te_errno
tester_scenario_mix(testing_scenario *scenario, tester_cfgs *cfgs)
{
    testing_act         *act;
    tester_cfg_walk_ctl  ctl;
    tester_cfg_walk      mix_cbs;
    tester_mix_data      data;

    for (act = scenario->tqh_first; act != NULL; act = act->links.tqe_next)
    {
        ctl = tester_configs_walk(cfgs, &mix_cbs, &data);
    }
}
#endif
