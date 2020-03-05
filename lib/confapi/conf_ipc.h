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

#ifdef __cplusplus
}
#endif
#endif /* __TE_CONF_IPC_H__ */
