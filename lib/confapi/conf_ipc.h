/** @file
 * @brief Configurator
 *
 * Configurator IPC message preparation functions
 *
 * Copyright (C) 2020 OKTET Labs. All rights reserved.
 *
 * @author Viacheslav Galaktionov <Viacheslav.Galaktionov@oktetlabs.ru>
 */

#ifndef __TE_CONF_IPC_H__
#define __TE_CONF_IPC_H__

#include "conf_api.h"
#include "conf_messages.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Prepare a cfg_get_instance message.
 *
 * @param msg           message buffer
 * @param msg_buf_size  length of the message buffer
 * @param handle        object instance handle
 * @param sync          synchronization flag
 *
 * @return Status code (see te_errno.h)
 */
extern te_errno cfg_ipc_mk_get(cfg_get_msg *msg, size_t msg_buf_size,
                               cfg_handle handle, te_bool sync);

/**
 * Prepare a cfg_find message.
 *
 * @param msg           message buffer
 * @param msg_buf_size  length of the message buffer
 * @param oid           object identifier in string representation
 *
 * @return Status code (see te_errno.h)
 * @return TE_EMSGSIZE  the OID is too long
 */
extern te_errno cfg_ipc_mk_find_str(cfg_find_msg *msg, size_t msg_buf_size,
                                    const char *oid);

/**
 * Prepare a cfg_find message.
 *
 * @param msg           message buffer
 * @param msg_buf_size  length of the message buffer
 * @param oid_fmt       format string for the object identifier
 *
 * @return Status code (see te_errno.h)
 * @retval TE_EMSGSIZE    the OID is too long
 */
extern te_errno cfg_ipc_mk_find_fmt(cfg_find_msg *msg, size_t msg_buf_size,
                                    const char *oid_fmt, ...)
                                    __attribute__((format(printf, 3, 4)));

/**
 * Prepare a cfg_set_instance message.
 *
 * @param msg           message buffer
 * @param msg_buf_size  length of the message buffer
 * @param handle        object instance handle
 * @param local         whether the set is local
 * @param type          value type (necessary for fast processing)
 * @param value         new value to be assigned to the instance
 *
 * @return Status code (see te_errno.h)
 */
extern te_errno cfg_ipc_mk_set(cfg_set_msg *msg, size_t msg_buf_size,
                               cfg_handle handle, te_bool local,
                               cfg_val_type type, cfg_inst_val value);

/** Same function as cfg_ipc_mk_set, but type is fixed as CVT_INTEGER */
static inline te_errno
cfg_ipc_mk_set_int(cfg_set_msg *msg, size_t msg_buf_size,
               cfg_handle handle, te_bool local, int value)
{
    cfg_inst_val val;

    val.val_int = value;
    return cfg_ipc_mk_set(msg, msg_buf_size, handle, local, CVT_INTEGER, val);
}

/** Same function as cfg_ipc_mk_set, but type is fixed as CVT_STRING */
static inline te_errno
cfg_ipc_mk_set_str(cfg_set_msg *msg, size_t msg_buf_size,
               cfg_handle handle, te_bool local, char *value)
{
    cfg_inst_val val;

    val.val_str = value;
    return cfg_ipc_mk_set(msg, msg_buf_size, handle, local, CVT_STRING, val);
}

#ifdef __cplusplus
}
#endif
#endif /* __TE_CONF_IPC_H__ */
