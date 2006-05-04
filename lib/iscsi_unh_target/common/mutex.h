/** @file
 * @brief Inter-process mutexes
 *
 * Inter-process mutexes
 *
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
 * @author Artem Andreev <Artem.Andreev@oktetlabs.ru>
 *
 * $Id: te_defs.h 25602 2006-03-23 06:52:54Z arybchik $
 */

#ifndef __TE_MUTEX_H__
#define __TE_MUTEX_H__

#include "te_config.h"
#include "te_defs.h"
#include "te_errno.h"

typedef int ipc_mutex_t;
typedef int ipc_sem_t;

extern te_errno ipc_mutexes_init(int nsems);

extern ipc_mutex_t ipc_mutex_alloc(void);
extern int ipc_mutex_free(ipc_mutex_t mutex);
extern int ipc_mutex_lock(ipc_mutex_t mutex);
extern int ipc_mutex_unlock(ipc_mutex_t mutex);

extern ipc_sem_t ipc_sem_alloc(int initial_value);
extern int ipc_sem_free(ipc_sem_t sem);
extern int ipc_sem_post(ipc_sem_t sem);
extern int ipc_sem_wait(ipc_sem_t sem);

#endif /* __TE_MUTEX_H__ */
