/** @file
 * @brief RPC for Agent job control
 *
 * Definition of RPC structures and functions for Agent job control
 *
 * Copyright (C) 2019 OKTET Labs.
 *
 * @author Igor Romanov <Igor.Romanov@oktetlabs.ru>
 */

struct tarpc_char_array {
    char str<>;
};

/* job_create() */
struct tarpc_job_create_in {
    struct tarpc_in_arg common;

    string spawner<>;
    string tool<>;
    tarpc_char_array argv<>;
    tarpc_char_array env<>;
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

/* job_allocate_channels */
struct tarpc_job_allocate_channels_in {
    struct tarpc_in_arg common;

    tarpc_uint job_id;
    tarpc_bool input_channels;
    tarpc_uint n_channels;
    tarpc_uint channels<>;
};

struct tarpc_job_allocate_channels_out {
    struct tarpc_out_arg common;

    tarpc_uint channels<>;
    tarpc_int retval;
};

/* job_attach_filter */
struct tarpc_job_attach_filter_in {
    struct tarpc_in_arg common;

    tarpc_uint channels<>;
    tarpc_bool readable;
    tarpc_uint log_level;
    string filter_name<>;
};

struct tarpc_job_attach_filter_out {
    struct tarpc_out_arg common;

    tarpc_uint filter;
    tarpc_int retval;
};

/* job_filter_add_regexp */
struct tarpc_job_filter_add_regexp_in {
    struct tarpc_in_arg common;

    tarpc_uint filter;
    string re<>;
    tarpc_uint extract;
};

struct tarpc_job_filter_add_regexp_out {
    struct tarpc_out_arg common;

    tarpc_int retval;
};

/* job_filter_add_channels */
struct tarpc_job_filter_add_channels_in {
    struct tarpc_in_arg common;

    tarpc_uint filter;
    tarpc_uint channels<>;
};

struct tarpc_job_filter_add_channels_out {
    struct tarpc_out_arg common;

    tarpc_int retval;
};

/* job_receive */
struct tarpc_job_receive_in {
    struct tarpc_in_arg common;

    tarpc_uint filters<>;
    tarpc_int timeout_ms;
};

struct tarpc_job_buffer {
    tarpc_uint channel;
    tarpc_uint filter;
    tarpc_bool eos;
    char data<>;
    tarpc_size_t dropped;
};

struct tarpc_job_receive_out {
    struct tarpc_out_arg common;

    tarpc_job_buffer buffer;
    tarpc_int retval;
};

/* job_receive_last */
struct tarpc_job_receive_last_in {
    struct tarpc_in_arg common;

    tarpc_uint filters<>;
    tarpc_int timeout_ms;
};

struct tarpc_job_receive_last_out {
    struct tarpc_out_arg common;

    tarpc_job_buffer buffer;
    tarpc_int retval;
};

struct tarpc_job_receive_many_in {
    struct tarpc_in_arg common;

    tarpc_uint filters<>;
    tarpc_int timeout_ms;
    tarpc_uint count;
};

struct tarpc_job_receive_many_out {
    struct tarpc_out_arg common;

    tarpc_job_buffer buffers<>;
    tarpc_int retval;
};

/* job_clear */
struct tarpc_job_clear_in {
    struct tarpc_in_arg common;

    tarpc_uint filters<>;
};

struct tarpc_job_clear_out {
    struct tarpc_out_arg common;

    tarpc_int retval;
};

/* job_send */
struct tarpc_job_send_in {
    struct tarpc_in_arg common;

    tarpc_uint channel;
    uint8_t buf<>;
};

struct tarpc_job_send_out {
    struct tarpc_out_arg common;

    tarpc_int retval;
};

/* job_poll */
struct tarpc_job_poll_in {
    struct tarpc_in_arg common;

    tarpc_uint channels<>;
    tarpc_int timeout_ms;
};

struct tarpc_job_poll_out {
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

/* job_killpg */
struct tarpc_job_killpg_in {
    struct tarpc_in_arg common;

    tarpc_uint job_id;
    tarpc_signum signo;
};

struct tarpc_job_killpg_out {
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

/* job_stop */
struct tarpc_job_stop_in {
    struct tarpc_in_arg common;

    tarpc_uint job_id;
    tarpc_signum signo;
    tarpc_int term_timeout_ms;
};

struct tarpc_job_stop_out {
    struct tarpc_out_arg common;

    tarpc_int retval;
};

/* job_destroy */
struct tarpc_job_destroy_in {
    struct tarpc_in_arg common;

    tarpc_uint job_id;
    tarpc_int term_timeout_ms;
};

struct tarpc_job_destroy_out {
    struct tarpc_out_arg common;

    tarpc_int retval;
};

enum tarpc_job_wrapper_priority {
    TARPC_JOB_WRAPPER_PRIORITY_LOW,
    TARPC_JOB_WRAPPER_PRIORITY_DEFAULT,
    TARPC_JOB_WRAPPER_PRIORITY_HIGH
};

/* job_wrapper_add() */
struct tarpc_job_wrapper_add_in {
    struct tarpc_in_arg common;

    tarpc_uint job_id;
    string tool<>;
    tarpc_char_array argv<>;
    tarpc_job_wrapper_priority priority;
};

struct tarpc_job_wrapper_add_out {
    struct tarpc_out_arg common;

    tarpc_int retval;
    tarpc_uint wrapper_id;
};

/* job_wrapper_delete() */
struct tarpc_job_wrapper_delete_in {
    struct tarpc_in_arg common;

    tarpc_uint job_id;
    tarpc_uint wrapper_id;
};

struct tarpc_job_wrapper_delete_out {
    struct tarpc_out_arg common;

    tarpc_int retval;
};

struct tarpc_job_process_setting {
    uint16_t cpu_ids<>;
    int8_t process_priority;
};

struct tarpc_job_sched_affinity {
    int cpu_ids<>;
};

struct tarpc_job_sched_priority {
    int priority;
};

enum tarpc_job_sched_param_type {
    TARPC_JOB_SCHED_AFFINITY = 0,
    TARPC_JOB_SCHED_PRIORITY = 1
};

union tarpc_job_sched_param_data
    switch (tarpc_job_sched_param_type type)
{
    case TARPC_JOB_SCHED_AFFINITY: struct tarpc_job_sched_affinity affinity;
    case TARPC_JOB_SCHED_PRIORITY: struct tarpc_job_sched_priority prio;

    default: void;
};

struct tarpc_job_sched_param {
    tarpc_job_sched_param_data data;
};

struct tarpc_job_add_sched_param_in {
    struct tarpc_in_arg common;

    tarpc_uint job_id;
    struct tarpc_job_sched_param param<>;
};

struct tarpc_job_add_sched_param_out {
    struct tarpc_out_arg common;

    tarpc_int retval;
};

program job
{
    version ver0
    {
        RPC_DEF(job_create)
        RPC_DEF(job_start)
        RPC_DEF(job_allocate_channels)
        RPC_DEF(job_attach_filter)
        RPC_DEF(job_filter_add_regexp)
        RPC_DEF(job_filter_add_channels)
        RPC_DEF(job_receive)
        RPC_DEF(job_receive_last)
        RPC_DEF(job_receive_many)
        RPC_DEF(job_clear)
        RPC_DEF(job_send)
        RPC_DEF(job_poll)
        RPC_DEF(job_kill)
        RPC_DEF(job_killpg)
        RPC_DEF(job_wait)
        RPC_DEF(job_stop)
        RPC_DEF(job_destroy)
        RPC_DEF(job_wrapper_add)
        RPC_DEF(job_wrapper_delete)
        RPC_DEF(job_add_sched_param)
    } = 1;
} = 2;
