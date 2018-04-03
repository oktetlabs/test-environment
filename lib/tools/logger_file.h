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
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
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

/**
 * Log message to the file stream @e te_log_message_file_out.
 *
 * This function complies with te_log_message_f prototype.
 */
extern void te_log_message_file(const char *file, unsigned int line,
                                te_log_ts_sec sec, te_log_ts_usec usec,
                                unsigned int level,
                                const char *entity, const char *user,
                                const char *fmt, va_list ap);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_LIB_LOGGER_FILE_H__ */
/**@} <!-- END te_tools_logger_file --> */
