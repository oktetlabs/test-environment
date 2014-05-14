/** @file
 * @brief Command monitor process defenition
 *
 * Defenition of command monitor function.
 *
 * Copyright (C) 2004-2014 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
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

#ifndef __TE_CMD_MONITOR_H__
#define __TE_CMD_MONITOR_H__

#include "te_defs.h"
#include "te_queue.h"
#include <pthread.h>

/** Structure defining command monitor */
typedef struct cmd_monitor_t {
    pthread_t thread;         /*< Monitoring thread */
    te_bool   enable;         /*< Whether monitoring thread
                                  is run or not */
    char     *name;           /*< Command monitor object name */
    char     *command;        /*< Command to be monitored */
    char     *time_to_wait;   /*< Time to wait between subsequent
                                  command calls */

    TAILQ_ENTRY(cmd_monitor_t)  links;  /*< Linking this structure
                                            in a queue */
} cmd_monitor_t;

/**
 * Launch the command monitor.
 *
 * @param arg   Pointer to cmd_monitor_t structure
 *
 * @return @c NULL
 */
extern void *te_command_monitor(void *arg);

#endif /* ndef __TE_CMD_MONITOR_H__ */
