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

#define TE_LGR_USER     "ACSE EPC messaging"

#include "te_config.h"


#include <string.h>
#include <stdlib.h>
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


#define EPC_MMAP_AREA "/epc_mmap_area"
#define EPC_ACSE_SOCK "/tmp/epc_acse_sock"


static int epc_socket = -1;
static acse_epc_role_t epc_role = ACSE_EPC_SERVER;

void *epc_shmem = NULL;
const char *epc_shmem_name = NULL;
size_t epc_shmem_size = 0;


const char *local_sock_name = NULL;
const char *remote_sock_name = NULL;

/**
 * Create unix socket, bind it and connect it to another one if specified.
 * If connection is need, but occasionaly is refused, this function tryes
 * to wait a bit and connect once more. 
 *
 * @param unix_path     Unix path to bind to.
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

    VERB("%s(): local path '%s', connect to '%s'",
         __FUNCTION__, unix_path, connect_to);

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
            else /* if (errno == ECONNREFUSED) */
            {
                struct timeval tv = {0, 30000}; /* wait 30 ms */
                select(0, NULL, NULL, NULL, &tv);
                if (connect(s, (struct sockaddr *)&addr, sizeof addr) 
                    != -1)
                    return s;
            }
            ERROR("%s(): connect '%s' to '%s' failed, OS errno %s",
                    __FUNCTION__, unix_path, connect_to,
                    strerror(errno));
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
    else
        perror("shm open failed");

    return shm_ptr;
}


/* see description in acse_epc.h */
te_errno
acse_epc_open(const char *msg_sock_name, const char *shmem_name,
                              acse_epc_role_t role)
{
    epc_role = role;

    if (msg_sock_name == NULL)
        msg_sock_name = strdup(EPC_ACSE_SOCK);

    if (shmem_name == NULL)
        shmem_name = EPC_MMAP_AREA;

    switch(role)
    {
        case ACSE_EPC_SERVER:
            local_sock_name = msg_sock_name;
            remote_sock_name = NULL;
            break;
        case ACSE_EPC_CLIENT:
            local_sock_name = mktemp(strdup("/tmp/acse_epc_XXXXXX"));
            remote_sock_name = msg_sock_name;
            break;
        default:
            return TE_RC(TE_ACSE, TE_EINVAL);
    }

    VERB("%s(): sock_name = %s, shmem = %s, role = %d", 
        __FUNCTION__, msg_sock_name, shmem_name, (int)role);

    epc_shmem_size = 32 * 1024;

    /* Create shared memory on the client, before EPC pipe connecting */
    if (role == ACSE_EPC_CLIENT)
        epc_shmem = shared_mem(TRUE, &epc_shmem_size, shmem_name); 

    if ((epc_socket = unix_socket(local_sock_name, remote_sock_name))
            == -1)
    {
        int saved_errno = errno;
        ERROR("create EPC socket failed, errno %d", saved_errno);

        if (epc_shmem != NULL)
        {
            shm_unlink(shmem_name);
            epc_shmem = NULL;
        }
        if (local_sock_name)
            unlink(local_sock_name);
        return TE_RC(TE_ACSE, saved_errno);
    }

    if (role == ACSE_EPC_SERVER)
    {
        int accepted;
        listen(epc_socket, 1);
        accepted = accept(epc_socket, NULL, NULL);
        close(epc_socket);
        epc_socket = accepted;
        epc_shmem = shared_mem(FALSE, &epc_shmem_size, shmem_name); 
    }

    if (epc_shmem == NULL)
    {
        int saved_errno = errno;
        ERROR("open shared mem failed, errno %d", saved_errno);
        return TE_OS_RC(TE_ACSE, saved_errno);
    }

    epc_shmem_name = strdup(shmem_name);
    RING("%s(): shmem addr: %p, shmem size %u, socket %d", __FUNCTION__,
            epc_shmem, epc_shmem_size, epc_socket);
    return 0;
}

int
acse_epc_socket(void)
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
    if (epc_shmem_name)
    {
        shm_unlink(epc_shmem_name);
        free((char *)epc_shmem_name); epc_shmem_name = NULL;
    }
    if (local_sock_name)
        unlink(local_sock_name);
    if (remote_sock_name && epc_role == ACSE_EPC_SERVER)
        unlink(remote_sock_name);
    return 0;
}




/* see description in acse_epc.h */
te_errno
acse_epc_send(const acse_epc_msg_t *user_message)
{
    ssize_t sendrc;
    acse_epc_msg_t message = *user_message;

    if (user_message == NULL || user_message->data.p == NULL)
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
                ERROR("Try call EPC, but role is not CLIENT!");
                return TE_EINVAL;
            }
            break;
        case EPC_CONFIG_RESPONSE:
        case EPC_CWMP_RESPONSE:
            if (epc_role != ACSE_EPC_SERVER)
            {
                ERROR("Try response EPC, but role is not SERVER!");
                return TE_EINVAL;
            }
            break;
        default:
            ERROR("Try send EPC with wrong opcode!");
            return TE_EINVAL; 
    }

    /* Now prepare data */
    message.data.p = epc_shmem;

    switch (user_message->opcode)
    {
        case EPC_CONFIG_CALL:
        case EPC_CONFIG_RESPONSE:
        {
            acse_epc_config_data_t *cfg_data = user_message->data.cfg;
            message.length = sizeof(*cfg_data);
            memcpy(epc_shmem, cfg_data, message.length);
            VERB("%s(): send config message, op.fun '%d', msglen %d", 
                 __FUNCTION__, cfg_data->op.fun, message.length);
            if (cfg_data->op.fun == EPC_CFG_LIST)
            {
                VERB("EPC send list '%s'", cfg_data->list);
            }
        }
            break;

        default: /* CWMP operation */
        {
            acse_epc_cwmp_data_t *cwmp_data = user_message->data.cwmp;
            uint8_t *buf;
            ssize_t packed_len;
            size_t len;

            memcpy(epc_shmem, cwmp_data, sizeof(*cwmp_data));
            buf = ((acse_epc_cwmp_data_t *)epc_shmem)->enc_start;
            len = epc_shmem_size - sizeof(*cwmp_data);

            if (message.opcode == EPC_CWMP_CALL)
                packed_len = cwmp_pack_call_data(buf, len, cwmp_data);
            else
                packed_len = cwmp_pack_response_data(buf, len, cwmp_data);
            if (packed_len < 0)
            {
                ERROR("%s(): pack data failed, not send", __FUNCTION__);
                return TE_RC(TE_ACSE, TE_EFAIL);
            }
            message.length = packed_len + sizeof(*cwmp_data);
            break;
        }
    }

    sendrc = send(epc_socket, &message, sizeof(message), 0);
    if (sendrc < 0)
    {
        ERROR("%s(): send failed %r", __FUNCTION__, errno);
        return TE_RC(TE_ACSE, errno);
    }

    return 0;
}



/* see description in acse_epc.h */
te_errno
acse_epc_recv(acse_epc_msg_t **user_message)
{
    acse_epc_cwmp_data_t *cwmp_data;
    acse_epc_config_data_t *cfg_data;
    acse_epc_msg_t message;
    ssize_t recvrc;
    te_errno rc = 0;

    if (user_message == NULL)
        return TE_EINVAL;

    recvrc = recv(epc_socket, &message, sizeof(message), 0);
    if (recvrc < 0)
    {
        ERROR("%s(): send failed %r", __FUNCTION__, errno);
        return TE_OS_RC(TE_ACSE, errno);
    }
    if (recvrc == 0) /* Connection normally closed */
    {
        RING("EPC recv: connection closed by peer");
        close(epc_socket);
        epc_socket = -1;
        return TE_ENOTCONN;
    }
    if (recvrc != sizeof(message))
        ERROR("EPC recv: wrong recv rc %d", (int)recvrc);
    /* TODO with this error should be done something more serious */

    switch (message.opcode) 
    {/* check role */
        case EPC_CONFIG_CALL:
        case EPC_CWMP_CALL:
            if (epc_role != ACSE_EPC_SERVER)
            {
                ERROR("Receive EPC call, but role is not SERVER!");
                return TE_EINVAL;
            }
            break;
        case EPC_CONFIG_RESPONSE:
        case EPC_CWMP_RESPONSE:
            if (epc_role != ACSE_EPC_CLIENT)
            {
                ERROR("receive response EPC, but role is not CLIENT!");
                return TE_EINVAL;
            }
            break;
        default:
            ERROR("Received EPC wrong opcode! %d", (int)message.opcode);
            return TE_EINVAL;
    }

    /* TODO check magic here */

    message.data.p = malloc(message.length);
    memcpy(message.data.p, epc_shmem, message.length);

    cwmp_data = message.data.cwmp;
    switch (message.opcode)
    {
        case EPC_CWMP_CALL:
            rc = cwmp_unpack_call_data(cwmp_data->enc_start,
                        message.length, cwmp_data);
            break;
        case EPC_CWMP_RESPONSE:
            if (message.status == 0)
                rc = cwmp_unpack_response_data(cwmp_data->enc_start,
                            message.length, cwmp_data);
            break;

        case EPC_CONFIG_CALL:
        case EPC_CONFIG_RESPONSE:
            cfg_data = message.data.cfg;

#if 0 /* Debug printf.. */
            if (cfg_data->value[0])
            {
                char *p = cfg_data->value;
                fprintf(stderr, "EPC RECV MSG, value in cfg %d (%c)\n",
                        (int)cfg_data->value[0], (char)cfg_data->value[0]);
                while (*p)
                {
                    fprintf(stderr, "EPC RECV MSG, value[] (%c)\n",
                            (char)(*p));
                    p++;
                }
            }
            else
                fprintf(stderr, "EPC RECV MSG, value in cfg is ZERO\n");
#endif
            if (cfg_data->op.magic != EPC_CONFIG_MAGIC)
            {
                ERROR("EPC: wrong magic for config message: 0x%x",
                     (int)cfg_data->op.magic);
                free(message.data.p);
                return TE_RC(TE_ACSE, TE_EFAIL);
            }
            VERB("%s(): recv cfg msg, cfg_data %p, value '%s', msglen %u", 
                 __FUNCTION__, cfg_data, cfg_data->value, message.length);
            break;
    }
    *user_message = malloc(sizeof(acse_epc_msg_t));
    memcpy(*user_message, &message, sizeof(message));
    return rc;
}
