/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test API to report verdicts and control a test execution flow
 *
 * @defgroup te_ts_tapi_test_log Test execution flow
 * @ingroup te_ts_tapi_test
 * @{
 *
 * Macros to be used in tests. The header must be included from test
 * sources only. It is allowed to use the macros only from @b main()
 * function of the test.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAPI_TEST_LOG_H__
#define __TE_TAPI_TEST_LOG_H__

#include "te_errno.h"
#include "logger_api.h"
#include "te_log_stack.h"
#include "tapi_jmp.h"
#include "tapi_test_behaviour.h"
#include "tester_msg.h"


#ifdef __cplusplus
extern "C" {
#endif

/** @group Logs nesting controls */

/**
 * Logging of nesting level step.
 *
 * Reset nesting level to 0, log message with zero nesting level and increment
 * it for subsequent messages (level equal to 1).
 * The macro should be used in the test main function only.
 * The macro is used to extract the test description (generated using Doxygen).
 *
 * @param _fs - format string and arguments
 */
#define TEST_STEP(_fs...) \
    do {                                                            \
        LGR_MESSAGE(TE_LL_CONTROL | TE_LL_RING, TE_USER_STEP, _fs); \
        te_test_fail_state_update(_fs);                             \
        te_test_fail_substate_update(NULL);                         \
    } while (0)

/**
 * Logging of nesting level sub-step.
 *
 * Reset nesting level to 1, log message with the nesting level and increment
 * it for subsequent messages (level equal to 2).
 * The macro should be used in the test main function only.
 * The macro is used to extract the test description (generated using Doxygen).
 *
 * @param _fs - format string and arguments
 */
#define TEST_SUBSTEP(_fs...) \
    do {                                                                \
        LGR_MESSAGE(TE_LL_CONTROL | TE_LL_RING, TE_USER_SUBSTEP, _fs);  \
        te_test_fail_substate_update(_fs);                              \
    } while(0)                                                          \


/**
 * Logging of nesting level step push.
 *
 * Log message at current nesting level and increment nesting level for
 * subsequent log messages.
 * Subsequent messages will have greater nesting level and will be considered
 * as details of the step implementation.
 *
 * @param _fs - format string and arguments
 */
#define TEST_STEP_PUSH(_fs...) \
    LGR_MESSAGE(TE_LL_CONTROL | TE_LL_RING, TE_USER_STEP_PUSH, _fs)

/**
 * Logging of nesting level step pop.
 *
 * Decrement log nesting level and log the message (if not empty).
 * It wraps the block and message could be used to summarize results.
 *
 * @param _fs - format string and arguments
 */
#define TEST_STEP_POP(_fs...) \
    LGR_MESSAGE(TE_LL_CONTROL | TE_LL_RING, TE_USER_STEP_POP, _fs)

/**
 * Logging of nesting level step next.
 *
 * Keep current nesting level, but log the message with previous nesting level.
 * So, the message will have the same nesting level as previous step-push and
 * subsequent step-next messages.
 *
 * @param _fs - format string and arguments
 */
#define TEST_STEP_NEXT(_fs...) \
    LGR_MESSAGE(TE_LL_CONTROL | TE_LL_RING, TE_USER_STEP_NEXT, _fs)

/**
 * Logging of nesting level step reset
 *
 * Reset nesting level to 0
 */
#define TEST_STEP_RESET() \
    LGR_MESSAGE(TE_LL_CONTROL | TE_LL_RING, TE_USER_STEP_RESET, "")

/*@}*/

/** @addtogroup te_ts_tapi_test
 * @{
 */

/**
 * Terminate a test with success status.
 */
#define TEST_SUCCESS    (TAPI_JMP_DO(0))

/**
 * Terminate a test with failure status. It is assumed that error is
 * already reported.
 */
#define TEST_STOP       (TAPI_JMP_DO(TE_EFAIL))

/**
 * Terminate a test with skip status, optionally reporting the reason as
 * a verdict.
 */
#define TEST_SKIP(...) \
    do {                                                \
        RING("Test Skipped in %s, line %d, %s()",       \
             __FILE__, __LINE__, __FUNCTION__);         \
        if (TE_VA_HEAD(__VA_ARGS__)[0] != '\0')         \
            RING_VERDICT(__VA_ARGS__);                  \
        TAPI_JMP_DO(TE_ESKIP);                          \
    } while (0)

/**
 * Terminate a test with failure status, report an error.
 *
 * @param fmt       error message format string with parameters
 */
#define TEST_FAIL(fmt...) \
    do {                                                            \
        ERROR("Test Failed in %s, line %d, %s()",                   \
              __FILE__, __LINE__, __FUNCTION__);                    \
        te_log_stack_dump(TE_LL_ERROR);                             \
        if (TEST_BEHAVIOUR(fail_verdict))                           \
            ERROR_VERDICT(fmt);                                     \
        else                                                        \
            ERROR(fmt);                                             \
        TEST_STOP;                                                  \
    } while (0)

/**
 * Set test termination status to failure, report an error.
 * Should be used instead of TEST_FAIL in the cleanup section.
 *
 * @param fmt       error message format string with parameters
 */
#define CLEANUP_TEST_FAIL(fmt...) \
    do {                                                            \
        ERROR("Test Failed in %s, line %d, %s()",                   \
              __FILE__, __LINE__, __FUNCTION__);                    \
        if (TEST_BEHAVIOUR(fail_verdict))                           \
            ERROR_VERDICT(fmt);                                     \
        else                                                        \
            ERROR(fmt);                                             \
        result = EXIT_FAILURE;                                      \
    } while (0)

/**@} <!-- END te_ts_tapi_test --> */

/**
 * A string used to identify per-iteration objectives,
 * generated by test control messages
 */
#define TE_TEST_OBJECTIVE_ID "<<OBJECTIVE>>"


/**
 * The macro should be used to output per-iteration test objectives
 *
 * @param fmt   Format string for the following arguments
 */
#define TEST_OBJECTIVE(fmt...) \
    TE_LOG_RING(TE_LOG_CMSG_USER, TE_TEST_OBJECTIVE_ID fmt)

/**
 * Macro should be used to output verdict from tests.
 *
 * @param fmt  the content of the verdict as format string with arguments
 */
#define RING_VERDICT(fmt...) \
    do {                                                                   \
        LGR_MESSAGE(TE_LL_RING | TE_LL_CONTROL, TE_LOG_VERDICT_USER, fmt); \
        te_test_tester_message(TE_TEST_MSG_VERDICT, fmt);                  \
    } while (0)

/**
 * Macro should be used to output verdict with WARN log level from tests.
 *
 * @param fmt  the content of the verdict as format string with arguments
 */
#define WARN_VERDICT(fmt...) \
    do {                                                                   \
        LGR_MESSAGE(TE_LL_WARN | TE_LL_CONTROL, TE_LOG_VERDICT_USER, fmt); \
        te_test_tester_message(TE_TEST_MSG_VERDICT, fmt);                  \
    } while (0)

/**
 * Macro should be used to output verdict with ERROR log level from tests.
 *
 * @param fmt  the content of the verdict as format string with arguments
 */
#define ERROR_VERDICT(fmt...) \
    do {                                                                   \
        LGR_MESSAGE(TE_LL_ERROR | TE_LL_CONTROL, TE_LOG_VERDICT_USER,      \
                    fmt);                                                  \
        te_test_tester_message(TE_TEST_MSG_VERDICT, fmt);                  \
    } while (0)

/**
 * Terminate a test with failure status, report an error as verdict.
 *
 * @param fmt       error message format string with parameters
 */
#define TEST_VERDICT(fmt...) \
    do {                        \
        ERROR_VERDICT(fmt);     \
        TEST_STOP;              \
    } while (0)


/**
 * Print test artifact to log. Artifact is a string describing
 * test results like a verdict. But it is not taken into account
 * when matching obtained results to TRC database.
 *
 * @param _level     Level of the message/artifact that will go into the log
 * @param _fmt       Message describing the artifact (format string
 *                   with parameters)
 */
#define REGISTER_ARTIFACT(_level, _fmt...) \
    do {                                   \
        LGR_MESSAGE((_level) | TE_LL_CONTROL, TE_LOG_ARTIFACT_USER, _fmt); \
        te_test_tester_message(TE_TEST_MSG_ARTIFACT, _fmt);                \
    } while(0)

/**
 * Macro should be used to output artifact from tests.
 *
 * @param fmt  the content of the artifact as format string with arguments
 */
#define RING_ARTIFACT(_fmt...) \
    REGISTER_ARTIFACT(TE_LL_RING, _fmt)

/**
 * Macro should be used to output artifact with WARN log level from tests.
 *
 * @param fmt  the content of the artifact as format string with arguments
 */
#define WARN_ARTIFACT(_fmt...) \
    REGISTER_ARTIFACT(TE_LL_WARN, _fmt)

/**
 * Macro should be used to output artifact with ERROR log level from tests.
 *
 * @param fmt  the content of the artifact as format string with arguments
 */
#define ERROR_ARTIFACT(_fmt...) \
    REGISTER_ARTIFACT(TE_LL_ERROR, _fmt)

/**
 * Macro should be used to output artifact with MI log level from tests.
 *
 * @param fmt  the content of the artifact as format string with arguments
 */
#define MI_ARTIFACT(_fmt...) \
    REGISTER_ARTIFACT(TE_LL_MI, _fmt)

/**
 * Print test artifact to log. Artifact is a string describing
 * test results like a verdict. But it is not taken into account
 * when matching obtained results to TRC database.
 *
 * @param _fmt       Message describing the artifact (format string
 *                   with parameters)
 */
#define TEST_ARTIFACT(_fmt...) \
    RING_ARTIFACT(_fmt)

/**
 * Compose test message and send it to Tester.
 *
 * @param type          Message type
 * @param fmt           printf()-like format string with TE extensions
 *
 * @note The function uses @e te_test_id global variable.
 */
extern void te_test_tester_message(te_test_msg_type type,
                                   const char *fmt, ...);

/**
 * Update state of the test to be dumped in case of failure.
 *
 * @param fmt     printf()-like format string w/o! TE extensions
 */
extern void te_test_fail_state_update(const char *fmt, ...)
    __attribute__((format(printf, 1, 2)));

/**
 * Update substate of the test to be dumped in case of failure.
 *
 * @param fmt     printf()-like format string w/o! TE extensions
 */
extern void te_test_fail_substate_update(const char *fmt, ...)
    __attribute__((format(printf, 1, 2)));

/**
 * Get the current test state string or @c NULL if it's not filled in.
 */
extern const char *te_test_fail_state_get(void);

/**
 * Get the current test substate string or @c NULL if it's not filled in.
 */
extern const char *te_test_fail_substate_get(void);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_TEST_LOG_H__ */

/**@} <!-- END te_ts_tapi_test_log --> */
