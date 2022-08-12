/** @file
 * @brief Logger subsystem API - TA side
 *
 * TA side Logger functionality for
 * forked TA processes and newly created threads, internal definitions.
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Mamadou Ngom <Mamadou.Ngom@oktetlabs.ru>
 *
 */

#ifndef __TE_LIB_LOGFORK_INT_H__
#define __TE_LIB_LOGFORK_INT_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "te_defs.h"
#include "te_stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum length of the logging message */
#define LOGFORK_MAXLEN   4096

/** Maximum length of the Logger user name or logfork user name */
#define LOGFORK_MAXUSER  32

/** Type of a logfork message */
typedef enum logfork_msg_type {
    LOGFORK_MSG_ADD_USER,     /**< Process registration or process name change */
    LOGFORK_MSG_DEL_USER,     /**< Process removal */
    LOGFORK_MSG_LOG,        /**< Log message */
    LOGFORK_MSG_SET_ID_LOGGING, /**< Enable or disable id logging in messages */
} logfork_msg_type;

/** Common information in the message */
typedef struct logfork_msg {
    logfork_msg_type type;
    pid_t    pid;
    uint32_t tid;
    union {
        struct {
            char        name[LOGFORK_MAXUSER]; /**< Logfork user name */
        } add;
        struct {
            te_bool     enabled;    /**< @c TRUE - enable, @c FALSE - disable
                                         logging of name and pid in messages */
        } set_id_logging;
        struct {
            te_log_ts_sec   sec;                  /**< Seconds */
            te_log_ts_usec  usec;                 /**< Microseconds */
            unsigned int    level;                /**< Log level */
            char            user[32];             /**< Log user */
            char            msg[LOGFORK_MAXLEN];  /**< Message */
        } log;
    } msg;
} logfork_msg;

/** Macros for fast access to structure fields */
#define __log_sec    msg.log.sec
#define __log_usec   msg.log.usec
#define __log_level  msg.log.level
#define __lgr_user   msg.log.user
#define __log_msg    msg.log.msg
#define __add_name   msg.add.name

#ifdef __cplusplus
}  /* extern "C" */
#endif
#endif /* !__TE_LIB_LOGFORK_INT_H__ */
