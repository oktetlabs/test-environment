/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Configurator
 *
 * OID conversion routines
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "Configurator API"

#include "te_config.h"

#include <stdio.h>
#include <stdarg.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif
#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif

#include "te_alloc.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "logger_api.h"
#include "te_log_stack.h"
#include "conf_api.h"
#include "conf_ipc.h"
#include "conf_messages.h"
#include "conf_types.h"
#include "rcf_api.h"
#include "ipc_client.h"


/** Maximum space for IPC message */
#define CFG_MSG_MAX  4096

/** IPC client name maximum length */
#define CFG_NAME_MAX  25


/** IPC client initialization */
#define INIT_IPC \
    do {                                                            \
        if (cfgl_ipc_client == NULL)                                \
        {                                                           \
            char name[CFG_NAME_MAX];                                \
                                                                    \
            sprintf(name, "cfg_client_%u", (unsigned int)getpid()); \
            (void)ipc_init_client(name, CONFIGURATOR_IPC,           \
                                  &cfgl_ipc_client);                \
            if (cfgl_ipc_client != NULL)                            \
                atexit(cfg_api_cleanup);                            \
        }                                                           \
    } while (0)

/** Begining of all agents oid */
#define AGENT_BOID       "/agent"
#define BOID_LEN         strlen("/agent")

/*
 * Common part for specific *_fmt functions.
 * It assumes and provides some variables.
 */
#define _CFG_HANDLE_BY_FMT                      \
    va_list    ap;                              \
    te_errno   rc;                              \
    cfg_handle handle;                          \
                                                \
    va_start(ap, oid_fmt);                      \
    rc = cfg_find_vfmt(&handle, oid_fmt, ap);   \
    va_end(ap);                                 \
    if (rc != 0)                                \
        return rc;

/*
 * Common part for specific *_str functions.
 * It assumes and provides some variables.
 */
#define _CFG_HANDLE_BY_STR                      \
    te_errno   rc;                              \
    cfg_handle handle;                          \
                                                \
    rc = cfg_find_str(oid, &handle);            \
    if (rc != 0)                                \
        return rc;

#ifdef HAVE_PTHREAD_H
static pthread_mutex_t cfgl_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

/** IPC client to interact with Configurator */
static ipc_client *cfgl_ipc_client = NULL;

/** Message buffer */
static char cfgl_msg_buf[CFG_MSG_MAX];


static te_errno cfg_get_family_member(cfg_handle handle,
                                      uint8_t who,
                                      cfg_handle *member);
static te_errno kill_all(cfg_handle handle, te_bool local);
static te_errno kill(cfg_handle handle, te_bool local);


/* See description in conf_api.h */
te_errno
cfg_register_object_str(const char *oid, cfg_obj_descr *descr,
                        cfg_handle *handle)
{
    size_t  len, def_val_len;
    int     ret_val = 0;

    cfg_register_msg *msg;

    if ((oid == NULL) || (descr == NULL))
        return TE_RC(TE_CONF_API, TE_EINVAL);

    len = strlen(oid) + 1;
    def_val_len = descr->def_val == NULL ? 0 : strlen(descr->def_val) + 1;

    if (sizeof(*msg) + len + def_val_len > CFG_MSG_MAX ||
        len > RCF_MAX_ID)
    {
        ERROR("Too long OID or default value");
        return TE_RC(TE_CONF_API, TE_EINVAL);
    }

#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&cfgl_lock);
#endif
    INIT_IPC;
    if (cfgl_ipc_client == NULL)
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&cfgl_lock);
#endif
        return TE_RC(TE_CONF_API, TE_EIPC);
    }

    memset(cfgl_msg_buf, 0, sizeof(cfgl_msg_buf));
    msg = (cfg_register_msg *)cfgl_msg_buf;
    msg->type = CFG_REGISTER;
    msg->val_type = descr->type;
    msg->access = descr->access;

    len = strlen(oid) + 1;
    memcpy(msg->oid, oid, len);
    if (descr->def_val != NULL)
    {
        msg->def_val = len;
        memcpy(msg->oid + len, descr->def_val, def_val_len);
    }
    else
    {
        msg->def_val = 0;
    }
    msg->len = sizeof(cfg_register_msg) + len + def_val_len;

    len = CFG_MSG_MAX;
    if (((ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                                 CONFIGURATOR_SERVER,
                                                 msg, msg->len,
                                                 msg, &len)) == 0) &&
        ((ret_val = msg->rc) == 0))
    {
        if (handle != NULL)
            *handle = msg->handle;
    }
#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cfgl_lock);
#endif
    return TE_RC(TE_CONF_API, ret_val);
}

/* See description in conf_api.h */
te_errno
cfg_register_object(const cfg_oid *oid, cfg_obj_descr *descr,
                    cfg_handle *handle)
{
    te_errno    ret_val;
    char       *str;

    if ((oid == NULL) || (descr == NULL))
    {
        return TE_RC(TE_CONF_API, TE_EINVAL);
    }

    str = cfg_convert_oid(oid);
    if (str == NULL)
    {
        return TE_RC(TE_CONF_API, TE_ENOMEM);
    }

    ret_val = cfg_register_object_str(str, descr, handle);

    free(str);

    return ret_val;
}

/* See description in conf_api.h */
te_errno
cfg_register_object_fmt(cfg_obj_descr *descr, cfg_handle *handle,
                        const char *oid_fmt, ...)
{
    va_list ap;
    char    oid[CFG_OID_MAX];

    va_start(ap, oid_fmt);
    vsnprintf(oid, sizeof(oid), oid_fmt, ap);
    va_end(ap);

    return cfg_register_object_str(oid, descr, handle);
}

/* See description in conf_api.h */
te_errno
cfg_get_object_descr(cfg_handle handle, cfg_obj_descr *descr)
{
    cfg_get_descr_msg *msg;

    size_t  len;
    int     ret_val = 0;

    cfg_oid    *oid;
    cfg_handle  object;
    te_bool     inst;

    if (handle == CFG_HANDLE_INVALID)
    {
        return TE_RC(TE_CONF_API, TE_EINVAL);
    }

    ret_val = cfg_get_oid(handle, &oid);
    if (ret_val != 0)
    {
        return TE_RC(TE_CONF_API, ret_val);
    }

    inst = oid->inst;
    cfg_free_oid(oid);

    if (inst == TRUE)
    {
        ret_val = cfg_find_object_by_instance(handle, &object);
        if (ret_val != 0)
        {
            return TE_RC(TE_CONF_API, ret_val);
        }
        handle = object;
    }

#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&cfgl_lock);
#endif
    INIT_IPC;
    if (cfgl_ipc_client == NULL)
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&cfgl_lock);
#endif
        return TE_RC(TE_CONF_API, TE_EIPC);
    }

    memset(cfgl_msg_buf, 0, sizeof(cfgl_msg_buf));
    msg = (cfg_get_descr_msg *)cfgl_msg_buf;

    msg->type = CFG_GET_DESCR;
    msg->len = sizeof(cfg_get_descr_msg);
    msg->handle = handle;
    len = CFG_MSG_MAX;

    ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                           CONFIGURATOR_SERVER,
                                           msg, msg->len, msg, &len);
    if ((ret_val == 0) && ((ret_val = msg->rc) == 0))
    {
        memcpy((void *)descr, (void *)(&(msg->descr)),
               sizeof(cfg_obj_descr));
    }
#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cfgl_lock);
#endif
    return ret_val;
}

/* See description in conf_api.h */
te_errno
cfg_get_oid_str(cfg_handle handle, char **oid)
{
    cfg_get_oid_msg *msg;

    size_t  len;
    int     ret_val = 0;

    char *str;

    *oid = NULL;

    if (handle == CFG_HANDLE_INVALID)
    {
        return TE_RC(TE_CONF_API, TE_EINVAL);
    }

#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&cfgl_lock);
#endif
    INIT_IPC;
    if (cfgl_ipc_client == NULL)
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&cfgl_lock);
#endif
        return TE_RC(TE_CONF_API, TE_EIPC);
    }

    memset(cfgl_msg_buf, 0, sizeof(cfgl_msg_buf));
    msg = (cfg_get_oid_msg *)cfgl_msg_buf;

    msg->type = CFG_GET_OID;
    msg->len = sizeof(cfg_get_oid_msg);
    msg->handle = handle;
    len = CFG_MSG_MAX;

    ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                           CONFIGURATOR_SERVER,
                                           msg, msg->len, msg, &len);
    if ((ret_val == 0) && ((ret_val = msg->rc) == 0))
    {
        len = strlen(msg->oid) + 1;
        str = (char *)calloc(1, len);
        if (str == NULL)
        {
            ret_val = TE_ENOMEM;
        }
        else
        {
            memcpy((void *)str, (void *)(msg->oid), len);
            *oid = str;
        }
    }

#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cfgl_lock);
#endif

    return TE_RC(TE_CONF_API, ret_val);
}

/* See description in conf_api.h */
te_errno
cfg_get_oid(cfg_handle handle, cfg_oid **oid)
{
    char   *str;
    int     ret_val = 0;

    *oid = NULL;

    if (handle == CFG_HANDLE_INVALID)
    {
        return TE_RC(TE_CONF_API, TE_EINVAL);
    }

    if ((ret_val = cfg_get_oid_str(handle, &str)) != 0)
    {
        return TE_RC(TE_CONF_API, ret_val);
    }
    *oid = cfg_convert_oid_str(str);
    free(str);
    if (*oid == NULL)
    {
        return TE_RC(TE_CONF_API, TE_ENOMEM);
    }
    return 0;
}

/* See description in conf_api.h */
te_errno
cfg_get_subid(cfg_handle handle, char **subid)
{
    cfg_get_id_msg *msg;

    size_t  len;
    int     ret_val = 0;

    char *str;

    *subid = NULL;

    if (handle == CFG_HANDLE_INVALID)
    {
        return TE_RC(TE_CONF_API, TE_EINVAL);
    }

#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&cfgl_lock);
#endif
    INIT_IPC;
    if (cfgl_ipc_client == NULL)
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&cfgl_lock);
#endif
        return TE_RC(TE_CONF_API, TE_EIPC);
    }

    memset(cfgl_msg_buf, 0, sizeof(cfgl_msg_buf));
    msg = (cfg_get_id_msg *)cfgl_msg_buf;

    msg->type = CFG_GET_ID;
    msg->len = sizeof(cfg_get_id_msg);
    msg->handle = handle;
    len = CFG_MSG_MAX;

    ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                           CONFIGURATOR_SERVER,
                                           msg, msg->len, msg, &len);
    if ((ret_val == 0) && ((ret_val = msg->rc) == 0))
    {
        len = strlen(msg->id) + 1;
        str = (char *)calloc(1, len);
        if (str == NULL)
        {
            ret_val = TE_ENOMEM;
        }
        else
        {
            memcpy(str, msg->id, len);
            *subid = str;
        }
    }
#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cfgl_lock);
#endif
    return TE_RC(TE_CONF_API, ret_val);
}

/* See description in conf_api.h */
te_errno
cfg_get_inst_name(cfg_handle handle, char **name)
{
    cfg_get_id_msg *msg;

    size_t  len;
    int     ret_val = 0;

    char *str;

    *name = NULL;

    if (handle == CFG_HANDLE_INVALID)
    {
        return TE_RC(TE_CONF_API, TE_EINVAL);
    }

#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&cfgl_lock);
#endif
    INIT_IPC;
    if (cfgl_ipc_client == NULL)
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&cfgl_lock);
#endif
        return TE_RC(TE_CONF_API, TE_EIPC);
    }

    memset(cfgl_msg_buf, 0, sizeof(cfgl_msg_buf));
    msg = (cfg_get_id_msg *)cfgl_msg_buf;

    msg->type = CFG_GET_ID;
    msg->len = sizeof(cfg_get_id_msg);
    msg->handle = handle;
    len = CFG_MSG_MAX;

    ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                           CONFIGURATOR_SERVER,
                                           msg, msg->len, msg, &len);
    if ((ret_val == 0) && ((ret_val = msg->rc) == 0))
    {
        len = strlen(msg->id) + 1;
        str = (char *)calloc(1, len);
        if (str == NULL)
        {
            ret_val = TE_ENOMEM;
        }
        else
        {
            memcpy(str, msg->id, len);
            *name = str;
        }
    }
#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cfgl_lock);
#endif
    return TE_RC(TE_CONF_API, ret_val);
}

/* See description in conf_api.h */
te_errno
cfg_get_inst_name_type(cfg_handle handle, cfg_val_type type,
                       cfg_inst_val *val)
{
    te_errno    rc;
    char       *str;

    rc = cfg_get_inst_name(handle, &str);
    if (rc != 0)
        return rc;

    rc = cfg_types[type].str2val(str, val);
    if (rc != 0)
    {
        ERROR("%s(): Failed to convert '%s' to value of type %u: %r",
              __FUNCTION__, str, type, rc);
        free(str);
        return rc;
    }
    free(str);

    return 0;
}


/* See description in conf_api.h */
te_errno
cfg_get_ith_inst_name(const char *str_oid, unsigned int i, char **name)
{
    cfg_oid *oid;

    if ((str_oid == NULL) || (name == NULL))
    {
        return TE_RC(TE_CONF_API, TE_EINVAL);
    }

    oid = cfg_convert_oid_str(str_oid);
    if (oid == NULL || !oid->inst || oid->len <= i)
    {
        cfg_free_oid(oid);
        ERROR("Invalid value in network node: %s", str_oid);
        return TE_RC(TE_CONF_API, TE_EFMT);
    }

    *name = strdup(CFG_OID_GET_INST_NAME(oid, i));

    cfg_free_oid(oid);

    return 0;

}

/* See description in conf_api.h */
te_errno
cfg_find_str(const char *oid, cfg_handle *handle)
{
    cfg_find_msg *msg;

    size_t      len;
    te_errno    ret_val = 0;

    if (oid == NULL)
    {
        return TE_RC(TE_CONF_API, TE_EINVAL);
    }

#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&cfgl_lock);
#endif
    INIT_IPC;
    if (cfgl_ipc_client == NULL)
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&cfgl_lock);
#endif
        return TE_RC(TE_CONF_API, TE_EIPC);
    }

    memset(cfgl_msg_buf, 0, sizeof(cfgl_msg_buf));
    msg = (cfg_find_msg *)cfgl_msg_buf;
    len = strlen(oid) + 1;

    ret_val = cfg_ipc_mk_find_str(msg, CFG_MSG_MAX, oid);
    if (ret_val != 0)
        return ret_val;

    len = CFG_MSG_MAX;

    ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                           CONFIGURATOR_SERVER,
                                           msg, msg->len, msg, &len);
    if ((ret_val == 0) && ((ret_val = msg->rc) == 0) && (handle != NULL))
    {
        *handle = msg->handle;
    }
#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cfgl_lock);
#endif
    if (ret_val != 0)
        te_log_stack_push("Failed to find handle for oid=%s", oid);
    else
        te_log_stack_push("Operating on oid=%s", oid);

    return TE_RC(TE_CONF_API, ret_val);
}

/* See description in conf_api.h */
te_errno
cfg_find_fmt(cfg_handle *p_handle, const char *oid_fmt, ...)
{
    int     rc;
    va_list ap;

    va_start(ap, oid_fmt);
    rc = cfg_find_vfmt(p_handle, oid_fmt, ap);
    va_end(ap);

    return rc;
}

/* See description in conf_api.h */
te_errno
cfg_find(const cfg_oid *oid, cfg_handle *handle)
{
    te_errno    ret_val;
    char       *str;

    if (oid == NULL)
    {
        return TE_RC(TE_CONF_API, TE_EINVAL);
    }

    str = cfg_convert_oid(oid);
    if (str == NULL)
        return TE_RC(TE_CONF_API, TE_ENOMEM);

    ret_val = cfg_find_str(str, handle);

    free(str);

    return TE_RC(TE_CONF_API, ret_val);
}

/* See description in conf_api.h */
te_errno
cfg_find_object_by_instance(cfg_handle instance, cfg_handle *object)
{
    int      ret_val;
    size_t   i = 0;
    size_t   len;
    cfg_oid *instance_oid;
    cfg_oid *object_oid;

    char *instance_id;
    char *object_id;
    cfg_handle tmp;

    ret_val = cfg_get_oid(instance, &instance_oid);
    if (ret_val != 0)
    {
        fprintf(stderr, "Invalid handle is provided\n");
        return TE_RC(TE_CONF_API, ret_val);
    }

    len = instance_oid->len;

    object_oid = cfg_allocate_oid(len, FALSE);
    if (object_oid == NULL)
    {
        fprintf(stderr, "Memory allocation failure\n");
        return TE_RC(TE_CONF_API, TE_ENOMEM);
    }

    instance_id = (char *)(instance_oid->ids);
    object_id = (char *)(object_oid->ids);

    while (i < len)
    {
        strcpy(object_id, instance_id);
        object_id += sizeof(cfg_object_subid);
        instance_id += sizeof(cfg_inst_subid);
        i++;
    }

    cfg_free_oid(instance_oid);

    ret_val = cfg_find(object_oid, &tmp);
    cfg_free_oid(object_oid);
    if (ret_val == 0)
    {
        *object = tmp;
    }
    return TE_RC(TE_CONF_API, ret_val);
}

/* See description in conf_api.h */
te_errno
cfg_find_pattern(const char *pattern, unsigned int *num, cfg_handle **set)
{
    cfg_pattern_msg *msg;
    char            *alloc_msg = NULL;

    size_t  len;
    int     ret_val = 0;

    *num = 0;
    *set = NULL;

    if (pattern == NULL)
    {
        return TE_RC(TE_CONF_API, TE_EINVAL);
    }

#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&cfgl_lock);
#endif
    INIT_IPC;
    if (cfgl_ipc_client == NULL)
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&cfgl_lock);
#endif
        return TE_RC(TE_CONF_API, TE_EIPC);
    }

    memset(cfgl_msg_buf, 0, sizeof(cfgl_msg_buf));
    msg = (cfg_pattern_msg *)cfgl_msg_buf;
    len = strlen(pattern) + 1;

    msg->type = CFG_PATTERN;
    memcpy(msg->pattern, pattern, len);
    msg->len = sizeof(cfg_pattern_msg) + len;
    len = CFG_MSG_MAX;

    ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                           CONFIGURATOR_SERVER,
                                           msg, msg->len, msg, &len);
    if (TE_RC_GET_ERROR(ret_val) == TE_ESMALLBUF)
    {
        size_t  rest_len = len - CFG_MSG_MAX;

        assert(len > CFG_MSG_MAX);
        alloc_msg = malloc(len);
        if (alloc_msg == NULL)
            return TE_RC(TE_CONF_API, TE_ENOMEM);

        ret_val = ipc_receive_rest_answer(cfgl_ipc_client,
                                          CONFIGURATOR_SERVER,
                                          alloc_msg + CFG_MSG_MAX,
                                          &rest_len);
        if (ret_val == 0)
        {
            memcpy(alloc_msg, msg, CFG_MSG_MAX);
            msg = (cfg_pattern_msg *)alloc_msg;
        }
    }
    if ((ret_val == 0) && ((ret_val = msg->rc) == 0))
    {
        *num = (msg->len - sizeof(cfg_pattern_msg)) / sizeof(cfg_handle);
        if (*num > 0)
        {
            *set = (cfg_handle *)calloc(*num, sizeof(cfg_handle));
            if (set == NULL)
            {
                *num = 0;
                ret_val = TE_ENOMEM;
            }
            else
            {
                memcpy((void *)(*set), (void *)(msg->handles),
                       *num * sizeof(cfg_handle));
            }
        }
        else
        {
            *set = NULL;
        }
    }
#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cfgl_lock);
#endif
    free(alloc_msg);
    return TE_RC(TE_CONF_API, ret_val);
}

/* See description in conf_api.h */
te_errno
cfg_find_pattern_fmt(unsigned int *p_num, cfg_handle **p_set,
                     const char *ptrn_fmt, ...)
{
    va_list ap;
    char    ptrn[CFG_OID_MAX];

    va_start(ap, ptrn_fmt);
    vsnprintf(ptrn, sizeof(ptrn), ptrn_fmt, ap);
    va_end(ap);

    return cfg_find_pattern(ptrn, p_num, p_set);
}

/* See description in conf_api.h */
te_errno
cfg_find_pattern_iter_fmt(cfg_handle_cb_func cb_func, void *opaque,
                          const char *ptrn_fmt, ...)
{
    unsigned int count;
    unsigned int i;
    cfg_handle  *handles;
    te_errno     rc;
    va_list      ap;
    char         ptrn[CFG_OID_MAX];
    int          res;

    va_start(ap, ptrn_fmt);
    res = vsnprintf(ptrn, sizeof(ptrn), ptrn_fmt, ap);
    va_end(ap);
    if (res < 0 || res >= (int)sizeof(ptrn))
    {
        if (res < 0)
            rc = TE_OS_RC(TE_CONF_API, errno);
        else
            rc = TE_RC(TE_CONF_API, TE_ESMALLBUF);
        ERROR("Failed to compose pattern string: %r", rc);
        return rc;
    }

    rc = cfg_find_pattern(ptrn, &count, &handles);
    if (rc != 0)
    {
        ERROR("Cannot get objects list: %r", rc);
        return rc;
    }

    for (i = 0; i < count; i++)
    {
        rc = cb_func(handles[i], opaque);
        if (rc != 0)
            break;
    }
    free(handles);

    return rc;
}

/**
 * Get handle of the family member of the object or object instance.
 *
 * @param[in]  handle   handle of the object or object instance
 * @param[in]  who      family member, one of @ref CFG_FATHER, @ref CFG_BROTHER,
 *                      and @ref CFG_SON.
 * @param[out] member   location for family member's handle
 *
 * @return Status code
 */
static te_errno
cfg_get_family_member(cfg_handle handle, uint8_t who, cfg_handle *member)
{
    cfg_family_msg *msg;

    size_t  len;
    int     ret_val = 0;

    if (handle == CFG_HANDLE_INVALID)
    {
        return TE_EINVAL;
    }
#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&cfgl_lock);
#endif
    INIT_IPC;
    if (cfgl_ipc_client == NULL)
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&cfgl_lock);
#endif
        return TE_EIPC;
    }

    memset(cfgl_msg_buf, 0, sizeof(cfgl_msg_buf));
    msg = (cfg_family_msg *)cfgl_msg_buf;

    msg->type = CFG_FAMILY;
    msg->len = sizeof(cfg_family_msg);
    msg->handle = handle;
    msg->who = who;
    len = CFG_MSG_MAX;

    ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                           CONFIGURATOR_SERVER,
                                           msg, msg->len, msg, &len);
    if ((ret_val == 0) && ((ret_val = msg->rc) == 0))
    {
        *member = msg->handle;
    }
#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cfgl_lock);
#endif
    return TE_RC(TE_CONF_API, ret_val);
}


/* See description in conf_api.h */
te_errno
cfg_get_son(cfg_handle handle, cfg_handle *son)
{
    te_errno ret_val;
    cfg_handle out;

    ret_val = cfg_get_family_member(handle, CFG_SON, &out);
    if (ret_val == 0)
    {
        *son = out;
    }
    return TE_RC(TE_CONF_API, ret_val);
}

/* See description in conf_api.h */
te_errno
cfg_get_brother(cfg_handle handle, cfg_handle *brother)
{
    te_errno ret_val;
    cfg_handle out;

    ret_val = cfg_get_family_member(handle, CFG_BROTHER, &out);
    if (ret_val == 0)
    {
        *brother = out;
    }
    return TE_RC(TE_CONF_API, ret_val);
}

/* See description in conf_api.h */
te_errno
cfg_get_father(cfg_handle handle, cfg_handle *father)
{
    te_errno ret_val;
    cfg_handle out;

    ret_val = cfg_get_family_member(handle, CFG_FATHER, &out);
    if (ret_val == 0)
    {
        *father = out;
    }
    return TE_RC(TE_CONF_API, ret_val);
}

/**
 * Create object instance locally or on the agent.
 *
 * @param oid       object identifier of the new instance
 * @param handle    location for handle of the new instance
 * @param local     is local add?
 * @param type      value type (necessary for fast processing)
 * @param list      value to be assigned to the instance or NULL;
 *                    for integer values: int
 *                    for strings: char *
 *                    for addreses: struct sockaddr *
 *
 * @return status code (see te_errno.h)
 */
static te_errno
cfg_add_instance_gen(const char *oid, cfg_handle *handle, te_bool local,
                     cfg_val_type type, va_list list)
{
    cfg_add_msg  *msg;
    cfg_inst_val  value;
    char         *valstr = NULL;

    size_t  len;
    int     ret_val = 0;

    if (oid == NULL)
    {
        return TE_RC(TE_CONF_API, TE_EINVAL);
    }

#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&cfgl_lock);
#endif
    INIT_IPC;
    if (cfgl_ipc_client == NULL)
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&cfgl_lock);
#endif
        return TE_RC(TE_CONF_API, TE_EIPC);
    }

    memset(cfgl_msg_buf, 0, sizeof(cfgl_msg_buf));
    msg = (cfg_add_msg *)cfgl_msg_buf;
    msg->type = CFG_ADD;
    msg->local = local;
    msg->val_type = type;

    switch (type)
    {
        case CVT_INTEGER:
            value.val_int = va_arg(list, int);
            break;

        case CVT_UINT64:
            value.val_uint64 = va_arg(list, uint64_t);
            break;

        case CVT_STRING:
            value.val_str = va_arg(list, char *);
            break;

        case CVT_ADDRESS:
            value.val_addr = va_arg(list, struct sockaddr *);
            break;

        case CVT_NONE:
            break;

        case CVT_UNSPECIFIED:
            assert(FALSE);
    }

    cfg_types[type].put_to_msg(value, (cfg_msg *)msg);

    msg->oid_offset = msg->len;
    msg->len += strlen(oid) + 1;
    strcpy((char *)msg + msg->oid_offset, oid);

    len = CFG_MSG_MAX;

    ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                           CONFIGURATOR_SERVER,
                                           msg, msg->len, msg, &len);
    if ((ret_val == 0) && ((ret_val = msg->rc) == 0))
    {
        if (handle != NULL)
            *handle = msg->handle;

        cfg_types[type].val2str(value, &valstr);
        if (strncmp(oid, AGENT_BOID, BOID_LEN) == 0)
        {
            RING("Added %s%s = %s", local ? "locally " : "", oid,
                 (valstr != NULL) ? valstr : "(none)");
        }
        free(valstr);
    }

#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cfgl_lock);
#endif

    if (ret_val != 0)
        te_log_stack_push("Failed to add instance: oid='%s' "
                          "rc=%s-%s", oid, te_rc_mod2str(ret_val),
                          te_rc_err2str(ret_val));

    return TE_RC(TE_CONF_API, ret_val);
}

/* See description in conf_api.h */
te_errno
cfg_add_instance(const cfg_oid *oid, cfg_handle *handle,
                 cfg_val_type type, ...)
{
    char    *oid2str = NULL;
    te_errno ret_val = 0;
    va_list  list;

    if (oid == NULL)
        return TE_RC(TE_CONF_API, TE_EINVAL);

    if ((oid2str = cfg_convert_oid(oid)) == NULL)
        return TE_RC(TE_CONF_API, TE_ENOMEM);

    va_start(list, type);
    ret_val = cfg_add_instance_gen(oid2str, handle, FALSE, type, list);
    va_end(list);

    free(oid2str);

    return TE_RC(TE_CONF_API, ret_val);
}

/* See description in conf_api.h */
te_errno
cfg_add_instance_str(const char *oid, cfg_handle *handle,
                     cfg_val_type type, ...)
{
    te_errno ret_val = 0;
    va_list list;

    va_start(list, type);
    ret_val = cfg_add_instance_gen(oid, handle, FALSE, type, list);
    va_end(list);

    return TE_RC(TE_CONF_API, ret_val);
}

/* See description in conf_api.h */
te_errno
cfg_add_instance_fmt(cfg_handle *p_handle, cfg_val_type type,
                     const void *val, const char *oid_fmt, ...)
{
    va_list ap;
    char    oid[CFG_OID_MAX];

    va_start(ap, oid_fmt);
    vsnprintf(oid, sizeof(oid), oid_fmt, ap);
    va_end(ap);

    return cfg_add_instance_str(oid, p_handle, type, val);
}

/* See description in conf_api.h */
te_errno
cfg_add_instance_local(const cfg_oid *oid, cfg_handle *handle,
                       cfg_val_type type, ...)
{
    char    *oid2str = NULL;
    int      ret_val = 0;
    va_list  list;

    if (oid == NULL)
        return TE_RC(TE_CONF_API, TE_EINVAL);

    if ((oid2str = cfg_convert_oid(oid)) == NULL)
        return TE_RC(TE_CONF_API, TE_ENOMEM);

    va_start(list, type);
    ret_val = cfg_add_instance_gen(oid2str, handle, TRUE, type, list);
    va_end(list);

    free(oid2str);

    return TE_RC(TE_CONF_API, ret_val);
}

/* See description in conf_api.h */
te_errno
cfg_add_instance_local_str(const char *oid, cfg_handle *handle,
                           cfg_val_type type, ...)
{
    int     ret_val = 0;
    va_list list;

    va_start(list, type);
    ret_val = cfg_add_instance_gen(oid, handle, TRUE, type, list);
    va_end(list);

    return TE_RC(TE_CONF_API, ret_val);
}

/* See description in conf_api.h */
te_errno
cfg_add_instance_local_fmt(cfg_handle *p_handle, cfg_val_type type,
                           const void *val, const char *oid_fmt, ...)
{
    va_list ap;
    char    oid[CFG_OID_MAX];

    va_start(ap, oid_fmt);
    vsnprintf(oid, sizeof(oid), oid_fmt, ap);
    va_end(ap);

    return cfg_add_instance_local_str(oid, p_handle, type, val);
}

/* See description in conf_api.h */
te_errno
cfg_add_instance_child_fmt(cfg_handle *p_handle, cfg_val_type type,
                           const void *val, cfg_handle parent,
                           const char *suboid_fmt, ...)
{
    va_list     ap;
    int         rc;
    char       *parent_oid;
    char       *oid_fmt;
    char       *oid;


    if (suboid_fmt == NULL)
        return TE_RC(TE_CONF_API, TE_EINVAL);

    rc = cfg_get_oid_str(parent, &parent_oid);
    if (rc != 0)
        return rc;
    assert(parent_oid != NULL);

    oid_fmt = malloc(CFG_OID_MAX);
    if (oid_fmt == NULL)
    {
        free(parent_oid);
        return TE_RC(TE_CONF_API, TE_ENOMEM);
    }
    oid = malloc(CFG_OID_MAX);
    if (oid == NULL)
    {
        free(oid_fmt);
        free(parent_oid);
        return TE_RC(TE_CONF_API, TE_ENOMEM);
    }

    snprintf(oid_fmt, CFG_OID_MAX, "%s%s", parent_oid, suboid_fmt);
    free(parent_oid);

    va_start(ap, suboid_fmt);
    vsnprintf(oid, CFG_OID_MAX, oid_fmt, ap);
    va_end(ap);

    free(oid_fmt);

    rc = cfg_add_instance_str(oid, p_handle, type, val);

    free(oid);

    return rc;
}

/*
 * Delete an object instance with it's subtree
 *
 * @param handle    object instance handle
 * @param local     if @c TRUE, it is local delete
 *
 * @return Status code
 */
static te_errno
kill_all(cfg_handle handle, te_bool local)
{
    te_errno    ret_val;
    cfg_handle  son;
    cfg_handle  brother;

    if (handle == CFG_HANDLE_INVALID)
    {
        return TE_RC(TE_CONF_API, TE_EINVAL);
    }

    ret_val = cfg_get_son(handle, &son);
    if (ret_val != 0)
    {
        return TE_RC(TE_CONF_API, ret_val);
    }
    if (son != CFG_HANDLE_INVALID)
    {
        ret_val = kill_all(son, local);
        if (ret_val != 0 && TE_RC_GET_ERROR(ret_val) != TE_EACCES)
        {
            return TE_RC(TE_CONF_API, ret_val);
        }
    }

    ret_val = cfg_get_brother(handle, &brother);
    if (ret_val != 0)
    {
        return TE_RC(TE_CONF_API, ret_val);
    }
    if (brother != CFG_HANDLE_INVALID)
    {
        ret_val = kill_all(brother, local);
        if (ret_val != 0 && TE_RC_GET_ERROR(ret_val) != TE_EACCES)
        {
            return TE_RC(TE_CONF_API, ret_val);
        }
    }

    ret_val = kill(handle, local);
    if (ret_val != 0 && TE_RC_GET_ERROR(ret_val) != TE_EACCES)
    {
        return TE_RC(TE_CONF_API, ret_val);
    }

    return 0;
}

/*
 * Delete an object instance
 *
 * @param handle    Object instance handle
 * @param local     if @c TRUE, it is local delete
 *
 * @return Status code
 */
static te_errno
kill(cfg_handle handle, te_bool local)
{
    cfg_del_msg *msg;

    te_errno    ret_val = 0;
    size_t      len;

    char  *oidstr = NULL;


    if ((ret_val = cfg_get_oid_str(handle, &oidstr)) != 0)
    {
        return ret_val;
    }

#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&cfgl_lock);
#endif
    INIT_IPC;
    if (cfgl_ipc_client == NULL)
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&cfgl_lock);
#endif
        free(oidstr);
        return TE_RC(TE_CONF_API, TE_EIPC);
    }

    memset(cfgl_msg_buf, 0, sizeof(cfgl_msg_buf));
    msg = (cfg_del_msg *)cfgl_msg_buf;

    msg->type = CFG_DEL;
    msg->len = sizeof(cfg_del_msg);
    msg->handle = handle;
    msg->local = local;
    len = CFG_MSG_MAX;

    ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                           CONFIGURATOR_SERVER,
                                           msg, msg->len, msg, &len);
    if (ret_val == 0)
        ret_val = msg->rc;

#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cfgl_lock);
#endif

    if (ret_val == 0 &&  oidstr != NULL &&
        strncmp(oidstr, AGENT_BOID, BOID_LEN) == 0)
    {
        RING("Deleted %s%s", (local ? "locally " : ""), oidstr);
    }
    free(oidstr);
    return TE_RC(TE_CONF_API, ret_val);
}

/**
 * Remove instance locally or on the agent.
 *
 * @param handle          Instance handle.
 * @param with_children   If @c TRUE, remove children subtree.
 * @param local           If @c TRUE, remove locally (should be
 *                        committed later).
 *
 * @return Status code.
 */
static te_errno
cfg_del_instance_gen(cfg_handle handle, te_bool with_children,
                     te_bool local)
{
    cfg_handle  son;
    te_errno    ret_val;

    if (handle == CFG_HANDLE_INVALID)
    {
        return TE_RC(TE_CONF_API, TE_EINVAL);
    }

    if (with_children == TRUE)
    {
        ret_val = cfg_get_son(handle, &son);
        if (ret_val == 0 && son != CFG_HANDLE_INVALID)
        {
            ret_val = kill_all(son, local);
        }
        if (ret_val != 0)
            return TE_RC(TE_CONF_API, ret_val);
    }
    ret_val = kill(handle, local);
    return TE_RC(TE_CONF_API, ret_val);
}

/* See description in conf_api.h */
te_errno
cfg_del_instance(cfg_handle handle, te_bool with_children)
{
    return cfg_del_instance_gen(handle, with_children, FALSE);
}

/* See description in conf_api.h */
te_errno
cfg_del_instance_fmt(te_bool with_children, const char *oid_fmt, ...)
{
    _CFG_HANDLE_BY_FMT;
    return cfg_del_instance(handle, with_children);
}

/* See description in conf_api.h */
te_errno
cfg_del_instance_local(cfg_handle handle, te_bool with_children)
{
    return cfg_del_instance_gen(handle, with_children, TRUE);
}

/* See description in conf_api.h */
te_errno
cfg_del_instance_local_fmt(te_bool with_children, const char *oid_fmt, ...)
{
    _CFG_HANDLE_BY_FMT;
    return cfg_del_instance_local(handle, with_children);
}

/**
 * Change object instance value locally or on the agent.
 *
 * @param handle    object instance handle
 * @param local     is local set?
 * @param type      value type (necessary for fast processing)
 * @param list      new value to be assigned to the instance or NULL;
 *                    for integer values: int
 *                    for strings: char *
 *                    for addreses: struct sockaddr *
 *
 * @return status code (see te_errno.h)
 */
static te_errno
cfg_set_instance_gen(cfg_handle handle, te_bool local, cfg_val_type type,
                     va_list list)
{
    cfg_set_msg    *msg;
    cfg_inst_val    value;

    char    *oidstr = NULL;
    char    *valstr = NULL;

    int     ret_val = 0;
    size_t  len;

    if (handle == CFG_HANDLE_INVALID)
    {
        return TE_RC(TE_CONF_API, TE_EINVAL);
    }

#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&cfgl_lock);
#endif
    INIT_IPC;
    if (cfgl_ipc_client == NULL)
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&cfgl_lock);
#endif
        return TE_RC(TE_CONF_API, TE_EIPC);
    }

    switch (type)
    {
        case CVT_INTEGER:
            value.val_int = va_arg(list, int);
            break;

        case CVT_UINT64:
            value.val_uint64 = va_arg(list, uint64_t);
            break;

        case CVT_STRING:
            value.val_str = va_arg(list, char *);
            break;

        case CVT_ADDRESS:
            value.val_addr = va_arg(list, struct sockaddr *);
            break;

        case CVT_NONE:
            break;

        case CVT_UNSPECIFIED:
             assert(0);
    }

    memset(cfgl_msg_buf, 0, sizeof(cfgl_msg_buf));
    msg = (cfg_set_msg *)cfgl_msg_buf;
    ret_val = cfg_ipc_mk_set(msg, CFG_MSG_MAX, handle, local, type, value);
    if (ret_val != 0)
        return ret_val;

    len = CFG_MSG_MAX;

    ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                           CONFIGURATOR_SERVER,
                                           msg, msg->len, msg, &len);

#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cfgl_lock);
#endif

    if (ret_val == 0)
    {
        ret_val = msg->rc;

        cfg_types[type].val2str(value, &valstr);
        if (ret_val == 0 && cfg_get_oid_str(handle, &oidstr) == 0 &&
            strncmp(oidstr, AGENT_BOID, BOID_LEN) == 0 && valstr != NULL)
        {
            RING("Set %s%s = %s", local ? "locally " : "", oidstr, valstr);
        }
        free(oidstr);
        free(valstr);

    }
    return TE_RC(TE_CONF_API, ret_val);
}

/* See description in conf_api.h */
te_errno
cfg_set_instance(cfg_handle handle, cfg_val_type type, ...)
{
    te_errno ret_val;
    va_list list;

    va_start(list, type);
    ret_val = cfg_set_instance_gen(handle, FALSE, type, list);
    va_end(list);

    return TE_RC(TE_CONF_API, ret_val);
}

/* See description in conf_api.h */
te_errno
cfg_set_instance_fmt(cfg_val_type type, const void *val,
                     const char *oid_fmt, ...)
{
    _CFG_HANDLE_BY_FMT;
    return cfg_set_instance(handle, type, val);
}

/* See description in conf_api.h */
te_errno
cfg_set_instance_str(cfg_val_type type, const void *val,
                     const char *oid)
{
    _CFG_HANDLE_BY_STR;
    return cfg_set_instance(handle, type, val);
}

/* See description in conf_api.h */
te_errno
cfg_set_instance_local(cfg_handle handle, cfg_val_type type, ...)
{
    te_errno ret_val;
    va_list list;

    va_start(list, type);
    ret_val = cfg_set_instance_gen(handle, TRUE, type, list);
    va_end(list);

    return TE_RC(TE_CONF_API, ret_val);
}

/* See description in conf_api.h */
te_errno
cfg_set_instance_local_fmt(cfg_val_type type, const void *val,
                           const char *oid_fmt, ...)
{
    _CFG_HANDLE_BY_FMT;
    return cfg_set_instance_local(handle, type, val);
}

/* See description in conf_api.h */
te_errno
cfg_commit(const char *oid)
{
    cfg_commit_msg *msg;
    size_t          len;
    int             ret_val = 0;

#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&cfgl_lock);
#endif
    INIT_IPC;
    if (cfgl_ipc_client == NULL)
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&cfgl_lock);
#endif
        return TE_RC(TE_CONF_API, TE_EIPC);
    }

    memset(cfgl_msg_buf, 0, sizeof(cfgl_msg_buf));
    msg = (cfg_commit_msg *)cfgl_msg_buf;
    msg->type = CFG_COMMIT;

    len = strlen(oid) + 1;
    memcpy(msg->oid, oid, len);
    msg->len = sizeof(cfg_commit_msg) + len;

    len = CFG_MSG_MAX;

    if ((ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                                CONFIGURATOR_SERVER,
                                                msg, msg->len,
                                                msg, &len)) == 0)
    {
        ret_val = msg->rc;
        if (ret_val == 0)
            RING("Committed %s", oid);
    }

#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cfgl_lock);
#endif

    return TE_RC(TE_CONF_API, ret_val);
}

/* See description in conf_api.h */
te_errno
cfg_commit_fmt(const char *oid_fmt, ...)
{
    va_list ap;
    char    oid[CFG_OID_MAX];

    va_start(ap, oid_fmt);
    vsnprintf(oid, sizeof(oid), oid_fmt, ap);
    va_end(ap);

    return cfg_commit(oid);
}

/* See description in conf_api.h */
te_errno
cfg_get_instance(cfg_handle handle, cfg_val_type *type, ...)
{
    cfg_get_msg    *msg;
    va_list         list;
    cfg_inst_val    value;
    size_t          len;
    te_errno        rc = 0;

    if (handle == CFG_HANDLE_INVALID)
    {
        return TE_RC(TE_CONF_API, TE_EINVAL);
    }

#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&cfgl_lock);
#endif
    INIT_IPC;
    if (cfgl_ipc_client == NULL)
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&cfgl_lock);
#endif
        return TE_RC(TE_CONF_API, TE_EIPC);
    }

    memset(cfgl_msg_buf, 0, sizeof(cfgl_msg_buf));
    msg = (cfg_get_msg *)cfgl_msg_buf;
    rc = cfg_ipc_mk_get(msg, CFG_MSG_MAX, handle, FALSE);
    if (rc != 0)
        return rc;

    len = CFG_MSG_MAX;

    rc = ipc_send_message_with_answer(cfgl_ipc_client,
                                      CONFIGURATOR_SERVER,
                                      msg, msg->len, msg, &len);
    if ((rc != 0) || ((rc = msg->rc) != 0) ||
        ((rc = cfg_types[msg->val_type].get_from_msg((cfg_msg *)msg,
                                                     &value)) != 0))
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&cfgl_lock);
#endif
        return TE_RC(TE_CONF_API, rc);
    }

    if (type != NULL && *type != CVT_UNSPECIFIED && *type != msg->val_type)
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&cfgl_lock);
#endif
        return TE_RC(TE_CONF_API, TE_EBADTYPE);
    }

    va_start(list, type);
    switch (msg->val_type)
    {
         case CVT_INTEGER:
         {
             int *val_int = va_arg(list, int *);

             if (val_int != NULL)
                 *val_int = value.val_int;
             break;
         }
         case CVT_UINT64:
         {
             uint64_t *val_uint64 = va_arg(list, uint64_t *);

             if (val_uint64 != NULL)
                 *val_uint64 = value.val_uint64;
             break;
         }
         case CVT_STRING:
         {
             char **val_str  = va_arg(list, char **);

             if (val_str != NULL)
                 *val_str = value.val_str;
             else
                free(value.val_str);
             break;
         }
         case CVT_ADDRESS:
         {
             struct sockaddr **val_addr = va_arg(list, struct sockaddr **);

             if (val_addr != NULL)
                 *val_addr = value.val_addr;
             else
                 free(value.val_addr);
             break;
         }
         case CVT_NONE:
         {
             break;
         }
         default:
         {
             ERROR("Get Configurator instance of unknown type %u",
                   msg->val_type);
             rc = TE_RC(TE_CONF_API, TE_EINVAL);
             break;
         }
    }
    va_end(list);

    if ((type != NULL) && (*type == CVT_UNSPECIFIED))
    {
        *type = msg->val_type;
    }

#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cfgl_lock);
#endif

    return rc;
}

/* See description in conf_api.h */
te_errno
cfg_get_instance_fmt(cfg_val_type *p_type, void *val,
                     const char *oid_fmt, ...)
{
    _CFG_HANDLE_BY_FMT;
    return cfg_get_instance(handle, p_type, val);
}

/* See description in conf_api.h */
te_errno
cfg_get_instance_int_fmt(int *val, const char *oid_fmt, ...)
{
    cfg_val_type type = CVT_INTEGER;

    _CFG_HANDLE_BY_FMT;
    return cfg_get_instance(handle, &type, val);
}

/* See description in conf_api.h */
te_errno
cfg_get_instance_uint64_fmt(uint64_t *val, const char *oid_fmt, ...)
{
    cfg_val_type type = CVT_UINT64;

    _CFG_HANDLE_BY_FMT;
    return cfg_get_instance(handle, &type, val);
}

/* See description in conf_api.h */
te_errno
cfg_get_instance_string_fmt(char **val, const char *oid_fmt, ...)
{
    cfg_val_type type = CVT_STRING;

    _CFG_HANDLE_BY_FMT;
    return cfg_get_instance(handle, &type, val);
}

/* See description in conf_api.h */
te_errno
cfg_get_instance_addr_fmt(struct sockaddr **val, const char *oid_fmt, ...)
{
    cfg_val_type type = CVT_ADDRESS;

    _CFG_HANDLE_BY_FMT;
    return cfg_get_instance(handle, &type, val);
}

/* See description in conf_api.h */
te_errno
cfg_get_instance_str(cfg_val_type *p_type, void *val,
                     const char *oid)
{
    _CFG_HANDLE_BY_STR;
    return cfg_get_instance(handle, p_type, val);
}

/* See description in conf_api.h */
te_errno
cfg_get_instance_sync(cfg_handle handle, cfg_val_type *type, ...)
{
    cfg_get_msg *msg;

    va_list      list;
    cfg_inst_val value;

    size_t  len;
    int     ret_val = 0;

    if (handle == CFG_HANDLE_INVALID)
    {
        return TE_RC(TE_CONF_API, TE_EINVAL);
    }

#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&cfgl_lock);
#endif
    INIT_IPC;
    if (cfgl_ipc_client == NULL)
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&cfgl_lock);
#endif
        return TE_RC(TE_CONF_API, TE_EIPC);
    }

    memset(cfgl_msg_buf, 0, sizeof(cfgl_msg_buf));
    msg = (cfg_get_msg *)cfgl_msg_buf;
    ret_val = cfg_ipc_mk_get(msg, CFG_MSG_MAX, handle, TRUE);
    if (ret_val != 0)
        return ret_val;

    len = CFG_MSG_MAX;

    ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                           CONFIGURATOR_SERVER,
                                           msg, msg->len, msg, &len);
    if ((ret_val != 0) || ((ret_val = msg->rc) != 0) ||
        ((ret_val = cfg_types[msg->val_type].get_from_msg((cfg_msg *)msg,
                                                          &value)) != 0))
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&cfgl_lock);
#endif
        return ret_val;
    }

    if (type != NULL && *type != CVT_UNSPECIFIED && *type != msg->val_type)
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&cfgl_lock);
#endif
        return TE_EBADTYPE;
    }

    va_start(list, type);
    switch (msg->val_type)
    {
         case CVT_INTEGER:
         {
             int *val_int = va_arg(list, int *);

             *val_int = value.val_int;
             break;
         }
         case CVT_UINT64:
         {
             uint64_t *val_uint64 = va_arg(list, uint64_t *);

             *val_uint64 = value.val_uint64;
             break;
         }
         case CVT_STRING:
         {
             char **val_str  = va_arg(list, char **);

             *val_str = value.val_str;
             break;
         }
         case CVT_ADDRESS:
         {
             struct sockaddr **val_addr = va_arg(list, struct sockaddr **);

             *val_addr = value.val_addr;
             break;
         }
         case CVT_NONE:
         {
             break;
         }
         default:
         {
             fprintf(stderr, "cfg_get_instance: get unknown type\n");
             break;
         }
    }
    va_end(list);

    if (type != NULL)
        *type = msg->val_type;

#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cfgl_lock);
#endif
    return ret_val;
}

/* See description in conf_api.h */
te_errno
cfg_get_instance_sync_fmt(cfg_val_type *type, void *val,
                          const char *oid_fmt, ...)
{
    _CFG_HANDLE_BY_FMT;
    return cfg_get_instance_sync(handle, type, val);
}

/* See description in conf_api.h */
te_errno
cfg_get_instance_int_sync_fmt(int *val, const char *oid_fmt, ...)
{
    cfg_val_type type = CVT_INTEGER;

    _CFG_HANDLE_BY_FMT;
    return cfg_get_instance_sync(handle, &type, val);
}

/* See description in conf_api.h */
te_errno
cfg_get_instance_uint64_sync_fmt(uint64_t *val, const char *oid_fmt, ...)
{
    cfg_val_type type = CVT_UINT64;

    _CFG_HANDLE_BY_FMT;
    return cfg_get_instance_sync(handle, &type, val);
}

/* See description in conf_api.h */
te_errno
cfg_get_instance_string_sync_fmt(char **val, const char *oid_fmt, ...)
{
    cfg_val_type type = CVT_STRING;

    _CFG_HANDLE_BY_FMT;
    return cfg_get_instance_sync(handle, &type, val);
}

/* See description in conf_api.h */
te_errno
cfg_get_instance_addr_sync_fmt(struct sockaddr **val, const char *oid_fmt, ...)
{
    cfg_val_type type = CVT_ADDRESS;

    _CFG_HANDLE_BY_FMT;
    return cfg_get_instance_sync(handle, &type, val);
}

/* See description in conf_api.h */
te_errno
cfg_synchronize(const char *oid, te_bool subtree)
{
    cfg_sync_msg *msg;

    size_t  len;
    int     ret_val = 0;

    if (oid == NULL)
        return TE_EINVAL;

#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&cfgl_lock);
#endif
    INIT_IPC;
    if (cfgl_ipc_client == NULL)
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&cfgl_lock);
#endif
        return TE_EIPC;
    }

    memset(cfgl_msg_buf, 0, sizeof(cfgl_msg_buf));
    msg = (cfg_sync_msg *)cfgl_msg_buf;
    msg->type = CFG_SYNC;
    msg->subtree = subtree;

    len = strlen(oid) + 1;
    memcpy(msg->oid, oid, len);
    msg->len = sizeof(cfg_sync_msg) + len;
    len = CFG_MSG_MAX;

    ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                           CONFIGURATOR_SERVER,
                                           msg, msg->len, msg, &len);
    if (ret_val == 0)
    {
        ret_val = msg->rc;
    }

#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cfgl_lock);
#endif
    return TE_RC(TE_CONF_API, ret_val);
}

/* See description in conf_api.h */
te_errno
cfg_synchronize_fmt(te_bool subtree, const char *oid_fmt, ...)
{
    va_list ap;
    char    oid[CFG_OID_MAX];

    va_start(ap, oid_fmt);
    vsnprintf(oid, sizeof(oid), oid_fmt, ap);
    va_end(ap);

    return cfg_synchronize(oid, subtree);
}

/* See description in conf_api.h */
te_errno
cfg_enumerate(cfg_handle handle, cfg_inst_handler callback,
              void *user_data)
{
    te_errno    ret_val = 0;

    cfg_oid    *oid;
    char       *oid_subid;
    cfg_oid    *pattern;
    char       *subid;
    char       *name;
    int         length;

    const char *star = "*";
    const int   starlen = strlen(star) + 1;

    char       *pattern_name;

    unsigned int    num;
    cfg_handle     *instances;

    if (handle == CFG_HANDLE_INVALID)
    {
        return TE_RC(TE_CONF_API, TE_EINVAL);
    }

    ret_val = cfg_get_oid(handle, &oid);
    if (ret_val != 0)
    {
        return TE_RC(TE_CONF_API, ret_val);
    }

    length = oid->len;
    pattern = cfg_allocate_oid(length, TRUE);
    if (pattern == NULL)
    {
        cfg_free_oid(oid);
        return TE_RC(TE_CONF_API, TE_ENOMEM);
    }

    memset(pattern->ids, '\0', length * sizeof(cfg_inst_subid));

    oid_subid = (char *)(oid->ids) + sizeof(cfg_object_subid);
    subid = (char *)(pattern->ids) + sizeof(cfg_inst_subid);
    name = ((cfg_inst_subid *)subid)->name;

    length--;

    while (length-- > 0)
    {
        memcpy((void *)(subid), (void *)oid_subid,
               sizeof(cfg_object_subid));

        oid_subid += sizeof(cfg_object_subid);
        subid += sizeof(cfg_inst_subid);

        memcpy((void *)name, (void *)star, starlen);
        name += sizeof(cfg_inst_subid);
    }

    cfg_free_oid(oid);

    pattern_name = cfg_convert_oid(pattern);
    if (pattern_name == NULL)
    {
        cfg_free_oid(pattern);
        return TE_RC(TE_CONF_API, TE_ENOMEM);
    }
    cfg_free_oid(pattern);

    ret_val = cfg_find_pattern(pattern_name, &num, &instances);
    if (ret_val != 0)
    {
        free(pattern_name);
        return TE_RC(TE_CONF_API, ret_val);
    }
    free(pattern_name);

    while (num-- > 0)
    {
        ret_val = callback(*instances, user_data);
        if (ret_val != 0)
        {
            fprintf(stderr, "User callback returns non-zero\n");
            return TE_RC(TE_CONF_API, ret_val);
        }
        instances++;
    }
    /* Do we need to release instances? */
    return 0;
}


/* See description in conf_api.h */
te_errno
cfg_reboot_ta(const char *ta_name, te_bool restore,
              rcf_reboot_type reboot_type)
{
    cfg_reboot_msg *msg;

    size_t  len;
    int     ret_val = 0;

    if (ta_name == NULL)
    {
        return TE_RC(TE_CONF_API, TE_EINVAL);
    }
#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&cfgl_lock);
#endif
    INIT_IPC;
    if (cfgl_ipc_client == NULL)
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&cfgl_lock);
#endif
        return TE_RC(TE_CONF_API, TE_EIPC);
    }

    memset(cfgl_msg_buf, 0, sizeof(cfgl_msg_buf));
    msg = (cfg_reboot_msg *)cfgl_msg_buf;
    msg->type = CFG_REBOOT;
    msg->restore = restore;
    msg->reboot_type = reboot_type;

    len = strlen(ta_name) + 1;
    memcpy(msg->ta_name, ta_name, len);
    msg->len = sizeof(cfg_reboot_msg) + len;
    len = CFG_MSG_MAX;

    ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                           CONFIGURATOR_SERVER,
                                           msg, msg->len, msg, &len);
    if (ret_val == 0)
    {
        ret_val = msg->rc;
    }

#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cfgl_lock);
#endif
    return TE_RC(TE_CONF_API, ret_val);
}


/* See description in conf_api.h */
te_errno
cfg_create_backup(char **name)
{
    cfg_backup_msg *msg;

    size_t  len;
    int     ret_val = 0;

    *name = NULL;

#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&cfgl_lock);
#endif
    INIT_IPC;
    if (cfgl_ipc_client == NULL)
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&cfgl_lock);
#endif
        return TE_RC(TE_CONF_API, TE_EIPC);
    }

    memset(cfgl_msg_buf, 0, sizeof(cfgl_msg_buf));
    msg = (cfg_backup_msg *)cfgl_msg_buf;
    msg->type = CFG_BACKUP;
    msg->op = CFG_BACKUP_CREATE;
    msg->len = sizeof(cfg_backup_msg);
    msg->subtrees_num = 0;
    msg->subtrees_offset = msg->len;
    msg->filename_offset = msg->len;
    len = CFG_MSG_MAX;

    ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                           CONFIGURATOR_SERVER,
                                           msg, msg->len, msg, &len);
    if ((ret_val == 0) && ((ret_val = msg->rc) == 0))
    {
        len = msg->len - msg->filename_offset;
        *name = (char *)calloc(1, len);
        if (*name == NULL)
        {
            ret_val = TE_ENOMEM;
        }
        else
        {
            memcpy(*name, (const char *)msg + msg->filename_offset, len);
        }
    }
#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cfgl_lock);
#endif
    return TE_RC(TE_CONF_API, ret_val);
}

/**
 * Verify/release/restore backup.
 *
 * @param name      name returned by cfg_create_backup
 * @param op        backup operation
 *
 * @return status code (see te_errno.h)
 */
static te_errno
cfg_backup(const char *name, uint8_t op)
{
    cfg_backup_msg *msg;

    size_t  len;
    int     ret_val = 0;

    if (name == NULL)
        return TE_RC(TE_CONF_API, TE_EINVAL);

#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&cfgl_lock);
#endif
    INIT_IPC;
    if (cfgl_ipc_client == NULL)
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&cfgl_lock);
#endif
        return TE_RC(TE_CONF_API, TE_EIPC);
    }

    memset(cfgl_msg_buf, 0, sizeof(cfgl_msg_buf));
    msg = (cfg_backup_msg *)cfgl_msg_buf;
    msg->type = CFG_BACKUP;
    msg->op = op;
    msg->len = sizeof(cfg_backup_msg);

    msg->subtrees_num = 0;
    msg->subtrees_offset = msg->len;
    msg->filename_offset = msg->len;

    len = strlen(name) + 1;
    msg->len += len;

    memcpy((char *)msg + msg->filename_offset, name, len);
    len = CFG_MSG_MAX;

    ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                           CONFIGURATOR_SERVER,
                                           msg, msg->len, msg, &len);
    if (ret_val == 0)
    {
        ret_val = msg->rc;
    }

#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cfgl_lock);
#endif
    return TE_RC(TE_CONF_API, ret_val);
}

/* See description in conf_api.h */
te_errno
cfg_verify_backup(const char *name)
{
    return cfg_backup(name, CFG_BACKUP_VERIFY);
}

/* See description in conf_api.h */
te_errno
cfg_release_backup(char **name)
{
    te_errno rc;

    if (name == NULL)
        return TE_RC(TE_CONF_API, TE_EINVAL);

    rc = cfg_backup(*name, CFG_BACKUP_RELEASE);
    if (rc == 0)
    {
        free(*name);
        *name = NULL;
    }

    return rc;
}

/* See description in conf_api.h */
te_errno
cfg_restore_backup(const char *name)
{
    return cfg_backup(name, CFG_BACKUP_RESTORE);
}

/* See description in conf_api.h */
te_errno
cfg_restore_backup_nohistory(const char *name)
{
    return cfg_backup(name, CFG_BACKUP_RESTORE_NOHISTORY);
}


/* See description in conf_api.h */
te_errno
cfg_create_config(const char *name, te_bool history)
{
    cfg_config_msg *msg;

    size_t  len;
    int     ret_val = 0;

    if (name == NULL)
    {
        return TE_RC(TE_CONF_API, TE_EINVAL);
    }

#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&cfgl_lock);
#endif
    INIT_IPC;
    if (cfgl_ipc_client == NULL)
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&cfgl_lock);
#endif
        return TE_RC(TE_CONF_API, TE_EIPC);
    }

    memset(cfgl_msg_buf, 0, sizeof(cfgl_msg_buf));
    msg = (cfg_config_msg *)cfgl_msg_buf;
    msg->type = CFG_CONFIG;
    msg->history = history;

    len = strlen(name) + 1;
    memcpy(msg->filename, name, len);
    msg->len = sizeof(cfg_config_msg) + len;
    len = CFG_MSG_MAX;

    ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                           CONFIGURATOR_SERVER,
                                           msg, msg->len, msg, &len);
    if (ret_val == 0)
    {
        ret_val = msg->rc;
    }

#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cfgl_lock);
#endif
    return TE_RC(TE_CONF_API, ret_val);
}


/* See description in conf_api.h */
te_errno
cfg_process_history(const char *filename, const te_kvpair_h *expand_vars)
{
    cfg_process_history_msg *msg;

    size_t  len;
    int     ret_val = 0;
    const te_kvpair *p;

    if (filename == NULL)
        return TE_RC(TE_CONF_API, TE_EINVAL);

#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&cfgl_lock);
#endif

    INIT_IPC;
    if (cfgl_ipc_client == NULL)
    {
        ret_val = TE_EIPC;
        goto done;
    }

    memset(cfgl_msg_buf, 0, sizeof(cfgl_msg_buf));
    msg = (cfg_process_history_msg *)cfgl_msg_buf;
    msg->type = CFG_PROCESS_HISTORY;

    len = strlen(filename) + 1;
    memcpy(msg->filename, filename, len);

    msg->len = sizeof(cfg_config_msg) + len;
    if (expand_vars != NULL)
    {
        TAILQ_FOREACH(p, expand_vars, links)
        {
            if (msg->len + strlen(p->key) + 1 > CFG_MSG_MAX)
            {
                ret_val = TE_ESMALLBUF;
                goto done;
            }
            memcpy((char *)msg + msg->len, p->key, strlen(p->key) + 1);
            msg->len += strlen(p->key) + 1;

            if (msg->len + strlen(p->value) + 1 > CFG_MSG_MAX)
            {
                ret_val = TE_ESMALLBUF;
                goto done;
            }
            memcpy((char *)msg + msg->len, p->value, strlen(p->value) + 1);
            msg->len += strlen(p->value) + 1;
        }
    }
    len = CFG_MSG_MAX;

    ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                           CONFIGURATOR_SERVER,
                                           msg, msg->len, msg, &len);
    if (ret_val == 0)
        ret_val = msg->rc;

done:
#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cfgl_lock);
#endif
    return TE_RC(TE_CONF_API, ret_val);
}


/* See description in conf_api.h */
void
cfg_api_cleanup(void)
{
    int rc = ipc_close_client(cfgl_ipc_client);

    if (rc != 0)
    {
        ERROR("%s(): ipc_close_client() failed with rc=%d",
              __FUNCTION__, rc);
    }
}

/* See description in conf_api.h */
te_errno
cfg_wait_changes(void)
{
    cfg_msg *msg;

    size_t  len = CFG_MSG_MAX;
    int     ret_val = 0;

#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&cfgl_lock);
#endif
    INIT_IPC;
    if (cfgl_ipc_client == NULL)
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&cfgl_lock);
#endif
        return TE_RC(TE_CONF_API, TE_EIPC);
    }

    memset(cfgl_msg_buf, 0, sizeof(cfgl_msg_buf));
    msg = (cfg_msg *)cfgl_msg_buf;
    msg->type = CFG_CONF_DELAY;
    msg->len = sizeof(*msg);

    ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                           CONFIGURATOR_SERVER,
                                           msg, msg->len, msg, &len);
    if (ret_val == 0)
    {
        ret_val = msg->rc;
    }

#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cfgl_lock);
#endif
    return TE_RC(TE_CONF_API, ret_val);
}

/* See description in conf_api.h */
te_errno
cfg_touch_instance(const char *oid_tmpl, ...)
{
    cfg_conf_touch_msg *msg;

    size_t  len = CFG_MSG_MAX;
    int     ret_val = 0;
    va_list ap;

#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&cfgl_lock);
#endif
    INIT_IPC;
    if (cfgl_ipc_client == NULL)
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&cfgl_lock);
#endif
        return TE_RC(TE_CONF_API, TE_EIPC);
    }

    memset(cfgl_msg_buf, 0, sizeof(cfgl_msg_buf));
    msg = (cfg_conf_touch_msg *)cfgl_msg_buf;
    msg->type = CFG_CONF_TOUCH;

    va_start(ap, oid_tmpl);
    vsnprintf(msg->oid, sizeof(cfgl_msg_buf) - sizeof(*msg), oid_tmpl, ap);
    va_end(ap);

    msg->len = sizeof(*msg) + strlen(msg->oid) + 1;

    ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                           CONFIGURATOR_SERVER,
                                           msg, msg->len, msg, &len);
    if (ret_val == 0)
    {
        ret_val = msg->rc;
    }

#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cfgl_lock);
#endif
    return TE_RC(TE_CONF_API, ret_val);
}


/* See description in conf_api.h */
te_errno
cfg_tree_print(const char *filename,
               const unsigned int log_lvl,
               const char *id_fmt, ...)
{
    cfg_tree_print_msg *msg;

    size_t      id_len, flname_len, len;
    char        id[CFG_OID_MAX];
    te_errno    ret_val = 0;
    va_list ap;


    if (id_fmt == NULL)
        return TE_RC(TE_CONF_API, TE_EINVAL);
    va_start(ap, id_fmt);
    id_len = (size_t)vsnprintf(id, sizeof(id), id_fmt, ap);
    va_end(ap);
    if (id_len >= sizeof(id))
        return TE_RC(TE_CONF_API, TE_EINVAL);
    id_len += 1; /* '\0' */

    if (filename == NULL)
        flname_len = 0;
    else
        flname_len = strlen(filename) + 1;

    if (sizeof(cfg_tree_print_msg) + id_len + flname_len > CFG_MSG_MAX)
        return TE_RC(TE_CONF_API, TE_EMSGSIZE);

#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&cfgl_lock);
#endif
    INIT_IPC;
    if (cfgl_ipc_client == NULL)
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&cfgl_lock);
#endif
        return TE_RC(TE_CONF_API, TE_EIPC);
    }

    memset(cfgl_msg_buf, 0, sizeof(cfgl_msg_buf));
    msg = (cfg_tree_print_msg *)cfgl_msg_buf;
    msg->type = CFG_TREE_PRINT;
    msg->log_lvl = log_lvl;

    memcpy(msg->buf, id, id_len);
    msg->id_len = id_len;

    if (flname_len != 0)
        memcpy((msg->buf) + id_len, filename, flname_len);
    msg->flname_len = flname_len;

    msg->len = sizeof(cfg_tree_print_msg) + id_len + flname_len;

    len = CFG_MSG_MAX;
    ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                           CONFIGURATOR_SERVER,
                                           msg, msg->len, msg, &len);
    if (ret_val == 0)
        ret_val = msg->rc;

#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cfgl_lock);
#endif
    return TE_RC(TE_CONF_API, ret_val);
}


/* See description in conf_api.h */
te_errno
cfg_unregister_object_str(const char *id_fmt, ...)
{
    cfg_unregister_msg *msg;

    size_t      id_len, len;
    char        id[CFG_OID_MAX];
    te_errno    ret_val = 0;
    va_list ap;


    if (id_fmt == NULL)
        return TE_RC(TE_CONF_API, TE_EINVAL);
    va_start(ap, id_fmt);
    id_len = (size_t)vsnprintf(id, sizeof(id), id_fmt, ap);
    va_end(ap);
    if (id_len >= sizeof(id))
        return TE_RC(TE_CONF_API, TE_EINVAL);
    id_len += 1; /* '\0' */

    if (sizeof(cfg_unregister_msg) + id_len > CFG_MSG_MAX)
        return TE_RC(TE_CONF_API, TE_EMSGSIZE);

#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&cfgl_lock);
#endif
    INIT_IPC;
    if (cfgl_ipc_client == NULL)
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&cfgl_lock);
#endif
        return TE_RC(TE_CONF_API, TE_EIPC);
    }

    memset(cfgl_msg_buf, 0, sizeof(cfgl_msg_buf));
    msg = (cfg_unregister_msg *)cfgl_msg_buf;
    msg->type = CFG_UNREGISTER;

    memcpy(msg->id, id, id_len);

    msg->len = sizeof(cfg_unregister_msg) + id_len;

    len = CFG_MSG_MAX;
    ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                           CONFIGURATOR_SERVER,
                                           msg, msg->len, msg, &len);
    if (ret_val == 0)
        ret_val = msg->rc;

#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cfgl_lock);
#endif
    return TE_RC(TE_CONF_API, ret_val);
}

/* See description in 'conf_api.h' */
te_errno
cfg_copy_subtree_fmt(const char *dst_oid,
                     const char *src_oid_fmt,
                     ...)
{
    cfg_copy_msg   *msg;
    va_list         ap;
    char            src_oid[CFG_OID_MAX];
    size_t          src_oid_len;
    size_t          len;
    size_t          dst_oid_len;
    cfg_handle      src_handle;
    te_errno        rc = 0;

    va_start(ap, src_oid_fmt);
    vsnprintf(src_oid, sizeof(src_oid), src_oid_fmt, ap);
    va_end(ap);

    src_oid_len = strlen(src_oid);
    assert(src_oid_len != 0);

    rc = cfg_find_str(src_oid, &src_handle);
    if (rc != 0)
        return TE_RC(TE_CONF_API, rc);

#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&cfgl_lock);
#endif
    INIT_IPC;
    if (cfgl_ipc_client == NULL)
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&cfgl_lock);
#endif
        return TE_RC(TE_CONF_API, TE_EIPC);
    }

    memset(cfgl_msg_buf, 0, sizeof(cfgl_msg_buf));
    msg = (cfg_copy_msg *)cfgl_msg_buf;
    msg->type = CFG_COPY;
    msg->src_handle = src_handle;
    msg->is_obj = (strchr(dst_oid, ':') == NULL) ? TRUE : FALSE;
    msg->len = sizeof(cfg_copy_msg);

    dst_oid_len = strlen(dst_oid);
    if (msg->len + dst_oid_len + 1 > CFG_MSG_MAX)
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&cfgl_lock);
#endif
        return TE_RC(TE_CONF_API, TE_ENOBUFS);
    }
    else
    {
        memcpy(msg->dst_oid, dst_oid, dst_oid_len + 1);
        msg->len = sizeof(cfg_copy_msg) + dst_oid_len + 1;
    }

    len = CFG_MSG_MAX;
    rc = ipc_send_message_with_answer(cfgl_ipc_client,
                                      CONFIGURATOR_SERVER,
                                      msg, msg->len, msg, &len);
    if (rc == 0)
        rc = msg->rc;

#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cfgl_lock);
#endif

    return TE_RC(TE_CONF_API, rc);
}

