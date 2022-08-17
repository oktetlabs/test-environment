/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from sys/poll.h.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_RPC_SYS_POLL_H__
#define __TE_RPC_SYS_POLL_H__

#include "te_rpc_defs.h"
#ifndef WINDOWS
#include "tarpc.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif

/** Maximum number of file descriptors passed to the poll */
#define RPC_POLL_NFDS_MAX       256


typedef enum rpc_poll_event {
    RPC_POLLIN       = 0x0001, /**< There is data to read */
    RPC_POLLPRI      = 0x0002, /**< There is urgent data to read */
    RPC_POLLOUT      = 0x0004, /**< Writing now will not block */
    RPC_POLLRDNORM   = 0x0008, /**< Normal data is readable */
    RPC_POLLWRNORM   = 0x0010, /**< Normal data is writeable */
    RPC_POLLRDBAND   = 0x0020, /**< Out-of-band data is readable */
    RPC_POLLWRBAND   = 0x0040, /**< Out-of-band data is writeable */
    RPC_POLLERR      = 0x0080, /**< Error condition */
    RPC_POLLHUP      = 0x0100, /**< Hung up */
    RPC_POLLNVAL     = 0x0200, /**< Invalid request: fd not open */
    RPC_POLLRDHUP    = 0x0400, /**< Stream socket peer closed
                                    connection, or shut down
                                    writing half of connection. */
    RPC_POLL_UNKNOWN = 0x0800  /**< Invalid poll event */
} rpc_poll_event;

/** All known poll events */
#define RPC_POLL_ALL        (RPC_POLLIN | RPC_POLLPRI | RPC_POLLOUT | \
                             RPC_POLLRDNORM | RPC_POLLWRNORM | \
                             RPC_POLLRDBAND | RPC_POLLWRBAND | \
                             RPC_POLLERR | RPC_POLLHUP | RPC_POLLNVAL | \
                             RPC_POLLRDHUP)

/** List of mapping numerical value to string for 'rpc_poll_event' */
#define POLL_EVENT_MAPPING_LIST \
            RPC_BIT_MAP_ENTRY(POLLIN), \
            RPC_BIT_MAP_ENTRY(POLLPRI), \
            RPC_BIT_MAP_ENTRY(POLLOUT), \
            RPC_BIT_MAP_ENTRY(POLLRDNORM), \
            RPC_BIT_MAP_ENTRY(POLLWRNORM), \
            RPC_BIT_MAP_ENTRY(POLLRDBAND), \
            RPC_BIT_MAP_ENTRY(POLLWRBAND), \
            RPC_BIT_MAP_ENTRY(POLLERR), \
            RPC_BIT_MAP_ENTRY(POLLHUP), \
            RPC_BIT_MAP_ENTRY(POLLNVAL), \
            RPC_BIT_MAP_ENTRY(POLLRDHUP), \
            RPC_BIT_MAP_ENTRY(POLL_UNKNOWN)

/**
 * poll_event_rpc2str()
 */
RPCBITMAP2STR(poll_event, POLL_EVENT_MAPPING_LIST)

extern unsigned int poll_event_rpc2h(unsigned int events);

extern unsigned int poll_event_h2rpc(unsigned int events);

/**
 * Convert integer representation of I/O multiplexer into
 * a string one.
 *
 * @param iomux     Integer representation
 *
 * @return String representation
 */
static inline const char *
iomux2str(iomux_func iomux)
{
    switch (iomux)
    {
        case FUNC_SELECT:
            return "select";
        case FUNC_PSELECT:
            return "pselect";
        case FUNC_POLL:
            return "poll";
        case FUNC_PPOLL:
            return "ppoll";
        case FUNC_EPOLL:
            return "epoll";
        case FUNC_EPOLL_PWAIT:
            return "epoll_pwait";
        case FUNC_DEFAULT_IOMUX:
            return "default iomux";
        case FUNC_NO_IOMUX:
            return "no_iomux";
        default:
            return "<unknown>";
    }
}

/**
 * Convert string representation of I/O multiplexer into
 * an integer one.
 *
 * @param iomux     String representation
 *
 * @return Integer representation
 */
static inline int
str2iomux(const char *iomux)
{
    if (iomux == NULL)
        return 0;
    else if (strcmp(iomux, "select") == 0)
        return FUNC_SELECT;
    else if (strcmp(iomux, "pselect") == 0)
        return FUNC_PSELECT;
    else if (strcmp(iomux, "poll") == 0)
        return FUNC_POLL;
    else if (strcmp(iomux, "ppoll") == 0)
        return FUNC_PPOLL;
    else if (strcmp(iomux, "epoll") == 0)
        return FUNC_EPOLL;
    else if (strcmp(iomux, "epoll_pwait") == 0)
        return FUNC_EPOLL_PWAIT;
    else
        return 0;
}

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_RPC_SYS_POLL_H__ */
