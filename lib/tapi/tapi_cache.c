/** @file
 * @brief API to deal with cached data
 *
 * Copyright (C) 2004-2019 OKTET Labs. All rights reserved.
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 */

#define TE_LGR_USER     "TAPI cache"

#ifdef DEBUG
#define TE_LOG_LEVEL    0x003f
#endif /* DEBUG */

#include "tapi_cache.h"
#include "logger_api.h"
#include "te_string.h"
#include "te_tools.h"
#include "te_queue.h"


/* Item of areas list */
typedef struct tc_area {
    cfg_handle           area;      /* Object handle related to the area */
    tapi_cache_cb        func;      /* Callback function registered on area */
    SLIST_ENTRY(tc_area) next;
} tc_area;

/* Item of methods list */
typedef struct tc_method {
    char                  *method;  /* Actualization method */
    SLIST_HEAD(, tc_area)  areas;   /* List of registered areas */
    SLIST_ENTRY(tc_method) next;
} tc_method;

/* List of registered methods */
static SLIST_HEAD(, tc_method) methods = SLIST_HEAD_INITIALIZER();


/*
 * Get full path to cache area instance in Configurator
 *
 * @note The string is returned in a statically allocated buffer, which
 * subsequent calls will overwrite
 *
 * @param area_fmt      Area instance format string representation
 * @param ap            List of arguments corresponding to format string
 *
 * @return Full path to cache area, or @c NULL in case of string building
 *         failure
 */
static const char *
get_cache_area_instance(const char *area_fmt, va_list ap)
{
    static char buf[CFG_OID_MAX];
    te_string   oid = TE_STRING_BUF_INIT(buf);

    if (te_string_append(&oid, "%s/", TAPI_CACHE_ROOT_INST) != 0)
        return NULL;
    if (te_string_append_va(&oid, area_fmt, ap) != 0)
        return NULL;

    return oid.ptr;
}

#ifdef DEBUG
static void
dump_methods(void)
{
    tc_method *meth;
    tc_area   *ar;
    te_string  str = TE_STRING_INIT;

    SLIST_FOREACH(meth, &methods, next)
    {
        /* No need to handle te_string_append() errors in debugging routine */
        te_string_append(&str, "%s: { ", meth->method);
        SLIST_FOREACH(ar, &meth->areas, next)
        {
            te_string_append(&str, "(%u, %p),", ar->area, ar->func);
        }
        te_string_append(&str, " }\n");
    }
    INFO("List of registered methods:\n%s", str.ptr);
    te_string_free(&str);
}
#endif /* DEBUG */

/*
 * Get registered method by name. If @p method had not been registred, it will
 * be added
 *
 * @param method    Method name
 *
 * @return Method instance, or @c NULL in case of failure to register it
 */
static tc_method *
get_method(const char *method)
{
    tc_method *meth;

    assert(method != NULL);

    VERB("Look for method '%s' in the list", method);
    SLIST_FOREACH(meth, &methods, next)
    {
        if (te_str_is_null_or_empty(meth->method))
            WARN("There is a method with empty name in the list");
        else if (strcmp(meth->method, method) == 0)
            break;
    }

    if (meth == NULL)
    {
        VERB("Method '%s' has not been found, register it", method);
        meth = TE_ALLOC(sizeof(*meth));
        if (meth != NULL)
        {
            meth->method = strdup(method);
            if (meth->method != NULL)
            {
                SLIST_INIT(&meth->areas);
                SLIST_INSERT_HEAD(&methods, meth, next);
            }
            else
            {
                ERROR("%s:%d: Failed to allocate memory",
                      __FUNCTION__, __LINE__);
                free(meth);
                meth = NULL;
            }
        }
    }

    return meth;
}

/*
 * Get context of area of registered method by object handle
 *
 * @param method    Method name
 * @param object    Configurator object
 *
 * @return Area context, or @c NULL if it is not found
 */
static tc_area *
get_area_by_object(const char *method, cfg_handle object)
{
    tc_method *meth;
    tc_area   *ar = NULL;

    SLIST_FOREACH(meth, &methods, next)
    {
        if (strcmp(meth->method, method) != 0)
            continue;

        SLIST_FOREACH(ar, &meth->areas, next)
        {
            if (ar->area == object)
                break;
        }
        if (ar != NULL)
            break;
    }

    return ar;
}

/*
 * Calls get_area_by_object() inside
 */
static tc_area *
get_area_by_instance(const char *method, cfg_handle instance)
{
    cfg_handle object;
    te_errno   rc;

    rc = cfg_find_object_by_instance(instance, &object);
    if (rc != 0)
        return NULL;

    return get_area_by_object(method, object);
}

/*
 * Register callback function on certain area for particular method. If the area
 * exists, it updates its callback, in other words it rewrites it silently
 *
 * @param method    Method instance
 * @param area      Area
 * @param cb_func   User callback function
 *
 * @return Status code
 */
static te_errno
set_area(tc_method *method, const char *area, tapi_cache_cb cb_func)
{
    te_errno    rc;
    cfg_handle  handle;
    tc_area    *ar;

    assert(method != NULL);
    assert(area != NULL);
    assert(cb_func != NULL);

    rc = cfg_find_fmt(&handle, "%s/%s", TAPI_CACHE_ROOT_OID, area);
    if (rc != 0)
        return rc;

    VERB("Look for area '%s' in the list of method '%s'", area, method->method);
    SLIST_FOREACH(ar, &method->areas, next)
    {
        if (ar->area == handle)
            break;
    }

    if (ar == NULL)
    {
        VERB("Area '%s' has not been found, register it", area);
        ar = TE_ALLOC(sizeof(*ar));
        if (ar == NULL)
            return TE_RC(TE_TAPI, TE_ENOMEM);

        ar->area = handle;
        ar->func = cb_func;
        SLIST_INSERT_HEAD(&method->areas, ar, next);
    }
    else
    {
        VERB("Area had been registered before, its callback will be updated");
        ar->func = cb_func;
    }

    return rc;
}

#ifdef DEBUG
static void
dump_oid(cfg_handle handle, const char *name)
{
    char *oid;

    /* No need to handle cfg_get_oid_str() errors in debugging routine */
    cfg_get_oid_str(handle, &oid);
    VERB("%s (%u) is '%s'", name, handle, oid);
    free(oid);
}
#endif /* DEBUG */

/*
 * Recursively delete all particular leafs of descendants, and all descendants
 * if they do not have any leafs and registered callbacks
 *
 * @param method    Method, in other words the leaf instance name to be deleted
 * @param handle    Object instance handle
 *
 * @return Status code
 */
static te_errno
invalidate_descendants(const char *method, cfg_handle handle)
{
    te_errno    rc;
    cfg_handle  son;
    cfg_handle  brother;
    char       *name;

    assert(method != NULL);

    if (handle == CFG_HANDLE_INVALID)
        return TE_RC(TE_TAPI, TE_EINVAL);

#ifdef DEBUG
    dump_oid(handle, "current");
#endif /* DEBUG */

    /* Operate brothers */
    rc = cfg_get_brother(handle, &brother);
    if (rc != 0)
        return rc;

    if (brother != CFG_HANDLE_INVALID)
    {
#ifdef DEBUG
        dump_oid(brother, "brother");
#endif /* DEBUG */

        rc = invalidate_descendants(method, brother);
        if (rc != 0)
            return rc;
    }

    /* Operate sons */
    rc = cfg_get_son(handle, &son);
    if (rc != 0)
        return rc;

    if (son != CFG_HANDLE_INVALID)
    {
#ifdef DEBUG
        dump_oid(son, "son");
#endif /* DEBUG */

        rc = invalidate_descendants(method, son);
        if (rc != 0)
            return rc;

        /*
         * If we are here then we came back from recursion, and the current
         * node could be with no children if all leafs were deleted
         */
        rc = cfg_get_son(handle, &son);
        if (rc != 0)
            return rc;

        if (son == CFG_HANDLE_INVALID)
        {
            /* Do not remove an instance of area with registered callback */
            if (get_area_by_instance(method, handle) == NULL)
            {
                VERB("remove childless: %u", handle);
                rc = cfg_del_instance(handle, FALSE);
                if (rc != 0)
                    return rc;
            }
        }
        return rc;
    }

    /* This is a leaf */
    rc = cfg_get_inst_name(handle, &name);
    if (rc != 0)
        return rc;
    if (strcmp(method, name) == 0)
    {
        VERB("remove leaf: %u", handle);
        rc = cfg_del_instance(handle, FALSE);
    }

    return rc;
}

/*
 * Invalidate the area of particular method
 *
 * @param method    Method name, which area should be invalidated
 * @param instance  Object instance handle
 *
 * @return Status code
 */
static te_errno
invalidate_instance(const char *method, cfg_handle instance)
{
    cfg_handle son;
    te_errno   rc;

    rc = cfg_get_son(instance, &son);
    if (rc != 0)
        return rc;

    if (son != CFG_HANDLE_INVALID)
    {
        rc = invalidate_descendants(method, son);
        if (rc != 0)
            return rc;

        rc = cfg_get_son(instance, &son);
        if (rc != 0)
            return rc;
    }

    /* After invalidation the area could become childless */
    if (son == CFG_HANDLE_INVALID)
    {
        /* Do not remove an instance of area with registered callback */
        if (get_area_by_instance(method, instance) == NULL)
            rc = cfg_del_instance(instance, FALSE);
    }

    return rc;
}

/*
 * It is equivalent to the function tapi_cache_invalidate(), except that it is
 * called with full path to cache area instance
 */
static te_errno
invalidate_area(const char *method, const char *inst_oid)
{
    te_errno      rc;
    unsigned int  i;
    unsigned int  num;
    cfg_handle   *items;

    if (method == NULL)
        VERB("Invalidate '%s' of all methods", inst_oid);
    else
        VERB("Invalidate '%s' of method '%s'", inst_oid, method);

    rc = cfg_find_pattern(inst_oid, &num, &items);
    if (rc != 0)
        return rc;

    for (i = 0; i < num; i++)
    {
        if (method != NULL)
        {
            rc = invalidate_instance(method, items[i]);
        }
        else
        {
            tc_method *meth;

            SLIST_FOREACH(meth, &methods, next)
            {
                rc = invalidate_instance(meth->method, items[i]);
                if (rc != 0)
                    break;
            }
        }
        if (rc != 0)
            break;
    }
    free(items);

    return rc;
}

/*
 * Update actualization status
 *
 * @param status    Current actualization status
 * @param error     New actualization error
 *
 * @return Updated actualization status
 */
static inline te_errno
update_act_status(te_errno status, te_errno error)
{
    if (error == 0 && status == TE_ENOENT)
    {
        /* Detect patrial actulaization */
        status = TE_ECHILD;
    }
    else if (error != 0 && status != TE_ECHILD)
    {
        /* Do not lose a status of partial actualization */
        status = error;
    }

    return status;
}

/*
 * Recursively invoke top-level callbacks of all descendants
 *
 * @param method    Method name, which area should be actualized
 * @param opaque    Opaque argument to pass to the registered callback function
 * @param handle    Object instance handle
 *
 * @return Status code
 * @retval 0            Instance has been actualized completely
 * @retval TE_ECHILD    Some of subareas have not been actualized due to they
 *                      do not have actualization callbacks
 * @retval TE_ENOENT    Instance has not been actualized completely
 * @retval Any other    Another errors which should be handled as critical
 */
static te_errno
actualize_descendants(const char *method, void *opaque, cfg_handle handle)
{
    tc_area    *ar;
    te_errno    rc;
    te_errno    rc_act = 0;
    cfg_handle  son;
    cfg_handle  obj_handle;

#ifdef DEBUG
    char *oid;

    rc = cfg_get_oid_str(handle, &oid);
    if (rc != 0)
        return rc;
    ENTRY("The area '%s'(%u) does not have registerd callback, try to find it "
          "at its children", oid, handle);
#endif /* DEBUG */

    rc = cfg_get_son(handle, &son);
    if (rc != 0)
        return rc;

    if (son == CFG_HANDLE_INVALID)
    {
#ifdef DEBUG
        EXIT("Status '%s': %r", oid, TE_ENOENT);
        free(oid);
#endif /* DEBUG */

        return TE_ENOENT;
    }

    /* Try to invoke callbacks of all instances of the same level */
    handle = son;
    do {
        rc = cfg_find_object_by_instance(handle, &obj_handle);
        if (rc != 0)
            break;
        ar = get_area_by_object(method, obj_handle);
        if (ar == NULL)
        {
            rc = actualize_descendants(method, opaque, handle);
            if (rc != 0 && rc != TE_ENOENT && rc != TE_ECHILD)
                break;
            rc_act = update_act_status(rc_act, rc);
        }
        else
        {
            char *inst_oid;

            rc = cfg_get_oid_str(handle, &inst_oid);
            if (rc != 0)
                break;
            rc = invalidate_area(method, inst_oid);
            if (rc != 0)
            {
                free(inst_oid);
                break;
            }
            VERB("Actualize '%s'", inst_oid);
            rc = ar->func(inst_oid, opaque);
            free(inst_oid);
            if (rc != 0)
                break;
            rc_act = update_act_status(rc_act, rc);
        }

        /* Repeat for all instance's brothers */
        rc = cfg_get_brother(handle, &handle);
        if (rc != 0)
            break;
    } while (handle != CFG_HANDLE_INVALID);

    rc = (rc == 0 ? rc_act : rc);

#ifdef DEBUG
    EXIT("Status '%s': %r", oid, rc);
    free(oid);
#endif /* DEBUG */

    return rc;
}

/*
 * Actualize the instance of particular method
 *
 * @param method    Method name, which area should be actualized
 * @param opaque    Opaque argument to pass to the registered callback function
 * @param inst_oid  Object instance
 *
 * @return Status code
 * @retval 0            Instance has been actualized completely
 * @retval TE_ECHILD    Some of subareas have not been actualized due to they
 *                      do not have actualization callbacks
 * @retval TE_ENOENT    Instance has not been actualized completely
 * @retval Any other    Another errors which should be handled as critical
 */
static te_errno
actualize_instance(const char *method, void *opaque, const char *inst_oid)
{
    te_errno    rc;
    tc_area    *ar;
    char        obj_oid[CFG_OID_MAX];
    cfg_handle  handle;

    /*
     * cfg_oid_inst2obj() is the only function which allows to get object OID
     * from inexistent instance OID.
     * It is important to handle the case when a user's callback function
     * creates required instances itself, i.e. does not expect they exist
     */
    cfg_oid_inst2obj(inst_oid, obj_oid);
    INFO("Instance oid: '%s'\nObject oid: '%s'", inst_oid, obj_oid);

    rc = cfg_find_str(obj_oid, &handle);
    if (rc != 0)
        return rc;

    VERB("Look for registered method '%s' of area '%s'", method, obj_oid);
    ar = get_area_by_object(method, handle);
    if (ar != NULL)
    {
        rc = invalidate_area(method, inst_oid);
        if (rc != 0)
            return rc;

        rc = ar->func(inst_oid, opaque);
    }
    else
    {
        rc = cfg_find_str(inst_oid, &handle);
        if (rc != 0)
            return rc;
        rc = actualize_descendants(method, opaque, handle);
        if (rc == TE_ENOENT)
        {
            WARN("The area of instance '%s' does not have registered callback "
                 "of method '%s'", inst_oid, method);
        }
        else if (rc == TE_ECHILD)
        {
            WARN("Some of child areas of instance '%s' do not have registered "
                 "callbacks of method '%s'", inst_oid, method);
        }
    }

    return rc;
}


/* See description in tapi_cache.h */
te_errno
tapi_cache_register(const char *method, const char *area, tapi_cache_cb cb_func)
{
    tc_method *meth;
    te_errno   rc;

    meth = get_method(method);
    if (meth != NULL)
        rc = set_area(meth, area, cb_func);
    else
        rc = TE_RC(TE_TAPI, TE_ENOENT);

#ifdef DEBUG
    dump_methods();
#endif /* DEBUG */

    return rc;
}

/* See description in tapi_cache.h */
te_errno
tapi_cache_actualize(const char *method, void *opaque,
                     const char *area_ptrn, ...)
{
    te_errno      rc;
    te_errno      rc_act = 0;
    const char   *inst_oid;
    va_list       ap;
    unsigned int  i;
    unsigned int  num;
    cfg_handle   *items;

    assert(method != NULL);

    va_start(ap, area_ptrn);
    inst_oid = get_cache_area_instance(area_ptrn, ap);
    va_end(ap);

    rc = cfg_find_pattern(inst_oid, &num, &items);
    if (rc != 0)
        return rc;

    if (num == 0)
    {
        /*
         * User requested to actualize the instance which does not exist.
         * Assume he intends to create it with callback
         */
        VERB("Actualize inexistent '%s'", inst_oid);
        rc_act = actualize_instance(method, opaque, inst_oid);
    }
    else for (i = 0; i < num; i++)
    {
        char *oid;

        /* Actualize all instances match the pattern */
        rc = cfg_get_oid_str(items[i], &oid);
        if (rc != 0)
            break;
        VERB("Actualize '%s'", oid);
        rc = actualize_instance(method, opaque, oid);
        free(oid);
        if (rc != 0 && rc != TE_ENOENT && rc != TE_ECHILD)
            break;
        rc_act = update_act_status(rc_act, rc);
        /* Reset rc to avoid returning it instead of rc_act */
        rc = 0;
    }
    free(items);

    return TE_RC(TE_TAPI, (rc == 0 ? rc_act : rc));
}

/* See description in tapi_cache.h */
te_errno
tapi_cache_invalidate(const char *method, const char *area_ptrn, ...)
{
    va_list     ap;
    const char *inst_oid;

    va_start(ap, area_ptrn);
    inst_oid = get_cache_area_instance(area_ptrn, ap);
    va_end(ap);

    return invalidate_area(method, inst_oid);
}

/* Get a number of identifier elements of cache root instance */
static uint8_t
tapi_cache_root_instance_oid_len(void)
{
    static uint8_t len;
    cfg_oid *oid;

    if (len != 0)
        return len;     /* It is enough to calculate static only once */

    oid = cfg_convert_oid_str(TAPI_CACHE_ROOT_INST);
    if (oid != NULL)
        len = oid->len;

    free(oid);

    return len;
}

/*
 * Recursively create cache parent instances as needed
 *
 * @param oid       Instance OID in structure representation
 *
 * @return Status code
 */
static te_errno
create_parents(cfg_oid *oid)
{
    te_errno    rc;
    cfg_handle  handle;
    char       *oid_str;

    if (oid->len <= tapi_cache_root_instance_oid_len())
    {
        VERB("%s(): It's a cache root instance, stop recursion", __FUNCTION__);
        return 0;
    }

    oid_str = cfg_convert_oid(oid);
    if (oid_str == NULL)
        return TE_ENOMEM;

    rc = cfg_find_str(oid_str, &handle);
    VERB("%s(): oid='%s', rc=%r", __FUNCTION__, oid_str, rc);
    if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
    {
        oid->len--;
        rc = create_parents(oid);
        if (rc == 0)
            rc = cfg_add_instance_str(oid_str, NULL, CVT_NONE, NULL);
    }

    free(oid_str);
    return rc;
}

/*
 * It is equivalent to the function tapi_cache_add(), except that it is called
 * with a va_list instead of a variable number of arguments. This function does
 * not call the va_end macro
 */
static te_errno
tapi_cache_add_va(cfg_val_type type, const void *value,
                  const char *area_inst, va_list ap)
{
    te_errno    rc;
    const char *oid_str;
    cfg_oid    *oid;

    oid_str = get_cache_area_instance(area_inst, ap);

    oid = cfg_convert_oid_str(oid_str);
    oid->len--;    /* We are interested only in parents */
    rc = create_parents(oid);
    free(oid);
    if (rc != 0)
        return TE_RC(TE_TAPI, rc);

    return cfg_add_instance_str(oid_str, NULL, type, value);
}

/* See description in tapi_cache.h */
te_errno
tapi_cache_add(cfg_val_type type, const void *value, const char *area_inst, ...)
{
    te_errno rc;
    va_list  ap;

    va_start(ap, area_inst);
    rc = tapi_cache_add_va(type, value, area_inst, ap);
    va_end(ap);

    return rc;
}

/* See description in tapi_cache.h */
te_errno
tapi_cache_add_string(const char *value, const char *area_inst, ...)
{
    cfg_val_type type = CVT_STRING;
    te_errno rc;
    va_list  ap;

    va_start(ap, area_inst);
    rc = tapi_cache_add_va(type, value, area_inst, ap);
    va_end(ap);

    return rc;
}

/* See description in tapi_cache.h */
te_errno
tapi_cache_add_int(int value, const char *area_inst, ...)
{
    cfg_val_type type = CVT_INTEGER;
    te_errno rc;
    va_list  ap;

    va_start(ap, area_inst);
    rc = tapi_cache_add_va(type, (const void *)(long)value, area_inst, ap);
    va_end(ap);

    return rc;
}

/* See description in tapi_cache.h */
te_errno
tapi_cache_add_addr(const struct sockaddr *addr, const char *area_inst, ...)
{
    cfg_val_type type = CVT_ADDRESS;
    te_errno rc;
    va_list  ap;

    va_start(ap, area_inst);
    rc = tapi_cache_add_va(type, addr, area_inst, ap);
    va_end(ap);

    return rc;
}

/* See description in tapi_cache.h */
te_errno
tapi_cache_del(const char *area_ptrn, ...)
{
    const char   *pattern;
    va_list       ap;
    unsigned int  i;
    unsigned int  num;
    cfg_handle   *items;
    cfg_handle    root_handle;
    cfg_handle    handle;
    te_errno      rc;

    assert(area_ptrn != NULL);

    va_start(ap, area_ptrn);
    pattern = get_cache_area_instance(area_ptrn, ap);
    va_end(ap);

    rc = cfg_find_str(TAPI_CACHE_ROOT_INST, &root_handle);
    if (rc != 0)
        return rc;

    rc = cfg_find_pattern(pattern, &num, &items);
    if (rc != 0)
        return rc;

    for (i = 0; i < num; i++)
    {
        if (items[i] != root_handle)
        {
            rc = cfg_del_instance(items[i], TRUE);
        }
        else
        {
            /* Remove all cache root children */
            rc = cfg_get_son(items[i], &handle);
            if (rc != 0)
                return rc;

            while (handle != CFG_HANDLE_INVALID)
            {
                cfg_handle brother;

                /* Save brother handle before removing current's one */
                rc = cfg_get_brother(handle, &brother);
                if (rc != 0)
                    break;

                rc = cfg_del_instance(handle, TRUE);
                if (rc != 0)
                    break;

                /* Repeat for all instance's brothers */
                handle = brother;
            }
        }
        if (rc != 0)
            break;
    }

    return rc;
}

/* See description in tapi_cache.h */
te_errno
tapi_cache_find(cfg_handle_cb_func cb_func, void *opaque,
                const char *area_ptrn, ...)
{
    const char *pattern;
    va_list     ap;

    va_start(ap, area_ptrn);
    pattern = get_cache_area_instance(area_ptrn, ap);
    va_end(ap);

    return cfg_find_pattern_iter_fmt(cb_func, opaque, "%s", pattern);
}


/*
 * It is equivalent to the function tapi_cache_get(), except that it is called
 * with a va_list instead of a variable number of arguments. This function does
 * not call the va_end macro
 */
static te_errno
tapi_cache_get_va(cfg_val_type *type, void *value,
                  const char *area_inst, va_list ap)
{
    const char *oid;

    oid = get_cache_area_instance(area_inst, ap);

    return cfg_get_instance_str(type, value, oid);
}

/* See description in tapi_cache.h */
te_errno
tapi_cache_get(cfg_val_type *type, void *value, const char *area_inst, ...)
{
    te_errno rc;
    va_list  ap;

    va_start(ap, area_inst);
    rc = tapi_cache_get_va(type, value, area_inst, ap);
    va_end(ap);

    return rc;
}

/* See description in tapi_cache.h */
te_errno
tapi_cache_get_string(char **value, const char *area_inst, ...)
{
    cfg_val_type type = CVT_STRING;
    te_errno rc;
    va_list  ap;

    va_start(ap, area_inst);
    rc = tapi_cache_get_va(&type, value, area_inst, ap);
    va_end(ap);

    return rc;
}

/* See description in tapi_cache.h */
te_errno
tapi_cache_get_int(int *value, const char *area_inst, ...)
{
    cfg_val_type type = CVT_INTEGER;
    te_errno rc;
    va_list  ap;

    va_start(ap, area_inst);
    rc = tapi_cache_get_va(&type, value, area_inst, ap);
    va_end(ap);

    return rc;
}

/* See description in tapi_cache.h */
te_errno
tapi_cache_get_addr(struct sockaddr **addr, const char *area_inst, ...)
{
    cfg_val_type type = CVT_ADDRESS;
    te_errno rc;
    va_list  ap;

    va_start(ap, area_inst);
    rc = tapi_cache_get_va(&type, addr, area_inst, ap);
    va_end(ap);

    return rc;
}
