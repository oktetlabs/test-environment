/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Configurator
 *
 * Configurator IPC message preparation functions
 *
 * Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "conf_ipc.h"
#include "te_str.h"

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

/* See description in conf_ipc.h */
te_errno
cfg_ipc_mk_find_str(cfg_find_msg *msg, size_t msg_buf_size, const char *oid)
{
    size_t len;

    len = strlen(oid) + 1;

    if (len > CFG_OID_MAX)
        return TE_RC(TE_CONF_API, TE_EMSGSIZE);

    if (msg_buf_size < sizeof(cfg_find_msg) + len)
        return TE_RC(TE_CONF_API, TE_ESMALLBUF);

    memset(msg, 0, sizeof(cfg_find_msg) + len);
    msg->type = CFG_FIND;
    memcpy(msg->oid, oid, len);
    msg->len = sizeof(cfg_find_msg) + len;

    return 0;
}

/* See description in conf_ipc.h */
te_errno
cfg_ipc_mk_find_fmt(cfg_find_msg *msg, size_t msg_buf_size,
                    const char *oid_fmt, ...)
{
    va_list ap;
    te_errno rc;
    char oid[CFG_OID_MAX];

    va_start(ap, oid_fmt);
    rc = te_vsnprintf(oid, sizeof(oid), oid_fmt, ap);
    va_end(ap);

    if (rc != 0)
        return TE_RC(TE_CONF_API, TE_RC_GET_ERROR(rc));

    return cfg_ipc_mk_find_str(msg, msg_buf_size, oid);
}

/* See description in conf_ipc.h */
te_errno
cfg_ipc_mk_set(cfg_set_msg *msg, size_t msg_buf_size,
               cfg_handle handle, te_bool local,
               cfg_val_type type, cfg_inst_val value)
{
    size_t value_size = 0;

    /* Check if type requires additional space in the message body */
    if (type == CVT_STRING || type == CVT_ADDRESS)
        value_size = cfg_types[type].value_size(value);

    if (msg_buf_size < sizeof(cfg_set_msg) + value_size)
        return TE_RC(TE_CONF_API, TE_ESMALLBUF);

    memset(msg, 0, sizeof(cfg_set_msg) + value_size);
    msg->type = CFG_SET;
    msg->len = sizeof(cfg_set_msg) + value_size;
    msg->local = local;
    msg->handle = handle;
    msg->val_type = type;
    cfg_types[type].put_to_msg(value, (cfg_msg *)msg);

    return 0;
}

te_errno
cfg_ipc_mk_del(cfg_del_msg *msg, size_t msg_buf_size,
               cfg_handle handle, te_bool local)
{
    if (msg_buf_size < sizeof(cfg_get_msg))
        return TE_RC(TE_CONF_API, TE_ESMALLBUF);

    memset(msg, 0, sizeof(cfg_del_msg));
    msg->type = CFG_DEL;
    msg->len = sizeof(cfg_del_msg);
    msg->local = local;
    msg->handle = handle;

    return 0;
}
