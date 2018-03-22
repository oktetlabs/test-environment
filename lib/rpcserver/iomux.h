/** @file
 * @brief Auxiliary I/O multiplexers API
 *
 * Auxiliary I/O multiplexers API definitions to be used in RPC calls.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
 *
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __RPCSERVER_IOMUX_H__
#define __RPCSERVER_IOMUX_H__

#include "rpc_server.h"

#define RPC_TYPE_NS_IOMUX_STATE     "iomux_state"

/**
 * Pointers to a multiplexer functions.
 *
 * TODO: iomux_funcs should include iomux type, making arg list for all the
 * functions shorter.
 */
typedef union iomux_funcs {
    api_func        select;
    api_func_ptr    poll;
#if HAVE_STRUCT_EPOLL_EVENT
    struct {
        api_func    wait;
        api_func    create;
        api_func    ctl;
        api_func    close;
    } epoll;
#endif
} iomux_funcs;

/** Maximum file descriptors number which can be used in a multiplexer
 * set. */
#define IOMUX_MAX_POLLED_FDS 64

/**
 * A multiplexer context.
 */
typedef union iomux_state {
    struct {
        int maxfds;
        fd_set rfds, wfds, exfds;
        int nfds;
        int fds[IOMUX_MAX_POLLED_FDS];
    } select;
    struct {
        int nfds;
        struct pollfd fds[IOMUX_MAX_POLLED_FDS];
    } poll;
#if HAVE_STRUCT_EPOLL_EVENT
    int epoll;
#endif
} iomux_state;

/**
 * Return events of a multiplexer.
 */
typedef union iomux_return {
    struct {
        fd_set rfds, wfds, exfds;
    } select;
#if HAVE_STRUCT_EPOLL_EVENT
    struct {
        struct epoll_event events[IOMUX_MAX_POLLED_FDS];
        int nevents;
    } epoll;
#endif
} iomux_return;

/** Iterator for iomux_return structure. */
typedef int iomux_return_iterator;
#define IOMUX_RETURN_ITERATOR_START 0
#define IOMUX_RETURN_ITERATOR_END   -1

/**
 * Iterate through all iomux result and return fds and events.
 *
 * @param iomux     Multiplexer function type.
 * @param st        The multiplexer context.
 * @param ret       Returned multiplexer data.
 * @param it        iterator value, @c IOMUX_RETURN_ITERATOR_START should be
 *                  use for the first iteration.
 * @param p_fd      File descriptor location.
 * @param p_events  Events location.
 *
 * @return New iterator value, @c IOMUX_RETURN_ITERATOR_END means no events
 *         left.
 */
extern iomux_return_iterator iomux_return_iterate(iomux_func iomux,
                                                  iomux_state *st,
                                                  iomux_return *ret,
                                                  iomux_return_iterator it,
                                                  int *p_fd, int *p_events);

/**
 * Initialize iomux_state with zero value. Possibly, we should pass
 * maximum number of fds and use that number instead of
 * IOMUX_MAX_POLLED_FDS.
 *
 * @param iomux     Multiplexer function type.
 * @param funcs     Pointer to the multiplexer functions.
 * @param state     The multiplexer context.
 *
 * @return @c 0 on success or @c -1 in the case of failure.
 */
extern int iomux_create_state(iomux_func iomux, iomux_funcs *funcs,
                              iomux_state *state);

/**
 * Close iomux state when necessary.
 *
 * @param iomux     Multiplexer function type.
 * @param funcs     Pointer to the multiplexer functions.
 * @param state     The multiplexer context.
 *
 * @return @c 0 on success or @c -1 in the case of failure.
 */
extern int iomux_close(iomux_func iomux, iomux_funcs *funcs,
                       iomux_state *state);

/**
 * Add fd to the list of watched fds, with given events (in POLL-events).
 * For select, all fds are added to exception list.
 * For some iomuxes, the function will produce error when adding the same
 * fd twice, so iomux_mod_fd() should be used.
 *
 * @param iomux     Multiplexer function type.
 * @param funcs     Pointer to the multiplexer functions.
 * @param state     The multiplexer context.
 * @param fd        The file descriptor.
 * @param events    Expected events.
 *
 * @return @c 0 on success or @c -1 in the case of failure.
 */
extern int iomux_add_fd(iomux_func iomux, iomux_funcs *funcs,
                        iomux_state *state, int fd, int events);

/**
 * Add fd to the list of watched fds, with given events (in POLL-events).
 * For select, all fds are added to exception list.
 * For some iomuxes, the function will produce error when adding the same
 * fd twice, so iomux_mod_fd() should be used.
 *
 * @param iomux     Multiplexer function type.
 * @param funcs     Pointer to the multiplexer functions.
 * @param state     The multiplexer context.
 * @param ret       Return events, may be @c NULL if user is not interested
 *                  in the event list.
 * @param timeout   Multiplexer timeout value, milliseconds.
 *
 * @return @c 0 on success or @c -1 in the case of failure.
 */
extern int iomux_wait(iomux_func iomux, iomux_funcs *funcs,
                      iomux_state *state, iomux_return *ret, int timeout);


/**
 * Resolve all functions used by particular iomux and store them into
 * iomux_funcs.
 *
 * @param use_libc  Use libc flag.
 * @param iomux     Multiplexer function type.
 * @param funcs     Pointer to the multiplexer functions.
 *
 * @return @c 0 on success or @c -1 in the case of failure.
 */
extern int iomux_find_func(te_bool use_libc, iomux_func *iomux,
                           iomux_funcs *funcs);

/**
 * Modify events for already-watched fds.
 *
 * @param iomux     Multiplexer function type.
 * @param funcs     Pointer to the multiplexer functions.
 * @param state     The multiplexer context.
 * @param fd        The file descriptor.
 * @param events    Expected events.
 *
 * @return @c 0 on success or @c -1 in the case of failure.
 */
extern int iomux_mod_fd(iomux_func iomux, iomux_funcs *funcs,
                        iomux_state *state, int fd, int events);

/**
 * Process a multiplexer call results to detrmine if a file descriptor is
 * writable. It is expected to get no more then one event.
 *
 * @param fd_exp    Endpoint where the event is expected.
 * @param iomux     Multiplexer function type.
 * @param iomux_st  The multiplexer context.
 * @param iomux_ret Returned multiplexer data.
 * @param rc        Return code of the multiplexer function call.
 * @param writable  Location for writable.
 *
 * @return @c 0 on success or @c -1 in the case of failure.
 */
extern int iomux_fd_is_writable(int fd_exp, iomux_func iomux,
                                iomux_state *iomux_st,
                                iomux_return *iomux_ret, int rc,
                                te_bool *writable);

/**
 * Initialize a multiplexer context so that it is safe to call
 * @c iomux_close().
 *
 * @param iomux     Multiplexer function type.
 * @param state     The multiplexer context.
 */
static inline void
iomux_state_init_invalid(iomux_func iomux, iomux_state *state)
{
#if HAVE_STRUCT_EPOLL_EVENT
    if (iomux == FUNC_EPOLL || iomux == FUNC_EPOLL_PWAIT)
        state->epoll = -1;
#endif
}

/**
 * Get default multiplexer type.
 *
 * @return The multiplexer type.
 */
static inline iomux_func
get_default_iomux()
{
    char    *default_iomux = getenv("TE_RPC_DEFAULT_IOMUX");
    return (default_iomux == NULL) ? FUNC_POLL :
                str2iomux(default_iomux);
}

#endif /* __RPCSERVER_IOMUX_H__ */
