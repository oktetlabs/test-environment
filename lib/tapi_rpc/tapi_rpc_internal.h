/** @file
 * @brief Test API - RPC
 *
 * Internal definitions
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Oleg Sadakov <Oleg.Sadakov@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAPI_RPC_INTERNAL_H__
#define __TE_TAPI_RPC_INTERNAL_H__

/** Logger user */
#undef  TE_LGR_USER
#define TE_LGR_USER     "TAPI RPC"

#include "te_defs.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "rcf_rpc.h"
#include "logger_api.h"
#include "tapi_sockaddr.h"
#include "te_rpc_types.h"
#include "tapi_rpc_socket.h"

/*
 * It is mandatory to include signal.h before tarpc.h, since tarpc.h
 * defines _kill as a number.
 */
#if HAVE_SIGNAL_H
#include <signal.h>
#endif
#include "tarpc.h"
#include "tapi_jmp.h"
#include "tapi_test_run_status.h"


/** Extra time in seconds to be added to time2run before RPC timeout */
#define TAPI_RPC_TIMEOUT_EXTRA_SEC      10

/**
 * Log TAPI RPC call.
 * If RPC call status is OK, log as ring, else log as error.
 */
#define TAPI_RPC_LOG(rpcs, func, in_format, out_format, _x...) \
do {                                                                    \
    if (!rpcs->silent)                                                  \
    {                                                                   \
        if (RPC_IS_CALL_OK(rpcs))                                       \
        {                                                               \
            /*                                                          \
             * Preserve err_log set before and, may be, upgrade in the  \
             * case of retval independent error indications.            \
             */                                                         \
            if (RPC_ERRNO(rpcs) == RPC_ERPCNOTSUPP)                     \
            {                                                           \
                RING("Function %s() is not supported",  #func);         \
                if (rpcs->iut_err_jump)                                 \
                    rpcs->err_log = TRUE;                               \
            }                                                           \
            else if (rpcs->errno_change_check &&                        \
                     out.common.errno_changed)                          \
            {                                                           \
                ERROR("Function %s() returned correct value, but "      \
                      "changed errno to " RPC_ERROR_FMT, #func,         \
                      RPC_ERROR_ARGS(rpcs));                            \
                rpcs->_errno = TE_RC(TE_TAPI, TE_ECORRUPTED);           \
                if (rpcs->iut_err_jump)                                 \
                    rpcs->err_log = TRUE;                               \
            }                                                           \
        }                                                               \
        else                                                            \
        {                                                               \
            /* Log error regardless RPC error expectations */           \
            rpcs->err_log = TRUE;                                       \
        }                                                               \
        LOG_MSG(rpcs->err_log ? TE_LL_ERROR : TE_LL_RING,               \
                "RPC (%s,%s)%s%s: " #func "(" in_format ") -> "         \
                out_format " (%s)",                                     \
                rpcs->ta, rpcs->name, rpcop2str(rpcs->last_op),         \
                (rpcs->last_use_libc || rpcs->use_libc) ? " libc" : "", \
                _x, errno_rpc2str(RPC_ERRNO(rpcs)));                    \
        rpcs->err_log = FALSE;                                          \
    }                                                                   \
    rpcs->silent = rpcs->silent_default;                                \
} while (0)

/**
 * Free memory, check RPC error, jump in the case of RPC error or if
 * @a _res is @c TRUE, set jump condition to default value.
 */
#define TAPI_RPC_OUT(_func, _res) \
    do {                                                                \
        if (rpcs != NULL)                                               \
        {                                                               \
            rcf_rpc_free_result(&out,                                   \
                                (xdrproc_t)xdr_tarpc_##_func##_out);    \
            if (!RPC_IS_CALL_OK(rpcs))                                  \
            {                                                           \
                if (rpcs->err_jump)                                     \
                {                                                       \
                    rpcs->iut_err_jump = TRUE;                          \
                    TAPI_JMP_DO(TE_EFAIL);                              \
                }                                                       \
            }                                                           \
            else if ((_res) && rpcs->iut_err_jump)                      \
            {                                                           \
                TAPI_JMP_DO(TE_EFAIL);                                  \
            }                                                           \
            else if (tapi_test_run_status_get() !=                      \
                                            TE_TEST_RUN_STATUS_OK)      \
            {                                                           \
                if (!tapi_jmp_stack_is_empty())                         \
                {                                                       \
                    ERROR("Jumping because a test execution error "     \
                          "occured earlier");                           \
                    TAPI_JMP_DO(TE_EFAIL);                              \
                }                                                       \
            }                                                           \
            rpcs->iut_err_jump = TRUE;                                  \
            rpcs->err_jump = TRUE;                                      \
        }                                                               \
        else                                                            \
        {                                                               \
            /* Try to jump */                                           \
            TAPI_JMP_DO(TE_EFAIL);                                      \
        }                                                               \
    } while (0)

/**
 * If RPC call status is OK, check condition @a _cond and set specified
 * variable @a _var to @a _error_val and RPC server errno to #TE_ECORRUPTED,
 * if it is true.
 * If RPC call status is not OK, variable @a _var is set to @a _error_val
 * and RPC server errno is not updated.
 *
 * Error logging is requested if error is not expected and @a _err_cond
 * condition is @c TRUE.
 *
 * The function assumes to have RPC server handle as @b rpcs variable in
 * the context.
 *
 * @param _func         function name
 * @param _var          variable with return value
 * @param _cond         condition to be checked, if RPC call status is OK
 * @param _error_val    value to be assigned in the case of error
 * @param _err_cond     error logging condition, if RPC call status is OK
 */
#define CHECK_RETVAL_VAR_ERR_COND(_func, _var, _cond, _error_val, _err_cond) \
    do {                                                            \
        if (!RPC_IS_CALL_OK(rpcs))                                  \
        {                                                           \
            (_var) = (_error_val);                                  \
        }                                                           \
        else if (RPC_ERRNO(rpcs) == RPC_ERPCNOTSUPP)                \
        {                                                           \
            (_var) = (_error_val);                                  \
        }                                                           \
        else                                                        \
        {                                                           \
            if (_cond)                                              \
            {                                                       \
                ERROR("Function %s() returned incorrect value %lu", \
                      #_func, (unsigned long)(_var));               \
                rpcs->_errno = TE_RC(TE_TAPI, TE_ECORRUPTED);       \
                (_var) = (_error_val);                              \
            }                                                       \
            else if (rpcs->errno_change_check &&                    \
                     out.common.errno_changed)                      \
            {                                                       \
                if (_err_cond)                                      \
                    /* errno change is expected */                  \
                    out.common.errno_changed = FALSE;               \
                else                                                \
                    (_var) = (_error_val);                          \
            }                                                       \
            /*                                                      \
             * Recalculate _err_cond to pick up _var changes made   \
             * above.                                               \
             */                                                     \
            if (rpcs->iut_err_jump && (_err_cond))                  \
                rpcs->err_log = TRUE;                               \
        }                                                           \
    } while (0)

/**
 * If RPC call status is OK, check condition @a _cond and set specified
 * variable @a _var to @a _error_val and RPC server errno to #TE_ECORRUPTED,
 * if it is true.
 * If RPC call status is not OK, variable @a _var is set to @a _error_val
 * and RPC server errno is not updated.
 *
 * Error logging is requested if error is not expected and finally @a _var
 * is equal to @a __error_val.
 *
 * The function assumes to have RPC server handle as @b rpcs variable in
 * the context.
 *
 * @param _func         function name
 * @param _var          variable with return value
 * @param _cond         condition to be checked, if RPC call status is OK
 * @param _error_val    value to be assigned in the case of error
 */
#define CHECK_RETVAL_VAR(_func, _var, _cond, _error_val) \
    CHECK_RETVAL_VAR_ERR_COND(_func, _var, _cond, _error_val,   \
                              ((_var) == (_error_val)))

/**
 * If RPC call status is OK, check that variable @a _var with function
 * return value is greater or equal to @c -1 and set specified variable
 * to @c -1 and RPC server errno to #TE_ECORRUPTED, if it is not true.
 * If RPC call status is not OK, variable @a _var is set to @c -1 and
 * RPC server errno is not updated.
 *
 * Error logging is requested if error is not expected and finally @a _var
 * is equal to @c -1.
 *
 * The function assumes to have RPC server handle as @b rpcs variable in
 * the context.
 *
 * @param _func     function name
 * @param _var      variable with return value
 */
#define CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(_func, _var) \
    CHECK_RETVAL_VAR(_func, _var, ((_var) < -1), -1)

/**
 * If RPC call status is OK, check that variable @a _var with function
 * return value is @c 0 or @c -1 and set specified variable to @c -1 and
 * RPC server errno to #TE_ECORRUPTED, if it is not true.
 * If RPC call status is not OK, variable @a _var is set to @c -1 and
 * RPC server errno is not updated.
 *
 * Error logging is requested if error is not expected and finally @a _var
 * is equal to @c -1.
 *
 * The function assumes to have RPC server handle as @b rpcs variable in
 * the context.
 *
 * @param _func     function
 * @param _var      variable with return value
 */
#define CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(_func, _var) \
    CHECK_RETVAL_VAR(_func, _var, (((_var) != 0) && ((_var) != -1)), -1)

/**
 * If RPC call status is OK, check that variable @a _var with function
 * return value is @c 0 or negative and set specified variable to @c -1 and
 * RPC server errno to #TE_ECORRUPTED, if it is not true.
 * If RPC call status is not OK, variable @a _var is set to @c -1 and
 * RPC server errno is not updated.
 *
 * Error logging is requested if error is not expected and finally @a _var
 * is negative.
 *
 * The function assumes to have RPC server handle as @b rpcs variable in
 * the context.
 *
 * @param _func     function
 * @param _var      variable with return value
 */
#define CHECK_RETVAL_VAR_IS_ZERO_OR_NEGATIVE(_func, _var) \
    CHECK_RETVAL_VAR_ERR_COND(_func, _var, ((_var) > 0), -1, ((_var) < 0))

/**
 * If RPC call status is OK, check that variable @a _var with function
 * return value is @c 0 or negative errno and set specified variable
 * to @c -TE_RC(TE_TAPI, TE_ECORRUPTED) and RPC server errno to
 * #TE_ECORRUPTED, if it is not true.
 * If RPC call status is not OK, variable @a _var is set to
 * @c -TE_RC(TE_TAPI, TE_ECORRUPTED) and RPC server errno is not updated.
 *
 * Error logging is requested if error is not expected and finally @a _var
 * is negative.
 *
 * The function assumes to have RPC server handle as @b rpcs variable in
 * the context.
 *
 * @param _func     function
 * @param _var      variable with return value
 */
#define CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(_func, _var) \
    CHECK_RETVAL_VAR_ERR_COND(_func, _var, ((_var) > 0), \
                              -TE_RC(TE_TAPI, TE_ECORRUPTED), ((_var) < 0))

/**
 * If RPC call status is OK, check that variable @a _var with function
 * return value is @c TRUE or @c FALSE and set specified variable to
 * @c FALSE and RPC server errno to #TE_ECORRUPTED, if it is not true.
 * If RPC call status is not OK, variable @a _var is set to @c FALSE and
 * RPC server errno is not updated.
 *
 * Error logging is requested if error is not expected and finally @a _var
 * is equal to @c FALSE.
 *
 * The function assumes to have RPC server handle as @b rpcs variable in
 * the context.
 *
 * @param _func     function
 * @param _var      variable with return value
 */
#define CHECK_RETVAL_VAR_IS_BOOL(_func, _var) \
    CHECK_RETVAL_VAR(_func, _var, (((_var) != FALSE) && ((_var) != TRUE)), \
                     FALSE)

/**
 * Auxiliary check with false condition required for RPC logging with
 * a correct log level in case when return value is an RPC pointer;
 * it should be used in functions which @a normally don't return @c NULL
 *
 * @param _func     function
 * @param _var      variable with return value
 */
#define CHECK_RETVAL_VAR_RPC_PTR(_func, _var) \
    CHECK_RETVAL_VAR(_func, _var, ((_var) == RPC_UNKNOWN_ADDR), RPC_NULL)


/** Return with check (for functions returning zero value) */
#define RETVAL_ZERO_INT(_func, _retval) \
    do {                                                            \
        int __retval = (_retval);                                   \
                                                                    \
        TAPI_RPC_OUT(_func, (__retval) != 0);                       \
                                                                    \
        return __retval;                                            \
    } while (0)

/** Return with check (for functions returning boolean value) */
#define RETVAL_BOOL(_func, _retval) \
    do {                                  \
        te_bool __retval = _retval;       \
                                          \
        TAPI_RPC_OUT(_func, !(__retval)); \
                                          \
        return __retval;                  \
    } while (0)


/** Return with check (for functions returning value >= @c -1) */
#define RETVAL_INT(_func, _retval) \
    do {                                                            \
        int __retval = (_retval);                                   \
                                                                    \
        TAPI_RPC_OUT(_func, (__retval) < 0);                        \
                                                                    \
        return __retval;                                            \
    } while (0)

/** Return with check (for functions returning int64_t value >= @c -1) */
#define RETVAL_INT64(_func, _retval) \
    do {                                                            \
        int64_t __retval = (_retval);                               \
                                                                    \
        TAPI_RPC_OUT(_func, (__retval) < 0);                        \
                                                                    \
        return __retval;                                            \
    } while (0)

/** Return with check (for functions returning pointers) */
#define RETVAL_PTR(_func, _retval) \
    do {                                                            \
        void *__retval = (void *)(_retval);                         \
                                                                    \
        TAPI_RPC_OUT(_func, __retval == NULL);                      \
                                                                    \
        return __retval;                                            \
    } while (0)

/** Return with check (for functions returning pointers) */
#define RETVAL_PTR64(_func, _retval) \
    do {                                                            \
        int64_t __retval = (int64_t)(_retval);                      \
                                                                    \
        TAPI_RPC_OUT(_func, __retval == 0);                         \
                                                                    \
        return __retval;                                            \
    } while (0)

/** Return with check (for functions returning pointers) */
#define RETVAL_RPC_PTR(_func, _retval) \
    do {                                                            \
        rpc_ptr __retval = (_retval);                               \
                                                                    \
        TAPI_RPC_OUT(_func, __retval == RPC_NULL);                  \
                                                                    \
        return __retval;                                            \
    } while (0)

/** Return with check (for void functions) */
#define RETVAL_VOID(_func) \
    do {                                                            \
        TAPI_RPC_OUT(_func, FALSE);                                 \
        return;                                                     \
    } while(0)

/**
 * Return wait_status with check.
 */
#define RETVAL_WAIT_STATUS(_func, _retval) \
    do {                                                            \
        rpc_wait_status __retval = (_retval);                       \
                                                                    \
        TAPI_RPC_OUT(_func,                                         \
                     __retval.flag != RPC_WAIT_STATUS_EXITED ||     \
                     __retval.value != 0);                          \
                                                                    \
        return __retval;                                            \
    } while(0)

/**
 * Return int value and check it and wait_status.
 */
#define RETVAL_INT_CHECK_WAIT_STATUS(_func, _retval, _status) \
    do {                                                            \
        int __retval = (_retval);                                   \
                                                                    \
        TAPI_RPC_OUT(_func, __retval < 0 ||                         \
                     _status.flag != RPC_WAIT_STATUS_EXITED ||      \
                     _status.value != 0);                           \
                                                                    \
        return __retval;                                            \
    } while(0)

/**
 * Return with check (to be used in functions where @c NULL is not
 * considered as an error value)
 */
#define RETVAL_RPC_PTR_OR_NULL(_func, _retval) \
    do {                                                            \
        rpc_ptr __retval = (_retval);                               \
                                                                    \
        TAPI_RPC_OUT(_func, __retval == RPC_UNKNOWN_ADDR);          \
                                                                    \
        return __retval;                                            \
    } while (0)

/** Follow pointer if not @c NULL; otherwise return @c 0 */
#define PTR_VAL(_param) ((_param == NULL) ? 0 : *(_param))

/**
 * Generic format string of @b rpc_ptr pointers
 *
 * @note It is used with @b RPC_PTR_VAL.
 */
#define RPC_PTR_FMT "%s(%#x)"

/**
 * Description of the pointer @p _val fields according to the generic format
 * string
 *
 * @param _val      pointer value
 *
 * @note It is used with @b RPC_PTR_FMT.
 */
#define RPC_PTR_VAL(_val)                                   \
    tapi_rpc_namespace_get(rpcs, (_val)), (unsigned)(_val)

/**
 * Wrapper for @b tapi_rpc_namespace_check() with details for error messages
 */
#define TAPI_RPC_NAMESPACE_CHECK(_rpcs, _ptr, _ns)      \
    tapi_rpc_namespace_check(                           \
        (_rpcs), (_ptr), (_ns), __FUNCTION__, __LINE__)

/**
 * Wrapper for @b tapi_rpc_namespace_check() - jumps to @b cleanup in case
 * of RPC pointer namespace check fail.
 */
#define TAPI_RPC_NAMESPACE_CHECK_JUMP(_rpcs, _ptr, _ns) \
do {                                                    \
    if (TAPI_RPC_NAMESPACE_CHECK(_rpcs, _ptr, _ns))     \
        TAPI_JMP_DO(TE_EFAIL);                          \
} while (0)

/**
 * Check membership of pointer in the namespace @a ns.
 *
 * @param rpcs      RPC server handle
 * @param ptr       Pointer ID
 * @param ns        Namespace as string
 * @param function  Name of function (for more detailed error messages)
 * @param line      Line in file (for more detailed error messages)
 *
 * @return          Status code
 */
extern te_errno tapi_rpc_namespace_check(
        rcf_rpc_server *rpcs, rpc_ptr ptr, const char *ns,
        const char *function, int line);

/**
 * Get namespace as string according to a pointer id.
 *
 * @param rpcs      RPC server handle
 * @param ptr       Pointer ID
 *
 * @return          Namespace as string or @c NULL for invalid namespace
 */
extern const char *tapi_rpc_namespace_get(
        rcf_rpc_server *rpcs, rpc_ptr ptr);

/**
 * Initialize and check @b rpc_msghdr.msg_flags value in RPC only if the
 * variable is @c TRUE.
 */
extern te_bool rpc_msghdr_msg_flags_init_check_enabled;

/**
 * Initialize @b msg_flags by a random value.
 *
 * @param _msg      Message to check @b msg_flags_mode if the initialization
 *                  is requested
 * @param _msg_set  Message where the new value should be assigned
 */
#define MSGHDR_MSG_FLAGS_INIT(_msg, _msg_set) \
    do {                                                                \
        if (rpc_msghdr_msg_flags_init_check_enabled &&                  \
            ((_msg)->msg_flags_mode & RPC_MSG_FLAGS_NO_SET) == 0)       \
        {                                                               \
            (_msg_set)->msg_flags = tapi_send_recv_flags_rand();        \
            (_msg_set)->in_msg_flags = (_msg_set)->msg_flags;           \
        }                                                               \
    } while (0)

/**
 * Transform value of rpc_msghdr type to value of tarpc_msghdr type.
 *
 * @note This function does not copy data, so do not release
 *       pointers to data buffers in iov_base in cleanup.
 *
 * @param rpc_msg           Pointer to value of type rpc_msghdr.
 * @param tarpc_msg         Where value of type tarpc_msghdr should be
 *                          saved (call @b tarpc_msghdr_free() to
 *                          free memory after this function, even if
 *                          it failed).
 * @param recv_call         Set to @c TRUE if conversion is done for
 *                          receive function call.
 *
 * @return Status code.
 */
extern te_errno msghdr_rpc2tarpc(const rpc_msghdr *rpc_msg,
                                 tarpc_msghdr *tarpc_msg,
                                 te_bool recv_call);

/**
 * Release memory allocated by @b msghdr_rpc2tarpc() for converted value.
 *
 * @param msg       Pointer to tarpc_msghdr value filled by
 *                  @b msghdr_rpc2tarpc().
 */
extern void tarpc_msghdr_free(tarpc_msghdr *msg);

/**
 * Transform value of tarpc_msghdr type to value of rpc_msghdr type.
 *
 * @param tarpc_msg         Pointer to value of type tarpc_msghdr.
 * @param rpc_msg           Where value of type rpc_msghdr should be
 *                          saved.
 *
 * @return Status code.
 */
extern te_errno msghdr_tarpc2rpc(const tarpc_msghdr *tarpc_msg,
                                 rpc_msghdr *rpc_msg);

/**
 * Convert array of rpc_mmsghdr structures to array of
 * tarpc_mmsghdr structures.
 *
 * @note @b tarpc_mmsghdrs_free() should be used to release memory
 *       after successful call of this function.
 *
 * @param rpc_mmsgs       Pointer to array of rpc_mmsghdr structures.
 * @param num             Number of elements in @p rpc_mmsgs.
 * @param tarpc_mmsgs     Where to save a pointer to array of converted
 *                        values.
 * @param recv_call       If @c TRUE, conversion is done for receive call.
 *
 * @return Status code.
 *
 * @sa mmsghdrs_tarpc2rpc, tarpc_mmsghdrs_free
 */
extern te_errno mmsghdrs_rpc2tarpc(const struct rpc_mmsghdr *rpc_mmsgs,
                                   unsigned int num,
                                   tarpc_mmsghdr **tarpc_mmsgs,
                                   te_bool recv_call);

/**
 * Release memory allocated by @b mmsghdrs_rpc2tarpc() for converted values.
 *
 * @param taprc_mmsgs       Pointer to array of tarpc_mmsghdr structures.
 * @param num               Number of elements in the array.
 *
 * @return Status code.
 */
extern void tarpc_mmsghdrs_free(tarpc_mmsghdr *tarpc_mmsgs,
                                unsigned int num);

/**
 * Convert array of tarpc_mmsghdr structures back to array of
 * rpc_mmsghdr structures after RPC call.
 *
 * @param tarpc_mmsgs     Pointer to array of tarpc_mmsghdr structures.
 * @param rpc_mmsgs       Pointer to array of rpc_mmsghdr structures.
 * @param num             Number of elements in arrays.
 *
 * @return Status code.
 *
 * @sa mmsghdrs_rpc2tarpc
 */
extern te_errno mmsghdrs_tarpc2rpc(const tarpc_mmsghdr *tarpc_mmsgs,
                                   struct rpc_mmsghdr *rpc_mmsgs,
                                   unsigned int num);

#endif /* !__TE_TAPI_RPC_INTERNAL_H__ */
