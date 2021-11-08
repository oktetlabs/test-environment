/** @file
 * @brief generic tool options TAPI
 *
 * TAPI to handle generic tool options (implementation).
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 * @author Igor Romanov <Igor.Romanov@oktetlabs.ru>
 */

#include "te_defs.h"
#include "te_string.h"
#include "logger_api.h"
#include "tapi_job_opt.h"
#include "te_sockaddr.h"

typedef struct tapi_job_opt_array_impl {
    const tapi_job_opt_array *array;
    const void *opt;
} tapi_job_opt_array_impl;

te_errno
tapi_job_opt_create_uint_t(const void *value, te_vec *args)
{
    tapi_job_opt_uint_t *p = (tapi_job_opt_uint_t *)value;

    if (!p->defined)
        return TE_ENOENT;

    return te_vec_append_str_fmt(args, "%u", p->value);
}

te_errno
tapi_job_opt_create_uint_t_hex(const void *value, te_vec *args)
{
    tapi_job_opt_uint_t *p = (tapi_job_opt_uint_t *)value;

    if (!p->defined)
        return TE_ENOENT;

    return te_vec_append_str_fmt(args, "0x%x", p->value);
}

te_errno
tapi_job_opt_create_uint(const void *value, te_vec *args)
{
    unsigned int uint = *(const unsigned int *)value;

    return te_vec_append_str_fmt(args, "%u", uint);
}

te_errno
tapi_job_opt_create_uint_omittable(const void *value, te_vec *args)
{
    if (*(const unsigned int *)value == TAPI_JOB_OPT_OMIT_UINT)
        return TE_ENOENT;

    return tapi_job_opt_create_uint(value, args);
}

te_errno
tapi_job_opt_create_double_t(const void *value, te_vec *args)
{
    tapi_job_opt_double_t *p = (tapi_job_opt_double_t *)value;

    if (!p->defined)
        return TE_ENOENT;

    return te_vec_append_str_fmt(args, "%f", p->value);
}

te_errno
tapi_job_opt_create_string(const void *value, te_vec *args)
{
    const char *str = *(const char *const *)value;

    if (str == NULL)
        return TE_ENOENT;

    return te_vec_append_str_fmt(args, "%s", str);
}

te_errno
tapi_job_opt_create_bool(const void *value, te_vec *args)
{
    UNUSED(args);

    return (*(const te_bool *)value) ? 0 : TE_ENOENT;
}

te_errno
tapi_job_opt_create_dummy(const void *value, te_vec *args)
{
    UNUSED(value);
    UNUSED(args);
    /*
     * Function is just a dummy stuff which is required to
     * handle options without arguments in tapi_job_opt_build_args().
     */
    return 0;
}

te_errno
tapi_job_opt_create_sockaddr_ptr(const void *value, te_vec *args)
{
    const struct sockaddr **sa = (const struct sockaddr **)value;

    if (sa == NULL || *sa == NULL)
        return TE_ENOENT;

    return te_vec_append_str_fmt(args, "%s", te_sockaddr_get_ipstr(*sa));
}

te_errno
tapi_job_opt_create_addr_port_ptr(const void *value, te_vec *args)
{
    const struct sockaddr **sa = (const struct sockaddr **)value;

    if (sa == NULL || *sa == NULL)
        return TE_ENOENT;

    return te_vec_append_str_fmt(args, "%s:%u",
                                 te_sockaddr_get_ipstr(*sa),
                                 (unsigned int)te_sockaddr_get_port(*sa));
}

/**
 * Append an argument @p arg that was processed by a formatting function
 * to arguments array @p args, with suffix/prefix if present.
 *
 * @param[in]  bind         Argument's prefix/suffix information.
 * @param[in]  arg          Argument vector to append, may be 0 size to append
 *                          only prefix/suffix. Prefix is concatenated with the
 *                          first element if the appropriate flag is set in
 *                          @p bind, suffix is always concatenated with the last
 *                          element.
 * @param[inout] args       Vector to put resulting argument to.
 */
static te_errno
tapi_job_opt_append_arg_with_affixes(const tapi_job_opt_bind *bind,
                                      te_vec *arg, te_vec *args)
{
    te_bool do_concat_prefix = bind->concatenate_prefix && bind->prefix != NULL;
    size_t size;
    te_errno rc;
    size_t i;

    if (!do_concat_prefix && bind->prefix != NULL)
    {
        rc = te_vec_append_str_fmt(args, "%s", bind->prefix);
        if (rc != 0)
            return rc;
    }

    size = te_vec_size(arg);
    for (i = 0; i < size; i++)
    {
        const char *pfx = (do_concat_prefix && i == 0 ? bind->prefix : "");
        const char *suff = ((i + 1 == size && bind->suffix != NULL) ?
                            bind->suffix : "");

        rc = te_vec_append_str_fmt(args, "%s%s%s",
                                   pfx, TE_VEC_GET(const char *, arg, i), suff);
        if (rc != 0)
            return rc;
    }

    return 0;
}

static te_errno
tapi_job_opt_bind2str(const tapi_job_opt_bind *bind, const void *opt,
                       te_vec *args)
{
    te_vec arg_vec = TE_VEC_INIT(char *);
    const uint8_t *ptr = (const uint8_t *)opt + bind->opt_offset;
    te_errno rc;

    if (bind->fmt_func == tapi_job_opt_create_array)
        rc = bind->fmt_func(&(tapi_job_opt_array_impl){(const tapi_job_opt_array *)ptr, opt}, &arg_vec);
    else
        rc = bind->fmt_func(ptr, &arg_vec);

    if (rc != 0)
    {
        te_vec_deep_free(&arg_vec);

        if (rc == TE_ENOENT)
            return 0;

        return rc;
    }

    rc = tapi_job_opt_append_arg_with_affixes(bind, &arg_vec, args);
    te_vec_deep_free(&arg_vec);
    if (rc != 0)
        return rc;

    return 0;
}

te_errno
tapi_job_opt_build_args(const char *path, const tapi_job_opt_bind *binds,
                         const void *opt, te_vec *tool_args)
{
    te_vec args = TE_VEC_INIT(char *);
    const char *end_arg = NULL;
    const tapi_job_opt_bind *bind;
    te_errno rc;

    rc = te_vec_append_str_fmt(&args, "%s", path);
    if (rc != 0)
        goto out;

    if (binds != NULL)
    {
        for (bind = binds; bind->fmt_func != NULL; bind++)
        {
            rc = tapi_job_opt_bind2str(bind, opt, &args);
            if (rc != 0)
                goto out;
        }
    }

    rc = TE_VEC_APPEND(&args, end_arg);

out:
    if (rc != 0)
        te_vec_deep_free(&args);

    *tool_args = args;

    return rc;
}

te_errno
tapi_job_opt_create_array(const void *value, te_vec *args)
{
    const tapi_job_opt_array_impl *impl = (const tapi_job_opt_array_impl *)value;
    const tapi_job_opt_array *array = impl->array;
    tapi_job_opt_bind bind = array->bind;
    te_errno rc;
    size_t i;

    for (i = 0; i < array->array_length; i++,
         bind.opt_offset += array->element_size)
    {
        rc = tapi_job_opt_bind2str(&bind, impl->opt, args);
        if (rc != 0)
            return rc;
    }

    return 0;
}
