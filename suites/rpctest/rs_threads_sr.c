/** @file
 * @brief Test Environment
 *
 * RPC servers threads send and receive each other.
 * 
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Evgeniy Vladimirovich Bobkov <kavri@oktetlabs.ru>
 * 
 * $Id$
 */

#define TE_TEST_NAME    "rs_threads_sr"

#include "rpc_suite.h"

#define CLIENTS_NUM      32
#define BUF_SIZE1        100123
#define BUF_SIZE2        100543
#define PATTERN1         0x54
#define PATTERN2         0x37

/**
 * Set a free port to be used with the given address.
 *
 * @param addr_      Pointer to the address structure.
 */
#define SET_FREE_PORT(addr_)                                       \
    do {                                                           \
        int rc_;                                                   \
                                                                   \
        rc_ = tapi_allocate_port(&(SIN(addr_)->sin_port));         \
        if (rc_ != 0)                                              \
            TEST_FAIL("tapi_allocate_port() returned %d", rc_);    \
    } while (0)

/**
 * Send the whole buffer.
 *
 * @param pco_        PCO where to send from
 * @param sock_       Socket descriptor at PCO
 * @param buf_        Pointer to a buffer
 * @param buflen_     Buffer size
 */
#define SEND_WHOLE_BUF(pco_, sock_, buf_, buflen_)                         \
    do {                                                                   \
        int total_ = 0;  /* Total bytes has been sent */                   \
        int current_;  /* sent by a single send() */                       \
                                                                           \
        while(total_ < buflen_)                                            \
        {                                                                  \
            current_ = rpc_send(pco_, sock_, (void *)buf_ + total_,        \
                                buflen_ - total_, 0);                      \
            if (current_ <= 0)                                             \
            {                                                              \
                ERROR("rpc_send() unexpectedly returned %d in %dth "       \
                      "thread, errno=%s", current_, this_num,              \
                       errno_rpc2str(RPC_ERRNO(pco_)));                    \
                goto finish;                                               \
            }                                                              \
            total_ += current_;                                            \
        }                                                                  \
    } while(0)

/**
 * Receive the whole buffer.
 *
 * @param pco_        PCO where to receive at
 * @param sock_       Socket descriptor at PCO
 * @param buf_        Pointer to a buffer
 * @param buflen_     Buffer size
 */
#define RECV_WHOLE_BUF(pco_, sock_, buf_, buflen_)                         \
    do {                                                                   \
        int total_ = 0;    /* Total bytes has been received */             \
        int current_;  /* Received by a single recv() */                   \
                                                                           \
        while(total_ < buflen_)                                            \
        {                                                                  \
            current_ = rpc_recv(pco_, sock_, (void *)buf_ + total_,        \
                                buflen_ - total_, 0);                      \
            if (current_ <= 0)                                             \
            {                                                              \
                ERROR("rpc_recv() unexpectedly returned %d in %dth "       \
                      "thread, errno=%s", current_, this_num,              \
                       errno_rpc2str(RPC_ERRNO(pco_)));                    \
                goto finish;                                               \
            }                                                              \
            total_ += current_;                                            \
        }                                                                  \
    } while(0)

/** Describes the threads start routines arguments. */
typedef struct _wt_arg_s {
    int               this_num;  /**< This thread number */
    rcf_rpc_server   *rs;        /**< RPC server */
    int               sock;      /**< Socket descriptor */
} wt_arg_s;

/** A server_thread start routine. */
static void *
server_thread(void *arg)
{
#define wt_arg ((wt_arg_s *)arg)
    int              this_num = wt_arg->this_num;
    rcf_rpc_server  *rs = wt_arg->rs;
    int              sock = wt_arg->sock;
    char             *recvbuf;
    char             *sendbuf;
    void             *retval = (void *)-1;
    int               i;

    sendbuf = malloc(BUF_SIZE2);
    recvbuf = malloc(BUF_SIZE1);
    memset(sendbuf, PATTERN2, BUF_SIZE2);
    memset(recvbuf, 0, BUF_SIZE1);

    RPC_AWAIT_IUT_ERROR(rs);
    RECV_WHOLE_BUF(rs, sock, recvbuf, BUF_SIZE1);

    RPC_AWAIT_IUT_ERROR(rs);
    SEND_WHOLE_BUF(rs, sock, sendbuf, BUF_SIZE2);
    
    retval = (void *)0;

    for (i = 0; i < BUF_SIZE1; i++)
    {
        if (recvbuf[i] != PATTERN1)
        {
            ERROR("ST recvbuf[%d]=%x differs from %x",
                          i, recvbuf[i], PATTERN1);
            retval = (void *)-1;        
        }
    }

finish:
    free(sendbuf);
    free(recvbuf);

    return retval;
#undef wt_arg
}

/** A client_thread start routine. */
static void *
client_thread(void *arg)
{
#define wt_arg ((wt_arg_s *)arg)
    int              this_num = wt_arg->this_num;
    rcf_rpc_server  *rs = wt_arg->rs;
    int              sock = wt_arg->sock;
    char             *recvbuf;
    char             *sendbuf;
    void             *retval = (void *)-1;
    int               i;

    sendbuf = malloc(BUF_SIZE1);
    recvbuf = malloc(BUF_SIZE2);
    memset(sendbuf, PATTERN1, BUF_SIZE1);
    memset(recvbuf, 0, BUF_SIZE2);

    RPC_AWAIT_IUT_ERROR(rs);
    SEND_WHOLE_BUF(rs, sock, sendbuf, BUF_SIZE1);

    RPC_AWAIT_IUT_ERROR(rs);
    RECV_WHOLE_BUF(rs, sock, recvbuf, BUF_SIZE2);
    
    retval = (void *)0;

    for (i = 0; i < BUF_SIZE2; i++)
    {
        if (recvbuf[i] != PATTERN2)
        {
            ERROR("CT recvbuf[%d]=%x differs from %x",
                          i, recvbuf[i], PATTERN2);
            retval = (void *)-1;        
        }
    }

finish:
    free(sendbuf);
    free(recvbuf);

    return retval;
#undef wt_arg
}

int 
main(int argc, char *argv[])
{
    rcf_rpc_server          *pco_iut = NULL;
    rcf_rpc_server          *pco_tst = NULL;
    rcf_rpc_server         **pco_iut_st = NULL; /* IUT RPC server threads */
    rcf_rpc_server         **pco_tst_ct = NULL; /* TST RPC server threads */
    int                      iut_sl = -1;
    int                      iut_s[CLIENTS_NUM];
    int                      tst_s[CLIENTS_NUM];
    const struct sockaddr   *iut_addr;
    socklen_t                iut_addrlen;
    const struct sockaddr   *tst_addr;
    struct sockaddr         *tst_addrs[CLIENTS_NUM];
    socklen_t                tst_addrlen;

    char          **st_name = NULL;   /* IUT RPC server threads names */
    char          **ct_name = NULL;   /* TST RPC server threads names */
    wt_arg_s       *st_args = NULL;   /* Server threads arguments */
    wt_arg_s       *ct_args = NULL;   /* Client threads arguments */
    pthread_t      *st_ids = NULL;    /* Server threads identifiers */
    pthread_t      *ct_ids = NULL;    /* Client threads identifiers */
    te_bool         one_of_threads_failed = FALSE;
    void           *tret;
    int             i;

    TEST_START;

    TEST_GET_PCO(pco_iut);
    TEST_GET_PCO(pco_tst);
    TEST_GET_ADDR(iut_addr, iut_addrlen);
    TEST_GET_ADDR(tst_addr, tst_addrlen);

    for (i = 0; i < CLIENTS_NUM; i++)
    {
        /* Create an address */
        tst_addrs[i] =
            (struct sockaddr *)malloc(sizeof(struct sockaddr_storage));
        memcpy(tst_addrs[i], tst_addr, sizeof(struct sockaddr));
        if (i > 0)
            SET_FREE_PORT(tst_addrs[i]);

        /* Create the client sockets */
        tst_s[i] = rpc_socket(pco_tst, RPC_PF_INET, RPC_SOCK_STREAM, 
                              RPC_PROTO_DEF);
        RPC_BIND(pco_tst, tst_s[i], tst_addrs[i], iut_addrlen);
    }

    iut_sl = rpc_socket(pco_iut, RPC_PF_INET, RPC_SOCK_STREAM, 
                        RPC_PROTO_DEF);
    RPC_BIND(pco_iut, iut_sl, iut_addr, iut_addrlen);
    RPC_LISTEN(pco_iut, iut_sl, 64);

    pco_iut_st =
        (rcf_rpc_server **)calloc(CLIENTS_NUM, sizeof(rcf_rpc_server *));

    pco_tst_ct =
        (rcf_rpc_server **)calloc(CLIENTS_NUM, sizeof(rcf_rpc_server *));

    st_name = (char **)calloc(CLIENTS_NUM, sizeof(char *));
    ct_name = (char **)calloc(CLIENTS_NUM, sizeof(char *));

    st_args = (wt_arg_s *)calloc(CLIENTS_NUM, sizeof(wt_arg_s));
    ct_args = (wt_arg_s *)calloc(CLIENTS_NUM, sizeof(wt_arg_s));

    st_ids = (pthread_t *)calloc(CLIENTS_NUM, sizeof(pthread_t));
    ct_ids = (pthread_t *)calloc(CLIENTS_NUM, sizeof(pthread_t));

    for (i = 0; i < CLIENTS_NUM; i++)
    {
        /* Connect a client socket to server */
        RPC_CONNECT(pco_tst, tst_s[i], iut_addr, iut_addrlen);

        iut_s[i] = rpc_accept(pco_iut, iut_sl, NULL, NULL);

        st_name[i] = (char *)malloc(RCF_MAX_NAME);
        sprintf(st_name[i], "%s_%d", pco_iut->name, i);
        if (rcf_rpc_server_thread_create(pco_iut, st_name[i],
                                         &pco_iut_st[i]) != 0)
        {
            TEST_FAIL("ST %dth rcf_rpc_server_thread_create() failed");
        }
        
        /* "server" threads arguments */
        st_args[i].this_num = i;
        st_args[i].rs = pco_iut_st[i];
        st_args[i].sock = iut_s[i];

        ct_name[i] = (char *)malloc(RCF_MAX_NAME);
        sprintf(ct_name[i], "%s_%d", pco_tst->name, i);
        if (rcf_rpc_server_thread_create(pco_tst, ct_name[i],
                                         &pco_tst_ct[i]) != 0)
        {
            TEST_FAIL("CT %dth rcf_rpc_server_thread_create() failed");
        }

        /* "client" threads arguments */
        ct_args[i].this_num = CLIENTS_NUM + i;
        ct_args[i].rs = pco_tst_ct[i];
        ct_args[i].sock = tst_s[i];
    }

    /* Start server threads */
    for (i = 0; i < (int)CLIENTS_NUM; i++)
    {
        if (pthread_create(&st_ids[i], NULL, server_thread,
                           (void *)&st_args[i]) != 0)
        {
            rc = errno;
            TEST_FAIL("Failed to create %dth server thread", i);
        }
    }

    /* Start client threads */
    for (i = 0; i < CLIENTS_NUM; i++)
    {
        if (pthread_create(&ct_ids[i], NULL, client_thread,
                           (void *)&ct_args[i]) != 0)
        {
            rc = errno;
            TEST_FAIL("Failed to create %dth client thread", i);
        }
    }

    /* Wait for server threads termination */
    for (i = 0; i < (int)CLIENTS_NUM; i++)
    {
        if ((rc = pthread_join(st_ids[i], &tret)) != 0)
            TEST_FAIL("ST pthread_join() failed with 0x%X", rc);

        st_ids[i] = 0;

        if (tret != (void*)0) /* At least one of threads failed */
            one_of_threads_failed = TRUE;
    }

    /* Wait for client threads termination */
    for (i = 0; i < CLIENTS_NUM; i++)
    {
        if ((rc = pthread_join(ct_ids[i], &tret)) != 0)
            TEST_FAIL("CT pthread_join() failed with 0x%X", rc);

        ct_ids[i] = 0;

        if (tret != (void*)0) /* At least one of threads failed */
            one_of_threads_failed = TRUE;
    }

    if (one_of_threads_failed == TRUE)
        TEST_STOP;

    TEST_SUCCESS;

cleanup:
    for (i = 0; i < (int)CLIENTS_NUM; i++)
    {
        if (st_ids[i] != 0)
            pthread_join(st_ids[i], NULL);

        rcf_rpc_server_destroy(pco_iut_st[i]);

        if (ct_ids[i] != 0)
            pthread_join(ct_ids[i], NULL);

        rcf_rpc_server_destroy(pco_tst_ct[i]);

        CLEANUP_RPC_CLOSE(pco_iut, iut_s[i]);
        CLEANUP_RPC_CLOSE(pco_tst, tst_s[i]);

        free(tst_addrs[i]);
        if (st_name != NULL)
            free(st_name[i]);
        if (ct_name != NULL)
            free(ct_name[i]);
    }

    CLEANUP_RPC_CLOSE(pco_iut, iut_sl);

    free(pco_iut_st);
    free(pco_tst_ct);
    free(st_args);
    free(ct_args);
    free(st_ids);
    free(ct_ids);

    TEST_END;
}
