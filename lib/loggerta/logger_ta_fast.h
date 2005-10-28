/** @file
 * @brief Logger subsystem API - TA side
 *
 * Macros and functions for fast logging.
 *
 *
 * Copyright (C) 2003-2005 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Igor B. Vasiliev <Igor.Vasiliev@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_LOGGER_TA_FAST_H__
#define __TE_LOGGER_TA_FAST_H__

#include "logger_defs.h"
#include "logger_int.h"
#include "logger_ta_lock.h"
#include "logger_ta_internal.h"


#define LGRF_MESSAGE(_lvl, _lgruser, _fs, _args...) \
    do {                                                            \
        if (TE_LOG_LEVEL & (_lvl))                                  \
        {                                                           \
            ta_log_message_fast(_lvl, _lgruser, _fs, LARG12(_args));\
        }                                                           \
    } while (0) 
    
/** @name Logging abnormal/unexpected situations */
#define LOGF_ERROR(_us, _fs...)  LGRF_MESSAGE(TE_LL_ERROR, _us, _fs)
#define F_ERROR(_fs...)          LOGF_ERROR(TE_LGR_USER, _fs)
/*@}*/

/**
 * @name Logging situations same failed initialization of the optional
 *       feature
 */
#define LOGF_WARN(_us, _fs...)  LGRF_MESSAGE(TE_LL_WARN, _us, _fs)
#define F_WARN(_fs...)          LOGF_WARN(TE_LGR_USER, _fs)
/*@}*/
  
/** @name Logging very important event in TE and tests */ 
#define LOGF_RING(_us, _fs...)  LGRF_MESSAGE(TE_LL_RING, _us, _fs)
#define F_RING(_fs...)          LOGF_RING(TE_LGR_USER, _fs)
/*@}*/

/** @name Logging important event for debugging of the test */
#define LOGF_INFO(_us, _fs...)  LGRF_MESSAGE(TE_LL_INFO, _us, _fs)
#define F_INFO(_fs...)          LOGF_INFO(TE_LGR_USER, _fs)
/*@}*/
   
/** @name Logging additional events for detalization of processing */
#define LOGF_VERB(_us, _fs...)  LGRF_MESSAGE(TE_LL_VERB, _us, _fs)
#define F_VERB(_fs...)          LOGF_VERB(TE_LGR_USER, _fs)
/*@}*/


    
/**< Logging of entry to and exit from function */    
#define LOGF_SUPENTRY(_us, _fs, _args...) \
    do {                                                 \
        ta_log_message_fast(TE_LL_ENTRY_EXIT, _us,       \
                            "ENTRY to %s(): " _fs,       \
                            1, (uint32_t)__FUNCTION__,   \
                            LARG11(_args));              \
    } while (0)


#define LOGF_ENTRY(_us, _fs...) \
    do {                                                        \
        if (TE_LOG_LEVEL & TE_LL_ENTRY_EXIT)                    \
        {                                                       \
            if (!!(#_fs[0]))                                    \
                LOGF_SUPENTRY(_us, _fs);                        \
            else                                                \
                ta_log_message_fast(TE_LL_ENTRY_EXIT, _us,      \
                                    "ENTRY to %s()",            \
                                    1, (uint32_t)__FUNCTION__,  \
                                    LARG11());                  \
        }                                                       \
    } while (0)
    
#define F_ENTRY(_fs...)  LOGF_ENTRY(TE_LGR_USER, _fs)    
    
#define LOGF_SUPEXIT(_us, _fs, _args...) \
    do {                                                        \
        ta_log_message_fast(TE_LL_ENTRY_EXIT, _us,              \
                            "EXIT in line %d from %s(): " _fs,  \
                            1, __LINE__,                        \
                            1, (uint32_t)__FUNCTION__,          \
                            LARG9(_args));                      \
    } while (0)
  
#define LOGF_EXIT(_us, _fs...)                                  \
    do {                                                        \
        if (TE_LOG_LEVEL & TE_LL_ENTRY_EXIT)                    \
        {                                                       \
            if (!!(#_fs[0]))                                    \
                LOGF_SUPEXIT(_us, _fs);                         \
            else                                                \
                ta_log_message_fast(TE_LL_ENTRY_EXIT, _us,      \
                                    "EXIT in line %d from %s()",\
                                    1, __LINE__,                \
                                    1, (uint32_t)__FUNCTION__,  \
                                    LARG9());                   \
        }                                                       \
    } while (0)

                
#define F_EXIT(_fs...)  LOGF_EXIT(TE_LGR_USER, _fs)     


/** Maximum arguments processed in this implementation */
#define TA_LOG_FAST_MAX_ARGS    12

/*
 * These macros carry out preliminary argument processing
 * at compilation time to make logging as quick as possible.
 * Each macro processes only one argument and calls following macro.
 * This way only limited number of arguments can be processed
 * (twelve arguments for this implementation).
 * Each macro produces one pair of elements:
 *   - "1, (uint32_t)(arg + 0)"  , if argument exists
 *   or
 *   - "0, (uint32_t)(+0)"      , if argument is not specified.
 * This is done by following way: #a[0] converts argument to string;
 * if argument is not present, first symbol of this string is equal
 * to 0, otherwise it is not; the obtained symbol is converted
 * to 0 or 1 by means of !!.
 * So, this way  valid numbers of arguments can be known in advance.
 */
#define LARG1(a, args...)  !!(#a[0]), (ta_log_arg)(a +0)
#define LARG2(a, args...)  !!(#a[0]), (ta_log_arg)(a +0), LARG1(args)
#define LARG3(a, args...)  !!(#a[0]), (ta_log_arg)(a +0), LARG2(args)
#define LARG4(a, args...)  !!(#a[0]), (ta_log_arg)(a +0), LARG3(args)
#define LARG5(a, args...)  !!(#a[0]), (ta_log_arg)(a +0), LARG4(args)
#define LARG6(a, args...)  !!(#a[0]), (ta_log_arg)(a +0), LARG5(args)
#define LARG7(a, args...)  !!(#a[0]), (ta_log_arg)(a +0), LARG6(args)
#define LARG8(a, args...)  !!(#a[0]), (ta_log_arg)(a +0), LARG7(args)
#define LARG9(a, args...)  !!(#a[0]), (ta_log_arg)(a +0), LARG8(args)
#define LARG10(a, args...) !!(#a[0]), (ta_log_arg)(a +0), LARG9(args)
#define LARG11(a, args...) !!(#a[0]), (ta_log_arg)(a +0), LARG10(args)
#define LARG12(a, args...) !!(#a[0]), (ta_log_arg)(a +0), LARG11(args)


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Log message (fast mode).
 *
 * @param level         Log level
 * @param user          Arbitrary user name
 * @param fmt           Message format string
 * @param argl1..argl12 Auxiliary args containing information about
 *                      appropriate arg
 * @param arg1..arg12   Arguments passed into the function according to
 *                      message format string
 */
static inline void
ta_log_message_fast(unsigned int level, const char *user, const char *fmt,
                    int argl1,  ta_log_arg arg1,
                    int argl2,  ta_log_arg arg2,
                    int argl3,  ta_log_arg arg3,
                    int argl4,  ta_log_arg arg4,
                    int argl5,  ta_log_arg arg5,
                    int argl6,  ta_log_arg arg6,
                    int argl7,  ta_log_arg arg7,
                    int argl8,  ta_log_arg arg8,
                    int argl9,  ta_log_arg arg9,
                    int argl10, ta_log_arg arg10,
                    int argl11, ta_log_arg arg11,
                    int argl12, ta_log_arg arg12)
{
    ta_log_lock_key     key;
    uint32_t            position;
    int                 res;

    struct lgr_mess_header *msg;

    if (log_buffer.rb == NULL)
        return;

    if (ta_log_lock(&key) != 0)
        return;

    res = lgr_rb_allocate_head(&log_buffer, LGR_RB_FORCE, &position);
    if (res == 0)
    {
        (void)ta_log_unlock(&key);
        return;
    }

    msg = (struct lgr_mess_header *)LGR_GET_MESSAGE_ARRAY(&log_buffer,
                                                          position);

    ta_log_timestamp(&msg->sec, &msg->usec);
    msg->level  = level;
    msg->user   = user;
    msg->fmt    = fmt;
    msg->n_args = argl1 + argl2 + argl3 + argl4  + argl5  + argl6 +
                  argl7 + argl8 + argl9 + argl10 + argl11 + argl12;

    if (argl1)
    {
        msg->arg1 = arg1;
        if (argl2)
        {
            msg->arg2 = arg2;
            if (argl3)
            {
                msg->arg3 = arg3;
                if (argl4)
                {
                    msg->arg4 = arg4;
                    if (argl5)
                    {
                        msg->arg5 = arg5;
                        if (argl6)
                        {
                            msg->arg6 = arg6;
                            if (argl7)
                            {
                                msg->arg7 = arg7;
                                if (argl8)
                                {
                                    msg->arg8 = arg8;
                                    if (argl9)
                                    {
                                        msg->arg9 = arg9;
                                        if (argl10)
                                        {
                                            msg->arg10 = arg10;
                                            if (argl11)
                                            {
                                                msg->arg11 = arg11;
                                                if (argl12)
                                                    msg->arg12 = arg12;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    (void)ta_log_unlock(&key);
}

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_LOGGER_TA_FAST_H__ */
