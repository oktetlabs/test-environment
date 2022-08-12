/** @file
 * @brief Command monitor process defenition
 *
 * Defenition of command monitor function.
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 *
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
