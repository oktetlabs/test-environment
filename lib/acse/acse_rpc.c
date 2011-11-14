/** @file
 * @brief ACSE RPC server
 *
 * ACS Emulator support
 *
 * Copyright (C) 2011 Test Environment authors (see file AUTHORS
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

#define TE_LGR_USER     "ACSE RPC server"

#include "te_config.h"

#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#if HAVE_SYS_TYPE_H
#include <sys/type.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_STDARG_H
#include <stdarg.h>
#endif

#if HAVE_SIGNAL_H
#include <signal.h>
#endif

#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#else
#error <sys/socket.h> is definitely needed for acse_epc.c
#endif

#if HAVE_SYS_UN_H
#include <sys/un.h>
#else
#error <sys/un.h> is definitely needed for acse_epc.c
#endif

#include <string.h>
#include <ctype.h>
#include <pthread.h>

#include "acse_user.h"
#include "acse_internal.h"

#include "te_stdint.h"
#include "te_errno.h"
#include "te_queue.h"
#include "te_defs.h"
#include "logger_api.h"
#include "logfork.h"

/* Methods forward declarations */
static te_errno stop_acse(void);
static te_errno start_acse(char *cfg_pipe_name);

int epc_to_acse_pipe[2] = {-1, -1};
int epc_from_acse_pipe[2] = {-1, -1};


static pthread_t acse_thread;


#if 0
static pthread_key_t epc_role_key;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;

static void
make_key(void)
{
    pthread_key_create(&epc_role_key, NULL)
}

void
epc_role_set(acse_epc_role_t r)
{
    int l_errno;
    int *val = malloc(sizeof(*val));
    *val = (int)r;
    RING("epc_role_set to %d", (int)r);
    if ((errno = pthread_setspecific(epc_role_key, val)) != 0)
    {
        ERROR("EPC role set failed with errno %d(%s)",
              l_errno, strerror(l_errno));
    }
}

acse_epc_role_t
epc_role_get(void)
{
    int *val = pthread_getspecific(epc_role_key);

    if (val == NULL)
    {
        ERROR("No EPC role pthread key! return default role, SERVER");
        return ACSE_EPC_SERVER;
    }
    RING("epc_role_get:  %d", (int)(*val));

    return (acse_epc_role_t)(*val);
}
#endif

typedef struct {
    int listen_socket;
} acse_thread_arg_t;
/**
 * Start routine for the ACSE thread within TA-associated
 * RPC server process.
 *
 * @param arg           Start argument
 *
 * @return NULL
 */
static void *
acse_pthread_main(void *p_a)
{
    te_errno rc;
    acse_thread_arg_t *arg = (acse_thread_arg_t *)p_a;
    epc_site_t *s = calloc(1, sizeof(*s));

    logfork_register_user("ACSE");

    s->role = ACSE_EPC_SERVER;
    s->fd_in = epc_to_acse_pipe[0];
    s->fd_out = epc_from_acse_pipe[1];

    if ((rc = acse_epc_disp_init(arg->listen_socket, s)) != 0)
    {
        ERROR("Fail create EPC dispatcher %r", rc);
        return NULL;
    }

    acse_loop();
    /* TODO: maybe, pass some exit status? */
    db_clear();

    acse_clear_channels();

    free(arg);
    RING("ACSE stopped");
    return NULL;
}

/**
 * Initialize necessary entities and start ACSE.
 *
 * @return              Status
 */
static te_errno
start_acse(char *cfg_pipe_name)
{
    te_errno rc = 0;
    int pth_ret;
    acse_thread_arg_t *arg = malloc(sizeof(*arg));

    epc_site_t *s = calloc(1, sizeof(*s));

    if (epc_to_acse_pipe[1] >= 0)
    {
        ERROR("Try start ACSE while it is already running");
        return TE_EFAIL;
    }

    RING("Start ACSE process");

    if (pipe(epc_to_acse_pipe) || pipe(epc_from_acse_pipe))
    {
        int saved_errno = errno;
        ERROR("create of EPC ops pipes, errno %d", saved_errno);
        return TE_OS_RC(TE_ACSE, saved_errno);
    }
    else
    {
        RING("Init cwmp pipes: RPCS %d -> ACSE %d;   ACSE %d -> RPCS %d",
             epc_to_acse_pipe[1],
             epc_to_acse_pipe[0],
             epc_from_acse_pipe[1],
             epc_from_acse_pipe[0]);
    }

    if ((rc = acse_epc_init(cfg_pipe_name, &(arg->listen_socket))) != 0)
        return TE_RC(TE_ACSE, rc);

    s->role = ACSE_EPC_OP_CLIENT;
    s->fd_in = epc_from_acse_pipe[0];
    s->fd_out = epc_to_acse_pipe[1];
    acse_epc_user_init(s);

    pth_ret = pthread_create(&acse_thread, NULL, acse_pthread_main, arg);
    if (pth_ret != 0)
        return TE_OS_RC(TE_ACSE, pth_ret);

    return 0;
}

/**
 * Stop ACSE and clean up previously initialized entities.
 *
 * @return              Status
 */
static te_errno
stop_acse(void)
{
    void *retval;
    int r;
    static ssize_t msg_len = 0;

    RING("STOP ACSE called");

    if (epc_to_acse_pipe[1] < 0)
    {
        ERROR("Try stop ACSE while it is not running");
        return TE_EFAIL;
    }

    RING("STOP ACSE: issue zero msg_len ");
    msg_len = 0;
    write(epc_to_acse_pipe[1], &msg_len, sizeof(msg_len));

    RING("STOP ACSE: join thread ... ");

    if ((r = pthread_join(acse_thread, &retval)) != 0)
    {
        ERROR("Join to ACSE thread fails: %s", strerror(r));
        return TE_OS_RC(TE_ACSE, te_rc_os2te(r));
    }
    RING("STOP ACSE: thread finished, clear EPC user, close pipes.");
    acse_epc_user_init(NULL);
    close(epc_to_acse_pipe[1]);  epc_to_acse_pipe[1] = -1;
    close(epc_to_acse_pipe[0]);  epc_to_acse_pipe[0]   = -1;
    close(epc_from_acse_pipe[0]);epc_from_acse_pipe[0] = -1;
    close(epc_from_acse_pipe[1]);epc_from_acse_pipe[1] = -1;

    return 0;
}


int
cwmp_acse_start(tarpc_cwmp_acse_start_in *in,
                tarpc_cwmp_acse_start_out *out)
{
    static char buf[256] = {0,};

    RING("%s() called", __FUNCTION__);

    if (in->oper == 1)
    {
        out->status = start_acse(buf);
        out->pipe_name = strdup(buf);
        RING("%s(): status %r, pipe name '%s'",
             __FUNCTION__, out->status, buf);
    }
    else
    {
        out->status = stop_acse();
        out->pipe_name = strdup("");
    }

    return 0;
}

int
cwmp_conn_req(tarpc_cwmp_conn_req_in *in,
              tarpc_cwmp_conn_req_out *out)
{
    te_errno rc;
    acse_epc_cwmp_data_t *cwmp_data = NULL;

    INFO("Issue CWMP Connection Request to %s/%s ",
         in->acs_name, in->cpe_name);

    rc = acse_cwmp_connreq(in->acs_name, in->cpe_name, &cwmp_data);
    if (rc)
    {
        WARN("issue CWMP ConnReq failed %r", rc);
        if (TE_ETIMEDOUT == TE_RC_GET_ERROR(rc) &&
            TE_TA_UNIX == TE_RC_GET_MODULE(rc))
        {
            WARN("There was EPC timeout, kill ACSE");
            stop_acse();
        }
    }

    out->status = cwmp_data->status;
    free(cwmp_data);

    return 0;
}

int
cwmp_op_call(tarpc_cwmp_op_call_in *in,
             tarpc_cwmp_op_call_out *out)
{
    te_errno rc = 0;
    acse_epc_cwmp_data_t *cwmp_data = NULL;


    RING("cwmp RPC %s to %s/%s called",
         cwmp_rpc_cpe_string(in->cwmp_rpc), in->acs_name, in->cpe_name);

    rc = acse_cwmp_prepare(in->acs_name, in->cpe_name,
                           EPC_RPC_CALL, &cwmp_data);
    cwmp_data->rpc_cpe = in->cwmp_rpc;

    if (in->buf.buf_len > 0)
    {
        rc = epc_unpack_call_data(in->buf.buf_val, in->buf.buf_len,
                                    cwmp_data);
        if (rc != 0)
        {
            ERROR("%s(): unpack cwmp data failed %r", __FUNCTION__, rc);
            out->status = rc;
            return 0;
        }
    }

    rc = acse_cwmp_call(NULL, &cwmp_data);
    if (0 != rc)
    {
        ERROR("%s(): ACSE call failed %r", __FUNCTION__, rc);
        if (TE_ETIMEDOUT == TE_RC_GET_ERROR(rc) &&
            TE_TA_UNIX == TE_RC_GET_MODULE(rc))
        {
            WARN("There was EPC timeout, kill ACSE");
            stop_acse();
        }
        out->status = TE_RC(TE_TA_ACSE, rc);
    }
    else
    {
        out->request_id = cwmp_data->request_id;
        out->status = TE_RC(TE_ACSE, cwmp_data->status);
    }

    return 0;
}


int
cwmp_op_check(tarpc_cwmp_op_check_in *in,
              tarpc_cwmp_op_check_out *out)
{
    te_errno rc = 0;
    acse_epc_cwmp_data_t *cwmp_data = NULL;
    size_t d_len;

    INFO("cwmp_op_check No %d (for rpc %s) to %s/%s called;",
         (int)in->request_id,
         cwmp_rpc_cpe_string(in->cwmp_rpc), in->acs_name, in->cpe_name);

    rc = acse_cwmp_prepare(in->acs_name, in->cpe_name,
                           EPC_RPC_CHECK, &cwmp_data);
    cwmp_data->request_id = in->request_id;

    if (in->cwmp_rpc != CWMP_RPC_ACS_NONE)
        cwmp_data->rpc_acs = in->cwmp_rpc;

    rc = acse_cwmp_call(&d_len, &cwmp_data);
    if (rc != 0)
    {
        ERROR("%s(): EPC recv failed %r", __FUNCTION__, rc);
        if (TE_ETIMEDOUT == TE_RC_GET_ERROR(rc) &&
            TE_TA_UNIX == TE_RC_GET_MODULE(rc))
        {
            WARN("There was EPC timeout, kill ACSE");
            stop_acse();
        }
        out->status = TE_RC(TE_TA_ACSE, rc);
    }
    else
    {
        ssize_t packed_len;

        out->status = TE_RC(TE_ACSE, cwmp_data->status);

        INFO("%s(): status is %r, buflen %d", __FUNCTION__,
             cwmp_data->status, d_len);

        if (0 == cwmp_data->status || TE_CWMP_FAULT == cwmp_data->status)
        {
            out->buf.buf_val = malloc(d_len);
            out->buf.buf_len = d_len;
            packed_len = epc_pack_response_data(out->buf.buf_val,
                                                d_len, cwmp_data);
#if 0 /* Debug print */
            if (TE_CWMP_FAULT == msg_resp.status)
            {
                _cwmp__Fault *f = cwmp_data->from_cpe.fault;
                RING("pass Fault %s (%s)", f->FaultCode, f->FaultString);
            }
#endif
        }
        else
        {
            out->buf.buf_val = NULL;
            out->buf.buf_len = 0;
        }
        if (cwmp_data->rpc_cpe != CWMP_RPC_NONE)
            out->cwmp_rpc = cwmp_data->rpc_cpe;
        else if (cwmp_data->rpc_acs != CWMP_RPC_ACS_NONE)
            out->cwmp_rpc = cwmp_data->rpc_acs;
    }

    return 0;
}

#if 0
static te_errno
cwmp_conn_req_util(const char *acs, const char *cpe,
                   acse_epc_cwmp_op_t op, int *request_id)
{
    acse_epc_msg_t msg;
    acse_epc_msg_t msg_resp;
    acse_epc_cwmp_data_t c_data;

    te_errno rc;

    if (NULL == acs || NULL == cpe || NULL == request_id)
        return TE_EINVAL;

    if (!acse_value())
    {
        return TE_EFAIL;
    }

    RING("Issue CWMP Connection Request to %s/%s, op %d ",
         acs, cpe, op);

    msg.opcode = EPC_CWMP_CALL;
    msg.data.cwmp = &c_data;
    msg.length = sizeof(c_data);

    memset(&c_data, 0, sizeof(c_data));

    c_data.op = op;

    strcpy(c_data.acs, acs);
    strcpy(c_data.cpe, cpe);

    rc = acse_epc_send(&msg);
    if (rc != 0)
        ERROR("%s(): EPC send failed %r", __FUNCTION__, rc);

    rc = acse_epc_recv(&msg_resp);
    if (rc != 0)
        ERROR("%s(): EPC recv failed %r", __FUNCTION__, rc);

    out->status = msg_resp.status;

    return 0;
}
#endif


