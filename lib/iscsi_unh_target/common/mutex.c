/** @file
 * @brief iSCSI target
 *
 * IPC mutexes and semaphores
 *
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
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
 * @author Artem V. Andreev <Artem.Andreev@oktetlabs.ru>
 *
 * $Id$
 */


#include "te_config.h"
#include "te_defs.h"
#include "te_errno.h"
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <unistd.h>
#include "debug.h"
#include "mutex.h"


#define MAX_MUTEXES 250
#define MUTEX_FREE_BIT (1 << 14)
#define MUTEX_FREE_VALUE ((MUTEX_FREE_BIT << 1) - 1)


static int no_of_mutexes;
static int master_sem_id = -1;
static pid_t creator_pid;

static unsigned short sem_values_buf[MAX_MUTEXES];
static int local_locks[MAX_MUTEXES];

static void
remove_semaphore(void)
{
    if (getpid() == creator_pid)
    {
        semctl(master_sem_id, 0, IPC_RMID);
    }
}

extern te_bool shared_memory_cleanup_was_done;

te_errno
ipc_mutexes_init(int nsems)
{
    int rc;
    ipc_mutex_t tmp = 0;
    int i;
    key_t mutex_key = ftok("/tmp", 'L');

    if (nsems >= MAX_MUTEXES)
    {
        WARN("Too many mutexes for ipc_mutexes_init()");
        nsems = MAX_MUTEXES - 1;
    }

    no_of_mutexes = nsems;
    master_sem_id = semget(mutex_key, nsems + 1, IPC_CREAT | IPC_EXCL | S_IREAD | S_IWRITE);
    if (master_sem_id < 0)
    {
        if (shared_memory_cleanup_was_done && errno == EEXIST)
        {
            int tmp_id;

            WARN("Stale master semaphore detected, cleaning up");
            tmp_id = semget(mutex_key, nsems + 1, S_IREAD | S_IWRITE);
            if ((tmp_id < 0) || (semctl(tmp_id, 0, IPC_RMID) != 0))
            {
                rc = TE_OS_RC(TE_ISCSI_TARGET, errno);
                ERROR("Cannot cleanup stale master semaphore: %r", rc);
                return rc;
            }
            return ipc_mutexes_init(nsems);
        }
        
        rc = TE_OS_RC(TE_ISCSI_TARGET, errno);
        ERROR("Cannot allocate master semaphore: %r", rc);
        return rc;
    }
    sem_values_buf[0] = 1;
    for (i = 1; i <= nsems; i++)
    {
        sem_values_buf[i] = MUTEX_FREE_VALUE;
    }
    if (semctl(master_sem_id, 0, SETALL, sem_values_buf) != 0)
    {
        rc = TE_OS_RC(TE_ISCSI_TARGET, errno);
        ERROR("Cannot initialize mutexes: %r", rc);
        semctl(master_sem_id, 0, IPC_RMID);
        return rc;
    }
    creator_pid = getpid();
    atexit(remove_semaphore);
    return 0;
}

ipc_mutex_t
ipc_mutex_alloc(void)
{
    int rc = 0;
    int i = -1;
    struct sembuf op;
    
    op.sem_num = 0;
    op.sem_op  = -1;
    op.sem_flg = 0;
    
    if (semop(master_sem_id, &op, 1) != 0)
    {
        rc = errno;
        ERROR("Cannot alloc mutex: %s", strerror(errno));
        errno = rc;
        return -1;
    }
    if (semctl(master_sem_id, 0, GETALL, &sem_values_buf) != 0)
    {
        rc = errno;
        ERROR("Cannot get free mutex info: %s", strerror(errno));
    }
    else
    {
        for (i = 1; i <= no_of_mutexes; i++)
        {
            if ((sem_values_buf[i] & MUTEX_FREE_BIT) != 0)
                break;
        }
        if (i > no_of_mutexes)
        {
            ERROR("No more free mutexes!!!");
            rc = ENOSPC;
        }
        else
        {
            RING("Allocated mutex id %d: %p", i - 1,
                 __builtin_return_address(0));
            if (semctl(master_sem_id, i, SETVAL, 1) != 0)
            {
                rc = errno;
                ERROR("Cannot initialize mutex %d: %s", i, 
                      strerror(errno));
            }
            local_locks[i] = 0;
        }
    }
    op.sem_num = 0;
    op.sem_op  = 1;
    op.sem_flg = 0;
    semop(master_sem_id, &op, 1);

    errno = rc;
    return rc == 0 ? i - 1 : -1;
}

int
ipc_mutex_free(ipc_mutex_t mutex)
{
    int rc;

    RING("Freeing mutex %d: %p", mutex, __builtin_return_address(0));
    if (semctl(master_sem_id, mutex + 1, SETVAL, MUTEX_FREE_VALUE) != 0)
    {
        rc = errno;
        ERROR("Unable to free mutex %d from %p: %s", mutex, 
              __builtin_return_address(0),
              strerror(errno));
        errno = rc;
        return -1;
    }
    local_locks[mutex + 1] = 0;
    return 0;
}

int
ipc_mutex_lock(ipc_mutex_t mutex)
{
    struct sembuf op;

    if (mutex < 0 || mutex >= no_of_mutexes)
    {
        ERROR("%s(): Invalid mutex id %d at %p", 
              __FUNCTION__,
              mutex,
              __builtin_return_address(0));
        errno = EINVAL;
        return -1;
    }

    if (local_locks[mutex + 1] != 0)
    {
        local_locks[mutex + 1]++;
        return 0;
    }

    for (;;)
    {
        op.sem_num = mutex + 1;
        op.sem_op  = -1;
        op.sem_flg = SEM_UNDO;
        if (semop(master_sem_id, &op, 1) == 0)
            break;
        else if (errno != EINTR)
            return -1;
    }
    local_locks[mutex + 1] = 1;
    return 0;
}

int
ipc_mutex_unlock(ipc_mutex_t mutex)
{
    if (mutex < 0 || mutex >= no_of_mutexes)
    {
        ERROR("%s(): Invalid mutex id %d at %p", 
              __FUNCTION__,
              mutex, 
              __builtin_return_address(0));
        errno = EINVAL;
        return -1;
    }

    if (local_locks[mutex + 1] == 0)
    {
        ERROR("Mutex %d has not been locked by %d",
              mutex, getpid());
        errno = EINVAL;
        return -1;
    }
    
    if (--local_locks[mutex + 1] != 0)
        return 0;
    else
    {
        struct sembuf op;
        
        op.sem_num = mutex + 1;
        op.sem_op  = 1;
        op.sem_flg = 0;
        return semop(master_sem_id, &op, 1);
    }
}

ipc_sem_t
ipc_sem_alloc(int initial_value)
{
    ipc_mutex_t mutex = ipc_mutex_alloc();
    
    if (mutex < 0)
        return -1;
    if (semctl(master_sem_id, mutex + 1, SETVAL, initial_value) != 0)
    {
        int rc = errno;
        ipc_mutex_free(mutex);
        errno = rc;
        return -1;
    }
    RING("Allocated semaphore %d at %p", mutex, __builtin_return_address(0));
    return (ipc_sem_t)mutex;
}

int
ipc_sem_free(ipc_sem_t sem)
{
    RING("Freeing semaphore %d at %p", sem, __builtin_return_address(0));
    if (semctl(master_sem_id, sem + 1, SETVAL, 0) != 0)
    {
        WARN("Unable to wake semaphore %d: %s", sem, 
             strerror(errno));
    }
    return ipc_mutex_free((ipc_mutex_t)sem);
}

int
ipc_sem_wait(ipc_sem_t sem)
{
    struct sembuf op;
    
    for (;;)
    {
        op.sem_num = sem + 1;
        op.sem_op  = -1;
        op.sem_flg = 0;
        if (semop(master_sem_id, &op, 1) == 0)
            break;
        else if (errno != EINTR)
            return -1;
    }
    return 0;
}

int
ipc_sem_post(ipc_sem_t sem)
{
    struct sembuf op;
    
    op.sem_num = sem + 1;
    op.sem_op  = 1;
    op.sem_flg = 0;
    return semop(master_sem_id, &op, 1);

}

