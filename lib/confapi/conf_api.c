/** @file
 * @brief Configurator
 *
 * OID conversion routines
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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


#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "conf_api.h"
#include "conf_messages.h"
#include "conf_types.h"
#include "rcf_api.h"
#include "ipc_client.h"

#define LGR_USER        "Configurator API"
#include "logger_api.h"

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
            cfgl_ipc_client = ipc_init_client(name);                \
        }                                                           \
    } while (0)


#ifdef HAVE_PTHREAD_H
static pthread_mutex_t cfgl_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

/** IPC client to interact with Configurator */
static ipc_client *cfgl_ipc_client = NULL;

/** Message buffer */
static char cfgl_msg_buf[CFG_MSG_MAX] = {0,};


static int cfg_get_family_member(cfg_handle handle,
                                 uint8_t who,
                                 cfg_handle *member);
static int kill_all(cfg_handle handle);
static int kill(cfg_handle handle);


/**
 * Register new object using string object identifiers.
 *
 * @param oid       object identifier in string representation
 * @param descr     object properties description
 * @param handle    location for registred object handle
 *
 * @return status code (see te_errno.h)
 */
int
cfg_register_object_str(const char *oid, cfg_obj_descr *descr,
                        cfg_handle *handle)
{
    size_t  len;
    int     ret_val = 0;

    cfg_register_msg *msg;

    if ((oid == NULL) || (descr == NULL))
    {
        return TE_RC(TE_CONF_API, EINVAL);
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
        return TE_RC(TE_CONF_API, ETEIO);
    }
    msg = (cfg_register_msg *)cfgl_msg_buf;
    msg->type = CFG_REGISTER;
    memcpy((void *)&(msg->descr), (void *)descr, sizeof(cfg_obj_descr));

    len = strlen(oid) + 1;
    memcpy((void *)(msg->oid), (void *)oid, len);
    msg->len = sizeof(cfg_register_msg) + len;
    len = CFG_MSG_MAX;

    if ((ret_val =
             ipc_send_message_with_answer(cfgl_ipc_client, CONFIGURATOR_SERVER,
                                         (char *)msg, msg->len,
                                         (char *)msg, &len)) != 0)
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&cfgl_lock);
#endif
        return TE_RC(TE_CONF_API, ret_val);
    }
    if ((ret_val = msg->rc) != 0)
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&cfgl_lock);
#endif
        return TE_RC(TE_CONF_API, ret_val);
    }
    *handle = msg->handle;
#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cfgl_lock);
#endif
    return TE_RC(TE_CONF_API, ret_val);
}

/**
 * Register new object using array object identifiers.
 *
 * @param oid       object identifier
 * @param descr     object properties description
 * @param handle    location for registred object handle
 *
 * @return status code (see te_errno.h)
 */
int
cfg_register_object(const cfg_oid *oid, cfg_obj_descr *descr,
                    cfg_handle *handle)
{
    const char *str;
    cfg_handle tmp;

    int ret_val;

    if ((oid == NULL) || (descr == NULL))
    {
        return TE_RC(TE_CONF_API, EINVAL);
    }

    str = cfg_convert_oid(oid);
    if (str == NULL)
    {
        return TE_RC(TE_CONF_API, ENOMEM);
    }

    ret_val = cfg_register_object_str(str, descr, &tmp);
    if (ret_val != 0)
    {
        *handle = CFG_HANDLE_INVALID;
        return TE_RC(TE_CONF_API, ret_val);
    }
    *handle = tmp;
    return 0;
}

/**
 * Obtain parameters configured for the object.
 *
 * @param handle        object handle
 * @param descr         OUT: location for the object properties description
 *
 * @return status code (see te_errno.h)
 */
int
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
        return TE_RC(TE_CONF_API, EINVAL);
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
        return TE_RC(TE_CONF_API, ETEIO);
    }
    msg = (cfg_get_descr_msg *)cfgl_msg_buf;

    msg->type = CFG_GET_DESCR;
    msg->len = sizeof(cfg_get_descr_msg);
    msg->handle = handle;
    len = CFG_MSG_MAX;

    ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                           CONFIGURATOR_SERVER,
                                           (char *)msg, msg->len,
                                           (char *)msg, &len);
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

/**
 * Obtain identifier of object or object instance by its handle.
 *
 * @param handle        handle of object or object instance
 * @param oid           OUT: location for the oid pointer (memory for the
 *                      string is allocated by the routine using malloc()
 *
 * @return 0 or EINVAL if invalid handle is provided
 */
int
cfg_get_oid_str(cfg_handle handle, char **oid)
{
    cfg_get_oid_msg *msg;

    size_t  len;
    int     ret_val = 0;

    char *str;

    *oid = NULL;

    if (handle == CFG_HANDLE_INVALID)
    {
        return TE_RC(TE_CONF_API, EINVAL);
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
        return TE_RC(TE_CONF_API, ETEIO);
    }
    msg = (cfg_get_oid_msg *)cfgl_msg_buf;

    msg->type = CFG_GET_OID;
    msg->len = sizeof(cfg_get_oid_msg);
    msg->handle = handle;
    len = CFG_MSG_MAX;

    ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                           CONFIGURATOR_SERVER,
                                           (char *)msg, msg->len,
                                           (char *)msg, &len);
    if ((ret_val == 0) && ((ret_val = msg->rc) == 0))
    {
        len = strlen(msg->oid) + 1;
        str = (char *)calloc(1, len);
        if (str == NULL)
        {
            ret_val = ENOMEM;
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

/**
 * Obtain identifier of object or object instance by its handle.
 *
 * @param handle        handle of object or object instance
 * @param oid           OUT: location for the oid pointer (memory for the array
 *                      is allocated by the routine using malloc()
 *
 * @return 0 or EINVAL if invalid handle is provided
 */
int
cfg_get_oid(cfg_handle handle, cfg_oid **oid)
{
    char   *str;
    int     ret_val = 0;

    *oid = NULL;

    if (handle == CFG_HANDLE_INVALID)
    {
        return TE_RC(TE_CONF_API, EINVAL);
    }

    if ((ret_val = cfg_get_oid_str(handle, &str)) != 0)
    {
        return TE_RC(TE_CONF_API, ret_val);
    }
    *oid = cfg_convert_oid_str(str);
    free(str);
    if (*oid == NULL)
    {
        return TE_RC(TE_CONF_API, ENOMEM);
    }
    return 0;
}

/**
 * Obtain sub-identifier of object by its handle.
 *
 * @param handle        handle of object
 * @param subid         OUT: location for the sub-identifier (should be at least
 *                      CFG_SUBID_MAX length)
 *
 * @return 0 or EINVAL if invalid handle is provided
 */
int
cfg_get_subid(cfg_handle handle, char **subid)
{
    cfg_get_id_msg *msg;

    size_t  len;
    int     ret_val = 0;

    char *str;

    *subid = NULL;

    if (handle == CFG_HANDLE_INVALID)
    {
        return TE_RC(TE_CONF_API, EINVAL);
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
        return TE_RC(TE_CONF_API, ETEIO);
    }
    msg = (cfg_get_id_msg *)cfgl_msg_buf;

    msg->type = CFG_GET_ID;
    msg->len = sizeof(cfg_get_id_msg);
    msg->handle = handle;
    len = CFG_MSG_MAX;

    ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                           CONFIGURATOR_SERVER,
                                           (char *)msg, msg->len,
                                           (char *)msg, &len);
    if ((ret_val == 0) && ((ret_val = msg->rc) == 0))
    {
        len = strlen(msg->id) + 1;
        str = (char *)calloc(1, len);
        if (str == NULL)
        {
            ret_val = ENOMEM;
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

/**
 * Obtain name of object instance by its handle.
 *
 * @param handle        handle of object instance
 * @param name          OUT: location for the name
 *
 * @return 0 or EINVAL if invalid handle is provided
 */
int
cfg_get_inst_name(cfg_handle handle, char **name)
{
    cfg_get_id_msg *msg;

    size_t  len;
    int     ret_val = 0;

    char *str;

    *name = NULL;

    if (handle == CFG_HANDLE_INVALID)
    {
        return TE_RC(TE_CONF_API, EINVAL);
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
        return TE_RC(TE_CONF_API, ETEIO);
    }
    msg = (cfg_get_id_msg *)cfgl_msg_buf;

    msg->type = CFG_GET_ID;
    msg->len = sizeof(cfg_get_id_msg);
    msg->handle = handle;
    len = CFG_MSG_MAX;

    ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                           CONFIGURATOR_SERVER,
                                           (char *)msg, msg->len,
                                           (char *)msg, &len);
    if ((ret_val == 0) && ((ret_val = msg->rc) == 0))
    {
        len = strlen(msg->id) + 1;
        str = (char *)calloc(1, len);
        if (str == NULL)
        {
            ret_val = ENOMEM;
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
int
cfg_get_ith_inst_name(const char *str_oid, unsigned int i, char **name)
{
    cfg_oid *oid;

    if ((str_oid == NULL) || (name == NULL))
    {
        return TE_RC(TE_CONF_API, EINVAL);
    }

    oid = cfg_convert_oid_str(str_oid);
    if (oid == NULL || !oid->inst || oid->len <= i)
    {
        ERROR("Invalid value in network node: %s", str_oid);
        return ETEFMT;
    }

    *name = strdup(CFG_OID_GET_INST_NAME(oid, i));

    cfg_free_oid(oid);

    return 0;

}


/**
 * Find the object or object instance by its object identifier.
 * Root object identifier is "/"; root object instance identifier is "/:".
 *
 * @param oid           object identifier in string representation
 * @param handle        location for object or instance handle
 *
 * @return      status code
 */
int
cfg_find_str(const char *oid, cfg_handle *handle)
{
    cfg_find_msg *msg;

    size_t  len;
    int     ret_val = 0;

    if (oid == NULL)
    {
        return TE_RC(TE_CONF_API, EINVAL);
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
        return TE_RC(TE_CONF_API, ETEIO);
    }
    msg = (cfg_find_msg *)cfgl_msg_buf;
    len = strlen(oid) + 1;

    msg->type = CFG_FIND;
    memcpy(msg->oid, oid, len);
    msg->len = sizeof(cfg_find_msg) + len;
    len = CFG_MSG_MAX;

    ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                           CONFIGURATOR_SERVER,
                                           (char *)msg, msg->len,
                                           (char *)msg, &len);
    if ((ret_val == 0) && ((ret_val = msg->rc) == 0))
    {
        *handle = msg->handle;
    }
#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cfgl_lock);
#endif
    return TE_RC(TE_CONF_API, ret_val);
}

/**
 * Find the object or object instance by its object identifier.
 *
 * @param oid           object identifier
 * @param handle        location for object or instance handle
 *
 * @return      status code
 */
int
cfg_find(const cfg_oid *oid, cfg_handle *handle)
{
    const char *str;
    cfg_handle tmp;
    int ret_val;

    if (oid == NULL)
    {
        return TE_RC(TE_CONF_API, EINVAL);
    }

    str = cfg_convert_oid(oid);
    if (str == NULL)
        return TE_RC(TE_CONF_API, ENOMEM);

    ret_val = cfg_find_str(str, &tmp);
    if (ret_val == 0)
        *handle = tmp;
    
    return TE_RC(TE_CONF_API, ret_val);
}

/*
 * Find object of the given instance.
 *
 * @param    instance    instance handle
 * @param    object    OUT: localtion for object handle
 *
 * @return    status code.
 */
int
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
        return TE_RC(TE_CONF_API, ENOMEM);
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

/**
 * Find all objects or object instances matching to pattern.
 *
 * @param pattern       string object identifier possibly containing '*'
 *                      (see Configurator documentation for details)
 * @param num           OUT: number of found objects or object instances
 * @param set           OUT: array of object/object instances handles;
 *                           memory for the array is allocated using malloc()
 *
 * @return 0 or EINVAL if pattern format is incorrect some argument is NULL
 */
int
cfg_find_pattern(const char *pattern, int *num, cfg_handle **set)
{
    cfg_pattern_msg *msg;
    char            *alloc_msg = NULL;

    size_t  len;
    int     ret_val = 0;

    *num = 0;
    *set = NULL;

    if (pattern == NULL)
    {
        return TE_RC(TE_CONF_API, EINVAL);
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
        return TE_RC(TE_CONF_API, ETEIO);
    }
    msg = (cfg_pattern_msg *)cfgl_msg_buf;
    len = strlen(pattern) + 1;

    msg->type = CFG_PATTERN;
    memcpy(msg->pattern, pattern, len);
    msg->len = sizeof(cfg_pattern_msg) + len;
    len = CFG_MSG_MAX;

    ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                           CONFIGURATOR_SERVER,
                                           (char *)msg, msg->len,
                                           (char *)msg, &len);
    if (ret_val == ETESMALLBUF)
    {
        size_t  rest_len = len - CFG_MSG_MAX;

        assert(len > CFG_MSG_MAX);
        alloc_msg = malloc(len);
        if (alloc_msg == NULL)
            return TE_RC(TE_CONF_API, ENOMEM);
        
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
        *set = (cfg_handle *)calloc(*num, sizeof(cfg_handle));
        if (set == NULL)
        {
            *num = 0;
            ret_val = ENOMEM;
        }
        else
        {
            memcpy((void *)(*set), (void *)(msg->handles),
                   *num * sizeof(cfg_handle));
        }
    }
#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cfgl_lock);
#endif
    free(alloc_msg);
    return TE_RC(TE_CONF_API, ret_val);
}


/**
 * Get handle of the family member of the object or object instance.
 *
 * @param handle    handle of the object or object instance
 * @param member    location for family member's handle
 *
 * @returns family member's handle or CFG_HANDLE_INVALID
 */
static int
cfg_get_family_member(cfg_handle handle, uint8_t who, cfg_handle *member)
{
    cfg_family_msg *msg;

    size_t  len;
    int     ret_val = 0;

    if (handle == CFG_HANDLE_INVALID)
    {
        return EINVAL;
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
        return ETEIO;
    }
    msg = (cfg_family_msg *)cfgl_msg_buf;

    msg->type = CFG_FAMILY;
    msg->len = sizeof(cfg_family_msg);
    msg->handle = handle;
    msg->who = who;
    len = CFG_MSG_MAX;

    ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                           CONFIGURATOR_SERVER,
                                           (char *)msg, msg->len,
                                           (char *)msg, &len);
    if ((ret_val == 0) && ((ret_val = msg->rc) == 0))
    {
        *member = msg->handle;
    }
#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cfgl_lock);
#endif
    return TE_RC(TE_CONF_API, ret_val);
}


/**
 * Get handle of the oldest son of the object or object instance.
 *
 * @param handle    handle of the object or object instance
 * @param son       location for son's handle
 *
 * @returns sun's handle or CFG_HANDLE_INVALID
 */
int
cfg_get_son(cfg_handle handle, cfg_handle *son)
{
    int ret_val;
    cfg_handle out;

    ret_val = cfg_get_family_member(handle, CFG_SON, &out);
    if (ret_val == 0)
    {
        *son = out;
    }
    return TE_RC(TE_CONF_API, ret_val);
}

/**
 * Get handle of the brother of the object or object instance.
 *
 * @param handle    handle of the object or object instance
 * @param brother   location for brother's handle
 *
 * @returns brother's handle or CFG_HANDLE_INVALID
 */
int
cfg_get_brother(cfg_handle handle, cfg_handle *brother)
{
    int ret_val;
    cfg_handle out;

    ret_val = cfg_get_family_member(handle, CFG_BROTHER, &out);
    if (ret_val == 0)
    {
        *brother = out;
    }
    return TE_RC(TE_CONF_API, ret_val);
}

/**
 * Get handle of the father of the object or object instance.
 *
 * @param handle    handle of the object or object instance
 * @param father    location for father's handle
 *
 * @returns father's handle or CFG_HANDLE_INVALID
 */
int
cfg_get_father(cfg_handle handle, cfg_handle *father)
{
    int ret_val;
    cfg_handle out;

    ret_val = cfg_get_family_member(handle, CFG_FATHER, &out);
    if (ret_val == 0)
    {
        *father = out;
    }
    return TE_RC(TE_CONF_API, ret_val);
}

/**
 * Create an object instance.
 *
 * @param  oid        object identifier of the new instance
 * @param  handle     location for handle of the new instance
 * @param  type       value type (necessary for fast processing)
 * @param  ...        value to be assigned to the new instance or NULL;
 *                    for integer values: int
 *                    for strings: char *
 *                    for addreses: struct sockaddr *
 *
 * @return status code (see te_errno.h)
 */
int
cfg_add_instance(const cfg_oid *oid, cfg_handle *handle, cfg_val_type type, ...)
{
    cfg_add_msg *msg;

    va_list      list;
    cfg_inst_val value;
    char        *oid2str;

    size_t  len;
    int     ret_val = 0;

    if (oid == NULL)
    {
        return TE_RC(TE_CONF_API, EINVAL);
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
        return TE_RC(TE_CONF_API, ETEIO);
    }
    msg = (cfg_add_msg *)cfgl_msg_buf;
    msg->type = CFG_ADD;
    msg->val_type = type;

    va_start(list, type);
    switch (type)
    {
        case CVT_INTEGER:
            value.val_int = va_arg(list, int);
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
    va_end(list);

    cfg_types[type].put_to_msg(value, (cfg_msg *)msg);

    oid2str = cfg_convert_oid(oid);
    if (oid2str == NULL)
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&cfgl_lock);
#endif
        return TE_RC(TE_CONF_API, ENOMEM);
    }
    msg->oid_offset = msg->len;
    msg->len += strlen(oid2str) + 1;
    strcpy((char *)msg + msg->oid_offset, oid2str);
    free(oid2str);

    len = CFG_MSG_MAX;

    ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                           CONFIGURATOR_SERVER,
                                           (char *)msg, msg->len,
                                           (char *)msg, &len);
    if ((ret_val == 0) && ((ret_val = msg->rc) == 0))
    {
        *handle = msg->handle;
    }

#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cfgl_lock);
#endif
    return TE_RC(TE_CONF_API, ret_val);
}

/**
 * Create an object instance.
 *
 * @param  oid        object identifier of the new instance
 *                    (string representation)
 * @param  handle     location for handle of the new instance
 * @param  type       value type (necessary for fast processing)
 * @param  ...        value to be assigned to the new instance or NULL;
 *                    for integer values: int
 *                    for strings: char *
 *                    for addreses: struct sockaddr *
 *
 * @return status code (see te_errno.h)
 */
int
cfg_add_instance_str(const char *oid, cfg_handle *handle, cfg_val_type type, ...)
{
    cfg_add_msg *msg;

    va_list      list;
    cfg_inst_val value;

    size_t  len;
    int     ret_val = 0;

    if (oid == NULL)
    {
        return TE_RC(TE_CONF_API, EINVAL);
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
        return TE_RC(TE_CONF_API, ETEIO);
    }
    msg = (cfg_add_msg *)cfgl_msg_buf;
    msg->type = CFG_ADD;
    msg->val_type = type;

    va_start(list, type);
    switch (type)
    {
        case CVT_INTEGER:
            value.val_int = va_arg(list, int);
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
    va_end(list);

    cfg_types[type].put_to_msg(value, (cfg_msg *)msg);

    msg->oid_offset = msg->len;
    msg->len += strlen(oid) + 1;
    strcpy((char *)msg + msg->oid_offset, oid);

    len = CFG_MSG_MAX;

    ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                           CONFIGURATOR_SERVER,
                                           (char *)msg, msg->len,
                                           (char *)msg, &len);
    if ((ret_val == 0) && ((ret_val = msg->rc) == 0))
    {
        if (handle != NULL)
            *handle = msg->handle;
    }

#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cfgl_lock);
#endif

    return TE_RC(TE_CONF_API, ret_val);
}

/*
 * Delete an object instance with it's subtree
 *
 * @param    handle    object instance handle
 *
 * @return
 *     status code
 */
static int
kill_all(cfg_handle handle)
{
    int ret_val;
    cfg_handle son;
    cfg_handle brother;

    if (handle == CFG_HANDLE_INVALID)
    {
        return TE_RC(TE_CONF_API, EINVAL);
    }

    ret_val = cfg_get_son(handle, &son);
    if (ret_val != 0)
    {
        return TE_RC(TE_CONF_API, ret_val);
    }
    if (son != CFG_HANDLE_INVALID)
    {
        ret_val = kill_all(son);
        if (ret_val != 0 && ret_val != EACCES)
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
        ret_val = kill_all(brother);
        if (ret_val != 0 && ret_val != EACCES)
        {
            return TE_RC(TE_CONF_API, ret_val);
        }
    }
    ret_val = kill(handle);
    if (ret_val != 0 && ret_val != EACCES)
    {
        return TE_RC(TE_CONF_API, ret_val);
    }
    return 0;
}

/*
 * Delete an object instance
 *
 * @param    handle    object instance handle
 *
 * @return
 *     status code
 */
static int
kill(cfg_handle handle)
{
    cfg_del_msg *msg;

    int     ret_val = 0;
    size_t  len;

    if (handle == CFG_HANDLE_INVALID)
    {
        return TE_RC(TE_CONF_API, EINVAL);
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
        return TE_RC(TE_CONF_API, ETEIO);
    }
    msg = (cfg_del_msg *)cfgl_msg_buf;

    msg->type = CFG_DEL;
    msg->len = sizeof(cfg_del_msg);
    msg->handle = handle;
    len = CFG_MSG_MAX;

    ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                           CONFIGURATOR_SERVER,
                                           (char *)msg, msg->len,
                                           (char *)msg, &len);
    if (ret_val == 0)
    {
        ret_val = msg->rc;
    }

#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cfgl_lock);
#endif

    return TE_RC(TE_CONF_API, ret_val);
}

/**
 * Delete an object instance.
 *
 * @param handle                object instance handle
 * @param with_children         delete the children subtree, if necessary
 *
 * @return status code (see te_errno.h)
 */
int
cfg_del_instance(cfg_handle handle, te_bool with_children)
{
    cfg_handle son;
    int ret_val;

    if (handle == CFG_HANDLE_INVALID)
    {
        return TE_RC(TE_CONF_API, EINVAL);
    }

    if (with_children == TRUE)
    {
        ret_val = cfg_get_son(handle, &son);
        if (son != CFG_HANDLE_INVALID)
        {
            ret_val = kill_all(son);
            if (ret_val != 0)
                return TE_RC(TE_CONF_API, ret_val);
        }
    }
    ret_val = kill(handle);
    return TE_RC(TE_CONF_API, ret_val);
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
static int
cfg_set_instance_gen(cfg_handle handle, te_bool local, cfg_val_type type,
                     va_list list)
{
    cfg_set_msg    *msg;
    cfg_inst_val    value;

    int     ret_val = 0;
    size_t  len;

    if (handle == CFG_HANDLE_INVALID)
    {
        return TE_RC(TE_CONF_API, EINVAL);
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
        return TE_RC(TE_CONF_API, ETEIO);
    }
    msg = (cfg_set_msg *)cfgl_msg_buf;
    msg->type = CFG_SET;
    msg->local = local;
    msg->handle = handle;
    msg->val_type = type;

    switch (type)
    {
        case CVT_INTEGER:
            value.val_int = va_arg(list, int);
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
    cfg_types[type].put_to_msg(value, (cfg_msg *)msg);

    len = CFG_MSG_MAX;

    if ((ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                                CONFIGURATOR_SERVER,
                                                msg, msg->len,
                                                msg, &len)) == 0)
    {
        ret_val = msg->rc;
    }

#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cfgl_lock);
#endif

    return TE_RC(TE_CONF_API, ret_val);
}

/* See description in conf_api.h */
int
cfg_set_instance(cfg_handle handle, cfg_val_type type, ...)
{
    int     ret_val;
    va_list list;

    va_start(list, type);
    ret_val = cfg_set_instance_gen(handle, FALSE, type, list);
    va_end(list);

    return TE_RC(TE_CONF_API, ret_val);
}

/* See description in conf_api.h */
int
cfg_set_instance_local(cfg_handle handle, cfg_val_type type, ...)
{
    int     ret_val;
    va_list list;

    va_start(list, type);
    ret_val = cfg_set_instance_gen(handle, TRUE, type, list);
    va_end(list);

    return TE_RC(TE_CONF_API, ret_val);
}

/* See description in conf_api.h */
int
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
        return TE_RC(TE_CONF_API, ETEIO);
    }

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
    }

#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cfgl_lock);
#endif

    return TE_RC(TE_CONF_API, ret_val);
}


/**
 * Obtain value of the object instance. Memory for strings and
 * addresses is allocated by the routine using malloc().
 *
 * @param handle      object instance handle
 * @param type        location for value type, may be NULL
 * @param  ...        OUT: location for the value
 *                    for integer values: int *
 *                    for strings: char **
 *                    for addreses: struct sockaddr **
 *
 * @return status code (see te_errno.h)
 */
int
cfg_get_instance(cfg_handle handle, cfg_val_type *type, ...)
{
    cfg_get_msg *msg;

    va_list      list;
    cfg_inst_val value;

    size_t  len;
    int     ret_val = 0;

    if (handle == CFG_HANDLE_INVALID)
    {
        return TE_RC(TE_CONF_API, EINVAL);
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
        return TE_RC(TE_CONF_API, ETEIO);
    }
    msg = (cfg_get_msg *)cfgl_msg_buf;
    msg->type = CFG_GET;
    msg->sync = FALSE;
    msg->handle = handle;
    msg->len = sizeof(cfg_get_msg);
    len = CFG_MSG_MAX;

    ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                           CONFIGURATOR_SERVER,
                                           (char *)msg, msg->len,
                                           (char *)msg, &len);
    if ((ret_val != 0) || ((ret_val = msg->rc) != 0) ||
        ((ret_val = cfg_types[msg->val_type].get_from_msg((cfg_msg *)msg,
                                                          &value)) != 0))
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&cfgl_lock);
#endif
        return TE_RC(TE_CONF_API, ret_val);
    }

    if (type != NULL && *type != CVT_UNSPECIFIED && *type != msg->val_type)
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&cfgl_lock);
#endif
        return TE_RC(TE_CONF_API, ETEBADTYPE);
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

    if ((type != NULL) && (*type == CVT_UNSPECIFIED))
    {
        *type = msg->val_type;
    }

#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cfgl_lock);
#endif
    return 0;
}


/**
 * Obtain value of the object instance with synchronization with
 * the managed object. Memory for strings and
 * addresses is allocated by the routine using malloc().
 *
 * @param handle      object instance handle
 * @param type        location for value type, may be NULL
 * @param  ...        OUT: location for the value
 *                    for integer values: int *
 *                    for strings: char **
 *                    for addreses: struct sockaddr **
 *
 * @return status code (see te_errno.h)
 */
int
cfg_get_instance_sync(cfg_handle handle, cfg_val_type *type, ...)
{
    cfg_get_msg *msg;

    va_list      list;
    cfg_inst_val value;

    size_t  len;
    int     ret_val = 0;

    if (handle == CFG_HANDLE_INVALID)
    {
        return TE_RC(TE_CONF_API, EINVAL);
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
        return TE_RC(TE_CONF_API, ETEIO);
    }
    msg = (cfg_get_msg *)cfgl_msg_buf;
    msg->type = CFG_GET;
    msg->sync = TRUE;
    msg->handle = handle;
    msg->len = sizeof(cfg_get_msg);
    len = CFG_MSG_MAX;

    ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                           CONFIGURATOR_SERVER,
                                           (char *)msg, msg->len,
                                           (char *)msg, &len);
    if ((ret_val != 0) || ((ret_val = msg->rc) != 0) ||
        ((ret_val = cfg_types[msg->val_type].get_from_msg((cfg_msg *)msg,
                                                          &value)) != 0))
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&cfgl_lock);
#endif
        return ret_val;
    }

#if 0
    if (type != NULL && *type != CVT_UNSPECIFIED && *type != msg->val_type)
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&cfgl_lock);
#endif
        return ETEBADTYPE;
    }
#endif
    va_start(list, type);
    switch (msg->val_type)
    {
         case CVT_INTEGER:
         {
             int *val_int = va_arg(list, int *);

             *val_int = value.val_int;
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


/**
 * Synchronize Configurator database with managed objects.
 *
 * @param oid           object identifier of the object instance or subtree
 *                      or NULL if whole database should be synchronized
 * @param subtree       1 if the subtree of the specified node should
 *                      be synchronized
 *
 * @return status code (see te_errno.h)
 */
int
cfg_synchronize(const char *oid, te_bool subtree)
{
    cfg_sync_msg *msg;

    size_t  len;
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
        return ETEIO;
    }
    msg = (cfg_sync_msg *)cfgl_msg_buf;
    msg->type = CFG_SYNC;
    msg->subtree = subtree;

    len = strlen(oid) + 1;
    memcpy(msg->oid, oid, len);
    msg->len = sizeof(cfg_sync_msg) + len;
    len = CFG_MSG_MAX;

    ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                           CONFIGURATOR_SERVER,
                                           (char *)msg, msg->len,
                                           (char *)msg, &len);
    if (ret_val == 0)
    {
        ret_val = msg->rc;
    }

#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cfgl_lock);
#endif
    return TE_RC(TE_CONF_API, ret_val);
}


/**
 * Enumerate all instances of the object (enumeration is stopped if
 * callback returns non-zero).
 *
 * @param handle        object handle
 * @param callback      the function to be called for each object instance
 * @param user_data     opaque user data
 *
 * @return status code (see te_errno.h)
 */
int
cfg_enumerate(cfg_handle handle, cfg_inst_handler callback, void *user_data)
{
    int ret_val = 0;

    cfg_oid    *oid;
    char       *oid_subid;
    cfg_oid    *pattern;
    char       *subid;
    char       *name;
    int         length;

    const char *star = "*";
    const int   starlen = strlen(star) + 1;

    char       *pattern_name;

    int num;
    cfg_handle *instances;

    if (handle == CFG_HANDLE_INVALID)
    {
        return TE_RC(TE_CONF_API, EINVAL);
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
        return TE_RC(TE_CONF_API, ENOMEM);
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
        return TE_RC(TE_CONF_API, ENOMEM);
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


/**
 * Reboot the Test Agent.
 *
 * @param ta_name   name of the Test Agent
 * @param restore   if TRUE, restore the current configuration
 *
 * @return status code (see te_errno.h)
 */
int
cfg_reboot_ta(const char *ta_name, te_bool restore)
{
    cfg_reboot_msg *msg;

    size_t  len;
    int     ret_val = 0;

    if (ta_name == NULL)
    {
        return TE_RC(TE_CONF_API, EINVAL);
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
        return TE_RC(TE_CONF_API, ETEIO);
    }
    msg = (cfg_reboot_msg *)cfgl_msg_buf;
    msg->type = CFG_REBOOT;
    msg->restore = restore;

    len = strlen(ta_name) + 1;
    memcpy(msg->ta_name, ta_name, len);
    msg->len = sizeof(cfg_reboot_msg) + len;
    len = CFG_MSG_MAX;

    ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                           CONFIGURATOR_SERVER,
                                           (char *)msg, msg->len,
                                           (char *)msg, &len);
    if (ret_val == 0)
    {
        ret_val = msg->rc;
    }

#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cfgl_lock);
#endif
    return TE_RC(TE_CONF_API, ret_val);
}


/**
 * Create a backup.
 *
 * @param name      OUT: location for backup file name
 *
 * @return status code (see te_errno.h)
 */
int
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
        return TE_RC(TE_CONF_API, ETEIO);
    }
    msg = (cfg_backup_msg *)cfgl_msg_buf;
    msg->type = CFG_BACKUP;
    msg->op = CFG_BACKUP_CREATE;
    msg->len = sizeof(cfg_backup_msg);
    len = CFG_MSG_MAX;

    ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                           CONFIGURATOR_SERVER,
                                           (char *)msg, msg->len,
                                           (char *)msg, &len);
    if ((ret_val == 0) && ((ret_val = msg->rc) == 0))
    {
        len = strlen(msg->filename) + 1;
        *name = (char *)calloc(1, len);
        if (*name == NULL)
        {
            ret_val = ENOMEM;
        }
        else
        {
            memcpy((void *)(*name), (void *)(msg->filename), len);
        }
    }
#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cfgl_lock);
#endif
    return TE_RC(TE_CONF_API, ret_val);
}


/**
 * Verify the backup.
 *
 * @param name      name returned by cfg_create_backup
 *
 * @return status code (see te_errno.h)
 */
int
cfg_verify_backup(const char *name)
{
    cfg_backup_msg *msg;

    size_t  len;
    int     ret_val = 0;

    if (name == NULL)
    {
        return TE_RC(TE_CONF_API, EINVAL);
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
        return TE_RC(TE_CONF_API, ETEIO);
    }

    msg = (cfg_backup_msg *)cfgl_msg_buf;
    msg->type = CFG_BACKUP;
    msg->op = CFG_BACKUP_VERIFY;

    len = strlen(name) + 1;
    memcpy(msg->filename, name, len);
    msg->len = sizeof(cfg_sync_msg) + len;
    len = CFG_MSG_MAX;

    ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                           CONFIGURATOR_SERVER,
                                           (char *)msg, msg->len,
                                           (char *)msg, &len);
    if (ret_val == 0)
    {
        ret_val = msg->rc;
    }

#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cfgl_lock);
#endif
    return TE_RC(TE_CONF_API, ret_val);
}


/**
 * Restore the backup.
 *
 * @param name      name returned by cfg_create_backup
 *
 * @return status code (see te_errno.h)
 */
int
cfg_restore_backup(const char *name)
{
    cfg_backup_msg *msg;

    size_t  len;
    int     ret_val = 0;

    if (name == NULL)
    {
        return TE_RC(TE_CONF_API, EINVAL);
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
        return TE_RC(TE_CONF_API, ETEIO);
    }

    msg = (cfg_backup_msg *)cfgl_msg_buf;
    msg->type = CFG_BACKUP;
    msg->op = CFG_BACKUP_RESTORE;

    len = strlen(name) + 1;
    memcpy(msg->filename, name, len);
    msg->len = sizeof(cfg_sync_msg) + len;
    len = CFG_MSG_MAX;

    ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                           CONFIGURATOR_SERVER,
                                           (char *)msg, msg->len,
                                           (char *)msg, &len);
    if (ret_val == 0)
    {
        ret_val = msg->rc;
    }

#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cfgl_lock);
#endif
    return TE_RC(TE_CONF_API, ret_val);
}


/**
 * Create a configuration file.
 *
 * @param name      configuration file name
 * @param history   if TRUE "history" configuration file is created;
 *                  otherwise "backup" configuration file is created
 *
 * @return status code (see te_errno.h)
 */
int
cfg_create_config(const char *name, te_bool history)
{
    cfg_config_msg *msg;

    size_t  len;
    int     ret_val = 0;

    if (name == NULL)
    {
        return TE_RC(TE_CONF_API, EINVAL);
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
        return TE_RC(TE_CONF_API, ETEIO);
    }
    msg = (cfg_config_msg *)cfgl_msg_buf;
    msg->type = CFG_CONFIG;
    msg->history = history;

    len = strlen(name) + 1;
    memcpy(msg->filename, name, len);
    msg->len = sizeof(cfg_config_msg) + len;
    len = CFG_MSG_MAX;

    ret_val = ipc_send_message_with_answer(cfgl_ipc_client,
                                           CONFIGURATOR_SERVER,
                                           (char *)msg, msg->len,
                                           (char *)msg, &len);
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
