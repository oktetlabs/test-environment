/** @file
 * @brief Linux Test Agent
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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

#define TE_LGR_USER      "FTP"
#include "logger_ta.h"

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
 * Read one string from the socket. Parameters are the same as in read().
 */
static int
read_string(int s, char *buf, int n)
{
    char *str = buf;
    int   len = 0;
    
    *buf = 0;
    while (len < n)
    {
        int l = read(s, str, n - len);
        if (l < 0)
            return l;
        len += l;
        
        if (buf[len - 1] == '\n')
            return len;
            
        str += l;
    }
    return len;
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
        if (tmp1 == NULL || *tmp1 != '/' || n == 0 || n > 0xFFFFFF)
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

/**
 * Open the connection for reading/writing the file.
 *
 * @param uri           FTP uri: ftp://user:password@server/directory/file
 * @param flags         O_RDONLY or O_WRONLY
 * @param passive       if 1, passive mode
 * @param offset        file offset
 * @param sock          pointer on socket
 *
 * @return file descriptor, which may be used for reading/writing data
 */
int
ftp_open(char *uri, int flags, int passive, int offset, int *sock)
{
    char  buf[1024];
    char *str;
    int   s;
    int   sd = -1;
    int   sd1 = -1;
    int   new_sock = 0;
    struct timeval tv;
    fd_set set;

    struct sockaddr_in addr;
    struct sockaddr_in addr1;
    socklen_t          addr1_len = sizeof(addr1);
    uint8_t            inaddr[4];
    
    char user[FTP_TEST_LOGIN_MAX];
    char passwd[FTP_TEST_PASSWD_MAX];
    char file[FTP_TEST_PATHNAME_MAX];
    
    if (parse_ftp_uri(uri, (struct sockaddr *)&addr,
                      user, passwd, file) != 0 ||
        (flags != O_RDONLY && flags != O_WRONLY))
    {
        ERROR("parse_ftp_uri() failed");
        return -1;
    }
    
#define RET_ERR(_msg...)    \
    do {                 \
        ERROR(_msg);     \
        if (sd >= 0)     \
            close(sd);   \
        if (sd1 >= 0)    \
            close(sd);   \
        close(s);        \
        return -1;       \
    } while (0)

#define PUT_CMD(_cmd...) \
    do {                                                \
        sprintf(buf, _cmd);                             \
        VERB("%s", buf);                                \
        if (write(s, buf, strlen(buf)) < 0)             \
            RET_ERR("write() failed; errno %d", errno); \
    } while (0)

#define READ_ANS \
    do {                                           \
        memset(buf, 0, sizeof(buf));               \
        if (read_string(s, buf, sizeof(buf)) < 0)  \
            RET_ERR("read_string() failed");       \
        VERB("%s", buf);                           \
        if (*buf == '4' || *buf == '5')            \
            RET_ERR("Invalid answer: %s", buf);    \
    } while (0)
    
#define CMD(_cmd...) \
    do {               \
        PUT_CMD(_cmd); \
        READ_ANS;      \
    } while (0)

    if ((sock == NULL) || (*sock == 0))
    {
        VERB("Connecting...");
        s = socket(AF_INET, SOCK_STREAM, 0);
        if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
            RET_ERR("connect() failed; errno %d", errno);
        if (sock != NULL)
        {
            *sock = s;
            new_sock = 1;
        }
        VERB("Connected");
    }
    else
    {
        s = *sock;

        FD_ZERO(&set);
        FD_SET(s, &set);
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        if (select(s + 1, &set, NULL, NULL, &tv) > 0)
            READ_ANS;
    }
    /* Determine parameters for PORT command */
    if (!passive)
    {
        if (getsockname(s, (struct sockaddr *)&addr1, &addr1_len) < 0)
            RET_ERR("getsockname() failed; errno %d", errno);

        memcpy(inaddr, &addr1.sin_addr, sizeof(inaddr));
        memset(&addr1, 0, sizeof(addr1));
        addr1.sin_family = AF_INET;
        sd1 = socket(AF_INET, SOCK_STREAM, 0);

        if (bind(sd1, (struct sockaddr *)&addr1, sizeof(addr1)) < 0)
            RET_ERR("bind() failed; errno %d", errno);
            
        if (listen(sd1, 1) < 0)
            RET_ERR("listen() failed; errno %d", errno);

        if (getsockname(sd1, (struct sockaddr *)&addr1, &addr1_len) < 0)
            RET_ERR("getsockname() failed; errno %d", errno);
    }

    /* Read greeting */
    if ((sock == NULL) || (new_sock))
    {
        read(s, buf, sizeof(buf));
        CMD("USER %s\n", user);
        CMD("PASS %s\n", passwd);
    }
    if (passive)
        CMD("PASV\n");
    else
        CMD("PORT %d,%d,%d,%d,%d,%d\n",
                inaddr[0], inaddr[1], inaddr[2], inaddr[3],
                ntohs(addr1.sin_port) / 256, ntohs(addr1.sin_port) % 256);
    
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

    CMD("TYPE I\n");
    
    CMD("REST %d\n", offset);
    
    if (flags == O_RDONLY)
        PUT_CMD("RETR %s\n", file);
    else
        PUT_CMD("STOR %s\n", file);
    
    if (passive)
    {
        sd = socket(AF_INET, SOCK_STREAM, 0);
        if (connect(sd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
             RET_ERR("connect() for data connection failed; "
                     "errno %d", errno);
        READ_ANS;
    }
    else
    {
        READ_ANS;
        if ((sd = accept(sd1, NULL, NULL)) < 0)
            RET_ERR("accept() failed; errno %d", errno);

        close(sd1);
    }
    if (sock == NULL)
        close(s);
    return sd;

#undef RET_ERR    
#undef READ_ANS
#undef PUT_CMD    
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
    int     sock = (int)arg;
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
            ERROR("FTP get select() error errno=%d",
                                     errno);
            return (void *)errno;
        }
        if (ret == 0)
        {
            ERROR("FTP get timed out: received %d",
                       len);
            return (void *)ETIMEDOUT;
        }

        if (FD_ISSET(sock, &rd))
        {
            l = read(sock, buf, sizeof(buf));
            if (l < 0)
            {
                ERROR("FTP get read() error "
                                         "errno=%d", errno);
                return (void *)errno;
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
                ERROR("FTP put write() error "
                                         "errno=%d", errno);
                return (void *)errno;
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
                    return (void *)errno;
                }
            }
            else
            {
                ret = 0;
            }
            if (ret == 0)
            {
                ERROR("FTP put timed out: sent %d",
                           len);
                return (void *)ETIMEDOUT;
            }
        }
        else if (l == 0)
        {
            ERROR("FTP put write() failed - "
                                     "connection broken");
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
        ERROR("Failed to open URI %s to read from",
                   uri_get);
        return EIO;
    }

    if (uri_put != NULL && *uri_put != 0 && 
        (so = ftp_open(uri_put, O_WRONLY, 1, 0, NULL)) < 0)
    {
        ERROR("Failed to open URI %s to write to",
                   uri_put);
        close(si);
        return EIO;
    }
    
    VERB("Open OK\n");

    if (si >= 0 && pthread_create(&ti, NULL, read_test, (void *)si) < 0)
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
            ERROR("Read test failed 0x%x", rc1);
    }
        
    if (so >= 0)
    {
        pthread_join(to, (void **)&rc2);
        close(so);
        if (rc2 != 0)
            ERROR("Write test failed 0x%x", rc2);
    }
    
    VERB("Results: %d %d\n", rc1, rc2);
        
    return (rc1 != 0) ? rc1 : rc2;
}

