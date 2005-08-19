/** @file
 * @brief Unix Test Agent
 *
 * Routines for FTP testing.
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
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
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "FTP"

#include "te_config.h"
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "te_errno.h"
#include "unix_internal.h"

#include "logger_ta.h"
#if 1
#undef ERROR
#define ERROR(_msg...)  PRINT(_msg)
#undef VERB
#define VERB(_msg...)   PRINT(_msg)
#undef ENTRY
#define ENTRY(_msg...)  PRINT(_msg)
#endif

#define FTP_TEST_LOGIN_MAX      32
#define FTP_TEST_PASSWD_MAX     32
#define FTP_TEST_PATHNAME_MAX   64

#define FTP_URI                 "ftp://"
#define FTP_PORT                21
#define FTP_DATA_PORT           20

static void
sigint_handler(int n)
{
    (void)n;
    exit(1);
}

/**
 * Read the answer from the socket. Parameters are the same as in read().
 * This function fails if it can't read full response.
 */
static int
read_all(int s, char *buf, int n)
{
    char *str = buf;
    int   len = 0;
    char *line = buf - 1;

    *buf = 0;

    while (TRUE)
    {
        struct timeval tv = {4, 0};
        fd_set         set;
        int            l;
        char          *new_line = line;

        FD_ZERO(&set);
        FD_SET(s, &set);
        if (select(s + 1, &set, NULL, NULL, &tv) == 0)
        {
            errno = ETIMEDOUT;
            return -1;
        }

        if ((l = read(s, str, n - len)) <= 0)
            return -1;

        len += l;
        str += l;

        while ((line == buf - 1) || (new_line = strchr(line, '\n')) != NULL)
        {
            new_line++;
            if (str - new_line >= 4)
            {
                if (new_line[3] == ' ')
                    return len;
                line = new_line;
            }
            else
                break;
        }
    }
}

/**
 * Parse the URI provided to ftp_open,
 * URI should comply to following format: 
 *     ftp://[user[:password]@]server[:port]/directory/file
 *
 * If user is empty, anonymous is used. If password is empty, empty
 * password is used. If server is empty, 127.0.0.1 is used.
 *
 * @param uri      URI string
 * @param srv      location for server
 * @param user     location for user (should be at least FTP_TEST_LOGIN_MAX
 * @param passwd   location for password (should be at least 
 *                 FTP_TEST_PASSWD_MAX)
 * @param pathname location for target pathname (should be at least
 *                 FTP_TEST_PATHNAME_MAX)
 *
 * @return 0 (success) or -1 (invalid argument)
 */
static int
parse_ftp_uri(const char *uri, struct sockaddr *srv,
              char *user, char *passwd, char *pathname)
{
    char *s = (char *)uri;
    char *tmp;
    char *tmp1;
    char *server;
    int   len;
    
    struct sockaddr_in *addr = (struct sockaddr_in *)srv;
    
    if (uri == NULL || srv == NULL || user == NULL || passwd == NULL ||
        pathname == NULL)
    {
        return -1;
    }
    
    if (strncmp(s, FTP_URI, strlen(FTP_URI)) != 0)
        return -1;
    s += strlen(FTP_URI);
    
    /* Parse user info */
    tmp = strchr(s, '@');
    if (tmp != NULL)
    {
        tmp1 = strchr(s, ':');

        if (tmp1 != NULL && tmp1 < tmp)
        {
            len = tmp1 - s;
            
            if (len >= FTP_TEST_LOGIN_MAX || 
                (tmp - tmp1) > FTP_TEST_PASSWD_MAX)
            {
                return -1;
            }
            strncpy(user, s, len);
            user[len] = 0;
            tmp1++;
            len = tmp - tmp1;
            strncpy(passwd, tmp1, len);
            passwd[len] = 0;
        }
        else
        {
            len = tmp - s;
            if (len >= FTP_TEST_LOGIN_MAX)
                return -1;
            strncpy(user, s, len);
            user[len] = 0;
            *passwd = 0;
        }
        s = tmp + 1;
    }
    else
    {
        strcpy(user, "anonymous");
        *passwd = 0;
    }
    
    if ((server = strdup(s)) == NULL)
        return -1;
        
    memset(addr, 0, sizeof(*addr));
    addr->sin_family = AF_INET;
    if ((tmp = strchr(server, ':')) != NULL)
    {
        long int n;
        
        *tmp++ = 0;
        n = strtol(tmp, &tmp1, 10);
        if (tmp1 == NULL || *tmp1 != '/' || n == 0 || n > 0xFFFF)
        {
            free(server);
            return -1;
        }
        addr->sin_port = htons((unsigned short)n);
        tmp = tmp1 + 1;
    }
    else
    {
        if ((tmp = strchr(server, '/')) == NULL)
        {
            free(server);
            return -1;
        }
        *tmp++ = 0;
        addr->sin_port = htons(FTP_PORT);
    }
    if (strlen(tmp) >= FTP_TEST_PATHNAME_MAX)
    {
        free(server);
        return -1;
    }
    strcpy(pathname, tmp);
    if (*server != 0)
    {
        struct hostent *hostinfo;
        
        if ((hostinfo = gethostbyname(server)) == NULL) 
        {
            free(server);
            return -1;
        }
        memcpy(&(addr->sin_addr), hostinfo->h_addr_list[0],
               hostinfo->h_length);
    }
    else
        addr->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        
    free(server);
    return 0;
}

/* See description in unix_internal.h */
int
ftp_open(const char *uri, int flags, int passive, int offset, int *sock)
{
    te_bool new_sock;
    int     control_socket = -1;
    int     data_socket = -1;
    int     active_listening = -1;

    char        buf[1024];
    char       *str;

    struct sockaddr_in addr;
    struct sockaddr_in addr1;
    socklen_t          addr1_len = sizeof(addr1);
    uint8_t            inaddr[4];
    
    char user[FTP_TEST_LOGIN_MAX];
    char passwd[FTP_TEST_PASSWD_MAX];
    char file[FTP_TEST_PATHNAME_MAX];


    ENTRY("%s(): %s %d %s %d %p(%d)", __FUNCTION__,
          uri, flags, passive ? "PASSIVE" : "", offset,
          sock, (sock != NULL) ? *sock : -1);

    if (parse_ftp_uri(uri, SA(&addr), user, passwd, file) != 0 ||
        (flags != O_RDONLY && flags != O_WRONLY))
    {
        ERROR("parse_ftp_uri() failed");
        errno = EINVAL;
        return -1;
    }

#define RET_ERR(_msg...) \
    do {                                \
        ERROR(_msg);                    \
        if (data_socket >= 0)           \
            close(data_socket);         \
        if (active_listening >= 0)      \
            close(active_listening);    \
        if (control_socket >= 0)        \
            close(control_socket);  \
        return -1;                      \
    } while (0)

#define PUT_CMD(_cmd...) \
    do {                                                    \
        sprintf(buf, _cmd);                                 \
        VERB("Request: %s", buf);                           \
        if (write(control_socket, buf, strlen(buf)) < 0)    \
            RET_ERR("write() failed; errno %d", errno);     \
    } while (0)

#define READ_ANS \
    do {                                                    \
        memset(buf, 0, sizeof(buf));                        \
        if (read_all(control_socket, buf, sizeof(buf)) < 0) \
            RET_ERR("read_all() failed");                   \
        VERB("Response: %s", buf);                          \
        if (*buf == '4' || *buf == '5')                     \
            RET_ERR("Invalid answer: %s", buf);             \
    } while (0)
    
#define CMD(_cmd...) \
    do {               \
        PUT_CMD(_cmd); \
        READ_ANS;      \
    } while (0)

    if ((sock == NULL) || (*sock == -1))
    {
        VERB("Connecting...");
        control_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (control_socket < 0)
            RET_ERR("socket() for control connection failed: errno %d",
                    errno);
        if (connect(control_socket, SA(&addr), sizeof(addr)) != 0)
            RET_ERR("connect() failed; errno %d", errno);
        VERB("Connected");
        new_sock = TRUE;
    }
    else
    {
        control_socket = *sock;
        new_sock = FALSE;
        memset(buf, 0, sizeof(buf));
        if (read_all(control_socket, buf, sizeof(buf)) > 0)
            VERB("Response: %s", buf);
    }

    /* Determine parameters for PORT command */
    if (!passive)
    {
        if (getsockname(control_socket, SA(&addr1), &addr1_len) != 0)
            RET_ERR("getsockname() failed; errno %d", errno);

        memcpy(inaddr, &addr1.sin_addr, sizeof(inaddr));
        memset(&addr1, 0, sizeof(addr1));
        addr1.sin_family = AF_INET;
        
        active_listening = socket(AF_INET, SOCK_STREAM, 0);
        if (active_listening < 0)
            RET_ERR("socket() to listen to failed: errno %d", errno);
        
        if (bind(active_listening, SA(&addr1), sizeof(addr1)) != 0)
            RET_ERR("bind() failed; errno %d", errno);
        
        if (listen(active_listening, 1) < 0)
            RET_ERR("listen() failed; errno %d", errno);
        
        if (getsockname(active_listening, SA(&addr1), &addr1_len) != 0)
            RET_ERR("getsockname() failed; errno %d", errno);
    }

    /* Read greeting */
    if (new_sock)
    {
        READ_ANS;
        CMD("USER %s\r\n", user);
        CMD("PASS %s\r\n", passwd);
    }

    if (passive)
        CMD("PASV\r\n");
    else
        CMD("PORT %d,%d,%d,%d,%d,%d\r\n",
            inaddr[0], inaddr[1], inaddr[2], inaddr[3],
            ntohs(addr1.sin_port) / 0x100, ntohs(addr1.sin_port) % 0x100);
    
    if ((str = strchr(buf, '(')) == NULL)
    {
        addr.sin_port = htons(FTP_DATA_PORT);
    }
    else
    {
        uint8_t *a = (uint8_t *)&(addr.sin_addr);
        int      i;
        
        /* Here we assume that FTP server provides correct answer */
        for (i = 0, str++; i < 4; str = strchr(str, ',') + 1, i++)
            *a++ = atoi(str);
        a = (unsigned char *)&(addr.sin_port);
        *a++ = atoi(str);
        *a = atoi(strchr(str, ',') + 1);
    }

    CMD("TYPE I\r\n");
    
    CMD("REST %d\r\n", offset);
    
    if (flags == O_RDONLY)
        PUT_CMD("RETR %s\r\n", file);
    else
        PUT_CMD("STOR %s\r\n", file);
    
    if (passive)
    {
        VERB("Connecting to data port");
        data_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (data_socket < 0)
            RET_ERR("socket() for data connection failed: errno %d",
                    errno);
        if (connect(data_socket, SA(&addr), sizeof(addr)) != 0)
            RET_ERR("connect() for data connection failed; errno %d",
                    errno);
        VERB("Data connection is established");                     
        READ_ANS;
    }
    else
    {
        READ_ANS;

        VERB("Accepting data connection");
        if ((data_socket = accept(active_listening, NULL, NULL)) < 0)
            RET_ERR("accept() failed; errno %d", errno);
        VERB("Data connection is established");                     

        if (close(active_listening) != 0)
            RET_ERR("close() of active listening socket failed: "
                    "errno %d", errno);
    }

    if (sock == NULL)
    {
        ERROR("You MUST provide location for control connection "
              "when using ftp_open! I'll close control connection, "
              "but this violate RFC959. Test MUST be rewritten!");
        close(control_socket);
    }
    else
        *sock = control_socket;

    return data_socket;

#undef CMD
#undef READ_ANS
#undef PUT_CMD    
#undef RET_ERR    
}

/* See description in unix_internal.h */
int 
ftp_close(int control_socket)
{
    char   *cmd = "QUIT\r\n";
    char    buf[1024];

    /* Read from control socket all responses that may be here. */
    memset(buf, 0, sizeof(buf));
    if (read_all(control_socket, buf, sizeof(buf)) > 0)
    {
        VERB("Response: %s", buf);
        memset(buf, 0, sizeof(buf));
    }

    VERB("Request: %s", cmd);
    if (write(control_socket, cmd, strlen(cmd)) < 0)
    {
        ERROR("%s: write(QUIT) failed; errno %d", __FUNCTION__, errno);
        close(control_socket);
        return -1;
    }
    if (read_all(control_socket, buf, sizeof(buf)) < 0)
    {
        ERROR("%s: read after QUIT failed; buf = %s, errno %d", 
              __FUNCTION__, buf, errno);
        close(control_socket);
        return -1;
    }
    VERB("Response: %s", buf);
    if (close(control_socket) != 0)
    {
        ERROR("close() of control connection socket failed: "
                "errno %d", errno);
        return -1;
    }
    return 0;
}

#define FTP_GET_BULK    6144  /**< Size to be read in one read() call */
#define FTP_PUT_BULK    6144  /**< Size to be write in one write() call */

typedef struct {
    int s;
    int size;
} test_proc_arg;

/**
 * Thread entry point for reading data via TP.
 */
static void *
read_test(void *arg)
{
    char    buf[FTP_GET_BULK];
    int     len = 0;
    int     sock = (int)(long int)arg;
    fd_set  rd;
    int     ret, l;

    struct timeval  total = { 600, 0 };
    struct timeval  before, after, timeout;


    while (total.tv_sec >= 0 && total.tv_usec >= 0)
    {
        FD_ZERO(&rd);
        FD_SET(sock, &rd);
        timeout = total;

        gettimeofday(&before, NULL);
        ret = select(sock + 1, &rd, NULL, NULL, &timeout);
        gettimeofday(&after, NULL);fflush(stdout);
        if (ret < 0)
        {
            ERROR("FTP get select() error errno=%d", errno);
            return (void *)(long)errno;
        }
        if (ret == 0)
        {
            ERROR("FTP get timed out: received %d", len);
            return (void *)ETIMEDOUT;
        }

        if (FD_ISSET(sock, &rd))
        {
            l = read(sock, buf, sizeof(buf));
            if (l < 0)
            {
                ERROR("FTP get read() error errno=%d", errno);
                return (void *)(long)errno;
            }
            if (l == 0)
            {
                return (void *)0;
            }
                
            len += l;
        }

        total.tv_sec -= after.tv_sec - before.tv_sec;
        total.tv_usec -= after.tv_usec - before.tv_usec;
        while (total.tv_usec < 0)
        {
            total.tv_sec--;
            total.tv_usec += 1000000;
        }
    }

    ERROR("FTP get timed out: received %d", len);

    return (void *)ETIMEDOUT;
}

/**
 * Thread entry point for writing data via TP.
 */
static void *
write_test(void *arg_void)
{
    char buf[FTP_PUT_BULK];
    int  len = 0;

    struct timeval  start, now, timeout;
    
    test_proc_arg *arg = (test_proc_arg *)arg_void;


    memset(buf, '1', sizeof(buf));
    
    gettimeofday(&start, NULL);
    while (len < arg->size)
    {
        int l = send(arg->s, buf, sizeof(buf), MSG_DONTWAIT);

        if (l < 0)
        {
            int ret;

            if (errno != EAGAIN)
            {
                ERROR("FTP put write() error errno=%d", errno);
                return (void *)(long)errno;
            }

            gettimeofday(&now, NULL);
            timeout.tv_sec = 600 - (now.tv_sec - start.tv_sec);
            timeout.tv_usec = 0 - (now.tv_usec - start.tv_usec);
            while (timeout.tv_usec < 0)
            {
                timeout.tv_sec--;
                timeout.tv_usec += 1000000;
            }

            if (timeout.tv_sec >= 0)
            {
                fd_set  wr;

                FD_ZERO(&wr);
                FD_SET(arg->s, &wr);
                ret = select(arg->s + 1, NULL, &wr, NULL, &timeout);
                if (ret < 0)
                {
                    ERROR("select() errno=%d", errno);
                    return (void *)(long)errno;
                }
            }
            else
            {
                ret = 0;
            }
            if (ret == 0)
            {
                ERROR("FTP put timed out: sent %d", len);
                return (void *)ETIMEDOUT;
            }
        }
        else if (l == 0)
        {
            ERROR("FTP put write() failed - connection broken");
            return (void *)ECONNRESET;
        }
        else
        {
            len += l;
        }
    }
    
    return (void *)0;
}

/**
 * Simple read/write test for FTP. Should be called via RCF.
 */
int
ftp_test(char *uri_get, char *uri_put, int size)
{
    int si = -1;
    int so = -1;
    
    pthread_t ti;
    pthread_t to;
    
    int rc1 = 0;
    int rc2 = 0;

    VERB("Get: %s Put: %s size %d\n", uri_get, uri_put,
               size);
    
    signal(SIGINT, sigint_handler); 
    
    if (uri_get != NULL && *uri_get != 0 && 
        (si = ftp_open(uri_get, O_RDONLY, 1, 0, NULL)) < 0)
    {
        ERROR("Failed to open URI %s to read from", uri_get);
        return TE_EIO;
    }

    if (uri_put != NULL && *uri_put != 0 && 
        (so = ftp_open(uri_put, O_WRONLY, 1, 0, NULL)) < 0)
    {
        ERROR("Failed to open URI %s to write to", uri_put);
        close(si);
        return TE_EIO;
    }
    
    VERB("Open OK\n");

    if (si >= 0 && pthread_create(&ti, NULL, read_test, 
                                  (void *)(long)si) < 0)
    {
        int rc = errno;

        ERROR("Failed to create 'read' thread");
        close(si);
        if (so >= 0)
            close(so);
            
        return rc;
    }
    
    if (so >= 0)
    {
        test_proc_arg arg;
        
        arg.s = so;
        arg.size = size;
        if (pthread_create(&to, NULL, write_test, (void *)&arg) < 0)
        {
            int rc = errno;

            ERROR("Failed to create 'write' thread");
            close(so);
            if (si >= 0)
            {
                pthread_kill(ti, SIGINT);
                close(si);
            }
            return rc;
        }
    }

    VERB("Waiting for finish of the transmission\n");
    
    if (si >= 0)
    {
        pthread_join(ti, (void **)&rc1);
        close(si);
        if (rc1 != 0)
            ERROR("Read test failed %X", rc1);
    }
        
    if (so >= 0)
    {
        pthread_join(to, (void **)&rc2);
        close(so);
        if (rc2 != 0)
            ERROR("Write test failed %X", rc2);
    }
    
    VERB("Results: %X %X\n", rc1, rc2);
        
    return (rc1 != 0) ? rc1 : rc2;
}

