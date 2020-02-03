/** @file
 * @brief Communications with DUT through conserver
 *
 * Communications with DUT through conserver implementation
 *
 * Copyright (C) 2003-2019 OKTET Labs. All rights reserved.
 *
 * @author Ivan Danilov <Ivan.Danilov@oktetlabs.ru>
 * @author Svetlana Menshikova <Svetlana.Menshikova@oktetlabs.ru>
 */

#define TE_LGR_USER     "RPC SERIAL"

#include "te_config.h"

#include <unistd.h>
#include <time.h>
#include <regex.h>

#include "logger_api.h"
#include "rpc_server.h"
#include "tarpc.h"
#include "te_dbuf.h"
#include "te_errno.h"
#include "te_kernel_log.h"
#include "te_queue.h"
#include "te_sockaddr.h"
#include "te_str.h"

#define CONSERVER_MAX_BUFLEN (RCF_MAX_PATH + 16)

/** Maximum length of accumulated log */
#define LOG_SERIAL_MAX_LEN      (2047)

/* Conserver escape sequences */
/** CTRL+ec sequence */
#define CONSERVER_ESCAPE        "\05c"
#define CONSERVER_CMD_LEN        (3)
#define CONSERVER_CMD_SPY        CONSERVER_ESCAPE "s"
#define CONSERVER_CMD_FORCE_RW   CONSERVER_ESCAPE "f"
#define CONSERVER_NEW_LINE      "\n"
#define CONSERVER_NEW_LINE_LEN  (1)
/** CTRL+C sequence */
#define CONSERVER_INTERRUPT     "\3"
#define CONSERVER_INTERRUPT_LEN (1)

/** Internal structure used to bufferize the regexp match */
typedef struct buffered_match {
    LIST_ENTRY(buffered_match) next; /**< Next entry */
    int     sock;   /**< Associated socket */

    te_dbuf dbuf;   /**< Catched data */
} buffered_match;

static LIST_HEAD(, buffered_match) buffered_matches =
    LIST_HEAD_INITIALIZER(buffered_matches);

/*
 * Get a buffered match for a socket
 *
 * @param sock Socket to get match for
 *
 * @return Pointer to a buffered match, or @c NULL on failure
 */
static buffered_match *
buffered_match_get(int sock)
{
    buffered_match *bm;

    LIST_FOREACH(bm, &buffered_matches, next)
    {
        if (bm->sock == sock)
            return bm;
    }

    return NULL;
}

/*
 * Add a buffered match for a socket
 *
 * @param sock  Socket to add/update match for
 *
 * @return Pointer to an created buffered match, or @c NULL on failure
 */
static buffered_match *
buffered_match_add(int sock)
{
    buffered_match *bm = malloc(sizeof(buffered_match));

    if (bm == NULL)
        return NULL;

    bm->sock = sock;
    bm->dbuf = (te_dbuf)TE_DBUF_INIT(100);
    LIST_INSERT_HEAD(&buffered_matches, bm, next);

    return bm;
}

/*
 * Delete a buffered match for a socket, so it no longer can be used
 *
 * @param sock  Socket to delete match for
 */
static void
buffered_match_delete(int sock)
{
    buffered_match *bm;

    LIST_FOREACH(bm, &buffered_matches, next)
    {
        if (bm->sock == sock)
        {
            LIST_REMOVE(bm, next);
            te_dbuf_free(&bm->dbuf);
            free(bm);
            return;
        }
    }
}

/*
 * Add/update a buffered match for a socket
 *
 * @param sock          Socket to add/update match for
 * @param buffer        Target buffer
 * @param buffer_len    Target buffer length
 *
 * @return Pointer to an updated/created buffered match, or @c NULL on failure
 */
static buffered_match *
buffered_match_add_update(int sock, const char *buffer, unsigned int buffer_len)
{
    buffered_match *bm;
    te_bool         created = FALSE;

    bm = buffered_match_get(sock);
    if (bm == NULL)
    {
        bm = buffered_match_add(sock);
        if (bm == NULL)
            return NULL;
        created = TRUE;
    }

    te_dbuf_reset(&bm->dbuf);
    if (buffer != NULL && te_dbuf_append(&bm->dbuf, buffer, buffer_len) != 0)
    {
        /*
         * If it is just created, let's remove, otherwise simply don't provide
         * the created match
         */
        if (created)
            buffered_match_delete(sock);
        bm = NULL;
    }
    return bm;
}

/* Open serial console session */
static int
serial_open(tarpc_serial_open_in *in, tarpc_serial_open_out *out)
{
    const char *inet_addr;
    char        address_buffer[CONSERVER_MAX_BUFLEN];
    struct sockaddr_storage sa;

    if (sockaddr_rpc2h(&in->sa, SA(&sa), sizeof(sa), NULL, NULL) != 0)
        return -1;

    inet_addr = te_sockaddr_get_ipstr(SA(&sa));
    if (inet_addr == NULL)
        return -1;

    TE_SPRINTF(address_buffer, "(%s):%u:%s:%s", inet_addr,
               (unsigned int)te_sockaddr_get_port(SA(&sa)),
               in->user, in->console);

    INFO("Conserver address: %s", address_buffer);
    out->sock = te_open_conserver(address_buffer);
    INFO("Conserver socket: %d", out->sock);

    if (out->sock == -1)
        return -1;

    return 0;
}

/* Convert read() or write() return value to buffer length */
static int
serial_readwrite_retval2buflen(int retval)
{
    return retval >= 0 ? retval : 0;
}

/* Convert write() return value to 0/-1 with respect to given expectation */
static int
serial_readwrite_retval_match_expectation(int retval, int expectation)
{
    return (retval == expectation) ? 0 : -1;
}

/*
 * Make socket blocking with given timeout.
 * @param sock          Target socket
 * @param timeout_ms    Target timeout. Set to @c -1 to make socket non-blocking
 *
 * @return @c 0 on success, @c -1 on error.
 */
static int
set_sock_blocking(int sock, int timeout_ms)
{
    struct timeval  tv;
    int             opts;

    TE_US2TV(TE_MS2US(timeout_ms), &tv);

    opts = fcntl(sock, F_GETFL);
    if (opts == -1)
    {
        ERROR("Failed to get file status flags for serial console: %s",
              strerror(errno));
        return -1;
    }

    if (fcntl(sock, F_SETFL,
              (timeout_ms != -1) ? (opts & (~O_NONBLOCK))
                                 : (opts | O_NONBLOCK)) == -1)
    {
        ERROR("Failed to set blocking file status flags for serial "
              "console: %s", strerror(errno));
        return -1;
    }

    if (timeout_ms != -1 &&
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv,
                   sizeof(struct timeval)) == -1)
    {
        ERROR("Failed to set receive timeout on serial console: %s",
              strerror(errno));
        if (fcntl(sock, F_SETFL, opts) == -1)
        {
            ERROR("Failed to reset file status flags for serial console: %s",
                  strerror(errno));
        }
        return -1;
    }

    return 0;
}

/* Read the data from serial console */
static int
serial_read(tarpc_serial_read_in *in, tarpc_serial_read_out *out)
{
    int             ret = -1;
    char           *buffer;
    buffered_match *buffered_match = buffered_match_get(in->sock);

    if (in->timeout >= 0)
        set_sock_blocking(in->sock, in->timeout);

    buffer = malloc(in->buflen);
    if (buffer == NULL)
    {
        ERROR("Failed to allocate output buffer: %s", strerror(errno));
        ret = -1;
        goto restore_opts;
    }

    /* Check if there is a saved buffer */
    if (buffered_match != NULL && buffered_match->dbuf.len != 0)
    {
        size_t new_len = buffered_match->dbuf.len;

        memcpy(buffer, buffered_match->dbuf.ptr,
               (in->buflen <= buffered_match->dbuf.len) ? in->buflen
                    : buffered_match->dbuf.len);
        if (in->buflen <= buffered_match->dbuf.len)
        {
            /*
             * Read less or equal to number of buffered bytes - simply copy.
             * That's done on previous step, so we only need to update buffer
             */
            new_len -= in->buflen;
            ret = in->buflen;
        }
        else
        {
            /* More than buffered - we need to read some more bytes */
            new_len = 0;
            ret = read(in->sock, &buffer[buffered_match->dbuf.len],
                       in->buflen - buffered_match->dbuf.len);
            if (ret >= 0)
            {
                /* Fix up returned length */
                ret += buffered_match->dbuf.len;
            }
            else
            {
                ret = -1;
                goto restore_opts;
            }
        }

        /*
         * Reallocate buffers. New length might be 0, if nothing should be
         * moved.
         *
         * Or > 0, then we have the following system of equations in CVC4. Let's
         * use it to prove
         * dbuf_len: BITVECTOR(32);
         * in_buf_len: BITVECTOR(32);
         * new_len: BITVECTOR(32);
         * ASSERT BVLE(in_buf_len, dbuf_len);
         * ASSERT new_len = BVSUB(32, dbuf_len, in_buf_len);
         * QUERY BVGE(in_buf_len, new_len);
         *
         * CVC4 claims that this query is invalid, therefore overlapping is
         * possible, then we cannot move the memory without allocating
         * an intermediate buffer.
         */
        if (new_len > 0)
        {
            char *new_buffer = malloc(new_len);

            if (new_buffer == NULL)
            {
                ERROR("Failed to reallocate buffered data: %s",
                      strerror(errno));
                ret = -1;
                goto restore_opts;
            }

            /* Replace buffers */
            memcpy(new_buffer, &buffered_match->dbuf.ptr[in->buflen], new_len);
            buffered_match_add_update(in->sock, new_buffer, new_len);
            free(new_buffer);
        }
        else
        {
            buffered_match_add_update(in->sock, NULL, 0);
        }
    }
    else
    {
        ret = read(in->sock, buffer, in->buflen);
    }

    if (ret >= 0)
    {
        out->buffer = buffer;
        out->buflen = serial_readwrite_retval2buflen(ret);
    }

restore_opts:
    if (ret == -1)
    {
        out->buffer = NULL;
        out->buflen = 0;
        free(buffer);
    }
    if (in->timeout >= 0)
        set_sock_blocking(in->sock, -1);

    return ret;
}

/* Close serial console session */
static int
serial_close(tarpc_serial_close_in *in)
{
    buffered_match_delete(in->sock);
    return close(in->sock);
}

/* Write to serial console with verbose output */
static int
serial_verbose_write(int fd, const void *buf, size_t count, int *p_errno)
{
    int ret;

    ret = write(fd, buf, count);
    if (ret == -1)
    {
        *p_errno = errno;
        ERROR("Failed to write to conserver's serial socket: %s",
              strerror(errno));
    }
    else if ((size_t)ret != count)
    {
        WARN("Not all data have been written to conserver's serial socket: "
             "%d < %zu", ret, count);
    }

    return ret;
}

/* Force read/write for console session */
static int
serial_force_rw(tarpc_serial_force_rw_in *in)
{
    int saved_errno = 0;
    int retval;

    retval = serial_readwrite_retval_match_expectation(
        serial_verbose_write(in->sock, CONSERVER_CMD_FORCE_RW,
                             CONSERVER_CMD_LEN, &saved_errno),
        CONSERVER_CMD_LEN);
    errno = saved_errno;
    return retval;
}

/* Force 'spy' mode for a console session */
static int
serial_spy(tarpc_serial_spy_in *in)
{
    int saved_errno = 0;
    int retval;

    retval = serial_readwrite_retval_match_expectation(
        serial_verbose_write(in->sock, CONSERVER_CMD_SPY, CONSERVER_CMD_LEN,
                             &saved_errno),
        CONSERVER_CMD_LEN);
    errno = saved_errno;
    return retval;
}

/* Write a string to a console session */
static int
serial_send_str(tarpc_serial_send_str_in *in, tarpc_serial_send_str_out *out)
{
    int saved_errno = 0;
    int retval;

    retval = serial_verbose_write(in->sock, in->str, in->buflen,
                                  &saved_errno);
    out->buflen = serial_readwrite_retval2buflen(retval);
    errno = saved_errno;
    return retval;
}

/* Send 'enter' to a console session */
static int
serial_send_enter(tarpc_serial_send_enter_in *in)
{
    int saved_errno = 0;
    int retval;

    retval = serial_readwrite_retval_match_expectation(
        serial_verbose_write(in->sock, CONSERVER_NEW_LINE,
                             CONSERVER_NEW_LINE_LEN, &saved_errno),
        CONSERVER_NEW_LINE_LEN);
    errno = saved_errno;
    return retval;
}

/* Send 'ctrl+c' to a console session */
static int
serial_send_ctrl_c(tarpc_serial_send_ctrl_c_in *in)
{
    int saved_errno = 0;
    int retval;

    retval = serial_readwrite_retval_match_expectation(
        serial_verbose_write(in->sock, CONSERVER_INTERRUPT,
                             CONSERVER_INTERRUPT_LEN, &saved_errno),
        CONSERVER_INTERRUPT_LEN);
    errno = saved_errno;
    return retval;
}

/* Flush buffers of a serial console session */
static int
serial_flush(tarpc_serial_flush_in *in)
{
    char    tmp;
    char   *tmp_ptr;
    size_t  flush_amount;

    if (in->amount > 0)
    {
        buffered_match *buffered_match = buffered_match_get(in->sock);

        if (buffered_match != NULL && buffered_match->dbuf.len != 0)
        {
            if (in->amount < buffered_match->dbuf.len)
            {
                size_t tmp_len;

                tmp_len = buffered_match->dbuf.len - in->amount;
                tmp_ptr = malloc(tmp_len);
                if (tmp_ptr == NULL)
                {
                    ERROR("Failed to flush buffered data: %s", strerror(errno));
                    return -1;
                }

                memcpy(tmp_ptr, &buffered_match->dbuf.ptr[in->amount], tmp_len);
                buffered_match_add_update(in->sock, tmp_ptr, tmp_len);
                free(tmp_ptr);
                return 0;
            }
            else
            {
                flush_amount = in->amount - buffered_match->dbuf.len;
                buffered_match_add_update(in->sock, NULL, 0);
            }
        }
        else
        {
            flush_amount = in->amount;
        }

        for (; flush_amount > 0; flush_amount--)
        {
            if (read(in->sock, &tmp, 1) <= 0)
            {
                break;
            }
        }
    }
    else
    {
        while (read(in->sock, &tmp, 1) > 0);
    }
    return 0;
}

/* Wait for a specific pattern in a console session */
static int
wait_pattern(int sock, const char *pattern, int *match_offset, int timeout_ms)
{
    regex_t     regexp;
    char        buffer[LOG_SERIAL_MAX_LEN] = { '\0' };
    size_t      offset;
    int         bytes_read;
    struct      timeval start;
    struct      timeval end;
    struct      timeval timeout;
    regmatch_t  matches[1];
    int         ret = -1;

    if (match_offset != NULL)
        *match_offset = -1;

    if (regcomp(&regexp, pattern, REG_ICASE) != 0)
    {
        ERROR("Regular expression (%s) is invalid: %s", pattern,
              strerror(errno));
        return -1;
    }

    if (gettimeofday(&start, NULL) == -1)
    {
        ERROR("Failed to get time of day: %s", strerror(errno));
        return -1;
    }

    if (timeout_ms != -1 && set_sock_blocking(sock, timeout_ms) == -1)
        return -1;

    TE_US2TV(TE_MS2US((timeout_ms != -1) ? timeout_ms : 0), &timeout);

    for (;;)
    {
        memset(buffer, 0, sizeof(buffer));
        offset = 0;
        for (;;)
        {
            bytes_read = read(sock, &buffer[offset], 1);
            if (bytes_read == 0)
            {
                /*
                 * No bytes means closed console. As we haven't found pattern,
                 * we can error out
                 */
                WARN("Console reached EOF before finding pattern");
                break;
            }
            else if (bytes_read < 0)
            {
                /*
                 * We got error. If it is temporary error and timeout = -1
                 * (wait until all), then we continue. Otherwise check if we are
                 * beyond the timeout
                 */
                if (errno == EAGAIN)
                {
                    if (timeout_ms != -1)
                    {
                        struct timeval diff;

                        gettimeofday(&end, NULL);

                        timersub(&end, &start, &diff);
                        if (timercmp(&diff, &timeout, >=))
                        {
                            ERROR("Wait pattern timed out");
                            goto restore_opts;
                        }
                    }
                    break;
                }

                /* Non-block read failed */
                ERROR("Failed to read from console, errno: %s",
                      strerror(errno));
                goto restore_opts;
            }

            if (buffer[offset] == '\r')
                continue;
            if (buffer[offset] == '\n' || buffer[offset] == '\0')
                break;
            if (offset < (LOG_SERIAL_MAX_LEN - 2))
            {
                offset++;
            }
            else
            {
                ERROR("Failed to find pattern as internal buffer space is "
                      "exceeded");
                goto restore_opts;
            }
        }

        if (regexec(&regexp, buffer, 1, matches, 0) == 0)
        {
            /* Found the regexp */
            break;
        }

        if (bytes_read == 0)
            goto restore_opts;
    }
    if (match_offset)
        *match_offset = matches->rm_so;

    buffered_match_add_update(sock, buffer, offset);
    ret = 0;

restore_opts:
    set_sock_blocking(sock, -1);
    return ret;
}

/* Check the pattern in a console session */
static int
serial_check_pattern(tarpc_serial_check_pattern_in *in,
                     tarpc_serial_check_pattern_out *out)
{
    return wait_pattern(in->sock, in->pattern, &out->offset, 0);
}

/* A wrapper for waiting a specific pattern in a console session */
static int
serial_wait_pattern(tarpc_serial_wait_pattern_in *in,
                    tarpc_serial_wait_pattern_out *out)
{
    return wait_pattern(in->sock, in->pattern, &out->offset, in->timeout);
}


TARPC_FUNC_STATIC(serial_open, {},
{
    MAKE_CALL(out->retval = func_ptr(in, out));
})

TARPC_FUNC_STATIC(serial_read, {},
{
    MAKE_CALL(out->retval = func_ptr(in, out));
})

TARPC_FUNC_STATIC(serial_spy, {},
{
    MAKE_CALL(out->retval = func_ptr(in));
})

TARPC_FUNC_STATIC(serial_send_enter, {},
{
    MAKE_CALL(out->retval = func_ptr(in));
})

TARPC_FUNC_STATIC(serial_send_ctrl_c, {},
{
    MAKE_CALL(out->retval = func_ptr(in));
})

TARPC_FUNC_STATIC(serial_flush, {},
{
    MAKE_CALL(out->retval = func_ptr(in));
})

TARPC_FUNC_STATIC(serial_send_str, {},
{
    MAKE_CALL(out->retval = func_ptr(in, out));
})

TARPC_FUNC_STATIC(serial_close, {},
{
    MAKE_CALL(out->retval = func_ptr(in));
})

TARPC_FUNC_STATIC(serial_force_rw, {},
{
    MAKE_CALL(out->retval = func_ptr(in));
})

TARPC_FUNC_STATIC(serial_check_pattern, {},
{
    MAKE_CALL(out->retval = func_ptr(in, out));
})

TARPC_FUNC_STATIC(serial_wait_pattern, {},
{
    MAKE_CALL(out->retval = func_ptr(in, out));
})
