/** @file
 * @brief Test API to configure command monitor.
 *
 * Definition of API to configure command monitor.
 *
 *
 * Copyright (C) 2014 Test Environment authors (see file AUTHORS
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
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAPI_CFG_CMD_MONITOR_H__
#define __TE_TAPI_CFG_CMD_MONITOR_H__

#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline te_errno
tapi_cfg_cmd_monitor_begin(char const *ta,
                           char const *name,
                           char const *command,
                           int time_to_wait)
{
    te_errno rc;

    rc = cfg_add_instance_fmt(NULL, CVT_NONE, NULL,
                              "/agent:%s/monitor:%s",
                              ta, name);
    if (rc != 0)
        return rc;

    if ((rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, time_to_wait),
                                   "/agent:%s/monitor:%s/time_to_wait:",
                                   ta, name)) != 0 ||
        (rc = cfg_set_instance_fmt(CFG_VAL(STRING, command),
                                   "/agent:%s/monitor:%s/command:",
                                   ta, name)) != 0 ||
        (rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, 1),
                                   "/agent:%s/monitor:%s/enable:",
                                   ta, name)) != 0)
    {
        cfg_del_instance_fmt(FALSE,
                             "/agent:%s/monitor:%s",
                             ta, name);
        return rc;
    }

    return 0;
}

static inline te_errno
tapi_cfg_cmd_monitor_end(char const *ta,
                         char const *name)
{
    return cfg_del_instance_fmt(FALSE,
                                "/agent:%s/monitor:%s",
                                ta, name);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_TAPI_CFG_CMD_MONITOR_H__ */
