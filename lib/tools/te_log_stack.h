/** @file
 * @brief Logger Stack API
 *
 * Definitions of the C API that allows to put logs into a stack of buffers.
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 */

#ifndef __TE_LOG_STACK_H__
#define __TE_LOG_STACK_H__

#include "logger_api.h"

/** @defgroup te_log_stack API: Logger messages stack
 *
 * In order to use @ref logger_stack you need to include te_log_stack.h
 *
 * Usage examples:
 * @code
 * #define TE_LGR_USER  "My module"
 *
 * #include "te_config.h"
 * ...
 * #include "te_log_stack.h"
 * ...
 * int main()
 * {
 *     te_log_stack_init();
 *
 *     ...
 *     te_log_stack_push(TE_LGR_USER, "An error condition happens: %s", result_string);
 *     CHECK_RC(FUNCTION_THAT_WILL_DO_PUSH);
 * @endcode
 *
 * And then if failure was unexpected you can call:
 *
 * @code
 * te_log_stack_dump(TE_LL_ERROR);
 * @endcode
 *
 * that will log full stack of errors and include them into your log file.
 *
 * The stack is per-thread and is not allocated/initialized unless you call
 * te_log_msg_stack_init() in the corresponding thread.
 *
 * One limitation is that you can't use '%r' specifier.
 *
 * @ingroup te_engine_logger
 * @{
 */

/*!
 * Initialize msg stack logic in the thread and set "top" point, i.e.
 * location that code-wise is the source of all calls.
 * te_log_stack_maybe_reset() calls will do nothing unless point at which
 * we're calling things is the top one.
 *
 * @param top       String identifier of the top point. See 'reset' handling.
 */
void te_log_stack_init_here(const char *top);

#define te_log_stack_init() te_log_stack_init_here(__FILE__)

/*!
 * Push a message under specified user
 *
 * @param user     Log user to be used, if NULL - internal one will be used
 * @param fmt      Format string ! %r is not supported
 */
void te_log_stack_push_under(const char *user, const char *fmt, ...);

/*!
 * Wrapper to used pre-defined TE_LGR_USER
 */
#define te_log_stack_push(_fs...) te_log_stack_push_under(TE_LGR_USER, _fs)

/*!
 * Pop message from the stack. Note, that you should free() the result once
 * done working with it.
 */
char *te_log_stack_pop(void);

/*!
 * Dump stack under given log level.
 *
 * @param log_level     Log level (TE_LL_ERROR etc.)
 */
void te_log_stack_dump(te_log_level log_level);

/*!
 * Reset/empty stack w/o logging things. Does not release any resources.
 *
 * @param here       Name of the point we're resetting things from.
 *                   Normally it's a file/function.
 */
void te_log_stack_maybe_reset(const char *here);

#define te_log_stack_reset() te_log_stack_maybe_reset(__FILE__)

/**@} <!-- END te_log_stack --> */

#endif  /* __TE_LOG_STACK_H__ */
