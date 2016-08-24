/** @file
 * @brief Auxiliary I/O multiplexers API implementation
 *
 * Auxiliary I/O multiplexers API implementation.
 *
 * Copyright (C) 2004-2016 Test Environment authors (see file AUTHORS
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
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "RPC iomux"

#include "rpc_server.h"

/*-------------- generic iomux functions --------------------------*/

/* TODO: iomux_funcs should include iomux type, making arg list for all the
 * functions shorter. */
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

#define IOMUX_MAX_POLLED_FDS 64
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
 * To have these constants defined in Linux, specify
 * -D_GNU_SOURCE (or other related feature test macro) in
 * TE_PLATFORM macro in your builder.conf
 */
#ifndef POLLRDNORM
#define POLLRDNORM  0
#endif
#ifndef POLLWRNORM
#define POLLWRNORM  0
#endif
#ifndef POLLRDBAND
#define POLLRDBAND  0
#endif
#ifndef POLLWRBAND
#define POLLWRBAND  0
#endif

/* Mapping to/from select and POLL*.  Copied from Linux kernel. */
#define IOMUX_SELECT_READ \
    (POLLRDNORM | POLLRDBAND | POLLIN | POLLHUP | POLLERR)
#define IOMUX_SELECT_WRITE \
    (POLLWRBAND | POLLWRNORM | POLLOUT | POLLERR)
#define IOMUX_SELECT_EXCEPT (POLLPRI)

static iomux_func
get_default_iomux()
{
    char    *default_iomux = getenv("TE_RPC_DEFAULT_IOMUX");
    return (default_iomux == NULL) ? FUNC_POLL :
                str2iomux(default_iomux);
}

/** Resolve all functions used by particular iomux and store them into
 * iomux_funcs. */
static inline int
iomux_find_func(te_bool use_libc, iomux_func *iomux, iomux_funcs *funcs)
{
    int rc = 0;

    if (*iomux == FUNC_DEFAULT_IOMUX)
        *iomux = get_default_iomux();

    switch (*iomux)
    {
        case FUNC_SELECT:
            rc = tarpc_find_func(use_libc, "select", &funcs->select);
            break;
        case FUNC_PSELECT:
            rc = tarpc_find_func(use_libc, "pselect", &funcs->select);
            break;
        case FUNC_POLL:
            rc = tarpc_find_func(use_libc, "poll",
                                 (api_func *)&funcs->poll);
            break;
        case FUNC_PPOLL:
            rc = tarpc_find_func(use_libc, "ppoll",
                                 (api_func *)&funcs->poll);
            break;
#if HAVE_STRUCT_EPOLL_EVENT
        case FUNC_EPOLL:
        case FUNC_EPOLL_PWAIT:
            if (*iomux == FUNC_EPOLL)
                rc = tarpc_find_func(use_libc, "epoll_wait",
                                     &funcs->epoll.wait);
            else
                rc = tarpc_find_func(use_libc, "epoll_pwait",
                                     &funcs->epoll.wait);
            rc = rc ||
                 tarpc_find_func(use_libc, "epoll_ctl",
                                 &funcs->epoll.ctl)  ||
                 tarpc_find_func(use_libc, "epoll_create",
                                 &funcs->epoll.create);
                 tarpc_find_func(use_libc, "close", &funcs->epoll.close);
            break;
#endif
        default:
            rc = -1;
            errno = ENOENT;
    }

    return rc;
}

/** Initialize iomux_state so that it is safe to call iomux_close() */
static inline void
iomux_state_init_invalid(iomux_func iomux, iomux_state *state)
{
#if HAVE_STRUCT_EPOLL_EVENT
    if (iomux == FUNC_EPOLL || iomux == FUNC_EPOLL_PWAIT)
        state->epoll = -1;
#endif
}

/** Initialize iomux_state with zero value. Possibly, we should pass
 * maximum number of fds and use that number instead of
 * IOMUX_MAX_POLLED_FDS. */
static inline int
iomux_create_state(iomux_func iomux, iomux_funcs *funcs,
                   iomux_state *state)
{
    switch (iomux)
    {
        case FUNC_SELECT:
        case FUNC_PSELECT:
            FD_ZERO(&state->select.rfds);
            FD_ZERO(&state->select.wfds);
            FD_ZERO(&state->select.exfds);
            state->select.maxfds = 0;
            state->select.nfds = 0;
            break;
        case FUNC_POLL:
        case FUNC_PPOLL:
            state->poll.nfds = 0;
            break;
#if HAVE_STRUCT_EPOLL_EVENT
        case FUNC_EPOLL:
        case FUNC_EPOLL_PWAIT:
            state->epoll = funcs->epoll.create(IOMUX_MAX_POLLED_FDS);
            return (state->epoll >= 0) ? 0 : -1;
#endif
        case FUNC_DEFAULT_IOMUX:
            ERROR("%s() function can't be used with default iomux",
                  __FUNCTION__);
            return -1;

    }
    return 0;
}

static inline void
iomux_select_set_state(iomux_state *state, int fd, int events,
                       te_bool do_clear)
{
    /* Hack: POLERR is present in both read and write. Do not set both if
     * not really necessary */
    if ((events & POLLERR))
    {
        if ((events & ((IOMUX_SELECT_READ | IOMUX_SELECT_WRITE) &
                       ~POLLERR)) == 0)
        {
            events |= POLLIN;
        }
        events &= ~POLLERR;
    }

    /* Set and clear events */
    if ((events & IOMUX_SELECT_READ))
        FD_SET(fd, &state->select.rfds);
    else if (do_clear)
        FD_CLR(fd, &state->select.rfds);
    if ((events & IOMUX_SELECT_WRITE))
        FD_SET(fd, &state->select.wfds);
    else if (do_clear)
        FD_CLR(fd, &state->select.wfds);
    if ((events & IOMUX_SELECT_EXCEPT))
        FD_SET(fd, &state->select.exfds);
    else if (do_clear)
        FD_CLR(fd, &state->select.exfds);
}

/** Add fd to the list of watched fds, with given events (in POLL-events).
 * For select, all fds are added to exception list.
 * For some iomuxes, the function will produce error when adding the same
 * fd twice, so iomux_mod_fd() should be used. */
static inline int
iomux_add_fd(iomux_func iomux, iomux_funcs *funcs, iomux_state *state,
             int fd, int events)
{

#define IOMUX_CHECK_LIMIT(_nfds)                                \
do {                                                              \
    if (_nfds >= IOMUX_MAX_POLLED_FDS)                            \
    {                                                             \
        ERROR("%s(): failed to add file descriptor to the list "  \
              "for %s(), it has reached the limit %d",            \
              __FUNCTION__, iomux2str(iomux),                     \
              IOMUX_MAX_POLLED_FDS);                              \
        errno = ENOSPC;                                           \
        return -1;                                                \
    }                                                             \
} while(0)


    switch (iomux)
    {
        case FUNC_SELECT:
        case FUNC_PSELECT:
            IOMUX_CHECK_LIMIT(state->select.nfds);
            iomux_select_set_state(state, fd, events, FALSE);
            state->select.maxfds = MAX(state->select.maxfds, fd);
            state->select.fds[state->select.nfds] = fd;
            state->select.nfds++;
            break;

        case FUNC_POLL:
        case FUNC_PPOLL:
            IOMUX_CHECK_LIMIT(state->poll.nfds);
            state->poll.fds[state->poll.nfds].fd = fd;
            state->poll.fds[state->poll.nfds].events = events;
            state->poll.nfds++;
            break;

#if HAVE_STRUCT_EPOLL_EVENT
        case FUNC_EPOLL:
        case FUNC_EPOLL_PWAIT:
        {
            struct epoll_event ev;
            ev.events = events;
            ev.data.fd = fd;
            return funcs->epoll.ctl(state->epoll, EPOLL_CTL_ADD, fd, &ev);
        }
#endif
        case FUNC_DEFAULT_IOMUX:
            ERROR("%s() function can't be used with default iomux",
                  __FUNCTION__);
            return -1;

        default:
            ERROR("Incorrect value of iomux function");
            return -1;
    }

#undef IOMUX_CHECK_LIMIT

    return 0;
}

/** Modify events for already-watched fds. */
static inline int
iomux_mod_fd(iomux_func iomux, iomux_funcs *funcs, iomux_state *state,
             int fd, int events)
{
    switch (iomux)
    {
        case FUNC_SELECT:
        case FUNC_PSELECT:
            iomux_select_set_state(state, fd, events, TRUE);
            return 0;

        case FUNC_POLL:
        case FUNC_PPOLL:
        {
            int i;

            for (i = 0; i < state->poll.nfds; i++)
            {
                if (state->poll.fds[i].fd != fd)
                    continue;
                state->poll.fds[i].events = events;
                return 0;
            }
            errno = ENOENT;
            return -1;
        }

#if HAVE_STRUCT_EPOLL_EVENT
        case FUNC_EPOLL:
        case FUNC_EPOLL_PWAIT:
        {
            struct epoll_event ev;
            ev.events = events;
            ev.data.fd = fd;
            return funcs->epoll.ctl(state->epoll, EPOLL_CTL_MOD, fd, &ev);
            break;
        }
#endif
        case FUNC_DEFAULT_IOMUX:
            ERROR("%s() function can't be used with default iomux",
                  __FUNCTION__);
            return -1;

        default:
            ERROR("Incorrect value of iomux function");
            return -1;

    }
    return 0;
}

/* iomux_return may be null if user is not interested in the event list
 * (for example, when only one event is possible) */
static inline int
iomux_wait(iomux_func iomux, iomux_funcs *funcs, iomux_state *state,
           iomux_return *ret, int timeout)
{
    int rc;

    INFO("%s: %s, timeout=%d", __FUNCTION__, iomux2str(iomux), timeout);
    switch (iomux)
    {
        case FUNC_SELECT:
        case FUNC_PSELECT:
        {
            iomux_return   sret;

            if (ret == NULL)
                ret = &sret;

            memcpy(&ret->select.rfds, &state->select.rfds,
                   sizeof(state->select.rfds));
            memcpy(&ret->select.wfds, &state->select.wfds,
                   sizeof(state->select.wfds));
            memcpy(&ret->select.exfds, &state->select.exfds,
                   sizeof(state->select.exfds));
            if (iomux == FUNC_SELECT)
            {
                struct timeval tv;
                tv.tv_sec = timeout / 1000UL;
                tv.tv_usec = timeout % 1000UL;
                rc = funcs->select(state->select.maxfds + 1,
                                   &ret->select.rfds,
                                   &ret->select.wfds,
                                   &ret->select.exfds,
                                   &tv);
            }
            else /* FUNC_PSELECT */
            {
                struct timespec ts;
                ts.tv_sec = timeout / 1000UL;
                ts.tv_nsec = (timeout % 1000UL) * 1000UL;
                rc = funcs->select(state->select.maxfds + 1,
                                   &ret->select.rfds,
                                   &ret->select.wfds,
                                   &ret->select.exfds,
                                   &ts, NULL);
            }
#if 0
            ERROR("got %d: %x %x %x", rc,
                  ((int *)&ret->select.rfds)[0],
                  ((int *)&ret->select.wfds)[0],
                  ((int *)&ret->select.exfds)[0]
                  );
#endif
            break;
        }
        case FUNC_POLL:
            rc = funcs->poll(&state->poll.fds[0], state->poll.nfds,
                             timeout);
            break;

        case FUNC_PPOLL:
        {
            struct timespec ts;
            ts.tv_sec = timeout / 1000UL;
            ts.tv_nsec = (timeout % 1000UL) * 1000UL;
            rc = funcs->poll(&state->poll.fds[0], state->poll.nfds,
                             &ts, NULL);
            break;
        }
#if HAVE_STRUCT_EPOLL_EVENT
        case FUNC_EPOLL:
        case FUNC_EPOLL_PWAIT:
        {
            if (ret != NULL)
            {
                rc = (iomux == FUNC_EPOLL) ?
                    funcs->epoll.wait(state->epoll, &ret->epoll.events[0],
                                      IOMUX_MAX_POLLED_FDS, timeout) :
                    funcs->epoll.wait(state->epoll, &ret->epoll.events[0],
                                      IOMUX_MAX_POLLED_FDS, timeout, NULL);
                ret->epoll.nevents = rc;
            }
            else
            {
                struct epoll_event ev[IOMUX_MAX_POLLED_FDS];
                rc = (iomux == FUNC_EPOLL) ?
                        funcs->epoll.wait(state->epoll, ev,
                                          IOMUX_MAX_POLLED_FDS, timeout) :
                        funcs->epoll.wait(state->epoll, ev,
                                          IOMUX_MAX_POLLED_FDS, timeout,
                                          NULL);
            }
            break;
        }
#endif

        default:
            errno = ENOENT;
            rc = -1;
    }
    INFO("%s done: %s, rc=%d", __FUNCTION__, iomux2str(iomux), rc);

    return rc;
}

/** Iterate through all iomux result and return fds and events.  See also
 * IOMUX_RETURN_ITERATOR_START and IOMUX_RETURN_ITERATOR_END. */
static inline iomux_return_iterator
iomux_return_iterate(iomux_func iomux, iomux_state *st, iomux_return *ret,
                     iomux_return_iterator it, int *p_fd, int *p_events)
{
    INFO("%s: %s, it=%d", __FUNCTION__, iomux2str(iomux), it);
    switch (iomux)
    {
        case FUNC_SELECT:
        case FUNC_PSELECT:
        {
            int i;
            int events = 0;

            for (i = it; i < st->select.nfds; i++)
            {
                int fd = st->select.fds[i];

                /* TODO It is incorrect, but everything works.
                 * In any case, we can't do better: POLLHUP is reported as
                 * part of rdset only... */
                if (FD_ISSET(fd, &ret->select.rfds))
                    events |= IOMUX_SELECT_READ;
                if (FD_ISSET(fd, &ret->select.wfds))
                    events |= IOMUX_SELECT_WRITE;
                if (FD_ISSET(fd, &ret->select.exfds))
                    events |= IOMUX_SELECT_EXCEPT;
                if (events != 0)
                {
                    *p_fd = fd;
                    *p_events = events;
                    it = i + 1;
                    goto out;
                }
            }
            it = IOMUX_RETURN_ITERATOR_END;
            break;
        }

        case FUNC_POLL:
        case FUNC_PPOLL:
        {
            int i;

            for (i = it; i < st->poll.nfds; i++)
            {
                if (st->poll.fds[i].revents == 0)
                    continue;
                *p_fd = st->poll.fds[i].fd;
                *p_events = st->poll.fds[i].revents;
                it = i + 1;
                goto out;
            }
            it = IOMUX_RETURN_ITERATOR_END;
            break;
        }

#if HAVE_STRUCT_EPOLL_EVENT
        case FUNC_EPOLL:
        case FUNC_EPOLL_PWAIT:
            if (it >= ret->epoll.nevents)
            {
                it = IOMUX_RETURN_ITERATOR_END;
                break;
            }
            *p_fd = ret->epoll.events[it].data.fd;
            *p_events = ret->epoll.events[it].events;
            it++;
            break;
#endif
        default:
            it = IOMUX_RETURN_ITERATOR_END;
    }
out:
    INFO("%s done: %s, it=%d", __FUNCTION__, iomux2str(iomux), it);
    return it;
}

/** Close iomux state when necessary. */
static inline int
iomux_close(iomux_func iomux, iomux_funcs *funcs, iomux_state *state)
{
#if HAVE_STRUCT_EPOLL_EVENT
    if (iomux == FUNC_EPOLL || iomux == FUNC_EPOLL_PWAIT)
        return funcs->epoll.close(state->epoll);
#endif
    return 0;
}
