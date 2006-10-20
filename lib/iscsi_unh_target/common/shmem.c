/** @file
 * @brief iSCSI target
 *
 * Shared memory supporting routines
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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include "debug.h"
#include "mutex.h"
#include "my_memory.h"

#define SHARED_MEM_GUARDIAN 0xC0D1F1ED

typedef struct reserved_block {
    uint32_t guardian;
    SHARED struct reserved_block *chain;
    size_t size;
} reserved_block;

static reserved_block the_block_is_occupied;

typedef struct master_block_record {
    SHARED void *free_area;
    SHARED reserved_block *reserved_list;
} master_block_record;

static int shared_mem_id;
static int shared_mem_lock;
static pid_t creator_pid;
static SHARED master_block_record *master_block;
static SHARED void *block_end;

static void
shared_mem_finish(void)
{
    shmdt((void *)master_block);
    if (getpid() == creator_pid)
    {
        shmctl(shared_mem_id, IPC_RMID, NULL);
        semctl(shared_mem_lock, 0, IPC_RMID);
    }
}

/* this is used by mutex.c too */
te_bool shared_memory_cleanup_was_done;

te_errno
shared_mem_init(size_t size)
{
    int   rc = 0;
    key_t sh_key = ftok("/tmp", 'S');
    key_t shlock_key = ftok("/tmp", 'l');

    shared_mem_id = shmget(sh_key, size, IPC_CREAT | IPC_EXCL | S_IREAD | S_IWRITE); 
    if (shared_mem_id < 0)
    {
        if (errno == EEXIST)
        {
            int tmp_id;
            struct shmid_ds buf;

            WARN("Stale shared memory detected, cleaning up");
            tmp_id = shmget(sh_key, size, S_IREAD | S_IWRITE);

            if ((tmp_id < 0) || (shmctl(tmp_id, IPC_STAT, &buf) != 0))
            {
                rc = TE_OS_RC(TE_ISCSI_TARGET, errno);
                TRACE_ERROR("Cannot cleanup stale shared memory: %r", rc);
                return rc;
            }
            if (kill(buf.shm_cpid, 0) == 0)
            {
                TRACE_ERROR("Non-stale shared memory found owned by %u",
                            (unsigned)buf.shm_cpid);
                return TE_RC(TE_ISCSI_TARGET, TE_EEXIST);
            }
            if (shmctl(tmp_id, IPC_RMID, NULL) != 0)
            {
                TRACE_ERROR("Unable to cleanup stale shared memory: %r", rc);
                return rc;
            }
            tmp_id = semget(shlock_key, 1, S_IREAD | S_IWRITE);
            if (tmp_id >= 0)
            {
                if (semctl(tmp_id, 0, IPC_RMID) != 0)
                {
                    rc = TE_OS_RC(TE_ISCSI_TARGET, errno);
                    TRACE_ERROR("Cannot cleanup stale shared memory lock: %r", rc);
                    return rc;
                }
            }
            shared_memory_cleanup_was_done = TRUE;
            /* recursive call to retry */
            return shared_mem_init(size);
        }
        else
        {
            rc = TE_OS_RC(TE_ISCSI_TARGET, errno);
            TRACE_ERROR("Cannot get a shared memory segment: %r", rc);
            return rc;
        }
    }

    shared_mem_lock = semget(shlock_key, 1, IPC_CREAT | IPC_EXCL | S_IREAD | S_IWRITE);
    if (shared_mem_lock < 0)
    {
        rc = TE_OS_RC(TE_ISCSI_TARGET, errno);
        TRACE_ERROR("Cannot get a shared memory lock: %r", rc);
        return rc;
    }
    semctl(shared_mem_lock, 0, SETVAL, 1);

    master_block = shmat(shared_mem_id, NULL, 0);
    if (master_block == NULL)
    {
        rc = TE_OS_RC(TE_ISCSI_TARGET, errno);
        TRACE_ERROR("Cannot attach a shared memory segment: %r", rc);
        shmctl(shared_mem_id, IPC_RMID, NULL);
        semctl(shared_mem_lock, 0, IPC_RMID);
        return TE_OS_RC(TE_ISCSI_TARGET, rc);
    }
    block_end = (char *)master_block + size;
    master_block->free_area = (char *)master_block + 
        ((sizeof(master_block) + sizeof(double) - 1) / sizeof(double) * sizeof(double));
    master_block->reserved_list = NULL;

    creator_pid = getpid();
    atexit(shared_mem_finish);

    return 0;
}

te_bool
is_shared_ptr(SHARED void *addr)
{
    return (addr >= (SHARED void *)master_block && 
            addr < master_block->free_area);
}

size_t
get_avail_shared_mem(void)
{
    return (char *)block_end - (char *)master_block->free_area;
}

SHARED void *
shalloc(size_t size)
{
    SHARED void *result;
    struct sembuf op;

    size = (size + sizeof(double) - 1) / sizeof(double) * sizeof(double);
    size += sizeof(reserved_block);
    size = (size + sizeof(double) - 1) / sizeof(double) * sizeof(double);

    TRACE(DEBUG_MEMORY, 
          "Allocating shared memory chunk %u, available %u (%p:%p)", 
          (unsigned)size,
          (unsigned)((char *)block_end - (char *)master_block->free_area),
          __builtin_return_address(0),
          __builtin_return_address(1));
    
    op.sem_num = 0;
    op.sem_op  = -1;
    op.sem_flg = 0;
    if (semop(shared_mem_lock, &op, 1) != 0)
    {
        int rc = errno;
        TRACE_ERROR("%s(): Aiye! Unable to lock a shared memory lock: %r",
                    __FUNCTION__,
                    TE_OS_RC(TE_ISCSI_TARGET, rc));
        errno = rc;
        return NULL;
    }

    if ((char *)master_block->free_area + size >= (char *)block_end)
    {
        SHARED reserved_block *free_block;
        SHARED reserved_block * SHARED *prev_ptr = &master_block->reserved_list;

        for (free_block = master_block->reserved_list; 
             free_block != NULL;
             prev_ptr = &free_block->chain, free_block = free_block->chain)
        {
            if (free_block->size >= size)
            {
                TRACE(DEBUG_MEMORY, "Taking from reserved list %p", free_block);
                *prev_ptr = free_block->chain;
                free_block->chain = &the_block_is_occupied;
                result = free_block;
                break;
            }
        }

        if (free_block == NULL)
        {
            op.sem_num = 0;
            op.sem_op  = 1;
            op.sem_flg = 0;
            semop(shared_mem_lock, &op, 1);
            
            TRACE_ERROR("%s(): Cannot allocate a shared memory block of size %u",
                        __FUNCTION__, (unsigned)size);
            errno = ENOMEM;
            return NULL;
        }
    }
    else
    {
        ((reserved_block *)master_block->free_area)->guardian  = SHARED_MEM_GUARDIAN;
        ((reserved_block *)master_block->free_area)->chain = &the_block_is_occupied;
        ((reserved_block *)master_block->free_area)->size  = size;
        result = (char *)master_block->free_area;
        master_block->free_area = (char *)master_block->free_area + size;

    }
    result += (sizeof(reserved_block) + sizeof(double) - 1) / sizeof(double) * sizeof(double);

    op.sem_num = 0;
    op.sem_op  = 1;
    op.sem_flg = 0;
    semop(shared_mem_lock, &op, 1);

    TRACE(DEBUG_MEMORY, "Allocated %p at %p", result, 
          __builtin_return_address(0));

    return result;
}

te_errno
shfree(SHARED void *addr)
{
    SHARED reserved_block *block;
    SHARED reserved_block * SHARED *prev_ptr;
    struct sembuf op;

    TRACE(DEBUG_MEMORY, "Freeing %p at %p", addr,
          __builtin_return_address(0));

    if (addr == NULL)
        return 0;

    if (addr < (void *)master_block || addr >= (void *)block_end)
    {
        TRACE_ERROR("%p is not a shared address (%p)", addr, 
                    __builtin_return_address(0));
        free((void *)addr);
        return TE_RC(TE_ISCSI_TARGET, TE_EINVAL);
    }
    block = (void *)((char *)addr - 
        (sizeof(reserved_block) + sizeof(double) - 1) / sizeof(double) * sizeof(double));

    if (block->chain != &the_block_is_occupied || 
        block->guardian != SHARED_MEM_GUARDIAN)
    {
        TRACE_ERROR("%p is not shalloc-allocated block", addr);
        return TE_RC(TE_ISCSI_TARGET, TE_EINVAL);
    }

    op.sem_num = 0;
    op.sem_op  = -1;
    op.sem_flg = 0;
    if (semop(shared_mem_lock, &op, 1) != 0)
    {
        int rc = TE_OS_RC(TE_ISCSI_TARGET, errno);
        TRACE_ERROR("%s(): Aiye! Unable to lock a shared memory lock: %r", 
                    "memory not freed", __FUNCTION__, rc);
        return rc;
    }
    
    block->chain = master_block->reserved_list;
    prev_ptr = &master_block->reserved_list;
    master_block->reserved_list = block;
        
    for (; block != NULL; block = block->chain)
    {
        if (block->guardian != SHARED_MEM_GUARDIAN)
        {
            TRACE_ERROR("Aiye! Shared memory heap is corrupted!!!");
            break;
        }
        if ((char *)block + block->size == master_block->free_area)
        {
            TRACE(DEBUG_MEMORY, "Popping %u memory", (unsigned)block->size);
            master_block->free_area = block;
            *prev_ptr = block->chain;
        }
        else
        {
            prev_ptr = &block->chain;
        }
    }

    op.sem_num = 0;
    op.sem_op  = 1;
    op.sem_flg = 0;
    semop(shared_mem_lock, &op, 1);

    TRACE(DEBUG_MEMORY, "After freeing available %u",
          (unsigned)((char *)block_end - (char *)master_block->free_area));

    return 0;
}

SHARED void *
shcalloc(size_t nmemb, size_t size)
{
    SHARED void *addr = shalloc(nmemb * size);
    
    if (addr != NULL)
        memset((void *)addr, 0, nmemb * size);
    return addr;
}
