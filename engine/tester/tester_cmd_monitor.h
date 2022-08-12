/** @file
 * @brief Tester Subsystem
 *
 * Header file for tester command monitore feature
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 *
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
