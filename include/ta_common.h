/** @file
 * @brief TA Common Definitions 
 *
 * Prototypes of functions to be implemented on all Test Agents.
 * 
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * 
 * $Id$
 */

#ifndef __TE_TA_COMMON_H__
#define __TE_TA_COMMON_H__

/** 
 * Get OS identifier of the current thread. 
 *
 * @return Thread identifier
 */
extern uint32_t thread_self(void);

/*-- These mutexes should be used in forked processes rather than on TA --*/

/** 
 * Create a mutex.
 *
 * @return Mutex handle
 */
extern void *thread_mutex_create(void);

/** 
 * Destroy a mutex.
 *
 * @param mutex     mutex handle
 */
extern void thread_mutex_destroy(void *mutex);

/** 
 * Lock the mutex.
 *
 * @param mutex     mutex handle
 */
extern void thread_mutex_lock(void *mutex);

/** 
 * Unlock the mutex.
 *
 * @param mutex     mutex handle
 */
extern void thread_mutex_unlock(void *mutex);

#endif /* !__TE_TA_COMMON_H__ */
