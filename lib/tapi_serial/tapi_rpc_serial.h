/** @file
 * @brief Test API - RPC
 *
 * Definition of TAPI for remote calls of serial console.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 */
#ifndef __TE_TAPI_RPC_SERIAL_H__
#define __TE_TAPI_RPC_SERIAL_H__

#include "te_config.h"

#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include "rcf_rpc.h"
#include "te_defs.h"
#include "tapi_serial.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Open serial @p console on provided RPC server handle and fill the handle
 *
 * @param rpcs      Target RPC server
 * @param p_handle  Pointer to handle to fill with the session information
 * @param user      User name
 * @param console   Console name
 * @param address   Console address, can be @c NULL to access it locally.
 *
 * @return @c 0 on success, @c -1 on failure.
 *
 * @remark Handle should be released using rpc_serial_close()
 */
extern int rpc_serial_open(rcf_rpc_server          *rpcs,
                           tapi_serial_handle      *p_handle,
                           const char              *user,
                           const char              *console,
                           const struct sockaddr   *address);

/**
 * Release the @p handle obtained using rpc_serial_open()
 *
 * @param handle Serial console session handle
 *
 * @return @c 0 on success, @c -1 on failure.
 */
extern int rpc_serial_close(tapi_serial_handle handle);

/**
 * Read data from the console designated by @p handle and fill the data to
 * @p buffer.
 *
 * @param handle        Serial console session handle obtained by
 *                      rpc_serial_open()
 * @param buffer        Target buffer
 * @param buffer_len    Pointer to a variable setting the expected length,
 *                      updated with actual length filled to @p buffer
 * @param timeout_ms    Read timeout (in milliseconds)
 *
 * @return Number of bytes read in case of success, @c -1 on failure.
 */
extern int rpc_serial_read(tapi_serial_handle handle,
                           char              *buffer,
                           size_t            *buffer_len,
                           int                timeout_ms);

/**
 * Force read/write operation on the given session @p handle
 *
 * @param handle Serial console session handle
 *
 * @return @c 0 on success, @c -1 on failure.
 */
extern int rpc_serial_force_rw(tapi_serial_handle handle);

/**
 * Force spy mode operation on the given session @p handle
 *
 * @param handle Serial console session handle
 *
 * @return @c 0 on success, @c -1 on failure.
 */
extern int rpc_serial_spy(tapi_serial_handle handle);

/**
 * Send string to the serial session designated by @p handle.
 *
 * @param handle Serial console session handle
 * @param fmt    printf-like format for the command
 * @param vlist  Variadic prinf-like argument list
 *
 * @return Number of bytes sent in case of success, @c -1 on failure.
 */
extern int rpc_serial_send_str(tapi_serial_handle handle, const char *fmt,
                               va_list vlist);

/**
 * Send 'enter' press to the session designated by @p handle
 *
 * @param handle Serial console session handle
 *
 * @return @c 0 on success, @c -1 on failure.
 */
extern int rpc_serial_send_enter(tapi_serial_handle handle);

/**
 * Send 'ctrl+c' press to the session designated by @p handle
 *
 * @param handle Serial console session handles
 *
 * @return @c 0 on success, @c -1 on failure.
 */
extern int rpc_serial_send_ctrl_c(tapi_serial_handle handle);

/**
 * Flush the data to the session
 *
 * @param handle Serial console session handle
 * @param amount Amount of data to drop or @c 0 to drop all
 *
 * @return @c 0 on success, @c -1 on failure.
 */
extern int rpc_serial_flush(tapi_serial_handle handle, size_t amount);

/**
 * Check that data matching specified pattern (regular expression)
 * are located in console session input buffer on the Test Agent.
 * The data may be read after that by rpc_serial_read() because it is buffered.
 *
 * @param [in] handle       Serial console session handle
 * @param [out] offset      If not @c NULL, is filled by offset of the
 *                          first match
 * @param [in] fmt          printf-like format for the pattern
 * @param [in] vlist        Variadic printf-like argument list with
 *
 * @return @c 0 on success, @c -1 on failure.
 */
extern int rpc_serial_check_pattern(tapi_serial_handle handle,
                                    int              *offset,
                                    const char       *fmt,
                                    va_list           vlist);

/**
 * Wait until data matching specified pattern (regular expression)
 * appear in console session input buffer on the Test Agent.
 * The data may be read after that by rpc_serial_read()
 *
 * @param [in] handle       Serial console sesssion handle
 * @param [out] offset      If not @c NULL, is filled by offset of the
 *                          first match
 * @param [in] timeout_ms   Timeout of the operation in milliseconds or @c -1
 *                          to block until data are received or error occurs
 * @param [in] fmt          printf-like format for the pattern
 * @param [in] vlist        Variadic printf-like argument list
 *
 * @return @c 0 on success, @c -1 on failure.
 */
extern int rpc_serial_wait_pattern(tapi_serial_handle handle,
                                   int               *offset,
                                   int                timeout_ms,
                                   const char        *fmt,
                                   va_list            vlist);
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_TAPI_RPC_SERIAL_H__ */
