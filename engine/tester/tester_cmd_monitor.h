/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Tester Subsystem
 *
 * Header file for tester command monitor feature
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TESTER_CMD_MONITOR_H__
#define __TE_TESTER_CMD_MONITOR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "te_queue.h"

/** Maximum length of command monitor object name */
#define TESTER_CMD_MONITOR_NAME_LEN 100

/** Command monitor type */
typedef enum cmd_monitor_type {
    TESTER_CMD_MONITOR_NONE = 0, /**< Dummy command monitor */
    TESTER_CMD_MONITOR_TA,       /**< TA based command monitor */
} cmd_monitor_type;

/** Command monitor description */
typedef struct cmd_monitor_descr {
    TAILQ_ENTRY(cmd_monitor_descr)   links;  /**< Queue links */

    cmd_monitor_type type; /**< Type of command monitor */

    char  name[TESTER_CMD_MONITOR_NAME_LEN]; /**< Object name */
    char *command;                           /**< Command to be
                                                  monitored */
    int   time_to_wait;                      /**< Time to wait before
                                                  executing command the
                                                  next time */
    bool enabled;                       /**< Whether command monitor
                                                  is enabled or not */
    bool run_monitor;                   /**< Whether we should run
                                                  this monitor or not */
    char     *ta;                            /**< Name of test agent on
                                                  which to run this
                                                  monitor */
} cmd_monitor_descr;

/** Head of a queue of cmd_monitor_descrs */
typedef TAILQ_HEAD(cmd_monitor_descrs, cmd_monitor_descr)
                                              cmd_monitor_descrs;

/**
 * Convert the type of the command monitor to readable string.
 *
 * @param type   Type of command monitor to convert.
 *
 * @return Type of command monitor as readable string.
 */
extern const char *cmd_monitor_type2str(cmd_monitor_type type);

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

/**
 * Set type of command monitor description.
 *
 * @param monitor   Monitor description structure pointer.
 * @param type      Type of command monitor to set.
 * @param reason    Reason to set this type.
 *
 * @return Status code.
 */
extern te_errno cmd_monitor_set_type(cmd_monitor_descr *monitor,
                                     cmd_monitor_type   type,
                                     const char        *reason);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TESTER_CMD_MONITOR_H__ */
