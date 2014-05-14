/** @file
 * @brief Tester Subsystem
 *
 * Header file for tester command monitore feature
 *
 *
 * Copyright (C) 2004-2014 Test Environment authors (see file AUTHORS
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
 * @author Konstantin Ushakov <Konstantin.Ushakov@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TESTER_CMD_MONITOR_H__
#define __TE_TESTER_CMD_MONITOR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "te_defs.h"
#include "te_queue.h"

/** Maximum length of command monitor object name */
#define TESTER_CMD_MONITOR_NAME_LEN 100

/** Command monitor description */
typedef struct cmd_monitor_descr {
    TAILQ_ENTRY(cmd_monitor_descr)   links;  /**< Queue links */

    char  name[TESTER_CMD_MONITOR_NAME_LEN]; /**< Object name */
    char *command;                           /**< Command to be
                                                  monitored */
    int   time_to_wait;                      /**< Time to wait before
                                                  executing command the
                                                  next time */
    te_bool   enabled;                       /**< Whether command monitor
                                                  is enabled or not */
    te_bool   run_monitor;                   /**< Whether we should run
                                                  this monitor or not */
    char     *ta;                            /**< Name of test agent on
                                                  which to run this
                                                  monitor */
} cmd_monitor_descr;

/** Head of a queue of cmd_monitor_descrs */
typedef TAILQ_HEAD(cmd_monitor_descrs, cmd_monitor_descr)
                                              cmd_monitor_descrs;
/**
 * Free memory occupied by command monitor description.
 *
 * @param monitor   Monitor description structure pointer
 */
extern void free_cmd_monitor(cmd_monitor_descr *monitor);

/**
 * Free memory occupied by command monitor descriptions.
 *
 * @param monitors   Queue of monitor descriptions
 */
extern void free_cmd_monitors(cmd_monitor_descrs *monitors);

/**
 * Start command monitors from the queue.
 *
 * @param monitors   Queue of monitor descriptions
 *
 * @return 0 on success, -1 on failure
 */
extern int start_cmd_monitors(cmd_monitor_descrs *monitors);

/**
 * Stop command monitors from the queue.
 *
 * @param monitors   Queue of monitor descriptions
 *
 * @return 0 on success, -1 on failure
 */
extern int stop_cmd_monitors(cmd_monitor_descrs *monitors);

extern int tester_monitor_id;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TESTER_CMD_MONITOR_H__ */
