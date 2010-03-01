/** @file
 * @brief ACSE 
 *
 * ACSE EPC messaging support library
 *
 *
 * Copyright (C) 2010 Test Environment authors (see file AUTHORS
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
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"


#include <string.h>
#include <ctype.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h> 
#include <fcntl.h>

#include "acse_epc.h"

#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "logger_api.h"

#include "cwmp_data.h"

static int epc_socket = -1;
static acse_epc_role_t epc_role = ACSE_EPC_SERVER;

static void *epc_shmem = NULL;
static const char *epc_shmem_name = NULL;
static size_t epc_shmem_size = 0;

/**
 * Create unix socket, bind it and connect it to another one if specified.
 *
 * @param unix_path     Unnix path to bind to.
 * @param connect_to    Unix path to connect to or
 *                      NULL if no connection is needed.
 *
 * @return              Socket or -1 in case of an error
 */
static int
unix_socket(char const *unix_path, char const *connect_to)
{
    struct sockaddr_un addr = { .sun_family = AF_UNIX };
    int s = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    int saved_errno;

    if (s == -1)
        return -1;

    if (strlen(unix_path) >= sizeof addr.sun_path)
    {
        close(s);
        errno = ENAMETOOLONG;
        return -1;
    }

    strcpy(addr.sun_path, unix_path);

    if (bind(s, (struct sockaddr *)&addr, sizeof addr) != -1)
    {
        if (connect_to == NULL)
            return s;

        if (strlen(connect_to) < sizeof addr.sun_path)
        {
            memset(addr.sun_path, 0, sizeof addr.sun_path);
            strcpy(addr.sun_path, connect_to);

            if (connect(s, (struct sockaddr *)&addr, sizeof addr) != -1)
              return s;
        }
        else
            errno = ENAMETOOLONG;
    }

    saved_errno = errno;
    close(s);
    errno = saved_errno;

    return -1;
}

/**
 * Create/open shared memory object and perform mapping for it.
 *
 * @param create        Whether to create rather than open a memory object
 * @param size          Desired size that will be rounded up on success
 * @param shmem_name    Name of shared memory area.
 *
 * @return              Address of allocated memory
 *                      or NULL in case of an error
 */
static void *
shared_mem(te_bool create, size_t *size, const char *shmem_name)
{
    void *shm_ptr = NULL;
    int   shm_fd;

    if (create)
        shm_unlink(shmem_name);

    if ((shm_fd = shm_open(shmem_name,
                           (create ? (O_CREAT | O_EXCL) : 0) | O_RDWR,
                           S_IRWXU | S_IRWXG | S_IRWXO)) != -1)
    {
        int  saved_errno;
        long sz = sysconf(_SC_PAGESIZE);

        if (sz != -1)
        {
            if (sz > 0)
            {
                sz = (*size / sz + (*size % sz ? 1 : 0)) * sz;

                if (ftruncate(shm_fd, sz) != -1 &&
                    (shm_ptr = mmap(NULL, sz, PROT_READ | PROT_WRITE,
                                    MAP_SHARED, shm_fd, 0)) != MAP_FAILED)
                {
                    *size = sz;
                }
                else
                    shm_ptr = NULL;
            }
            else
                errno = EINVAL;
        }

        saved_errno = errno;
        close(shm_fd);

        if (shm_ptr == NULL && create)
            shm_unlink(shmem_name);

        errno = saved_errno;
    }

    return shm_ptr;
}


te_errno
acse_epc_open(const char *msg_sock_name, const char *shmem_name,
                              acse_epc_role_t role)
{
    const char *local_sock_name;
    const char *remote_sock_name;
    epc_role = role;

    switch(role)
    {
        case ACSE_EPC_SERVER:
            local_sock_name = msg_sock_name;
            remote_sock_name = NULL;
            break;
        case ACSE_EPC_CLIENT:
            local_sock_name = mktemp("/tmp/acse_epc_XXXXXX");
            remote_sock_name = msg_sock_name;
            break;
        default:
            return TE_RC(TE_ACSE, TE_EINVAL);
    }
    if ((epc_socket = unix_socket(local_sock_name, remote_sock_name))
            == -1)
        return TE_RC(TE_ACSE, errno);

    epc_shmem_size = 32 * 1024;
    epc_shmem = shared_mem(role == ACSE_EPC_SERVER,
                           &epc_shmem_size, shmem_name);
    if (epc_shmem == NULL)
    {
        int saved_errno = errno;
        close(epc_socket);
        epc_socket = -1;
        return TE_RC(TE_ACSE, saved_errno);
    }
    epc_shmem_name = strdup(shmem_name);
    RING("%s(): shmem addr: %p, socket %d", __FUNCTION__,
            epc_shmem, epc_socket);
    return 0;
}

int
acse_epc_sock(void)
{
    return epc_socket;
}

void*
acse_epc_shmem(void)
{
    return epc_shmem;
}

te_errno
acse_epc_close(void)
{
    close(epc_socket); epc_socket = -1;
    shm_unlink(epc_shmem_name);
    free((char *)epc_shmem_name); epc_shmem_name = NULL;
    return 0;
}

static ssize_t
epc_pack_data(char *buf, acse_epc_cwmp_data_t *cwmp_data)
{
    switch
}

te_errno
acse_epc_send(const acse_epc_msg_t *user_message)
{
    ssize_t sendrc;

    if (user_message == NULL || user_message->data == NULL)
        return TE_EINVAL;
    if (epc_socket == -1)
    {
        ERROR("Try send, but EPC is not initialized");
        return TE_EBADFD;
    }
    
    /* check consistance of opcode and role */
    switch (user_message->opcode)
    {
    case EPC_CONFIG_CALL:
    case EPC_CWMP_CALL:
        if (epc_role != ACSE_EPC_CLIENT)
        {
            ERROR("Try call EPC function, but role is not CLIENT!");
            return TE_EINVAL;
        }
        break;
    case EPC_CONFIG_RESPONSE:
    case EPC_CWMP_RESPONSE:
        if (epc_role != ACSE_EPC_SERVER)
        {
            ERROR("Try response EPC function, but role is not SERVER!");
            return TE_EINVAL;
        }
        break;
    default:
        ERROR("Try send EPC with wrong opcode!");
        return TE_EINVAL;
        
    }

    /* Now prepare data */
    switch (user_message->opcode)
    {
    case EPC_CONFIG_CALL:
    case EPC_CONFIG_RESPONSE:
        acse_epc_config_data_t *cfg_data = user_message->data;
        memcpy(epc_shmem, cfg_data, sizeof(*cfg_data));
        sendrc = send(epc_socket, user_message, sizeof(*user_message), 0);
        if (sendrc < 0)
        {
            ERROR("%s(): send failed %r", __FUNCTION__, errno);
            return TE_RC(TE_ACSE, errno);
        }
        break;
    case EPC_CWMP_CALL:
    case EPC_CWMP_RESPONSE:
        acse_epc_cwmp_data_t *cwmp_data = user_message->data;
        break;
    }
    return 0;
}

te_errno
acse_epc_recv(acse_epc_msg_t **user_message)
{
    return 0;
}
