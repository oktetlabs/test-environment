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
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Igor B. Vasiliev <Igor.Vasiliev@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_LOGGER_TA_H__
#define __TE_LOGGER_TA_H__

#include "te_stdint.h"
#include "te_errno.h"


#ifdef _cplusplus
extern "C" {
#endif

/**
 * Initialize Logger resources on the Test Agent side (log buffer,
 * log file and so on).
 *
 * @return Status code (see te_errno.h)
 */
extern te_errno ta_log_init(void);

/**
 * Finish Logger activity on the Test Agent side (fluhes buffers
 * in the file if that means exists and so on).
 *
 * @return Status code (see te_errno.h)
 */
extern te_errno ta_log_shutdown(void);

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
