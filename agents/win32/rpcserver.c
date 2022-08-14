/** @file
 * @brief Windows Test Agent
 *
 * Standalone RPC server implementation
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "tarpc_server.h"
#include "rpc_xdr.h"

#ifdef WINDOWS

/**
 * Convert 'struct timeval' to 'struct tarpc_timeval'.
 *
 * @param tv_h      Pointer to 'struct timeval'
 * @param tv_rpc    Pointer to 'struct tarpc_timeval'
 */
int
timeval_h2rpc(const struct timeval *tv_h, struct tarpc_timeval *tv_rpc)
{
    tv_rpc->tv_sec  = tv_h->tv_sec;
    tv_rpc->tv_usec = tv_h->tv_usec;

    return 0;
}

/**
 * Convert 'struct tarpc_timeval' to 'struct timeval'.
 *
 * @param tv_rpc    Pointer to 'struct tarpc_timeval'
 * @param tv_h      Pointer to 'struct timeval'
 */
int
timeval_rpc2h(const struct tarpc_timeval *tv_rpc, struct timeval *tv_h)
{
    tv_h->tv_sec  = tv_rpc->tv_sec;
    tv_h->tv_usec = tv_rpc->tv_usec;

    return 0;
}

int
gettimeofday(struct timeval *tv, struct timezone *tz)
{
    SYSTEMTIME t;

    UNUSED(tz);

    GetSystemTime(&t);
    tv->tv_sec = time(NULL);
    tv->tv_usec = t.wMilliseconds * 1000;

    return 0;
}

#endif

#include "../../lib/rcfpch/rcf_pch_rpc_server.c"

extern int win32_process_exec(int argc, char **argv);

int
main(int argc, char **argv)
{
    WSADATA data;

    te_log_init("(win32_rpcserver)", logfork_log_message);

    if (win32_process_exec(argc, argv) != 0)
        return 1;

    if (argc > 2 && strcmp(argv[2], "net_init") == 0)
    {
        WSAStartup(MAKEWORD(2,2), &data);
        wsa_func_handles_discover();
    }

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    rcf_pch_rpc_server(argv[1] == NULL ? "Unnamed" : argv[1]);

    _exit(0);
}

