/** @file
 * @brief Logger subsystem API - TA side 
 *
 * TA side Logger functionality for
 * forked TA processes and newly created threads, internal definitions.
 *
 * Copyright (C) 2005-2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 * 
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Mamadou Ngom <Mamadou.Ngom@oktetlabs.ru>
 *
 * $Id$
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

/** Common information in the message */
typedef struct logfork_msg {
    te_bool  is_notif;
    pid_t    pid;
    uint32_t tid;
    union {
        struct {
            char        name[LOGFORK_MAXUSER]; /** Logfork user name */
            te_bool     to_delete;             /** Delete a record with
                                                   corresponding pid
                                                   and tid */
        } notify;
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
#define __name       msg.notify.name
#define __to_delete  msg.notify.to_delete

#ifdef __cplusplus
}  /* extern "C" */
#endif  
#endif /* !__TE_LIB_LOGFORK_INT_H__ */
