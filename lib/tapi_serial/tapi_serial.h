/** @file
 * @brief DUT serial console TAPI
 *
 * @defgroup tapi_conf_serial DUT serial console access
 * @ingroup tapi_conf
 * @{
 *
 * Definition of API for communicating with DUT via serial console.
 *
 * Copyright (C) 2018 OKTET Labs. All rights reserved.
 *
 *
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 */

#ifndef __TE_TAPI_SERIAL_H__
#define __TE_TAPI_SERIAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "rcf_rpc.h"

/** Default console user */
#define TAPI_SERIAL_DEFAULT_USER    "te_user"

/**
 * Session data. It's assumed that the structure may
 * contain TA name, RPC server handle, file descriptor
 * of the session.
 */
typedef struct tapi_serial {
    rcf_rpc_server *rpcs;       /**< RPC server handle */
    int             sock;       /**< Session file descriptor */
} tapi_serial;

/** Session handle */
typedef tapi_serial *tapi_serial_handle;

/**
 * Open new serial console session using existing RPC server
 * on the Test Agent that serves DUT console.
 *
 * @param [in]  rpcs         RPC server
 * @param [in]  console_name Console name in Configurator tree
 * @param [out] p_handle     New session handle location
 *
 * @return Status code.
 */
extern te_errno tapi_serial_open_rpcs(rcf_rpc_server       *rpcs,
                                      const char           *console_name,
                                      tapi_serial_handle   *p_handle);

/**
 * Close serial console session.
 *
 * @param handle       Session handle
 *
 * @return Status code.
 */
extern te_errno tapi_serial_close(tapi_serial_handle handle);

/**
 * Force rw access to the console.
 *
 * @param handle        Session handle
 *
 * @return Status code.
 */
extern te_errno tapi_serial_force_rw(tapi_serial_handle handle);

/**
 * Disable rw access to the console.
 *
 * @param handle        Session handle
 *
 * @return Status code.
 */
extern te_errno tapi_serial_spy(tapi_serial_handle handle);

/**
 * Write string to the console (without LF at the end).
 *
 * @param handle        Session handle
 * @param fmt           printf-like format for the string
 *
 * @return Status code.
 */
extern te_errno tapi_serial_send_str(tapi_serial_handle handle,
                                     const char        *fmt, ...)
                                     __attribute__((format(printf, 2, 3)));
/**
 * Send command and "Enter" (LF) to the console.
 *
 * @param handle        Session handle
 * @param fmt           printf-like format for the command
 *
 * @return Status code.
 */
extern te_errno tapi_serial_send_cmd(tapi_serial_handle handle,
                                     const char        *fmt, ...)
                                     __attribute__((format(printf, 2, 3)));

/**
 * Send "Enter" (LF) to the console.
 *
 * @param handle        Session handle
 *
 * @return Status code.
 */
extern te_errno tapi_serial_send_enter(tapi_serial_handle handle);

/**
 * Send "Ctrl+C" (break) to the console.
 *
 * @param handle        Session handle
 *
 * @return Status code.
 */
extern te_errno tapi_serial_send_ctrl_c(tapi_serial_handle handle);

/**
 * Flush console session input buffer on the Test Agent (data is dropped).
 *
 * @param handle        Session handle
 * @param amount        Amount of data to drop or @c 0 to drop all
 *
 * @return Status code.
 */
extern te_errno tapi_serial_flush(tapi_serial_handle handle, size_t amount);

/**
 * Read data from the console. Note, that @c '\0' is not put to the buffer
 * after data.
 *
 * @param [in]      handle      Session handle
 * @param [in]      buffer      Input buffer
 * @param [inout]   buffer_len  On input: buffer length; on output: length of
 *                              data written
 * @param [in]      timeout_ms  Timeout of the operation in milliseconds, @c -1
 *                              to block until data are received or error occurs
 *
 * @return Status code.
 */
extern te_errno tapi_serial_read(tapi_serial_handle handle,
                                 char              *buffer,
                                 size_t            *buffer_len,
                                 int                timeout_ms);

/**
 * Check that data matching specified pattern (regular expression)
 * are located in console session input buffer on the Test Agent.
 * The data may be read after that by tapi_serial_read()
 *
 * @param [in]  handle      Session handle
 * @param [out] offset      If not @c NULL, is filled by offset of the
 *                          first match
 * @param [in]  fmt         printf-like format for the pattern
 *
 * @return Status code.
 */
extern te_errno tapi_serial_check_pattern(tapi_serial_handle handle,
                                          int               *offset,
                                          const char        *fmt, ...)
                                          __attribute__((format(printf, 3, 4)));

/**
 * Wait until data matching specified pattern (regular expression)
 * appear in console session input buffer on the Test Agent.
 * The data may be read after that by tapi_serial_read()
 *
 * @param [in]  handle      Session handle
 * @param [out] offset      If not @c NULL, is filled by offset of the
 *                          first match
 * @param [in]  timeout_ms  Timeout of the operation in milliseconds
 * @param [in]  fmt         printf-like format for the pattern
 *
 * @return Status code.
 */
extern te_errno tapi_serial_wait_pattern(tapi_serial_handle handle,
                                         int               *offset,
                                         int                timeout_ms,
                                         const char        *fmt, ...)
                                         __attribute__((format(printf, 4, 5)));


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_TAPI_SERIAL_H__ */

/**@} <!-- END tapi_conf_serial --> */
