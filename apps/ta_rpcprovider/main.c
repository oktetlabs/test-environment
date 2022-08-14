/** @file
 * @brief TE RPC server
 *
 * TE-enabled RPC server
 *
 * Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "te_config.h"
#include "rpc_server.h"
#include "ta_common.h"
#include "agentlib.h"
#include "logger_ta.h"

#define MSG_PFX "ta_rpcs: "

static char ta_execname_storage[RCF_MAX_PATH];
const char *ta_execname = ta_execname_storage;
char ta_dir[RCF_MAX_PATH];

/**
 * Get identifier of the current thread.
 *
 * @return Thread identifier
 */
uint32_t
thread_self(void)
{
    return (unsigned int)pthread_self();
}

/**
 * Create a mutex.
 *
 * @return Mutex handle
 */
void *
thread_mutex_create(void)
{
    static pthread_mutex_t init = PTHREAD_MUTEX_INITIALIZER;

    pthread_mutex_t *mutex = (pthread_mutex_t *)calloc(1, sizeof(*mutex));

    if (mutex != NULL)
        *mutex = init;

    return (void *)mutex;
}

/**
 * Destroy a mutex.
 *
 * @param mutex     mutex handle
 */
void
thread_mutex_destroy(void *mutex)
{
    free(mutex);
}

/**
 * Lock the mutex.
 *
 * @param mutex     mutex handle
 *
 */
void
thread_mutex_lock(void *mutex)
{
    if (mutex == NULL)
        ERROR(MSG_PFX "%s: try to lock NULL mutex", __FUNCTION__);
    else
        pthread_mutex_lock(mutex);
}

/**
 * Unlock the mutex.
 *
 * @param mutex     mutex handle
 */
void
thread_mutex_unlock(void *mutex)
{
    if (mutex == NULL)
        ERROR(MSG_PFX "%s: try to unlock NULL mutex", __FUNCTION__);
    else
        pthread_mutex_unlock(mutex);
}


/* See description in ta_common.h */
int
rcf_rpc_server_init(void)
{
    return 0;
}

/* See description in ta_common.h */
int
rcf_rpc_server_finalize(void)
{
    return 0;
}

#if __linux__
/** ta_rpcprovider vsyscall page entrance */
const void *vsyscall_enter = NULL;
#endif

/** Default SIGINT action */
static struct sigaction sigaction_int;
/** Default SIGPIPE action */
static struct sigaction sigaction_pipe;

/**
 * Signal handler to be registered for SIGINT signal.
 */
static void
sigint_handler(void)
{
    fprintf(stderr, MSG_PFX "killed by SIGINT\n");
    exit(EXIT_FAILURE);
}

/**
 * Signal handler to be registered for SIGPIPE signal.
 */
static void
sigpipe_handler(void)
{
    static te_bool here = FALSE;

    if (!here)
    {
        here = TRUE;
        fprintf(stderr, MSG_PFX "SIGPIPE is received\n");
        here = FALSE;
    }
}

/*
 * TCE support
 */
int (*tce_stop_function)(void);
int (*tce_notify_function)(void);
int (*tce_get_peer_function)(void);
const char *(*tce_get_conn_function)(void);

extern rcf_symbol_entry generated_table[];

int
main(int argc, char **argv)
{
    int rc;
    char *sep;
    struct sigaction sigact;

    te_log_init("TARPCS", logfork_log_message);

    strcpy(ta_execname_storage, argv[0]);
    strcpy(ta_dir, ta_execname_storage);
    sep = strrchr(ta_dir, '/');
    if (sep == NULL)
        *ta_dir = '\0';
    else
        *sep = '\0';

    rcf_ch_register_symbol_table(generated_table);

    memset(&sigact, 0, sizeof(sigact));
    sigemptyset(&sigact.sa_mask);

    if (getenv("TE_LEAVE_SIGINT_HANDLER") == NULL)
    { /* some libinit tests need SIGINT handler untouched */
        sigact.sa_handler = (void *)sigint_handler;
        if (sigaction(SIGINT, &sigact, &sigaction_int) != 0)
        {
            rc = te_rc_os2te(errno);
            LOG_PRINT(MSG_PFX "Cannot set SIGINT action: %s", strerror(errno));
        }
    }
#if defined (__QNX__)
    /* 'getenv' may set errno to ESRCH even when successful */
    errno = 0;
#endif

    sigact.sa_handler = (void *)sigpipe_handler;
    if (sigaction(SIGPIPE, &sigact, &sigaction_pipe) != 0)
    {
        rc = te_rc_os2te(errno);
        LOG_PRINT(MSG_PFX "Cannot set SIGPIPE action: %s", strerror(errno));
    }

    rc = ta_process_mgmt_init();
    if (rc != 0)
    {
        LOG_PRINT(MSG_PFX "Cannot initialize process management: %d", rc);
        return rc;
    }

    if (argc < 2 || (strcmp(argv[1], "exec") == 0 && argc < 3))
    {
        ERROR(MSG_PFX "Invalid number of arguments: %d", argc);
        return -1;
    }

    if (strcmp(argv[1], "exec") == 0)
    {
        void (* func)(int, char **) = rcf_ch_symbol_addr(argv[2], 1);

        if (func == NULL)
        {
            ERROR(MSG_PFX "Cannot resolve address of the function %s", argv[2]);
            return 1;
        }
        func(argc - 3, argv + 3);
        return 0;
    }

    rcf_pch_rpc_server(argv[1]);
    return 0;
}
