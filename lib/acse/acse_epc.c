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
#include <poll.h>

#include "acse_epc.h"

#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "logger_api.h"

#include "cwmp_data.h"
#include "acse_mem.h"
#include "acse_user.h"


#define EPC_MMAP_AREA "/epc_mmap_area"
#define EPC_MMAP_SIZE (128 * 1024)
#define EPC_ACSE_SOCK "/tmp/epc_acse_sock"


int epc_socket = -1;
int epc_listen_socket = -1;

void *epc_shmem = NULL;
size_t epc_shmem_size = 0;
const char *epc_shmem_name = NULL;

static char *local_sock_name = NULL;
static char *remote_sock_name = NULL;

/**
 * Create unix socket, bind it and connect it to another one if specified.
 * If connection is need, but occasionaly is refused, this function tryes
 * to wait a bit and connect once more.
 *
 * @param unix_path     Unix path to bind to.
 * @param connect_to    Unix path of the peer to connect or
 *                      NULL if no connection is needed.
 *
 * @return              Socket or -1 in case of an error
 */
static int
unix_socket(char const *unix_path, char const *connect_to)
{
    struct sockaddr_un addr = { .sun_family = AF_UNIX };
    int s = socket(AF_UNIX, SOCK_STREAM, 0);
    int saved_errno;
    int ret;

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

    ret = bind(s, (struct sockaddr *)&addr, sizeof addr);

    if (ret == -1 && errno == EADDRINUSE)
    {
        RING("Binding AF_UNIX socket, try to fix EADDRINUSE,"
             " remove existing unix socket file");
        unlink(unix_path);
        ret = bind(s, (struct sockaddr *)&addr, sizeof addr);
    }

    if (ret != -1)
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

#if 0
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
#endif


/* see description in acse_epc.h */
te_errno
acse_epc_init(char *cfg_sock_name, int *listen_sock)
{
    te_errno rc;
    int s;

    if (NULL == cfg_sock_name || NULL == listen_sock)
        return EINVAL;

    epc_shmem_size = EPC_MMAP_SIZE;
    if ((epc_shmem = malloc(epc_shmem_size)) == NULL)
        return TE_RC(TE_ACSE, TE_ENOMEM);

    snprintf(cfg_sock_name, EPC_MAX_PATH,
             "/tmp/epc_srv.%d", (int)getpid());

    local_sock_name = strdup(cfg_sock_name);
    remote_sock_name = NULL;

    RING("%s(): EPC pipe name '%s'", __FUNCTION__, cfg_sock_name);

    if ((s = unix_socket(cfg_sock_name, NULL)) == -1)
    {
        int saved_errno = errno;
        ERROR("create EPC socket failed, errno %d", saved_errno);
        rc = te_rc_os2te(saved_errno);
        goto cleanup;
    }


    listen(s, 1);
    *listen_sock = s;

    return 0;

cleanup:
    if (epc_shmem != NULL)
    {
        free(epc_shmem);
        epc_shmem = NULL;
    }
    if (cfg_sock_name)
        unlink(cfg_sock_name);
    return TE_RC(TE_ACSE, rc);
}

te_errno
acse_epc_connect(const char *cfg_sock_name)
{
    epc_site_t *s;

    if (local_sock_name != NULL || epc_socket >= 0)
    {
        WARN("%s(): seems already connected, local pipe name '%s', sock %d",
             __FUNCTION__, local_sock_name, epc_socket);
        return TE_RC(TE_ACSE, TE_EISCONN);
    }

    s = malloc(sizeof(*s));

    local_sock_name = malloc(EPC_MAX_PATH);
    remote_sock_name = strdup(cfg_sock_name);

    snprintf(local_sock_name, EPC_MAX_PATH,
             "/tmp/epc_srv.%d", (int)getpid());
    RING("%s(): EPC pipe name '%s'", __FUNCTION__, local_sock_name);

    if ((epc_socket = unix_socket(local_sock_name, remote_sock_name)) < 0)
    {
        int saved_errno = errno;
        ERROR("Connect to EPC fails, errno %d", saved_errno);
        return TE_RC(TE_ACSE, te_rc_os2te(saved_errno));
    }
    s->role = ACSE_EPC_CFG_CLIENT;
    s->fd_in = s->fd_out = epc_socket;
    acse_epc_user_init(s);

    return 0;
}

te_errno
acse_epc_close(void)
{
    if (epc_socket >= 0)
        close(epc_socket);
    if (epc_listen_socket >= 0)
        close(epc_listen_socket);

    epc_socket = -1;
    epc_listen_socket = -1;

#if 0
    if (epc_shmem_name)
    {
        shm_unlink(epc_shmem_name);
        free((char *)epc_shmem_name); epc_shmem_name = NULL;
    }
#endif
    if (local_sock_name)
    {
        RING("%s(): EPC pipe name '%s', unlink it",
            __FUNCTION__, local_sock_name);

        unlink(local_sock_name);
        free(local_sock_name);
        local_sock_name = NULL;
    }

    if (remote_sock_name)
    {
        /* unlink of the file-system name should be done by the peer */
        free(remote_sock_name);
        remote_sock_name = NULL;
    }
    return 0;
}


te_errno
acse_epc_check(void)
{
    struct pollfd   pfd;
    int             pollrc;

    if (epc_socket < 0)
        return TE_ENOTCONN;

    pfd.events = POLLIN | POLLOUT;
    pfd.fd = epc_socket;
    pollrc = poll(&pfd, 1, 0);

    VERB("%s(): poll to IN/OUT for fd %d return %d, revents 0x%x",
         __FUNCTION__, pfd.fd, pollrc, pfd.revents);

    if (pollrc < 0)
    {
        int saved_errno = errno;
        ERROR("%s(): poll to EPC conf socket rc %d, errno is %s",
              __FUNCTION__, pollrc, strerror(saved_errno));
        return te_rc_os2te(saved_errno);
    }
    if (pollrc == 0)
    {
        /* This should not happen in any case! */
        ERROR("%s(): poll for OUT to EPC conf socket return zero!?"
              " Will close EPC.",
              __FUNCTION__);
        acse_epc_close();

        return TE_EFAIL;
    }
    if (pollrc > 0)
    {
        /* POLLIN set here could mean only close of connection;
           there should not be data in EPC Config pipe without
           direct request.
           TODO: check that there are no data in the pipe?
        */
        if ((pfd.revents & POLLHUP) || (pfd.revents & POLLOUT) == 0)
        {
            RING("%s(): no write to EPC socket. Will close EPC.",
                 __FUNCTION__);
            acse_epc_close();
            return TE_ENOTCONN;
        }
    }
    return 0;
}

int
acse_epc_socket(void)
{
    if (epc_socket >= 0)
        return epc_socket;
    else
        return epc_listen_socket;
}

void*
acse_epc_shmem(void)
{
    return epc_shmem;
}



te_errno
acse_epc_conf_send(const acse_epc_config_data_t *msg)
{
    ssize_t msg_len = 0, sendrc;
    te_errno rc = 0;

    if (msg == NULL)
        return TE_EINVAL;

    if (epc_socket < 0)
    {
        ERROR("Try send, but EPC is not initialized");
        return TE_EBADFD;
    }

    msg_len = sizeof(acse_epc_config_data_t);

    sendrc = write(epc_socket, msg, sizeof(*msg));

    if (sendrc < 0)
    {
        ERROR("%s(): write to fd %d failed, OS error %s",
              __FUNCTION__, epc_socket, strerror(errno));
        rc = te_rc_os2te(errno);
    }

    return TE_RC(TE_ACSE, rc);
}

te_errno
acse_epc_conf_recv(acse_epc_config_data_t *msg)
{
    ssize_t recvrc;

    recvrc = read(epc_socket, msg, sizeof(*msg));
    if (recvrc < 0)
    {
        int saved_errno = errno;
        ERROR("%s(): recv failed, errno %d", __FUNCTION__, saved_errno);
        return TE_OS_RC(TE_ACSE, te_rc_os2te(saved_errno));
    }
    if (recvrc == 0) /* Connection normally closed */
    {
        RING("EPC recv: connection closed by peer");
        acse_epc_close();
        return TE_ENOTCONN;
    }
    if (recvrc != sizeof(*msg))
    {
        ERROR("EPC recv: wrong recv rc %d", (int)recvrc);
        /* TODO investigate what to do in connection error. */
        return TE_EFAIL;
    }

    VERB("%s():%d fun %d, lvl %d",
         __FUNCTION__, __LINE__, (int)msg->op.fun, (int)msg->op.level);

    if (msg->op.magic != EPC_CONFIG_MAGIC)
    {
        ERROR("EPC: wrong magic for config message: 0x%x",
             (int)msg->op.magic);
        return TE_RC(TE_ACSE, TE_EFAIL);
    }
    return 0;
}


/* see description in acse_epc.h */
te_errno
acse_epc_cwmp_send(epc_site_t *s, const acse_epc_cwmp_data_t *cwmp_data)
{
    static ssize_t msg_len = 0;

    uint8_t *buf;
    ssize_t packed_len;
    ssize_t sendrc;
    size_t len;
    te_errno rc = 0;

    if (cwmp_data == NULL || s == NULL)
        return TE_EINVAL;

    memcpy(epc_shmem, cwmp_data, sizeof(*cwmp_data));
    buf = ((acse_epc_cwmp_data_t *)epc_shmem)->enc_start;
    len = epc_shmem_size - sizeof(*cwmp_data);

    if (s->role == ACSE_EPC_OP_CLIENT)
        packed_len = epc_pack_call_data(buf, len, cwmp_data);
    else
        packed_len = epc_pack_response_data(buf, len, cwmp_data);

    if (packed_len < 0)
    {
        /* TODO: if there is less mmap area size then necessary,
          pass data via pipe. This would happen rarely, and
          may be done via slow way. */
        /* TODO 2: Do not fail ACSE totally if send failed? */

        ERROR("%s(): pack data failed, not send", __FUNCTION__);
        return TE_RC(TE_ACSE, TE_EFAIL);
    }
    msg_len = packed_len + sizeof(*cwmp_data);

    if (ACSE_EPC_SERVER == s->role &&
        NULL != cwmp_data->from_cpe.p)
        mheap_free_user(MHEAP_NONE, cwmp_data);


    VERB("%s(r %d): fd_out %d; put to shmem %d bytes; packed len %d; "
         "op %s",
         __FUNCTION__, (int) s->role, s->fd_out, msg_len, packed_len,
         cwmp_rpc_cpe_string(cwmp_data->rpc_cpe));
    sendrc = write(s->fd_out, &msg_len, sizeof(msg_len));

    if (sendrc < 0)
    {
        ERROR("%s(): send failed %r", __FUNCTION__, errno);
        rc = te_rc_os2te(errno);
    }

    return TE_RC(TE_ACSE, rc);
}



/* see description in acse_epc.h */
te_errno
acse_epc_cwmp_recv(epc_site_t *s, acse_epc_cwmp_data_t **cwmp_data_ptr,
                   size_t *d_len)
{
    acse_epc_cwmp_data_t *cwmp_data = NULL;
    ssize_t recvrc;
    ssize_t msg_len;
    te_errno rc = 0;

    if (cwmp_data_ptr == NULL || s == NULL)
        return TE_EINVAL;

    recvrc = read(s->fd_in, &msg_len, sizeof(msg_len));

    if (recvrc < 0)
    {
        int saved_errno = errno;
        ERROR("%s(): recv failed, errno %d", __FUNCTION__, saved_errno);
        return TE_RC(TE_ACSE, te_rc_os2te(saved_errno));
    }
    if (recvrc == 0) /* Connection normally closed */
    {
        RING("EPC CWMP recv: connection closed by peer");
        return TE_ENOTCONN;
    }
    if (recvrc != sizeof(msg_len))
    {
        ERROR("EPC recv: wrong recv rc %d", (int)recvrc);
        /* TODO investigate what to do in connection error. */
        return TE_EFAIL;
    }

    if (msg_len == 0)
    {
        RING("EPC CWMP recv: msg len is zero, close connection");
        return TE_ENOTCONN;
    }

    if (msg_len < 0)
    {
        ERROR("non-positive msg_len in CWMP EPC pipe: %d", msg_len);
        return TE_EFAIL;
    }

    cwmp_data = malloc(msg_len);
    memcpy(cwmp_data, epc_shmem, msg_len);
    *cwmp_data_ptr = cwmp_data;

    if (NULL != d_len)
        *d_len = msg_len;

    VERB("%s(r %d): recv from sock %d shmem len %d, op %s",
         __FUNCTION__, s->role, s->fd_in, msg_len,
         cwmp_rpc_cpe_string(cwmp_data->rpc_cpe));

    switch (s->role)
    {
        case ACSE_EPC_SERVER:
            rc = epc_unpack_call_data(cwmp_data->enc_start,
                                      msg_len, cwmp_data);
            break;

        case ACSE_EPC_OP_CLIENT:
            if (0 == cwmp_data->status ||
                TE_CWMP_FAULT == cwmp_data->status)
                rc = epc_unpack_response_data(cwmp_data->enc_start,
                                              msg_len, cwmp_data);
            break;
        case ACSE_EPC_CFG_CLIENT:
            break;
    }

    return rc;
}



/* see description in acse_epc.h */
ssize_t
epc_pack_call_data(void *buf, size_t len,
                   const acse_epc_cwmp_data_t *cwmp_data)
{
    if (EPC_HTTP_RESP == cwmp_data->op)
    {
        size_t sz = strlen((char *)cwmp_data->enc_start) + 1;
        if (sz > len)
            return -1;
        memcpy(buf, cwmp_data->enc_start, sz);
        return sz;
    }
    if (cwmp_data->to_cpe.p == NULL || cwmp_data->op != EPC_RPC_CALL)
        return 0;
    return cwmp_pack_call_data(cwmp_data->to_cpe, cwmp_data->rpc_cpe,
                               buf, len);
}



ssize_t
epc_pack_response_data(void *buf, size_t len,
                       const acse_epc_cwmp_data_t *cwmp_data)
{
    if (cwmp_data->from_cpe.p == NULL)
        return 0;

    if (cwmp_data->op == EPC_GET_INFORM)
        return te_cwmp_pack__Inform(cwmp_data->from_cpe.inform, buf, len);

    /* other operations do not require passing of CWMP data */
    if (cwmp_data->op != EPC_RPC_CHECK)
        return 0;

    if (cwmp_data->rpc_cpe != CWMP_RPC_NONE)
        return cwmp_pack_response_data(cwmp_data->from_cpe,
                                       cwmp_data->rpc_cpe,
                                       buf, len);
    else if (cwmp_data->rpc_acs != CWMP_RPC_ACS_NONE)
        return cwmp_pack_acs_rpc_data(cwmp_data->from_cpe,
                                      cwmp_data->rpc_acs,
                                      buf, len);
    return 0;
}


te_errno
epc_unpack_call_data(void *buf, size_t len,
                   acse_epc_cwmp_data_t *cwmp_data)
{
    if (cwmp_data->op != EPC_RPC_CALL)
        return 0;

    cwmp_data->to_cpe.p = buf;

    return cwmp_unpack_call_data(buf, len, cwmp_data->rpc_cpe);
}

te_errno
epc_unpack_response_data(void *buf, size_t len,
                         acse_epc_cwmp_data_t *cwmp_data)
{
    if (cwmp_data->op == EPC_GET_INFORM)
    {
        cwmp_data->from_cpe.p = buf;
        if (te_cwmp_unpack__Inform(buf, len) < 0)
        {
            ERROR("%s(): unpack inform failed", __FUNCTION__);
            return TE_RC(TE_ACSE, TE_EFAIL);
        }
        else
            return 0;
    }

    /* other operations do not require passing of CWMP data */
    if (cwmp_data->op != EPC_RPC_CHECK)
        return 0;

    cwmp_data->from_cpe.p = buf;

    if (cwmp_data->rpc_cpe != CWMP_RPC_NONE)
        return cwmp_unpack_response_data(buf, len, cwmp_data->rpc_cpe);
    else if (cwmp_data->rpc_acs != CWMP_RPC_ACS_NONE)
        return cwmp_unpack_acs_rpc_data(buf, len, cwmp_data->rpc_acs);
    return 0;
}
