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
    int ret;
    char oid[CFG_OID_MAX];

    va_start(ap, oid_fmt);
    ret = vsnprintf(oid, sizeof(oid), oid_fmt, ap);
    va_end(ap);

    if (ret >= sizeof(oid))
        return TE_RC(TE_CONF_API, TE_EMSGSIZE);

    return cfg_ipc_mk_find_str(msg, msg_buf_size, oid);
}
