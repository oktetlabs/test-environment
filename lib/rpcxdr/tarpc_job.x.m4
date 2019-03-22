/** @file
 * @brief RPC for Agent job control
 *
 * Definition of RPC structures and functions for Agent job control
 *
 * Copyright (C) 2019 OKTET Labs.
 *
 * @author Igor Romanov <Igor.Romanov@oktetlabs.ru>
 */

/* job_create() */
struct tarpc_job_create_in {
    struct tarpc_in_arg common;

    string spawner<>;
    string tool<>;
    tarpc_string argv<>;
    tarpc_string env<>;
};

struct tarpc_job_create_out {
    struct tarpc_out_arg common;

    tarpc_int               retval;
    tarpc_uint job_id;
};

/* job_start */
struct tarpc_job_start_in {
    struct tarpc_in_arg common;

    tarpc_uint job_id;
};

struct tarpc_job_start_out {
    struct tarpc_out_arg common;

    tarpc_int retval;
};

/* job_kill */
struct tarpc_job_kill_in {
    struct tarpc_in_arg common;

    tarpc_uint job_id;
    tarpc_signum signo;
};

struct tarpc_job_kill_out {
    struct tarpc_out_arg common;

    tarpc_int retval;
};

/* job_wait */
struct tarpc_job_wait_in {
    struct tarpc_in_arg common;

    tarpc_uint job_id;
    tarpc_int timeout_ms;
};

enum tarpc_job_status_type {
    TARPC_JOB_STATUS_EXITED,
    TARPC_JOB_STATUS_SIGNALED,
    TARPC_JOB_STATUS_UNKNOWN
};

struct tarpc_job_status {
    tarpc_job_status_type type;
    tarpc_int value;
};

struct tarpc_job_wait_out {
    struct tarpc_out_arg common;

    tarpc_job_status status;
    tarpc_int retval;
};

/* job_destroy */
struct tarpc_job_destroy_in {
    struct tarpc_in_arg common;

    tarpc_uint job_id;
};

struct tarpc_job_destroy_out {
    struct tarpc_out_arg common;

    tarpc_int retval;
};

program job
{
    version ver0
    {
        RPC_DEF(job_create)
        RPC_DEF(job_start)
        RPC_DEF(job_kill)
        RPC_DEF(job_wait)
        RPC_DEF(job_destroy)
    } = 1;
} = 2;
