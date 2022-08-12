/** @file
 * @brief Implementation of I/O multiplexers test API
 *
 * Functions implementation.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 */

#define TE_LGR_USER     "Multiplexers TAPI"

#include "te_config.h"
#include "tapi_iomux.h"
#include "tapi_rpc.h"
#include "tapi_mem.h"
#include "tapi_test.h"
#include "tapi_rpc_unistd.h"

/* See the description in tapi_iomux.h. */
short int
tapi_iomux_evt_to_poll(tapi_iomux_evt iomux_evt_mask)
{
    short int poll_evts = 0;

    if (iomux_evt_mask & EVT_RD)
        poll_evts |= RPC_POLLIN;

    if (iomux_evt_mask & EVT_PRI)
        poll_evts |= RPC_POLLPRI;

    if (iomux_evt_mask & EVT_WR)
        poll_evts |= RPC_POLLOUT;

    if (iomux_evt_mask & EVT_RD_NORM)
        poll_evts |= RPC_POLLRDNORM;

    if (iomux_evt_mask & EVT_WR_NORM)
        poll_evts |= RPC_POLLWRNORM;

    if (iomux_evt_mask & EVT_RD_BAND)
    {
        poll_evts |= RPC_POLLPRI;
        poll_evts |= RPC_POLLRDBAND;
    }

    if (iomux_evt_mask & EVT_WR_BAND)
    {
        poll_evts |= RPC_POLLWRBAND;
    }


    /*
     * The following fields should not be used, because not extension
     * events may be passed in requested events to poll function.
     * Nevertheless, convert them.
     */

    if (iomux_evt_mask & EVT_EXC)
    {
        poll_evts |= RPC_POLLERR;
        poll_evts |= RPC_POLLHUP;
        poll_evts |= RPC_POLLNVAL;
    }

    if (iomux_evt_mask & EVT_ERR)
        poll_evts |= RPC_POLLERR;

    if (iomux_evt_mask & EVT_HUP)
        poll_evts |= RPC_POLLHUP;

    if (iomux_evt_mask & EVT_NVAL)
        poll_evts |= RPC_POLLNVAL;

    if (iomux_evt_mask & EVT_RDHUP)
        poll_evts |= RPC_POLLRDHUP;

    return poll_evts;
}

/* See the description in tapi_iomux.h. */
uint32_t
tapi_iomux_evt_to_epoll(tapi_iomux_evt iomux_evt_mask)
{
    uint32_t epoll_evts = 0;

    if (iomux_evt_mask & EVT_RD)
        epoll_evts |= RPC_EPOLLIN;

    if (iomux_evt_mask & EVT_PRI)
        epoll_evts |= RPC_EPOLLPRI;

    if (iomux_evt_mask & EVT_WR)
        epoll_evts |= RPC_EPOLLOUT;

    if (iomux_evt_mask & EVT_RD_NORM)
        epoll_evts |= RPC_EPOLLRDNORM;

    if (iomux_evt_mask & EVT_WR_NORM)
        epoll_evts |= RPC_EPOLLWRNORM;

    if (iomux_evt_mask & EVT_RD_BAND)
    {
        epoll_evts |= RPC_EPOLLPRI;
        epoll_evts |= RPC_EPOLLRDBAND;
    }

    if (iomux_evt_mask & EVT_WR_BAND)
    {
        epoll_evts |= RPC_EPOLLWRBAND;
    }

    if (iomux_evt_mask & EVT_EXC)
    {
        epoll_evts |= RPC_EPOLLERR;
        epoll_evts |= RPC_EPOLLHUP;
        epoll_evts |= RPC_EPOLLMSG;
    }

    if (iomux_evt_mask & EVT_ERR)
        epoll_evts |= RPC_EPOLLERR;

    if (iomux_evt_mask & EVT_HUP)
        epoll_evts |= RPC_EPOLLHUP;

    if (iomux_evt_mask & EVT_NVAL)
        epoll_evts |= RPC_EPOLLMSG;

    if (iomux_evt_mask & EVT_RDHUP)
        epoll_evts |= RPC_EPOLLRDHUP;

    if (iomux_evt_mask & EVT_ET)
        epoll_evts |= RPC_EPOLLET;

    if (iomux_evt_mask & EVT_ONESHOT)
        epoll_evts |= RPC_EPOLLONESHOT;

    return epoll_evts;
}

/* See the description in tapi_iomux.h. */
tapi_iomux_evt
tapi_iomux_poll_to_evt(short int poll_evt_mask)
{
    tapi_iomux_evt iomux_evts = 0;

    if (poll_evt_mask & RPC_POLLIN)
        iomux_evts |= EVT_RD;

    if (poll_evt_mask & RPC_POLLPRI)
        iomux_evts |= EVT_PRI;

    if (poll_evt_mask & RPC_POLLOUT)
        iomux_evts |= EVT_WR;

    if (poll_evt_mask & RPC_POLLRDNORM)
    {
        iomux_evts |= EVT_RD_NORM;
        iomux_evts |= EVT_RD;
    }

    if (poll_evt_mask & RPC_POLLWRNORM)
    {
        iomux_evts |= EVT_WR_NORM;
        iomux_evts |= EVT_WR;
    }

    if (poll_evt_mask & RPC_POLLRDBAND)
        iomux_evts |= EVT_RD_BAND;

    if (poll_evt_mask & RPC_POLLWRBAND)
        iomux_evts |= EVT_WR_BAND;

    if (poll_evt_mask & RPC_POLLPRI)
        iomux_evts |= EVT_PRI;

    if (poll_evt_mask & RPC_POLLERR)
        iomux_evts |= (EVT_EXC | EVT_ERR);

    if (poll_evt_mask & RPC_POLLHUP)
        iomux_evts |= (EVT_EXC | EVT_HUP);

    if (poll_evt_mask & RPC_POLLNVAL)
        iomux_evts |= (EVT_EXC | EVT_NVAL);

    if (poll_evt_mask & RPC_POLLRDHUP)
        iomux_evts |= EVT_RDHUP;

    return iomux_evts;
}

/* See the description in tapi_iomux.h. */
tapi_iomux_evt
tapi_iomux_epoll_to_evt(uint32_t epoll_evt_mask)
{
    tapi_iomux_evt iomux_evts = 0;

    if (epoll_evt_mask & RPC_EPOLLIN)
        iomux_evts |= EVT_RD;

    if (epoll_evt_mask & RPC_EPOLLOUT)
        iomux_evts |= EVT_WR;

    if (epoll_evt_mask & RPC_EPOLLRDNORM)
    {
        iomux_evts |= EVT_RD_NORM;
        iomux_evts |= EVT_RD;
    }

    if (epoll_evt_mask & RPC_EPOLLWRNORM)
    {
        iomux_evts |= EVT_WR_NORM;
        iomux_evts |= EVT_WR;
    }

    if (epoll_evt_mask & RPC_EPOLLRDBAND)
        iomux_evts |= EVT_RD_BAND;

    if (epoll_evt_mask & RPC_EPOLLWRBAND)
        iomux_evts |= EVT_WR_BAND;

    if (epoll_evt_mask & RPC_EPOLLPRI)
        iomux_evts |= EVT_PRI;

    if (epoll_evt_mask & RPC_EPOLLERR)
        iomux_evts |= (EVT_EXC | EVT_ERR);

    if (epoll_evt_mask & RPC_EPOLLHUP)
        iomux_evts |= (EVT_EXC | EVT_HUP);

    if (epoll_evt_mask & RPC_EPOLLRDHUP)
        iomux_evts |= EVT_RDHUP;

    if (epoll_evt_mask & RPC_EPOLLMSG)
        iomux_evts |= (EVT_EXC | EVT_NVAL);

    if (epoll_evt_mask & RPC_EPOLLET)
        iomux_evts |= EVT_ET;

    if (epoll_evt_mask & RPC_EPOLLONESHOT)
        iomux_evts |= EVT_ONESHOT;

    return iomux_evts;
}

/* See the iomux.h file for the description. */
tapi_iomux_type
tapi_iomux_call_str2en(const char *iomux)
{
    if (iomux == NULL)
        return TAPI_IOMUX_UNKNOWN;

    if (strcmp(iomux, "select") == 0)
        return TAPI_IOMUX_SELECT;

    if (strcmp(iomux, "pselect") == 0)
        return TAPI_IOMUX_PSELECT;

    if (strcmp(iomux, "poll") == 0)
        return TAPI_IOMUX_POLL;

    if (strcmp(iomux, "ppoll") == 0)
        return TAPI_IOMUX_PPOLL;

    if (strcmp(iomux, "epoll") == 0)
        return TAPI_IOMUX_EPOLL;

    if (strcmp(iomux, "epoll_pwait") == 0)
        return TAPI_IOMUX_EPOLL_PWAIT;

    if (strcmp(iomux, "reserved") == 0)
        return TAPI_IOMUX_RESERVED;

    return TAPI_IOMUX_UNKNOWN;
}

/* See the description in tapi_iomux.h. */
const char *
tapi_iomux_call_en2str(tapi_iomux_type iomux_type)
{
    switch(iomux_type)
    {
        case TAPI_IOMUX_UNKNOWN:     return "(unknown)";
        case TAPI_IOMUX_SELECT:      return "select";
        case TAPI_IOMUX_PSELECT:     return "pselect";
        case TAPI_IOMUX_POLL:        return "poll";
        case TAPI_IOMUX_PPOLL:       return "ppoll";
        case TAPI_IOMUX_EPOLL:       return "epoll";
        case TAPI_IOMUX_EPOLL_PWAIT: return "epoll_pwait";
        case TAPI_IOMUX_RESERVED:    return "reserved";
        case TAPI_IOMUX_DEFAULT:     return "default iomux";
    }

    return NULL;
}

/**
 * Get pointer to a file descriptor context by its file descriptor.
 *
 * @param iomux     A multiplexer handle.
 * @param fd        A file descriptor.
 *
 * @return Pointer to file descriptor context.
 */
static tapi_iomux_evts_list *
tapi_iomux_get_evt_by_fd(tapi_iomux_handle *iomux, int fd)
{
    tapi_iomux_evts_list *inst;

    SLIST_FOREACH(inst, &iomux->evts, tapi_iomux_evts_ent)
    {
        if (inst->evt.fd == fd)
            return inst;
    }

    TEST_FAIL("Cannot find fd=%d in the iomux set", fd);
    return NULL;
}

/**
 * Allocate file descritors sets for 'select' API.
 *
 * @param iomux     A multiplexer handle.
 */
static void
tapi_iomux_select_create(tapi_iomux_handle *iomux)
{
    iomux->select.read_fds = rpc_fd_set_new(iomux->rpcs);
    rpc_do_fd_zero(iomux->rpcs, iomux->select.read_fds);

    iomux->select.write_fds = rpc_fd_set_new(iomux->rpcs);
    rpc_do_fd_zero(iomux->rpcs, iomux->select.write_fds);

    iomux->select.exc_fds = rpc_fd_set_new(iomux->rpcs);
    rpc_do_fd_zero(iomux->rpcs, iomux->select.exc_fds);
}

/**
 * Destroy file descritors sets.
 *
 * @param iomux     A multiplexer handle.
 */
static void
tapi_iomux_select_destroy(tapi_iomux_handle *iomux)
{
    rpc_fd_set_delete(iomux->rpcs, iomux->select.read_fds);
    rpc_fd_set_delete(iomux->rpcs, iomux->select.write_fds);
    rpc_fd_set_delete(iomux->rpcs, iomux->select.exc_fds);
}

/**
 * Add a file descritor to sets.
 *
 * @param iomux     A multiplexer handle.
 * @param fd        The file descriptor.
 * @param evt       Requested events.
 */
static void
tapi_iomux_select_add(tapi_iomux_handle *iomux, int fd, tapi_iomux_evt evt)
{
    if ((evt & (EVT_RD | EVT_RD_NORM)) != 0)
        rpc_do_fd_set(iomux->rpcs, fd, iomux->select.read_fds);

    if ((evt & (EVT_WR | EVT_WR_NORM)) != 0)
        rpc_do_fd_set(iomux->rpcs, fd, iomux->select.write_fds);

    if ((evt & (EVT_EXC | EVT_HUP | EVT_ERR | EVT_NVAL |
                EVT_RD_BAND | EVT_WR_BAND | EVT_PRI | EVT_RDHUP)) != 0)
        rpc_do_fd_set(iomux->rpcs, fd, iomux->select.exc_fds);
}

/**
 * Add all file descritors saved in iomux context to 'select' sets.
 *
 * @param iomux     A multiplexer handle.
 *
 * @return Maximum file descriptor id increased by @c 1, can be passed to
 *         @c select() call.
 */
static int
tapi_iomux_select_add_events(tapi_iomux_handle *iomux)
{
    tapi_iomux_evts_list *inst;
    te_bool               await_err = RPC_AWAITING_ERROR(iomux->rpcs);
    rcf_rpc_op            op = iomux->rpcs->op;
    int                   max_fd = 0;

    if (op == RCF_RPC_WAIT)
        return 0;

    if (op == RCF_RPC_CALL)
        iomux->rpcs->op = RCF_RPC_CALL_WAIT;

    /* RPCs are called inside @p tapi_iomux_select_add to prepare
     * events sets, it is not applicable to fail in that functions - this
     * means invalid using of the select API. */
    RPC_DONT_AWAIT_IUT_ERROR(iomux->rpcs);

    SLIST_FOREACH(inst, &iomux->evts, tapi_iomux_evts_ent)
    {
        tapi_iomux_select_add(iomux, inst->evt.fd, inst->evt.events);
        if (max_fd < inst->evt.fd)
            max_fd = inst->evt.fd + 1;
    }

    if (await_err)
        RPC_AWAIT_IUT_ERROR(iomux->rpcs);

    iomux->rpcs->op = op;

    return max_fd;
}

/**
 * Process returned events by @c select() or @c pselect() call, convert and
 * save them in the iomux context.
 *
 * @param iomux     A multiplexer handle.
 *
 * @return Returned events array converted to the generic representation.
 */
static tapi_iomux_evt_fd *
tapi_iomux_select_get_events(tapi_iomux_handle *iomux)
{
    tapi_iomux_evts_list *inst;
    tapi_iomux_evt_fd *evts;
    int i = 0;

    if (iomux->rpcs->op == RCF_RPC_WAIT)
        return NULL;

    evts = tapi_calloc(iomux->fds_num, sizeof(*evts));

    SLIST_FOREACH(inst, &iomux->evts, tapi_iomux_evts_ent)
    {
        if (rpc_do_fd_isset(iomux->rpcs, inst->evt.fd,
                            iomux->select.read_fds) > 0)
            evts[i].revents |= EVT_RD;

        if (rpc_do_fd_isset(iomux->rpcs, inst->evt.fd,
                            iomux->select.write_fds) > 0)
            evts[i].revents |= EVT_WR;

        if (rpc_do_fd_isset(iomux->rpcs, inst->evt.fd,
                            iomux->select.exc_fds) > 0)
            evts[i].revents |= EVT_EXC;

        if (evts[i].revents != 0)
        {
            evts[i].events = inst->evt.events;
            evts[i].fd = inst->evt.fd;
            inst->evt.revents = evts[i].revents;
            i++;
        }
    }

    return evts;
}

/**
 * Perform @c select() call.
 *
 * @param iomux     The multiplexer handle.
 * @param timeout   Timeout to block in the call in milliseconds.
 * @param revts     Returned events.
 *
 * @return Events number.
 */
static int
tapi_iomux_select_call(tapi_iomux_handle *iomux, int timeout,
                       tapi_iomux_evt_fd **revts)
{
    struct tarpc_timeval  tv;
    struct tarpc_timeval *tv_ptr = &tv;

    int max_fd = 0;
    int rc;

    if (timeout < 0)
        tv_ptr = NULL;
    else
        TE_US2TV(TE_MS2US(timeout), &tv);

    max_fd = tapi_iomux_select_add_events(iomux);

    rc = rpc_select(iomux->rpcs, max_fd, iomux->select.read_fds,
                    iomux->select.write_fds, iomux->select.exc_fds, tv_ptr);

    if (rc <= 0)
        return rc;

    *revts = tapi_iomux_select_get_events(iomux);

    return rc;
}

/**
 * Perform @c pselect() call.
 *
 * @param iomux     The multiplexer handle.
 * @param timeout   Timeout to block in the call in milliseconds.
 * @param revts     Returned events.
 *
 * @return Events number.
 */
static int
tapi_iomux_pselect_call(tapi_iomux_handle *iomux, int timeout,
                        tapi_iomux_evt_fd **revts)
{
    struct tarpc_timespec  tv;
    struct tarpc_timespec *tv_ptr = &tv;

    int max_fd = 0;
    int rc;

    if (timeout < 0)
        tv_ptr = NULL;
    else
        TE_NS2TS(TE_MS2NS(timeout), &tv);

    max_fd = tapi_iomux_select_add_events(iomux);

    rc = rpc_pselect(iomux->rpcs, max_fd, iomux->select.read_fds,
                     iomux->select.write_fds, iomux->select.exc_fds, tv_ptr,
                     iomux->sigmask);

    if (rc <= 0)
        return rc;

    *revts = tapi_iomux_select_get_events(iomux);

    return rc;
}

/**
 * @c select API methods.
 */
static const tapi_iomux_methods tapi_iomux_methods_select =
    {
        .create  = tapi_iomux_select_create,
        .add     = NULL,
        .mod     = NULL,
        .del     = NULL,
        .call    = tapi_iomux_select_call,
        .destroy = tapi_iomux_select_destroy,
    };

/**
 * @c pselect API methods.
 */
static const tapi_iomux_methods tapi_iomux_methods_pselect =
    {
        .create  = tapi_iomux_select_create,
        .add     = NULL,
        .mod     = NULL,
        .del     = NULL,
        .call    = tapi_iomux_pselect_call,
        .destroy = tapi_iomux_select_destroy,
    };

/**
 * Allocate and initialize poll events set.
 *
 * @param iomux     The multiplexer handle.
 *
 * @return The poll events set.
 */
static struct rpc_pollfd *
tapi_iomux_poll_create_events(tapi_iomux_handle *iomux)
{
    struct rpc_pollfd *fds = NULL;
    rcf_rpc_op op = iomux->rpcs->op;
    tapi_iomux_evts_list *inst;
    int i = 0;

    if (op == RCF_RPC_WAIT)
        return iomux->poll.fds;

    fds = tapi_calloc(iomux->fds_num, sizeof(*fds));
    iomux->poll.fds = fds;

    SLIST_FOREACH(inst, &iomux->evts, tapi_iomux_evts_ent)
    {
        fds[i].fd = inst->evt.fd;
        fds[i].events = tapi_iomux_evt_to_poll(inst->evt.events);
        i++;
    }

    return fds;
}

/**
 * Process returned events by @c poll() or @c ppoll() call, convert and save
 * them in the iomux context.
 *
 * @param iomux     The multiplexer handle.
 * @param fds       Poll events set, it is freed after the conversion.
 * @param evts_num  Returned events number.
 *
 * @return Returned events array converted to the generic representation.
 */
static tapi_iomux_evt_fd *
tapi_iomux_poll_get_events(tapi_iomux_handle *iomux, struct rpc_pollfd *fds,
                           int evts_num)
{
    tapi_iomux_evts_list *inst;
    tapi_iomux_evt_fd *evts;
    int i_fds = -1;
    int i_evts = 0;

    if (iomux->rpcs->op == RCF_RPC_WAIT)
        return NULL;

    if (evts_num <= 0)
    {
        free(fds);
        return NULL;
    }

    evts = tapi_calloc(evts_num, sizeof(*evts));

    SLIST_FOREACH(inst, &iomux->evts, tapi_iomux_evts_ent)
    {
        i_fds++;
        if (fds[i_fds].fd != inst->evt.fd)
        {
            ERROR("Incorrect fd #%d: %d instead of %d", i_fds,
                  fds[i_fds].fd, inst->evt.fd);
            TEST_VERDICT("Events set has changed file descriptor after the"
                         "poll call");
        }

        if (fds[i_fds].revents == 0)
            continue;

        evts[i_evts].fd = inst->evt.fd;
        evts[i_evts].events = inst->evt.events;
        evts[i_evts].revents = tapi_iomux_poll_to_evt(fds[i_fds].revents);
        inst->evt.revents = evts[i_evts].revents;
        i_evts++;
    }

    free(fds);

    return evts;
}

/**
 * Perform @c poll() call.
 *
 * @param iomux     The multiplexer handle.
 * @param timeout   Timeout to block in the call in milliseconds.
 * @param revts     Returned events.
 *
 * @return Events number.
 */
static int
tapi_iomux_poll_call(tapi_iomux_handle *iomux, int timeout,
                       tapi_iomux_evt_fd **revts)
{
    struct rpc_pollfd *fds;
    int rc;

    fds = tapi_iomux_poll_create_events(iomux);

    rc = rpc_poll(iomux->rpcs, fds, iomux->fds_num, timeout);

    *revts = tapi_iomux_poll_get_events(iomux, fds, rc);

    return rc;
}

/**
 * Perform @c ppoll() call.
 *
 * @param iomux     The multiplexer handle.
 * @param timeout   Timeout to block in the call in milliseconds.
 * @param revts     Returned events.
 *
 * @return Events number.
 */
static int
tapi_iomux_ppoll_call(tapi_iomux_handle *iomux, int timeout,
                      tapi_iomux_evt_fd **revts)
{
    struct tarpc_timespec  tv;
    struct tarpc_timespec *tv_ptr = &tv;

    struct rpc_pollfd *fds;
    int rc;

    if (timeout < 0)
        tv_ptr = NULL;
    else
        TE_NS2TS(TE_MS2NS(timeout), &tv);

    fds = tapi_iomux_poll_create_events(iomux);

    rc = rpc_ppoll(iomux->rpcs, fds, iomux->fds_num, tv_ptr,
                   iomux->sigmask);

    *revts = tapi_iomux_poll_get_events(iomux, fds, rc);

    return rc;
}

/**
 * @c poll API methods.
 */
static const tapi_iomux_methods tapi_iomux_methods_poll =
    {
        .create  = NULL,
        .add     = NULL,
        .mod     = NULL,
        .del     = NULL,
        .call    = tapi_iomux_poll_call,
        .destroy = NULL,
    };

/**
 * @c ppoll API methods.
 */
static const tapi_iomux_methods tapi_iomux_methods_ppoll =
    {
        .create  = NULL,
        .add     = NULL,
        .mod     = NULL,
        .del     = NULL,
        .call    = tapi_iomux_ppoll_call,
        .destroy = NULL,
    };

/**
 * Creat new epoll set.
 *
 * @param iomux     The multiplexer handle.
 */
static void
tapi_iomux_epoll_create(tapi_iomux_handle *iomux)
{
    int rc;

    RPC_AWAIT_IUT_ERROR(iomux->rpcs);
    rc = rpc_epoll_create(iomux->rpcs, 1);
    if (rc <= 0)
        TEST_VERDICT("epoll_create() failed: %r",
                     RPC_ERRNO(iomux->rpcs));

    iomux->epoll.epfd = rc;
}

/**
 * Close a epoll set.
 *
 * @param iomux     The multiplexer handle.
 */
static void
tapi_iomux_epoll_destroy(tapi_iomux_handle *iomux)
{
    int rc;

    RPC_AWAIT_IUT_ERROR(iomux->rpcs);
    rc = rpc_close(iomux->rpcs, iomux->epoll.epfd);
    if (rc != 0)
        TEST_VERDICT("Failed to close epoll set: %r",
                     RPC_ERRNO(iomux->rpcs));
}

/**
 * Add a file descriptor to a epoll set.
 *
 * @param iomux     The multiplexer handle.
 * @param fd        The file descriptor to add to the multiplexer set.
 * @param evt       Requested events.
 */
static void
tapi_iomux_epoll_add(tapi_iomux_handle *iomux, int fd, tapi_iomux_evt evt)
{
    int rc;

    RPC_AWAIT_IUT_ERROR(iomux->rpcs);
    rc = rpc_epoll_ctl_simple(iomux->rpcs, iomux->epoll.epfd,
                              RPC_EPOLL_CTL_ADD, fd,
                              tapi_iomux_evt_to_epoll(evt));
    if (rc != 0)
        TEST_VERDICT("epoll_ctl() failed to add new fd to the set: %r",
                     RPC_ERRNO(iomux->rpcs));
}

/**
 * Modify a file descriptor events.
 *
 * @param iomux     The multiplexer handle.
 * @param fd        The file descriptor to modify events for.
 * @param evt       New events.
 */
static void
tapi_iomux_epoll_mod(tapi_iomux_handle *iomux, int fd, tapi_iomux_evt evt)
{
    int rc;

    RPC_AWAIT_IUT_ERROR(iomux->rpcs);
    rc = rpc_epoll_ctl_simple(iomux->rpcs, iomux->epoll.epfd,
                              RPC_EPOLL_CTL_MOD, fd,
                              tapi_iomux_evt_to_epoll(evt));
    if (rc != 0)
        TEST_VERDICT("epoll_ctl() failed to modify fd events: %r",
                     RPC_ERRNO(iomux->rpcs));
}

/**
 * Delete a file descriptor from a epoll set.
 *
 * @param iomux     The multiplexer handle.
 * @param fd        The file descriptor which should be deleted.
 */
static void
tapi_iomux_epoll_del(tapi_iomux_handle *iomux, int fd)
{
    int rc;

    RPC_AWAIT_IUT_ERROR(iomux->rpcs);
    rc = rpc_epoll_ctl_simple(iomux->rpcs, iomux->epoll.epfd,
                              RPC_EPOLL_CTL_DEL, fd, 0);
    if (rc != 0)
        TEST_VERDICT("epoll_ctl() failed to delete fd from the set: %r",
                     RPC_ERRNO(iomux->rpcs));
}

/* See the description in tapi_iomux.h. */
tapi_iomux_evt_fd *
tapi_iomux_epoll_get_events(tapi_iomux_handle *iomux,
                            struct rpc_epoll_event *events, int evts_num)
{
    tapi_iomux_evts_list *inst;
    tapi_iomux_evt_fd *evts;
    int i;

    evts = tapi_calloc(evts_num, sizeof(*evts));

    for (i = 0; i < evts_num; i++)
    {
        inst = tapi_iomux_get_evt_by_fd(iomux, events[i].data.fd);
        evts[i].fd = events[i].data.fd;
        evts[i].events = inst->evt.events;
        evts[i].revents = tapi_iomux_epoll_to_evt(events[i].events);
        inst->evt.revents = evts[i].revents;
    }

    return evts;
}

/**
 * Perform @c epoll_wait() call.
 *
 * @param iomux     The multiplexer handle.
 * @param timeout   Timeout to block in the call in milliseconds.
 * @param revts     Returned events.
 *
 * @return Events number.
 */
static int
tapi_iomux_epoll_call(tapi_iomux_handle *iomux, int timeout,
                       tapi_iomux_evt_fd **revts)
{
    int rc;

    if (iomux->rpcs->op != RCF_RPC_WAIT)
        iomux->epoll.events = tapi_calloc(iomux->fds_num,
                                          sizeof(*iomux->epoll.events));

    if (iomux->type == TAPI_IOMUX_EPOLL)
    {
        rc = rpc_epoll_wait(iomux->rpcs, iomux->epoll.epfd,
                            iomux->epoll.events, iomux->fds_num, timeout);
    }
    else
    {
        rc = rpc_epoll_pwait(iomux->rpcs, iomux->epoll.epfd,
                             iomux->epoll.events, iomux->fds_num, timeout,
                             iomux->sigmask);
    }

    if (iomux->rpcs->op == RCF_RPC_WAIT)
        return rc;

    if (rc > 0)
        *revts = tapi_iomux_epoll_get_events(iomux, iomux->epoll.events,
                                             rc);
    free(iomux->epoll.events);

    return rc;
}

/**
 * @c epoll_wait and @c epoll_pwait API methods.
 */
static const tapi_iomux_methods tapi_iomux_methods_epoll =
    {
        .create  = tapi_iomux_epoll_create,
        .add     = tapi_iomux_epoll_add,
        .mod     = tapi_iomux_epoll_mod,
        .del     = tapi_iomux_epoll_del,
        .call    = tapi_iomux_epoll_call,
        .destroy = tapi_iomux_epoll_destroy,
    };

/**
 * Supported iomux methods array.
 */
static const tapi_iomux_methods * tapi_iomux_methods_all[] =
    {
        [TAPI_IOMUX_SELECT]      = &tapi_iomux_methods_select,
        [TAPI_IOMUX_PSELECT]     = &tapi_iomux_methods_pselect,
        [TAPI_IOMUX_POLL]        = &tapi_iomux_methods_poll,
        [TAPI_IOMUX_PPOLL]       = &tapi_iomux_methods_ppoll,
        [TAPI_IOMUX_EPOLL]       = &tapi_iomux_methods_epoll,
        [TAPI_IOMUX_EPOLL_PWAIT] = &tapi_iomux_methods_epoll,
    };

/* See the description in tapi_iomux.h. */
tapi_iomux_handle *
tapi_iomux_create(rcf_rpc_server *rpcs, tapi_iomux_type type)
{
    tapi_iomux_handle *iomux;

    if (type < TAPI_IOMUX_MIN || type > TAPI_IOMUX_MAX)
        TEST_FAIL("Unknown multiplexer type: %d", type);

    iomux = tapi_calloc(1, sizeof(*iomux));
    iomux->type = type;
    iomux->rpcs = rpcs;
    iomux->methods = tapi_iomux_methods_all[type];
    SLIST_INIT(&iomux->evts);

    if (iomux->methods->create != NULL)
        iomux->methods->create(iomux);

    return iomux;
}

/* See the description in tapi_iomux.h. */
void
tapi_iomux_destroy(tapi_iomux_handle *iomux)
{
    tapi_iomux_evts_list *inst;

    if (iomux == NULL)
        return;

    if (iomux->methods->destroy != NULL)
        iomux->methods->destroy(iomux);

    while ((inst = SLIST_FIRST(&iomux->evts)) != NULL)
    {
        SLIST_REMOVE_HEAD(&iomux->evts, tapi_iomux_evts_ent);
        free(inst);
    }

    free(iomux->revts);
    free(iomux);
}

/* See the description in tapi_iomux.h. */
void
tapi_iomux_add(tapi_iomux_handle *iomux, int fd, tapi_iomux_evt evt)
{
    tapi_iomux_evts_list *inst = tapi_calloc(1, sizeof(*inst));

    if (iomux->methods->add != NULL)
        iomux->methods->add(iomux, fd, evt);

    iomux->fds_num++;

    inst->evt.fd = fd;
    inst->evt.events = evt;
    SLIST_INSERT_HEAD(&iomux->evts, inst, tapi_iomux_evts_ent);
}

/* See the description in tapi_iomux.h. */
void
tapi_iomux_mod(tapi_iomux_handle *iomux, int fd, tapi_iomux_evt evt)
{
    tapi_iomux_evts_list *inst = tapi_iomux_get_evt_by_fd(iomux, fd);

    if (iomux->methods->mod != NULL)
        iomux->methods->mod(iomux, fd, evt);

    inst->evt.events = evt;
}

/* See the description in tapi_iomux.h. */
void
tapi_iomux_del(tapi_iomux_handle *iomux, int fd)
{
    tapi_iomux_evts_list *inst;
    if (iomux->methods->del != NULL)
        iomux->methods->del(iomux, fd);

    iomux->fds_num--;
    inst = tapi_iomux_get_evt_by_fd(iomux, fd);
    SLIST_REMOVE(&iomux->evts, inst, tapi_iomux_evts_list,
                 tapi_iomux_evts_ent);
    free(inst);
}

/* See the description in tapi_iomux.h. */
int
tapi_iomux_call(tapi_iomux_handle *iomux, int timeout,
                tapi_iomux_evt_fd **revts)
{
    int rc;

    if (iomux->methods->call != NULL)
    {
        free(iomux->revts);
        iomux->revts = NULL;

        rc = iomux->methods->call(iomux, timeout, &iomux->revts);
        if (revts != NULL)
            *revts = iomux->revts;
        return rc;
    }

    return 0;
}

/* See the description in tapi_iomux.h. */
void
tapi_iomux_set_sigmask(tapi_iomux_handle *iomux, rpc_sigset_p sigmask)
{
    iomux->sigmask = sigmask;
}

/* See the description in tapi_iomux.h. */
int
tapi_iomux_pcall(tapi_iomux_handle *iomux, int timeout,
                 rpc_sigset_p sigmask, tapi_iomux_evt_fd **revts)
{
    tapi_iomux_set_sigmask(iomux, sigmask);
    return tapi_iomux_call(iomux, timeout, revts);
}
