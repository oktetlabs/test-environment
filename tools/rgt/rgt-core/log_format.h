/** @file
 * @brief Test Environment: Raw log format specific functions.
 *
 * Declarations of raw log file format version specific routines.
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 */

#ifndef __TE_RGT_CORE_LOG_FORMAT_H__
#define __TE_RGT_CORE_LOG_FORMAT_H__

#include <stdio.h>
#include "rgt_common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct log_msg;

/* Current version supported */
#define RGT_RLF_V1  (1)

/**
 * Determines RLF version of the format and returns pointer to the function
 * that should be used for extracting log messages from a raw log file.
 *
 * @param ctx  Rgt utility context.
 * @param err  Pointer to the pointer on string that can be set to
 *             an error message string if the function returns NULL.
 *
 * @return  pointer to the log message extracting function.
 *
 * @retval  !NULL   Pointer to the log message extracting function.
 * @retval  NULL    Unknown format of RLF.
 */
f_fetch_log_msg
rgt_define_rlf_format(rgt_gen_ctx_t *ctx, char **err);

/**
 * Extracts the next log message from a raw log file version 1.
 * The format of raw log file version 1 can be found in
 * OKTL-0000593.
 *
 * @param msg  Storage for log message to be extracted.
 * @param ctx  Rgt utility context.
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
int fetch_log_msg_v1(struct log_msg **msg, rgt_gen_ctx_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* __TE_RGT_CORE_LOG_FORMAT_H__ */
