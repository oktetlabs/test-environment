/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Logger subsystem API
 *
 * @defgroup te_tools_logger_file Logger subsystem
 * @ingroup te_tools
 * @{
 *
 * Logger internal API to be used by standalone TE off-line applications.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_LIB_LOGGER_FILE_H__
#define __TE_LIB_LOGGER_FILE_H__

#if HAVE_STDIO_H
#include <stdio.h>
#endif

#include "logger_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/** File stream to write log messages. */
extern FILE *te_log_message_file_out;

/** Log message to the file stream @e te_log_message_file_out. */
extern te_log_message_f te_log_message_file;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_LIB_LOGGER_FILE_H__ */
/**@} <!-- END te_tools_logger_file --> */
