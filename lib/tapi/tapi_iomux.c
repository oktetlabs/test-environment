/** @file
 * @brief Implementation of I/O multiplexers test API
 *
 * Functions implementation.
 *
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
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
 *
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 */

#define TE_LGR_USER     "Multiplexers TAPI"

#include "te_config.h"
#include "tapi_iomux.h"

/* See the description in tapi_iomux.h. */
short int
tapi_iomux_evt_to_poll(uint16_t iomux_evt_mask)
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
short int
tapi_iomux_evt_to_epoll(uint16_t iomux_evt_mask)
{
    short int poll_evts = 0;

    if (iomux_evt_mask & EVT_RD)
        poll_evts |= RPC_EPOLLIN;

    if (iomux_evt_mask & EVT_PRI)
        poll_evts |= RPC_EPOLLPRI;

    if (iomux_evt_mask & EVT_WR)
        poll_evts |= RPC_EPOLLOUT;

    if (iomux_evt_mask & EVT_RD_NORM)
        poll_evts |= RPC_EPOLLRDNORM;

    if (iomux_evt_mask & EVT_WR_NORM)
        poll_evts |= RPC_EPOLLWRNORM;

    if (iomux_evt_mask & EVT_RD_BAND)
    {
        poll_evts |= RPC_EPOLLPRI;
        poll_evts |= RPC_EPOLLRDBAND;
    }

    if (iomux_evt_mask & EVT_WR_BAND)
    {
        poll_evts |= RPC_EPOLLWRBAND;
    }

    if (iomux_evt_mask & EVT_EXC)
    {
        poll_evts |= RPC_EPOLLERR;
        poll_evts |= RPC_EPOLLHUP;
        poll_evts |= RPC_EPOLLMSG;
    }

    if (iomux_evt_mask & EVT_ERR)
        poll_evts |= RPC_EPOLLERR;

    if (iomux_evt_mask & EVT_HUP)
        poll_evts |= RPC_EPOLLHUP;

    if (iomux_evt_mask & EVT_NVAL)
        poll_evts |= RPC_EPOLLMSG;

    if (iomux_evt_mask & EVT_RDHUP)
        poll_evts |= RPC_EPOLLRDHUP;

    return poll_evts;
}

/* See the description in tapi_iomux.h. */
uint16_t
tapi_iomux_poll_to_evt(short int poll_evt_mask)
{
    uint16_t iomux_evts = 0;

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
uint16_t
tapi_iomux_epoll_to_evt(short int poll_evt_mask)
{
    uint16_t iomux_evts = 0;

    if (poll_evt_mask & RPC_EPOLLIN)
        iomux_evts |= EVT_RD;

    if (poll_evt_mask & RPC_EPOLLOUT)
        iomux_evts |= EVT_WR;

    if (poll_evt_mask & RPC_EPOLLRDNORM)
    {
        iomux_evts |= EVT_RD_NORM;
        iomux_evts |= EVT_RD;
    }

    if (poll_evt_mask & RPC_EPOLLWRNORM)
    {
        iomux_evts |= EVT_WR_NORM;
        iomux_evts |= EVT_WR;
    }

    if (poll_evt_mask & RPC_EPOLLRDBAND)
        iomux_evts |= EVT_RD_BAND;

    if (poll_evt_mask & RPC_EPOLLWRBAND)
        iomux_evts |= EVT_WR_BAND;

    if (poll_evt_mask & RPC_EPOLLPRI)
        iomux_evts |= EVT_PRI;

    if (poll_evt_mask & RPC_EPOLLERR)
        iomux_evts |= (EVT_EXC | EVT_ERR);

    if (poll_evt_mask & RPC_EPOLLHUP)
        iomux_evts |= (EVT_EXC | EVT_HUP);

    if (poll_evt_mask & RPC_EPOLLRDHUP)
        iomux_evts |= EVT_RDHUP;

    if (poll_evt_mask & RPC_EPOLLMSG)
        iomux_evts |= (EVT_EXC | EVT_NVAL);

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
