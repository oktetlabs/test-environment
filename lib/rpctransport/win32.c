/** @file
 * @brief RCF RPC
 *
 * WinPIPE RPC transport
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 */

#if (defined(__CYGWIN__) || defined(WINDOWS)) &&    \
    defined(ENABLE_LOCAL_TRANSPORT)
#include "te_config.h"

#define TE_LGR_USER     "RPC Transport"

#include "te_stdint.h"
#include "ta_common.h"
#include "te_errno.h"
#include "te_defs.h"
#include "rpc_transport.h"

/** Timeout for RPC operations */
#define RPC_TIMEOUT             10000   /* 10 seconds */


#ifdef __CYGWIN__
#include <windows.h>
#endif

#undef ERROR

#include "logger_api.h"

/** rpc_transport_log auxiliary variables */
static char log[1024 * 16];
static char *log_ptr = log;

/**
 * A mechanism allowing to log messages without affecting any
 * network-related state. The log may be printed in the case
 * of RPC server deapth.
 */
void
rpc_transport_log(char *str)
{
    log_ptr += sprintf(log_ptr, str);
}

/** Maximum number of simultaneous connections */
#define RPC_MAX_CONN    256

/** Prefix for pipe filenames */
#define PIPE_PREFIX     "\\\\.\\pipe\\"

typedef struct winpipe {
    te_bool    busy;       /**< The pipe entry in pipes array is busy */
    te_bool    valid;      /**< The pipe is valid */
    te_bool    wait;       /**< The pipe is scheduled for waiting */
    te_bool    read;       /**< The pipe is read now */
    HANDLE     in_handle;  /**< Pipe handle */
    HANDLE     out_handle; /**< Pipe handle */
    OVERLAPPED ov;         /**< Overlapped object with event inside */
} winpipe;

/** Location for information about pipes */
static winpipe pipes[RPC_MAX_CONN];

/** Events array */
static HANDLE events[RPC_MAX_CONN];

/** Number of valid events in the array */
static int events_num;

/** Maximum index of used pipe + 1 */
static int max_pipe;

/** Mutex to protect pipe state */
static HANDLE conn_mutex;

/** Pipe for listening for connection information */
static HANDLE lpipe;

/** Overlapped structure used with lpipe */
static OVERLAPPED lov;

/**
 * Initialize RPC transport.
 */
te_errno
rpc_transport_init(const char *tmp_path)
{
    char port[256];

    int  i;

    for (i = 0; i < (int)(sizeof(pipes) / sizeof(winpipe)); i++)
        pipes[i].in_handle = pipes[i].out_handle = INVALID_HANDLE_VALUE;

    conn_mutex = CreateMutex(NULL, FALSE, NULL);

    sprintf(port, PIPE_PREFIX "tarpc_%lu_%u",
            GetCurrentProcessId(), (unsigned)time(NULL));

    if (setenv("TE_RPC_PORT", port, 1) < 0)
        return TE_RC(TE_RCF_PCH, TE_EWIN);

    lpipe = CreateNamedPipe(port,
                            PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                            PIPE_TYPE_MESSAGE, 10, 128, 128, 100, NULL);

    if (lpipe == INVALID_HANDLE_VALUE)
    {
        ERROR("Failed to create listening pipe: %d", GetLastError());
        return TE_RC(TE_RCF_PCH, TE_EWIN);
    }

    if ((lov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL)
    {
        ERROR("Failed to create event: %d", GetLastError());
        CloseHandle(lpipe);
        return TE_RC(TE_RCF_PCH, TE_EWIN);
    }

    return 0;
}

/**
 * Shutdown RPC transport.
 */
void
rpc_transport_shutdown()
{
    int i;

    for (i = 0; i < max_pipe; i++)
    {
        CloseHandle(pipes[i].ov.hEvent);
        if (pipes[i].busy && pipes[i].valid)
        {
            CloseHandle(pipes[i].in_handle);
            CloseHandle(pipes[i].out_handle);
        }
    }

    CloseHandle(conn_mutex);
    CloseHandle(lov.hEvent);
    CloseHandle(lpipe);
}

static te_errno
get_free_pipe(int *p_handle)
{
    int i;

    for (i = 0; i < max_pipe && pipes[i].busy; i++);

    if (i == max_pipe)
    {
        if (max_pipe == (int)(sizeof(pipes) / sizeof(winpipe)))
            return TE_RC(TE_RCF_PCH, TE_ENOMEM);

        if ((pipes[i].ov.hEvent = CreateEvent(NULL, TRUE,
                                              FALSE, NULL)) == NULL)
        {
            return TE_RC(TE_RCF_PCH, TE_ENOMEM);
        }

        max_pipe++;
    }

    pipes[i].in_handle = pipes[i].out_handle = INVALID_HANDLE_VALUE;
    pipes[i].wait = FALSE;
    pipes[i].read = FALSE;
    pipes[i].busy = TRUE;

    *p_handle = i;

    return 0;
}

/**
 * Send name of RPC server pipe to TA.
 *
 * @param pname         pipe name
 *
 * @return Status code
 */
static te_errno
rpc_transport_send_pname(const char *pname)
{
    HANDLE handle;
    DWORD  mode = PIPE_READMODE_MESSAGE, num;
    char  *port = getenv("TE_RPC_PORT");
    int    tries = RPC_TIMEOUT / 10;

    if (port == NULL)
    {
        ERROR("TE_RPC_PORT is not exported\n");

        return TE_RC(TE_RCF_PCH, TE_EINVAL);
    }

#if 0
    if (!CallNamedPipe(port, pname, strlen(pname) + 1, NULL, 0,
                      NULL, RPC_TIMEOUT))
    {
        ERROR("CallNamedPipe() failed: %d", GetLastError());
        return TE_RC(TE_RCF_PCH, TE_EINVAL);
    }
#endif

    while (--tries > 0)
    {
        handle = CreateFile(port,
                            FILE_CREATE_PIPE_INSTANCE |
                            GENERIC_READ | GENERIC_WRITE,
                            0, NULL, OPEN_EXISTING, 0, NULL);

        if (handle != INVALID_HANDLE_VALUE)
            break;

        if (GetLastError() != ERROR_PIPE_BUSY)
        {
            ERROR("CreateFile() failed: %d", GetLastError());
            return TE_RC(TE_RCF_PCH, TE_EWIN);
        }
        SleepEx(10, FALSE);
    }

    if (handle == INVALID_HANDLE_VALUE)
    {
        ERROR("Connect timeout on auxiliary pipe");
        return TE_RC(TE_RCF_PCH, TE_EWIN);
    }

    if (!SetNamedPipeHandleState(handle, &mode, NULL, NULL))
    {
        ERROR("SetNamedPipeHandleState() failed: %d", GetLastError());
        CloseHandle(handle);
        return TE_RC(TE_RCF_PCH, TE_EWIN);
    }

    if (!WriteFile(handle, pname, strlen(pname) + 1, &num, NULL))
    {
        ERROR("WriteFile() to auxiliary pipe failed: %d", GetLastError());
        CloseHandle(handle);
        return TE_RC(TE_RCF_PCH, TE_EWIN);
    }

    CloseHandle(handle);

    return 0;
}

/**
 * Read name of RPC server pipe on TA.
 *
 * @param pname         pipe name location
 * @param len           length of pipe name location
 *
 * @return Status code
 */
static te_errno
rpc_transport_recv_pname(char *pname, int len)
{
    te_errno rc = 0;
    DWORD    num;

    if (!ConnectNamedPipe(lpipe, &lov) &&
        GetLastError() != ERROR_PIPE_CONNECTED)
    {
        if (GetLastError() != ERROR_IO_PENDING)
        {
            ERROR("ConnectNamedPipe failed: %d\n", GetLastError());
            return TE_RC(TE_RCF_PCH, TE_EWIN);
        }

        if (WaitForSingleObject(lov.hEvent, RPC_TIMEOUT) == WAIT_TIMEOUT ||
            !GetOverlappedResult(lpipe, &lov, &num, FALSE))
        {
            ERROR("Failed to connect auxiliary pipe\n");
            return TE_RC(TE_RCF_PCH, TE_EWIN);
        }
    }

    if (!ReadFile(lpipe, pname, len, &num, &lov))
    {
        if (GetLastError() != ERROR_IO_PENDING ||
            (WaitForSingleObject(lov.hEvent, RPC_TIMEOUT) == WAIT_TIMEOUT ||
             !GetOverlappedResult(lpipe, &lov, &num, FALSE)))
        {
            ERROR("Failed to read from the auxiliary pipe: %d",
                  GetLastError());
            rc = TE_RC(TE_RCF_PCH, TE_EWIN);
        }
    }

    if (!DisconnectNamedPipe(lpipe))
        ERROR("DisconnectNamedPipe() failed: %d", GetLastError());

    return rc;
}

/**
 * Open pipe for input - non-blocking!
 *
 * @param pname     base pipe name
 * @param postfix   pipe name postfix
 * @param pov       pointer to overlapped structure
 * @param p_handle  location for pipe handle
 *
 * @return Status code
 */
static te_errno
open_in(const char *pname, const char *postfix, LPOVERLAPPED pov,
        HANDLE *p_handle)
{
    HANDLE handle;
    DWORD  num;
    char   pipename[64];

    sprintf(pipename, "%s%s", pname, postfix);

    handle = CreateNamedPipe(pipename,
                             PIPE_ACCESS_DUPLEX |
                             FILE_FLAG_OVERLAPPED,
                             PIPE_TYPE_MESSAGE,
                             1, 1024 * 1024, 1024 * 1024,
                             100, NULL);

    if (handle == INVALID_HANDLE_VALUE)
    {
        ERROR("CreateNamedPipe() failed: %d", GetLastError());
        return TE_RC(TE_RCF_PCH, TE_EWIN);
    }

    if (!ConnectNamedPipe(handle, pov) &&
        GetLastError() != ERROR_PIPE_CONNECTED)
    {
        if (GetLastError() != ERROR_IO_PENDING)
        {
            ERROR("ConnectNamedPipe failed: %d\n", GetLastError());
            CloseHandle(handle);
            return TE_RC(TE_RCF_PCH, TE_EWIN);
        }
        if (WaitForSingleObject(pov->hEvent,
                                RPC_TIMEOUT) == WAIT_TIMEOUT ||
            !GetOverlappedResult(handle, pov, &num, FALSE))
        {
            ERROR("Failed to connect pipe\n");
            CloseHandle(handle);
            return TE_RC(TE_RCF_PCH, TE_EWIN);
        }
    }
    *p_handle = handle;
    return 0;
}

/**
 * Open pipe for output - blocking shit!
 *
 * @param pname     base pipe name
 * @param postfix   pipe name postfix
 * @param p_handle  location for pipe handle
 *
 * @return Status code
 */
static te_errno
open_out(const char *pname, const char *postfix, HANDLE *p_handle)
{
    HANDLE handle;
    char   pipename[64];
    DWORD  mode = PIPE_READMODE_MESSAGE;
    int    tries = RPC_TIMEOUT / 10;

    sprintf(pipename, "%s%s", pname, postfix);

    while (--tries > 0)
    {
        handle = CreateFile(pipename,
                            FILE_CREATE_PIPE_INSTANCE |
                            GENERIC_READ | GENERIC_WRITE |
                            FILE_FLAG_OVERLAPPED,
                            0, NULL, OPEN_EXISTING, 0, NULL);

        if (handle != INVALID_HANDLE_VALUE)
            break;

        if (GetLastError() != ERROR_FILE_NOT_FOUND)
        {
            ERROR("CreateFile() failed: %d", GetLastError());
            return TE_RC(TE_RCF_PCH, TE_EWIN);
        }
        SleepEx(10, FALSE);
    }

    if (handle == INVALID_HANDLE_VALUE)
    {
        ERROR("Connect timeout");
        return TE_RC(TE_RCF_PCH, TE_EWIN);
    }

   if (!SetNamedPipeHandleState(handle, &mode, NULL, NULL))
   {
        ERROR("SetNamedPipeHandleState() failed: %d", GetLastError());
        CloseHandle(handle);
        return TE_RC(TE_RCF_PCH, TE_EWIN);
    }
    *p_handle = handle;

    return 0;
}

/**
 * Await connection from RPC server.
 *
 * @param name          name of the RPC server
 * @param p_handle      connection handle location
 *
 * @return Status code.
 */
te_errno
rpc_transport_connect_rpcserver(const char *name,
                                rpc_transport_handle *p_handle)
{
    te_errno rc;
    char     pipename[64];
    int      i;
    HANDLE   in_handle = INVALID_HANDLE_VALUE;
    HANDLE   out_handle = INVALID_HANDLE_VALUE;

    UNUSED(name);

    if ((rc = rpc_transport_recv_pname(pipename, sizeof(pipename))) != 0)
        return rc;

    WaitForSingleObject(conn_mutex, INFINITE);

    if ((rc = get_free_pipe(&i)) != 0)
    {
        ReleaseMutex(conn_mutex);
        return rc;
    }
    ReleaseMutex(conn_mutex);

    if ((rc = open_out(pipename, "_1", &out_handle)) != 0)
    {
        rpc_transport_close(i);
        return rc;
    }

    if ((rc = open_in(pipename, "_2", &pipes[i].ov,
                      &in_handle)) != 0)
    {
        CloseHandle(out_handle);
        rpc_transport_close(i);
        return rc;
    }

    pipes[i].in_handle = in_handle;
    pipes[i].out_handle = out_handle;
    pipes[i].valid = TRUE;
    *p_handle = i;

    return 0;
}

/**
 * Connect from RPC server to Test Agent
 *
 * @param name  name of the RPC server
 * @param p_handle      connection handle location
 *
 * @return Status code.
 */
te_errno
rpc_transport_connect_ta(const char *name, rpc_transport_handle *p_handle)
{
    char *port = getenv("TE_RPC_PORT");

    if (port == NULL)
    {
        ERROR("TE_RPC_PORT is not exported\n");

        return TE_RC(TE_RCF_PCH, TE_EINVAL);
    }

    char     pipename[64];
    te_errno rc;
    int      i;
    HANDLE   in_handle = INVALID_HANDLE_VALUE;
    HANDLE   out_handle = INVALID_HANDLE_VALUE;

    SleepEx(500, TRUE); /* Let other thread send the response
                         on server creation */

    sprintf(pipename, PIPE_PREFIX "%s_%lu_%lu_%u",
            name, GetCurrentProcessId(), GetCurrentThreadId(),
            (unsigned)time(NULL));

    if ((rc = rpc_transport_send_pname(pipename)) != 0)
        return rc;

    WaitForSingleObject(conn_mutex, INFINITE);
    if ((rc = get_free_pipe(&i)) != 0)
    {
        ReleaseMutex(conn_mutex);
        return rc;
    }
    ReleaseMutex(conn_mutex);

    if ((rc = open_in(pipename, "_1", &pipes[i].ov,
                      &in_handle)) != 0)
    {
        rpc_transport_close(i);
        return rc;
    }

    if ((rc = open_out(pipename, "_2", &out_handle)) != 0)
    {
        CloseHandle(in_handle);
        rpc_transport_close(i);
        return rc;
    }

    pipes[i].in_handle = in_handle;
    pipes[i].out_handle = out_handle;
    pipes[i].valid = TRUE;
    *p_handle = i;

    return 0;
}

/**
 * Break the connection.
 *
 * @param handle      connection handle
 */
void
rpc_transport_close(rpc_transport_handle handle)
{
    assert(handle < max_pipe);

    WaitForSingleObject(conn_mutex, INFINITE);
    pipes[handle].busy = FALSE;

    if (pipes[handle].valid)
    {
      CloseHandle(pipes[handle].in_handle);
      CloseHandle(pipes[handle].out_handle);
    }
    pipes[handle].valid = FALSE;

    pipes[handle].in_handle = pipes[handle].out_handle =
    INVALID_HANDLE_VALUE;

    ReleaseMutex(conn_mutex);
}

/**
 * Reset set of descriptors to wait.
 */
void
rpc_transport_read_set_init()
{
    events_num = 0;
}

/**
 * Add the handle to the read set.
 *
 * @param handle        connection handle
 */
void
rpc_transport_read_set_add(rpc_transport_handle handle)
{
    assert(handle < max_pipe);
    if (pipes[handle].valid && !pipes[handle].read)
    {
        static DWORD tmp;

        ResetEvent(pipes[handle].ov.hEvent);
        ReadFile(pipes[handle].in_handle, NULL, 0, &tmp, &pipes[handle].ov);
        events[events_num++] = pipes[handle].ov.hEvent;
        pipes[handle].wait = TRUE;
    }
}

/**
 * Wait for the read event.
 *
 * @param timeout       timeout in seconds
 *
 * @return TRUE is the read event is received or FALSE otherwise
 */
te_bool
rpc_transport_read_set_wait(int timeout)
{
    int   i;
    DWORD rc;

    if (events_num == 0)
    {
        SleepEx(timeout * 100, TRUE);
        return FALSE;
    }

    rc = WaitForMultipleObjects(events_num, events, FALSE, timeout * 1000);

    for (i = 0; i < max_pipe; i++)
    {
        if (pipes[i].valid && pipes[i].wait && !pipes[i].read)
        {
            CancelIo(pipes[i].in_handle);
        }
        pipes[i].wait = FALSE;
    }

    return rc != WAIT_TIMEOUT;
}

/**
 * Check if data are pending on the connection.
 *
 * @param handle        connection handle
 *
 * @return TRUE is data are pending or FALSE otherwise
 */
te_bool
rpc_transport_is_readable(rpc_transport_handle handle)
{
    te_bool  rc = FALSE;

    if (pipes[handle].valid)
    {
        rc = ReadFile(pipes[handle].in_handle, NULL,
                      0, NULL, &pipes[handle].ov);

        if (!rc)
            CancelIo(pipes[handle].in_handle);
    }

    return rc;
}


/**
 * Receive message with specified timeout.
 *
 * @param handle   connection handle
 * @param buf      buffer for reading
 * @param p_len    IN: buffer length; OUT: length of received message
 * @param timeout  timeout in seconds
 *
 * @return Status code.
 * @retval TE_ETIMEDOUT         Timeout ocurred
 * @retval TE_ECONNRESET        Connection is broken
 * @retval TE_ENOMEM            Message is too long
 */
te_errno
rpc_transport_recv(rpc_transport_handle handle, uint8_t *buf,
                   size_t *p_len, int timeout)
{
    DWORD num;

    OVERLAPPED ov;

    if (handle >= max_pipe || !pipes[handle].valid)
        return TE_RC(TE_RCF_PCH, TE_ECONNRESET);

    memset(&ov, 0, sizeof(ov));
    ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    pipes[handle].read = TRUE;
    if (pipes[handle].wait)
    {
        pipes[handle].wait = FALSE;
        CancelIo(pipes[handle].in_handle);
    }

    if (!ReadFile(pipes[handle].in_handle, buf, *p_len, &num,
                  &ov))
    {
        if (GetLastError() != ERROR_IO_PENDING)
        {
            ERROR("Failed to read from the pipe: %d", GetLastError());
            if (*log != 0)
                RING("Dead log:\n%s", log);
            pipes[handle].read = FALSE;
            CloseHandle(ov.hEvent);
            return TE_RC(TE_RCF_PCH, TE_ECONNRESET);
        }

        while (TRUE)
        {
            num = WaitForSingleObjectEx(ov.hEvent,
                                        timeout * 1000, TRUE);
            if (num == WAIT_TIMEOUT)
            {
                CancelIo(pipes[handle].in_handle);
                pipes[handle].read = FALSE;
                CloseHandle(ov.hEvent);
                return TE_RC(TE_RCF_PCH, TE_ETIMEDOUT);
            }
            if (num == WAIT_OBJECT_0)
                break;
        }
        CloseHandle(ov.hEvent);

        if (!GetOverlappedResult(pipes[handle].in_handle,
                                 &ov, &num, FALSE))
        {
            ERROR("Failed to read from the pipe: %d", GetLastError());
            if (*log != 0)
                RING("Dead log:\n%s", log);
            pipes[handle].read = FALSE;
            return TE_RC(TE_RCF_PCH, TE_ECONNRESET);
        }

        if (num == 0)
        {
            ERROR("0 bytes are received");
            return TE_RC(TE_RCF_PCH, TE_ECONNRESET);
        }
    }

    pipes[handle].read = FALSE;

    *p_len = num;
    return 0;
}

/**
 * Send message.
 *
 * @param handle   connection handle
 * @param buf      buffer for writing
 * @param len      message length
 *
 * @return Status code.
 * @retval TE_ECONNRESET        Connection is broken
 */
te_errno
rpc_transport_send(rpc_transport_handle handle, const uint8_t *buf,
                   size_t len)
{
    DWORD num;

    if (handle >= max_pipe || !pipes[handle].valid)
        return TE_RC(TE_RCF_PCH, TE_ECONNRESET);

    SleepEx(1, TRUE); /* windows bug work-around: sometimes
                         datagram is lost without this */

    if (!WriteFile(pipes[handle].out_handle, buf, len, &num, NULL))
    {
        ERROR("Failed to write to the pipe: %d", GetLastError());
        return TE_RC(TE_RCF_PCH, TE_ECONNRESET);
    }

    return 0;
}

#endif /* CYGWIN || WINDOWS && ENABLE_LOCAL_TRANSPORT */
