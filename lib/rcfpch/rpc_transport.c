/** @file 
 * @brief RCF RPC
 *
 * Different transports which can be used for interaction between 
 * RPC server and TA: implementation.
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
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
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"
#include "te_stdint.h"
#include "ta_common.h"
#include "te_errno.h"
#include "te_defs.h"
#include "rpc_transport.h"

/** Timeout for RPC operations */
#define RPC_TIMEOUT             10000   /* 10 seconds */

#ifndef RPC_TRANSPORT
#if defined(__CYGWIN__) || defined(WINDOWS)
#define RPC_TRANSPORT   RPC_TRANSPORT_WINPIPE
#else
#define RPC_TRANSPORT   RPC_TRANSPORT_UNIX
#endif
#endif

#if (RPC_TRANSPORT == RPC_TRANSPORT_WINPIPE)

#ifdef __CYGWIN__
#include <windows.h>
#endif

#undef ERROR

#else

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_TIME_H
#include <time.h>
#endif
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#if (RPC_TRANSPORT == RPC_TRANSPORT_TCP)

#if HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif

#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#else

#if HAVE_SYS_UN_H
#include <sys/un.h>
#endif

/** Maximum length of the name in sockaddr_un */
#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX   sizeof(((struct sockaddr_un *)0)->sun_path)
#endif

#endif 
#endif

#include "logger_api.h"

#if (RPC_TRANSPORT == RPC_TRANSPORT_TCP || \
     RPC_TRANSPORT == RPC_TRANSPORT_UNIX)
/** Listening socket */
static int lsock = -1;

/** Set for listening multiple sessions */
static fd_set rset;

/* 
 * MSG_MORE is used for performance reasons.
 * If it is not supported, it is not critical.
 */
#ifndef MSG_MORE
#define MSG_MORE    0
#endif

#endif

#if (RPC_TRANSPORT == RPC_TRANSPORT_WINPIPE)

/** Maximum number of simultaneous connections */
#define RPC_MAX_CONN    256

/** Prefix for pipe filenames */
#define PIPE_PREFIX     "\\\\.\\pipe\\"

typedef struct winpipe {
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

#endif

#define PRINT(msg...) \
    do {                \
        printf(msg);    \
        printf("\n");   \
        fflush(stdout); \
    } while (0)

#if (RPC_TRANSPORT == RPC_TRANSPORT_TCP)

/**
 * Enable TCP_NODEALY on the socket, if possible.
 *
 * @param sock      A socket
 */
static void
tcp_nodelay_enable(int sock)
{
#ifdef TCP_NODELAY
    int nodelay = 1;

    if (setsockopt(sock,
#ifdef SOL_TCP
                   SOL_TCP,
#else
                   IPPROTO_TCP
#endif
                   TCP_NODELAY, (void *)&nodelay, sizeof(nodelay)) != 0)
    {
        int err = TE_OS_RC(TE_RCF_PCH, errno);

        WARN("Failed to enable TCP_NODELAY on RPC server socket: "
              "error=%r", err);
    }
#else
    UNUSED(sock);
#endif
}

#endif

/**
 * Initialize RPC transport.
 */
te_errno
rpc_transport_init()
{
    char port[64];
    
#if (RPC_TRANSPORT == RPC_TRANSPORT_WINPIPE)
    int  i;
    
    for (i = 0; i < (int)(sizeof(pipes) / sizeof(winpipe)); i++)
        pipes[i].in_handle = pipes[i].out_handle = INVALID_HANDLE_VALUE;

    conn_mutex = CreateMutex(NULL, FALSE, NULL);

    sprintf(port, PIPE_PREFIX "tarpc_%lu_%d", 
            GetCurrentProcessId(), time(NULL));

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
    
#else

#if RPC_TRANSPORT == RPC_TRANSPORT_TCP
    struct sockaddr_in  addr;
#endif    
#if RPC_TRANSPORT == RPC_TRANSPORT_UNIX
    struct sockaddr_un  addr;
#endif    

    int len;
    
#define RETERR(msg...) \
    do {                                           \
        te_errno rc = TE_OS_RC(TE_RCF_PCH, errno); \
                                                   \
        ERROR(msg);                                \
        if (lsock >= 0)                            \
            close(lsock);                          \
        return rc;                                 \
    } while (0)

#if (RPC_TRANSPORT == RPC_TRANSPORT_TCP)
    if ((lsock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        RETERR("Failed to open listening socket for RPC servers");
#endif        
        
#if (RPC_TRANSPORT == RPC_TRANSPORT_UNIX)
    if ((lsock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        RETERR("Failed to open listening socket for RPC servers");
#endif        

#if HAVE_FCNTL_H
    /* 
     * Try to set close-on-exec flag, but ignore failures, 
     * since it's not critical.
     */
    (void)fcntl(lsock, F_SETFD, FD_CLOEXEC);
#endif

    memset(&addr, 0, sizeof(addr));
#if (RPC_TRANSPORT == RPC_TRANSPORT_TCP)
    addr.sin_family = AF_INET;
    len = sizeof(struct sockaddr_in);
#else
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), 
             "/tmp/terpc_%lu", (unsigned long)getpid());
    len = sizeof(addr) - UNIX_PATH_MAX + strlen(addr.sun_path);
#endif    

#ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
    ((struct sockaddr *)&addr)->sa_len = len;
#endif

    if (bind(lsock, (struct sockaddr *)&addr, len) < 0)
        RETERR("Failed to bind RPC listening socket");
        
    if (listen(lsock, 1) < 0)
        RETERR("listen() failed for RPC listening socket");

#if (RPC_TRANSPORT == RPC_TRANSPORT_TCP)
    /* TCP_NODEALY is not critical */
    (void)tcp_nodelay_enable(lsock);

    if (getsockname(lsock, (struct sockaddr *)&addr, &len) < 0)
        RETERR("getsockname() failed for listening socket for RPC servers");

    sprintf(port, "%d", addr.sin_port);
#else    
    strcpy(port, addr.sun_path);
#endif
        
    if (setenv("TE_RPC_PORT", port, 1) < 0)
        RETERR("Failed to set TE_RPC_PORT environment variable");

    return 0;
    
#undef RETERR    

#endif
} 

/**
 * Shutdown RPC transport.
 */
void 
rpc_transport_shutdown()
{
#if (RPC_TRANSPORT == RPC_TRANSPORT_WINPIPE)
    int i;
    
    for (i = 0; i < max_pipe; i++)
    {
        CloseHandle(pipes[i].ov.hEvent);
        if (pipes[i].valid)
        {
            CloseHandle(pipes[i].in_handle);
            CloseHandle(pipes[i].out_handle);
        }
    }
    
    CloseHandle(conn_mutex);
    CloseHandle(lov.hEvent);
    CloseHandle(lpipe);
#endif
#if (RPC_TRANSPORT == RPC_TRANSPORT_UNIX)
    char *name = getenv("TE_RPC_PORT");

    if (name != NULL)
        unlink(name);
#endif        
}

#if (RPC_TRANSPORT == RPC_TRANSPORT_WINPIPE)
static te_errno
get_free_pipe(int *p_handle)
{
    int i;
    
    for (i = 0; i < max_pipe && pipes[i].valid; i++);
    
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
        ERROR("Connect timeout on auxiliary file");
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

#endif


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
#if (RPC_TRANSPORT == RPC_TRANSPORT_WINPIPE)
    te_errno rc;
    char     pipename[64];
    int      i;
    
    UNUSED(name);
    
    if ((rc = rpc_transport_recv_pname(pipename, sizeof(pipename))) != 0)
        return rc;
        
    WaitForSingleObject(conn_mutex, INFINITE);
    
    if ((rc = get_free_pipe(&i)) != 0)
    {
        ReleaseMutex(conn_mutex);
        return rc;
    }

    if ((rc = open_out(pipename, "_1", &pipes[i].out_handle)) != 0)
    {
        ReleaseMutex(conn_mutex);
        return rc;
    }                      
    
    if ((rc = open_in(pipename, "_2", &pipes[i].ov, 
                      &pipes[i].in_handle)) != 0)
    {
        CloseHandle(pipes[i].out_handle);
        ReleaseMutex(conn_mutex);
        return rc;
    }                      
    
    pipes[i].valid = TRUE;
    ReleaseMutex(conn_mutex);
    *p_handle = i;
    
    return 0;

#else

    struct timeval  tv = { RPC_TIMEOUT / 1000, 0 };
    fd_set          set;
    int             sock;
    int             rc;
    
#define RETERR(msg...) \
    do {                                           \
        te_errno rc = TE_OS_RC(TE_RCF_PCH, errno); \
                                                   \
        ERROR(msg);                                \
        return rc;                                 \
    } while (0)
    
    UNUSED(name);

    FD_ZERO(&set);
    FD_SET(lsock, &set);
    
    while ((rc = select(lsock + 1, &set, NULL, NULL, &tv)) <= 0)
    {
        if (rc == 0)
        {
            ERROR("RPC server '%s' does not try to connect", name);
            return TE_RC(TE_RCF_PCH, TE_ETIMEDOUT);
        }
#ifndef WINDOWS
        else if (errno != EINTR)
        {
            RETERR("select() failed with unexpected errno %d", errno);
        }
#endif        
    }
    
    if ((sock = accept(lsock, NULL, NULL)) < 0)
        RETERR("Failed to accept connection from RPC server %s", name);

#if HAVE_FCNTL_H
    /* 
     * Try to set close-on-exec flag, but ignore failures, 
     * since it's not critical.
     */
    (void)fcntl(sock, F_SETFD, FD_CLOEXEC);
#endif

    *p_handle = (rpc_transport_handle)sock;

    return 0;

#undef RETERR

#endif
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

#if (RPC_TRANSPORT == RPC_TRANSPORT_WINPIPE)
{
    char     pipename[64];
    te_errno rc;
    int      i;
    
    sprintf(pipename, PIPE_PREFIX "%s_%lu_%lu_%d", 
            name, GetCurrentProcessId(), GetCurrentThreadId(), time(NULL));

    if ((rc = rpc_transport_send_pname(pipename)) != 0)
        return rc;

    WaitForSingleObject(conn_mutex, INFINITE);
    if ((rc = get_free_pipe(&i)) != 0)
    {
        ReleaseMutex(conn_mutex);
        return rc;
    }
    
    if ((rc = open_in(pipename, "_1", &pipes[i].ov, 
                      &pipes[i].in_handle)) != 0)
    {
        ReleaseMutex(conn_mutex);
        return rc;
    }                      
    
    if ((rc = open_out(pipename, "_2", &pipes[i].out_handle)) != 0)
    {
        CloseHandle(pipes[i].in_handle);
        ReleaseMutex(conn_mutex);
        return rc;
    }                      

    pipes[i].valid = TRUE;
    ReleaseMutex(conn_mutex);
    *p_handle = i;

    return 0;
    
}

#else
{
#define RETERR(msg...) \
    do {                                           \
        te_errno rc = TE_OS_RC(TE_RCF_PCH, errno); \
                                                   \
        ERROR(msg);                                \
        if (s >= 0)                                \
            close(s);                              \
        return rc;                                 \
    } while (0)

#if (RPC_TRANSPORT == RPC_TRANSPORT_TCP)
    struct sockaddr_in  addr;
#else    
    struct sockaddr_un  addr;
#endif    

    int   s = -1;
    int   sock_len;
    
    UNUSED(name);

    if (lsock > 0)
        close(lsock);
    
    memset(&addr, 0, sizeof(addr));
#if (RPC_TRANSPORT == RPC_TRANSPORT_TCP)
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = atoi(port);
    sock_len = sizeof(struct sockaddr_in);
#else
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, port);
    sock_len = sizeof(addr) - UNIX_PATH_MAX + strlen(addr.sun_path);
#endif

#ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
    ((struct sockaddr *)&addr)->sa_len = sock_len;
#endif

#if (RPC_TRANSPORT == RPC_TRANSPORT_TCP)
    s = socket(AF_INET, SOCK_STREAM, 0);
#else
    s = socket(AF_UNIX, SOCK_STREAM, 0);
#endif    
    if (s < 0)
        RETERR("Failed to open socket");

    if (connect(s, (struct sockaddr *)&addr, sock_len) != 0)
        RETERR("Failed to connect to TA");
    
    /* Enable linger with positive timeout on the socket  */
    {
        struct linger l = { 1, 1 };

        if (setsockopt(s, SOL_SOCKET, SO_LINGER, (void *)&l, 
                       sizeof(l)) != 0)
        {
            RETERR("Failed to enable linger on RPC server socket");
        }
    }

#if (RPC_TRANSPORT == RPC_TRANSPORT_TCP)
    (void)tcp_nodelay_enable(s);
#endif

    *p_handle = (rpc_transport_handle)s;

    return 0;
}
#endif
}

/** 
 * Break the connection.
 *
 * @param handle      connection handle 
 */
void 
rpc_transport_close(rpc_transport_handle handle)
{
#if (RPC_TRANSPORT == RPC_TRANSPORT_WINPIPE)
    assert(handle < max_pipe);
    
    WaitForSingleObject(conn_mutex, INFINITE);
    pipes[handle].valid = FALSE;

    CloseHandle(pipes[handle].in_handle);
    CloseHandle(pipes[handle].out_handle);
    
    pipes[handle].in_handle = pipes[handle].out_handle =
    INVALID_HANDLE_VALUE;

    ReleaseMutex(conn_mutex);
        
#else
    if ((int)handle > 0 && close((int)handle) < 0)
        ERROR("close() for RPC transport socket failed with errno", errno);
#endif        
}

/**
 * Reset set of descriptors to wait.
 */
void 
rpc_transport_read_set_init()
{
#if (RPC_TRANSPORT == RPC_TRANSPORT_WINPIPE)
    events_num = 0;
#else
    FD_ZERO(&rset);
#endif
}

/**
 * Add the handle to the read set.
 *
 * @param handle        connection handle
 */
void 
rpc_transport_read_set_add(rpc_transport_handle handle)
{
#if (RPC_TRANSPORT == RPC_TRANSPORT_WINPIPE)
    assert(handle < max_pipe);
    if (pipes[handle].valid && !pipes[handle].read)
    {
        static DWORD tmp;
        
        ResetEvent(pipes[handle].ov.hEvent);
        ReadFile(pipes[handle].in_handle, NULL, 0, &tmp, &pipes[handle].ov);
        events[events_num++] = pipes[handle].ov.hEvent;
        pipes[handle].wait = TRUE;
    }
#else
    FD_SET((int)handle, &rset);
#endif
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
#if (RPC_TRANSPORT == RPC_TRANSPORT_WINPIPE)
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
    
#else
     struct timeval tv;
     
     tv.tv_sec = timeout;
     tv.tv_usec = 0;
     
    if (select(FD_SETSIZE, &rset, NULL, NULL, &tv) < 0)
    {
#ifndef WINDOWS 
        if (errno != EINTR)
#endif        
            return FALSE;
    }
    
    return TRUE;
#endif    
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
#if (RPC_TRANSPORT == RPC_TRANSPORT_WINPIPE)
    te_bool  rc = FALSE;
    
    if (pipes[handle].valid)
    {
        rc = ReadFile(pipes[handle].in_handle, NULL, 
                      0, NULL, &pipes[handle].ov);
    
        if (!rc)
            CancelIo(pipes[handle].in_handle);
    }
        
    return rc;
#else
    return FD_ISSET((int)handle, &rset);
#endif    
}

#if (RPC_TRANSPORT != RPC_TRANSPORT_WINPIPE)
/** Recieve exact number of bytes from the stream */
static te_errno
recv_from_stream(int handle, uint8_t *buf, size_t len, int timeout)
{
    size_t rcvd = 0;
    
    while (rcvd < len)
    {
        fd_set set;
        int    rc;
    
        struct timeval tv = { timeout, 0 };
    
        FD_ZERO(&set);
        FD_SET(handle, &set);
        
        rc = select(handle + 1, &set, NULL, NULL, &tv);
        
        if (rc == 0)
        {
            return TE_RC(TE_RCF_PCH, TE_ETIMEDOUT);
        }
        else if (rc < 0)
        {
#ifndef WINDOWS 
            if (errno == EINTR)
                continue;
#endif                
            return TE_OS_RC(TE_RCF_PCH, errno);
        }
    
        rc = recv(handle, buf + rcvd, len - rcvd, 0);
        
        if (rc <= 0)
            return TE_RC(TE_RCF_PCH, TE_ECONNRESET);
        
        rcvd += rc;
    }
        
    return 0;
}
#endif

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
#if (RPC_TRANSPORT == RPC_TRANSPORT_WINPIPE)
    DWORD num;
    
    if (handle >= max_pipe || !pipes[handle].valid)
        return TE_RC(TE_RCF_PCH, TE_ECONNRESET);
        
    pipes[handle].read = TRUE;
    if (pipes[handle].wait)
    {
        pipes[handle].wait = FALSE;
        CancelIo(pipes[handle].in_handle);
        SleepEx(10, TRUE);
    }
    again:
    ResetEvent(pipes[handle].ov.hEvent);
    
    if (!ReadFile(pipes[handle].in_handle, buf, *p_len, &num, 
                  &pipes[handle].ov))
    {
     
        if (GetLastError() != ERROR_IO_PENDING)
        {
            ERROR("Failed to read from the pipe: %d", GetLastError());
            pipes[handle].read = FALSE;
            return TE_RC(TE_RCF_PCH, TE_ECONNRESET);
        }
        
        while (TRUE)
        {
            num = WaitForSingleObjectEx(pipes[handle].ov.hEvent, 
                                        timeout * 1000, TRUE);
            if (num == WAIT_TIMEOUT)
            {
                CancelIo(pipes[handle].in_handle);
                pipes[handle].read = FALSE;
                return TE_RC(TE_RCF_PCH, TE_ETIMEDOUT);
            }
            if (num == WAIT_OBJECT_0)
                break;
        }
    
        if (!GetOverlappedResult(pipes[handle].in_handle, 
                                 &pipes[handle].ov, &num, FALSE))
        {
            ERROR("Failed to read from the pipe: %d", GetLastError());
            pipes[handle].read = FALSE;
            return TE_RC(TE_RCF_PCH, TE_ECONNRESET);
        }
        
        if (num == 0)
        {
            ERROR("0 bytes are received");
            goto again;
        }
    } 

    pipes[handle].read = FALSE;
        
    *p_len = num;
    return 0;
    
#else
    uint8_t  lenbuf[4];
    size_t   len;
    te_errno rc;
    
    if ((rc = recv_from_stream((int)handle, lenbuf, 
                               sizeof(lenbuf), timeout)) != 0)
    {
        return rc;
    }
    
    len = ((int)lenbuf[0] << 24) + ((int)lenbuf[1] << 16) +
          ((int)lenbuf[2] << 8) + (int)lenbuf[3];
          
    if (len > *p_len)
        return TE_RC(TE_RCF_PCH, TE_ENOMEM);

    if (timeout == 0)
        timeout = RPC_TIMEOUT / 1000;
        
    if ((rc = recv_from_stream((int)handle, buf, len, timeout)) != 0)
        return TE_RC(TE_RCF_PCH, TE_ECONNRESET);
        
    *p_len = len;
    
    return 0;

#endif
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
#if (RPC_TRANSPORT == RPC_TRANSPORT_WINPIPE)
    DWORD num;

    if (handle >= max_pipe || !pipes[handle].valid)
        return TE_RC(TE_RCF_PCH, TE_ECONNRESET);
    
    if (!WriteFile(pipes[handle].out_handle, buf, len, &num, NULL))
    {
        ERROR("Failed to write to the pipe: %d", GetLastError());
        return TE_RC(TE_RCF_PCH, TE_ECONNRESET);
    }
        
    return 0;
#else
    uint8_t lenbuf[4];

    lenbuf[0] = (uint8_t)(len >> 24);
    lenbuf[1] = (uint8_t)((len >> 16) & 0xFF);
    lenbuf[2] = (uint8_t)((len >> 8) & 0xFF);
    lenbuf[3] = (uint8_t)(len & 0xFF);
    
    if (send((int)handle, lenbuf, sizeof(lenbuf), 0) != sizeof(lenbuf) ||
        send((int)handle, buf, len, 0) != (int)len)
    {
        return TE_RC(TE_RCF_PCH, TE_ECONNRESET);
    }
    
    return 0;
#endif
}

