/** @file
 * @brief RPC for Serial
 *
 * Definition of RPC structures and functions for UPnP Control Point
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

/*
 * The is in fact appended to tarpc.x.m4, so it may reuse any types
 * defined there.
 */
 /** serial_open() arguments */
struct tarpc_serial_open_in {
    struct tarpc_in_arg  common;
    struct tarpc_sa      sa;            /**< Console address */
    string               user<>;        /**< Console user */
    string               console<>;     /**< Console name */
};

typedef struct tarpc_serial_open_in tarpc_serial_open_in;

/** serial_open() output data */
struct tarpc_serial_open_out {
    struct tarpc_out_arg common;
    tarpc_int            sock;          /**< Console socket */
    tarpc_int            retval;        /**< Return value */
};

typedef struct tarpc_serial_open_out tarpc_serial_open_out;

/** serial_read() arguments */
struct tarpc_serial_read_in {
    struct tarpc_in_arg  common;
    tarpc_int            timeout;       /**< Read timeout */
    tarpc_int            sock;          /**< Console socket */
    tarpc_size_t         buflen;        /**< Length of buffer to read */
};

typedef struct tarpc_serial_read_in tarpc_serial_read_in;

/** serial_read() output data */
struct tarpc_serial_read_out {
    struct tarpc_out_arg common;
    string               buffer<>;      /**< Output buffer */
    tarpc_size_t         buflen;        /**< Output buffer length */
    tarpc_int            retval;        /**< Return value */
};

typedef struct tarpc_serial_read_out tarpc_serial_read_out;

/** Common serial RPC argument data */
struct tarpc_serial_common_in {
    struct tarpc_in_arg common;
    tarpc_int           sock;           /**< Console socket */
};

typedef struct tarpc_serial_common_in tarpc_serial_common_in;

/** Common serial RPC output data */
struct tarpc_serial_common_out {
    struct tarpc_out_arg common;
    tarpc_size_t         len;           /**< Length of buffer */
    tarpc_int            retval;        /**< Return value */
};

typedef struct tarpc_serial_common_out tarpc_serial_common_out;

/** serial_close() arguments */
typedef tarpc_serial_common_in tarpc_serial_close_in;
/** serial_close() output data */
typedef tarpc_serial_common_out tarpc_serial_close_out;
/** serial_spy() arguments */
typedef tarpc_serial_common_in tarpc_serial_spy_in;
/** serial_spy() output data */
typedef tarpc_serial_common_out tarpc_serial_spy_out;
/** serial_force_rw() arguments */
typedef tarpc_serial_common_in tarpc_serial_force_rw_in;
/** serial_force_rw() output data */
typedef tarpc_serial_common_out tarpc_serial_force_rw_out;
/** serial_send_enter() arguments */
typedef tarpc_serial_common_in tarpc_serial_send_enter_in;
/** serial_send_enter() output data */
typedef tarpc_serial_common_out tarpc_serial_send_enter_out;
/** serial_send_ctrl_c() arguments */
typedef tarpc_serial_common_in tarpc_serial_send_ctrl_c_in;
/** serial_send_ctrl_c() output data */
typedef tarpc_serial_common_out tarpc_serial_send_ctrl_c_out;

/** serial_flush() arguments */
struct tarpc_serial_flush_in {
    struct tarpc_in_arg common;
    tarpc_int           sock;       /**< Console socket */
    tarpc_size_t        amount;     /**< Amount of data to flush */
};

typedef struct tarpc_serial_flush_in tarpc_serial_flush_in;

/** serial_flush() output data */
typedef tarpc_serial_common_out tarpc_serial_flush_out;

/** serial_send_str() arguments */
struct tarpc_serial_send_str_in {
    struct tarpc_in_arg common;
    tarpc_int           sock;       /**< Console socket */
    string              str<>;      /**< Target string */
    tarpc_size_t        buflen;     /**< String length */
};

typedef struct tarpc_serial_send_str_in tarpc_serial_send_str_in;

/** serial_send_str() output data */
struct tarpc_serial_send_str_out {
    struct tarpc_out_arg common;
    tarpc_size_t         buflen;    /**< Written buffer length */
    tarpc_int            retval;    /**< Return value */
};

typedef struct tarpc_serial_send_str_out tarpc_serial_send_str_out;

/** serial_check_pattern() arguments */
struct tarpc_serial_check_pattern_in {
    struct tarpc_in_arg  common;
    tarpc_int            sock;              /**< Console socket */
    string               pattern<>;         /**< Pattern buffer */
    tarpc_size_t         pattern_length;    /**< Pattern length */
};

typedef struct tarpc_serial_check_pattern_in tarpc_serial_check_pattern_in;

/** serial_check_pattern() output data */
struct tarpc_serial_check_pattern_out {
    struct tarpc_out_arg common;
    tarpc_int            offset;            /**< Offset of the pattern */
    tarpc_int            retval;            /**< Return value */
};

typedef struct tarpc_serial_check_pattern_out tarpc_serial_check_pattern_out;

/** serial_wait_pattern() arguments */
struct tarpc_serial_wait_pattern_in {
    struct tarpc_in_arg  common;
    tarpc_int            sock;              /**< Console socket */
    tarpc_int            timeout;           /**< Wait timeout in ms */
    string               pattern<>;         /**< Pattern buffer */
    tarpc_size_t         pattern_length;    /**< Pattern length */
};

typedef struct tarpc_serial_wait_pattern_in tarpc_serial_wait_pattern_in;

/** serial_wait_pattern() output data */
typedef struct tarpc_serial_check_pattern_out tarpc_serial_wait_pattern_out;

program serial
{
    version ver0
    {
        RPC_DEF(serial_open)
        RPC_DEF(serial_read)
        RPC_DEF(serial_send_str)
        RPC_DEF(serial_send_enter)
        RPC_DEF(serial_send_ctrl_c)
        RPC_DEF(serial_force_rw)
        RPC_DEF(serial_flush)
        RPC_DEF(serial_close)
        RPC_DEF(serial_spy)

        RPC_DEF(serial_check_pattern)
        RPC_DEF(serial_wait_pattern)
    } = 1;
} = 2;
