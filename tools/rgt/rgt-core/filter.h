/** @file
 * @brief Test Environment: Interface for filtering of log messages.
 *
 * The module is responsible for making descisions about message filtering.
 *
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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
 * @author Oleg N. Kravtsov  <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */


#ifndef __RGT_FILTER_H_
#define __RGT_FILTER_H_

#include "rgt_common.h"

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
 * @param fltr_file_name  Name of the TCL filter file
 *
 * @return  Status of operation
 *
 * @retval  0  Success
 * @retval -1  Failure
 */
extern int rgt_filter_init(const char *fltr_file_name);

/**
 * Destroys filter module.
 *
 * @return  Nothing
 */
extern void rgt_filter_destroy();

/**
 * Validates if log message with a particular tuple (entity name, 
 * user name and timestamp) passes through user defined filter.
 *
 * @param level      Log level
 * @param entity     Entity name
 * @param user_name  User name
 * @param timestamp  Timestamp
 *
 * @return  Returns filtering mode for the tuple.
 *          It never returns NFMODE_DEFAULT value.
 *
 * @retval  NFMODE_INCLUDE  the tuple is passed through the filter.
 * @retval  NFMODE_EXCLUDE  the tuple is rejected by the filter.
 */
extern enum node_fltr_mode rgt_filter_check_message(const char *level,
                                                    const char *entity,
                                                    const char *user,
                                                    const uint32_t
                                                        *timestamp);

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

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __RGT_FILTER_H_ */
