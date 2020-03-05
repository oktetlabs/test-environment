/** @file
 * @brief Configurator
 *
 * Configurator IPC message preparation functions
 *
 * Copyright (C) 2020 OKTET Labs. All rights reserved.
 *
 * @author Viacheslav Galaktionov <Viacheslav.Galaktionov@oktetlabs.ru>
 */

#include "conf_ipc.h"

/* See description in conf_ipc.h */
te_errno
cfg_ipc_mk_get(cfg_get_msg *msg, size_t msg_buf_size,
               cfg_handle handle, te_bool sync)
{
    if (msg_buf_size < sizeof(cfg_get_msg))
        return TE_RC(TE_CONF_API, TE_ESMALLBUF);

    memset(msg, 0, sizeof(cfg_get_msg));
    msg->type = CFG_GET;
    msg->len = sizeof(cfg_get_msg);
    msg->sync = sync;
    msg->handle = handle;

    return 0;
}
