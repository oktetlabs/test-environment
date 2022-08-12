/** @file
 * @brief Test Environment: Interface for filtering of log messages.
 *
 * The module is responsible for making descisions about message filtering.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 */


#ifndef __TE_RGT_FILTER_H__
#define __TE_RGT_FILTER_H__

#include "rgt_common.h"
#include "te_raw_log.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Possible filter modes */
enum node_fltr_mode {
    NFMODE_INCLUDE, /**< Log message should be included */
    NFMODE_EXCLUDE, /**< Log message should be rejected */
    NFMODE_DEFAULT, /**< Use some default mode for filtering */
};

/**
 * Initialize filter module.
 *
 * @param fltr_fname  Name of the XML filter file
 *
 * @return  Status of operation
 *
 * @retval  0  Success
 * @retval -1  Failure
 */
extern int rgt_filter_init(const char *fltr_fname);

/**
 * Destroys filter module.
 *
 * @return  Nothing
 */
extern void rgt_filter_destroy(void);

/**
 * Validates if log message with a particular tuple (entity name,
 * user name and timestamp) passes through user defined filter.
 * The function updates message flags.
 *
 * @param level      Log level
 * @param entity     Entity name
 * @param user_name  User name
 * @param timestamp  Timestamp
 * @param flags      Log message flags (OUT)
 *
 * @return  Returns filtering mode for the tuple.
 *          It never returns NFMODE_DEFAULT value.
 *
 * @retval  NFMODE_INCLUDE  the tuple is passed through the filter.
 * @retval  NFMODE_EXCLUDE  the tuple is rejected by the filter.
 */
extern enum node_fltr_mode rgt_filter_check_message(
                                const char *entity,
                                const char *user,
                                te_log_level level,
                                const uint32_t *timestamp,
                                uint32_t *flags);

/**
 * Verifies if the whole branch of execution flow should be excluded or
 * included from the log report.
 *
 * @param   path  Path (name) of the branch to be checked.
 *                Path is formed from names of packages and/or test
 *                of the execution flow separated by '/'. For example
 *                path "/a/b/c/d" means that execution flow is
 *                pkg "a" -> pkg "b" -> pkg "c" -> [test | pkg] "d"
 *
 * @return  Returns filtering mode for the branch.
 */
extern enum node_fltr_mode rgt_filter_check_branch(const char *path);

/**
 * Validates if the particular node (TEST, SESSION or PACKAGE) passes
 * through duration filter.
 *
 * @param node_type  Typo of the node ("TEST", "SESSION" or "PACKAGE")
 * @param start_ts   Start timestamp
 * @param end_ts     End timestamp
 *
 * @return Returns filtering mode for the node.
 *
 * @retval NFMODE_INCLUDE   the node is passed through the filter.
 * @retval NFMODE_EXCLUDE   the node is rejected by the filter.
 */
extern enum node_fltr_mode rgt_filter_check_duration(const char *node_type,
                                                     uint32_t *start_ts,
                                                     uint32_t *end_ts);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_RGT_FILTER_H__ */
