/** @file
 * @brief Test Environment: Raw log format specific functions.
 *
 * Declarations of raw log file format version specific routines.
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
 * @author  Oleg N. Kravtsov  <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_RGT_CORE_LOG_FORMAT_H__
#define __TE_RGT_CORE_LOG_FORMAT_H__

#include <stdio.h>
#include "log_msg.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Current version supported */
#define RGT_RLF_V1  (1)

/**
 * Type of function that is used for extracting log messages from
 * a raw log file. Such function is responsible for only raw-level
 * parsing and it doesn't generates the complete log string, so that
 * it shouldn't fill "txt_msg" field of "log_msg" structure.
 */
typedef int (* f_fetch_log_msg)(log_msg **msg, FILE *fd);

/**
 * Determines RLF version of the format and returns pointer to the function
 * that should be used for extracting log messages from a raw log file.
 *
 * @param  fd   Raw log file descriptor.
 * @param  err  Pointer to the pointer on string that can be set to
 *              an error message string if the function returns NULL.
 *
 * @return  pointer to the log message extracting function.
 *
 * @retval  !NULL   Pointer to the log message extracting function.
 * @retval  NULL    Unknown format of RLF.
 */
f_fetch_log_msg
rgt_define_rlf_format(FILE *fd, char **err);

/**
 * Extracts the next log message from a raw log file version 1.
 * The format of raw log file version 1 can be found in
 * OKT-HLD-0000095-TE_TS.
 *
 * @param  msg        Storage for log message to be extracted.
 * @param  fd         File descriptor of the raw log file.
 *
 * @return  Status of the operation.
 *
 * @retval  1   Message is successfuly read from Raw log file
 * @retval  0   There is no log messages left.
 *
 * @se
 *   If the structure of a log message doesn't comfim to the specification,
 *   this function never returns, but rather it throws an exception with
 *   longjmp call.
 */
int fetch_log_msg_v1(log_msg **msg, FILE *fd);

#ifdef __cplusplus
}
#endif

#endif /* __TE_RGT_CORE_LOG_FORMAT_H__ */
