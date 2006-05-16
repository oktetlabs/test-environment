/** @file
 * @brief TE tools
 *
 * Functions for different delays.
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_SLEEP_H__
#define __TE_SLEEP_H__

/**
 * Sleep specified number of seconds.
 *
 * @param to_sleep      number of seconds to sleep
 */
static inline void 
te_sleep(unsigned int to_sleep)
{
    RING("Sleeping %u seconds", to_sleep); 
    (void)sleep(to_sleep);
}

/**
 * Sleep specified number of milliseconds.
 *
 * @param to_sleep      number of milliseconds to sleep
 */
static inline void
te_msleep(unsigned int to_sleep)
{
    RING("Sleeping %u milliseconds", to_sleep); 
    (void)usleep(to_sleep * 1000); 
}

/**
 * Sleep specified number of microseconds.
 *
 * @param to_sleep      number of milliseconds to sleep
 */
static inline void
te_usleep(unsigned int to_sleep)
{
    RING("Sleeping %u microseconds", to_sleep); 
    (void)usleep(to_sleep); 
}

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_SLEEP_H__ */
