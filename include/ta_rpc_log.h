/** @file
 * @brief Test Agent Logging
 *
 * RPC server logging macros.
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */
 
#ifndef __TA_RPC_LOG_H__
#define __TA_RPC_LOG_H__

#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "logger_api.h"

extern struct sockaddr *ta_log_addr_s;    /**< Logging server address */
extern int              ta_log_addr_len;  /**< Logging server address length */

/* 
 * Only macros ERROR, WARN, RING, INFO and VERB are allowed.
 *
 * Attention! Macros assume presense of the parameter or variable "in" 
 * (pointer to structure, which first part is compatible with
 *  rcf_rpc_in_arg) in functions where they are used.
 */
 
#include <sys/socket.h>
 
/** Maximum length of the resulting log message */
#define RPC_LOG_MSG_MAX         256

/** Length of data passed with the log message */
#define RPC_LOG_OVERHEAD (sizeof(uint16_t) + RCF_RPC_NAME_LEN)

/** Maximum length of the packet sent from RPC server to TA */
#define RPC_LOG_PKT_MAX  (RPC_LOG_MSG_MAX + RPC_LOG_OVERHEAD)

#define RPC_LGR_MESSAGE(_lvl, _fs...) \
    do {                                                                \
        char  _buf[RPC_LOG_PKT_MAX] = { 0, };                           \
        int   _s;                                                       \
        char *_str = _buf;                                              \
                                                                        \
        if (((_lvl) & LOG_LEVEL) == 0)                                  \
            break;                                                      \
                                                                        \
        if ((_s = socket(ta_log_addr_s->sa_family, SOCK_DGRAM, 0)) < 0) \
            break;                                                      \
                                                                        \
        *(uint16_t *)_str = _lvl;                                       \
        _str += sizeof(uint16_t);                                       \
        strcpy(_str, ((tarpc_in_arg *)in)->name);                       \
        _str += RCF_RPC_NAME_LEN;                                       \
        snprintf(_str, RPC_LOG_MSG_MAX, _fs);                           \
        sendto(_s, _buf, sizeof(_buf), 0,                               \
               ta_log_addr_s, ta_log_addr_len);                         \
        close(_s);                                                      \
    } while (0)

#ifndef TA_RPC_INSIDE
#undef TE_LGR_USER
#undef LGR_MESSAGE
#define LGR_MESSAGE(_lvl, _lgruser, _fs...) RPC_LGR_MESSAGE(_lvl, _fs) 
#endif

#endif /* __TA_RPC_LOG_H__ */
