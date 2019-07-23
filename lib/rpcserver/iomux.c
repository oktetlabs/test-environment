/** @file
 * @brief Auxiliary I/O multiplexers API implementation
 *
 * Auxiliary I/O multiplexers API implementation.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "RPC iomux"

#include "iomux.h"

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

/* See description in iomux.h */
int
iomux_find_func(tarpc_lib_flags lib_flags, iomux_func *iomux, iomux_funcs *funcs)
{
    int rc = 0;

    if (*iomux == FUNC_DEFAULT_IOMUX)
        *iomux = get_default_iomux();

    switch (*iomux)
    {
        case FUNC_SELECT:
            rc = tarpc_find_func(lib_flags, "select", &funcs->select);
            break;
        case FUNC_PSELECT:
            rc = tarpc_find_func(lib_flags, "pselect", &funcs->select);
            break;
        case FUNC_POLL:
            rc = tarpc_find_func(lib_flags, "poll",
                                 (api_func *)&funcs->poll);
            break;
        case FUNC_PPOLL:
            rc = tarpc_find_func(lib_flags, "ppoll",
                                 (api_func *)&funcs->poll);
            break;
#if HAVE_STRUCT_EPOLL_EVENT
        case FUNC_EPOLL:
        case FUNC_EPOLL_PWAIT:
            if (*iomux == FUNC_EPOLL)
                rc = tarpc_find_func(lib_flags, "epoll_wait",
                                     &funcs->epoll.wait);
            else
                rc = tarpc_find_func(lib_flags, "epoll_pwait",
                                     &funcs->epoll.wait);
            rc = rc ||
                 tarpc_find_func(lib_flags, "epoll_ctl",
                                 &funcs->epoll.ctl)  ||
                 tarpc_find_func(lib_flags, "epoll_create",
                                 &funcs->epoll.create);
                 tarpc_find_func(lib_flags, "close", &funcs->epoll.close);
            break;
#endif
        case FUNC_NO_IOMUX:
            break;

        default:
            rc = -1;
            errno = ENOENT;
    }

    return rc;
}

/* See description in iomux.h */
int
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

        case FUNC_NO_IOMUX:
            break;
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

/* See description in iomux.h */
int
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

        case FUNC_NO_IOMUX:
            break;

        default:
            ERROR("Incorrect value of iomux function");
            return -1;
    }

#undef IOMUX_CHECK_LIMIT

    return 0;
}

/* See description in iomux.h */
int
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

        case FUNC_NO_IOMUX:
            break;

        default:
            ERROR("Incorrect value of iomux function");
            return -1;

    }
    return 0;
}

/* See description in iomux.h */
int
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
                tv.tv_usec = TE_MS2US(timeout % 1000UL);
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
                ts.tv_nsec = TE_MS2NS(timeout % 1000UL);
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
            ts.tv_nsec = TE_MS2NS(timeout % 1000UL);
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
        case FUNC_NO_IOMUX:
            rc = 0;
            break;

        default:
            errno = ENOENT;
            rc = -1;
    }
    INFO("%s done: %s, rc=%d", __FUNCTION__, iomux2str(iomux), rc);

    return rc;
}

/* See description in iomux.h */
iomux_return_iterator
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
        case FUNC_NO_IOMUX:
        default:
            it = IOMUX_RETURN_ITERATOR_END;
    }
out:
    INFO("%s done: %s, it=%d", __FUNCTION__, iomux2str(iomux), it);
    return it;
}

/* See description in iomux.h */
int
iomux_close(iomux_func iomux, iomux_funcs *funcs, iomux_state *state)
{
#if HAVE_STRUCT_EPOLL_EVENT
    if (iomux == FUNC_EPOLL || iomux == FUNC_EPOLL_PWAIT)
        return funcs->epoll.close(state->epoll);
#endif
    return 0;
}

/* See description in iomux.h */
int
iomux_fd_is_writable(int fd_exp, iomux_func iomux, iomux_state *iomux_st,
                     iomux_return *iomux_ret, int rc, te_bool *writable)
{
    iomux_return_iterator itr;
    int fd = -1;
    int events = 0;

    if (rc < 0)
    {
        ERROR("An error happened during iomux wait call");
        return -1;
    }
    else if (rc == 0)
    {
        *writable = FALSE;
        return 0;
    }

    itr = IOMUX_RETURN_ITERATOR_START;
    itr = iomux_return_iterate(iomux, iomux_st, iomux_ret, itr, &fd,
                               &events);
    if (fd != fd_exp)
    {
        ERROR("%s(): %s wait returned incorrect fd %d instead of %d",
              __FUNCTION__, iomux2str(iomux), fd, fd_exp);
        return -1;
    }

    if ((events & POLLOUT) != 0)
        *writable = TRUE;

    itr = iomux_return_iterate(iomux, iomux_st, iomux_ret, itr, &fd,
                               &events);
    if (itr != IOMUX_RETURN_ITERATOR_END)
    {
        ERROR("%s(): %s wait returned an extra event for fd %d",
              __FUNCTION__, iomux2str(iomux), fd);
        return -1;
    }

    return 0;
}
