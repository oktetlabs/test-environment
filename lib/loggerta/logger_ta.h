/** @file
 * @brief Logger subsystem API - TA side
 *
 * API provided by Logger subsystem to users on TA side.
 * Set of macros provides compete functionality for debugging
 * and tracing and highly recommended for using in the source
 * code.
 *
 * There are facilities for both fast and slow  logging.
 * Fast logging(LOGF_... macros) does not copy
 * bytes arrays and strings into local log buffer. Only start
 * memory address and length of dumped memory are regestered.
 * So, user has to take care about validity of logged data
 * (logged data should not be volatile).
 * Slow logging (LOGS_... macros) parses log message format string
 * and copy dumped memory and strings into local log buffer and user
 * has not to take care about validity of logged data.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_LOGGER_TA_H__
#define __TE_LOGGER_TA_H__

#include "te_stdint.h"
#include "te_errno.h"


#ifdef _cplusplus
extern "C" {
#endif

/** Logging backend for processed forked from Test Agents */
extern te_log_message_f logfork_log_message;

/**
 * Initialize Logger resources on the Test Agent side (log buffer,
 * log file and so on).
 *
 * @param lgr_entity    Logger entity name to use
 *
 * @return Status code (see te_errno.h)
 */
extern te_errno ta_log_init(const char *lgr_entity);

/**
 * Finish Logger activity on the Test Agent side (fluhes buffers
 * in the file if that means exists and so on).
 *
 * @return Status code (see te_errno.h)
 */
extern te_errno ta_log_shutdown(void);

/**
 * Register a log message in the raw log with log user stored as an argument
 * (slow mode).
 *
 * Log user string is usually stored in the raw log as a pointer to a
 * string in static memory region to avoid copying/memory allocation.
 * But in some cases log user has to be dynamic, and the function copies
 * the log user into raw log argument to support that.
 *
 * @param sec   Timestamp seconds
 * @param usec  Timestamp microseconds
 * @param level Log level
 * @param user  Pointer to log user (copied into raw log)
 * @param msg   Pointer to the log message.
 */
extern void ta_log_dynamic_user_ts(te_log_ts_sec sec, te_log_ts_usec usec,
                                   unsigned int level, const char *user,
                                   const char *msg);

/**
 * Request the log messages accumulated in the Test Agent local log
 * buffer. Passed messages will be deleted from local log.
 *
 * @param buf_length    Length of the transfer buffer.
 * @param transfer_buf  Pointer to the transfer buffer.
 *
 * @retval Length of the filled part of the transfer buffer in bytes
 */
extern uint32_t ta_log_get(uint32_t buf_length, uint8_t *transfer_buf);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */
#endif /* __TE_LOGGER_TA_H__ */
