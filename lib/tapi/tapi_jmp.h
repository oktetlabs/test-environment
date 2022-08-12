/** @file
 * @brief Test API to use setjmp/longjmp.
 *
 * @defgroup ts_tapi_jmp Stack of jumps
 * @ingroup te_ts_tapi
 * @{
 *
 * Definition of API to deal with thread-safe stack of jumps.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 */

#ifndef __TE_TAPI_JMP_H__
#define __TE_TAPI_JMP_H__

#if HAVE_SETJMP_H
#include <setjmp.h>
#else
#error Required setjmp.h not found
#endif

#include "te_defs.h"
#include "te_queue.h"


#ifdef __cplusplus
extern "C" {
#endif

/** Jump point (stack context to jump to) */
typedef struct tapi_jmp_point {
    SLIST_ENTRY(tapi_jmp_point) links;      /**< List links */

    jmp_buf         env;        /**< Stack context, if jumps are
                                     enabled */
    const char     *file;       /**< Name of the file where the point
                                     is added */
    unsigned int    lineno;     /**< Line number in the file where the
                                     point is added */
} tapi_jmp_point;


/**
 * Create jump point with actions to be done in the case of jump.
 *
 * @param x         Actions to be done in the case of long jump
 *                  (it is assumed that it does return, goto or exit)
 */
#define TAPI_ON_JMP(x) \
    do {                                                        \
        tapi_jmp_point *p = tapi_jmp_push(__FILE__, __LINE__);  \
        int             jmp_rc = TE_EFAULT;                     \
                                                                \
        if (p == NULL || (jmp_rc = setjmp(p->env)) != 0)        \
        {                                                       \
            x;                                                  \
            ERROR("%s:%u: Unexpected after jump actions");      \
        }                                                       \
    } while (0)


/**
 * Set jump point in case some function raise an exception.
 * We should capture it here and then decide whether to
 * push it back to user with jump, or just return.
 *
 * Should be used at the beginning of functions that return void.
 */
#define TAPI_ON_JMP_DO_SAFE_VOID \
    TAPI_ON_JMP(                        \
        if (!tapi_jmp_stack_is_empty()) \
            TAPI_JMP_DO(jmp_rc);        \
        else                            \
            return;                     \
    )

/**
 * The same as TAPI_ON_JMP_DO_SAFE_VOID.
 *
 * Should be used at the beginning of functions that return int.
 */
#define TAPI_ON_JMP_DO_SAFE_RC \
    TAPI_ON_JMP(                        \
        if (!tapi_jmp_stack_is_empty()) \
            TAPI_JMP_DO(jmp_rc);        \
        else                            \
            return jmp_rc;              \
    )


/**
 * Allocate and push long jump point in a stack (saved stack context
 * for non-local goto).
 *
 * The routine is thread-safe using per thread stacks of jump points.
 *
 * @param file      Name of the file where jump point is set
 * @param lineno    Number of line in the file
 *
 * @return Pointer to allocated long jump point.
 *
 * @sa tapi_jmp_do, tapi_jmp_pop
 */
extern tapi_jmp_point *tapi_jmp_push(const char *file,
                                     unsigned int lineno);


/**
 * Macro for convinient use of tapi_jmp_pop().
 */
#define TAPI_JMP_POP    tapi_jmp_pop(__FILE__, __LINE__)

/**
 * Remove jump point set using tapi_jmp_push().
 *
 * The routine is thread-safe using per thread stacks of jump points.
 *
 * @param file      Name of the file where jump point is removed
 * @param lineno    Number of line in the file
 *
 * @return Status code
 * @retval TE_ENOENT   Stack of of jump points is empty
 *
 * @sa tapi_jmp_push, tapi_jmp_do
 */
extern int tapi_jmp_pop(const char *file, unsigned int lineno);


/**
 * Macro for convinient use of tapi_jmp_do().
 */
#define TAPI_JMP_DO(_val) tapi_jmp_do((_val), __FILE__, __LINE__)

/**
 * Do non-local goto a stack context saved using tapi_jmp_push().
 *
 * The routine is thread-safe using per thread stacks of jump points.
 *
 * @param val       Positive value to be returned by tapi_jmp_push
 *                  (0 is mapped to TE_EOK)
 * @param file      Name of the file where jump is done
 * @param lineno    Number of line in the file
 *
 * @return Status code
 * @retval TE_ENOENT   Stack of of jump points is empty
 *
 * @sa tapi_jmp_push
 */
extern int tapi_jmp_do(int val, const char *file, unsigned int lineno);

/**
 * Is stack of jumps empty?
 *
 * @retval TRUE     Stack of jumps is empty
 * @retval FALSE    Stack of jumps is not empty
 */
extern te_bool tapi_jmp_stack_is_empty(void);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_JMP_H__ */

/**@} <!-- END ts_tapi_jmp --> */
