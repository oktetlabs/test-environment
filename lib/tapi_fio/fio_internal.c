/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Auxiliary functions to fio TAPI
 *
 * Auxiliary functions for internal use in fio TAPI
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER "TAPI FIO"

#include <math.h>
#include <stddef.h>
#include <signal.h>
#include "te_string.h"
#include "tapi_fio.h"
#include "tapi_job_opt.h"
#include "te_vector.h"
#include "te_enum.h"

static inline int16_t
get_default_timeout(const tapi_fio_opts *opts)
{
    const int16_t error = 30;
    const int16_t five_minutes = 5 * 60;
    const double numjobs_coef = ((double)opts->numjobs.value) /
                                TAPI_FIO_MAX_NUMJOBS;

    return opts->runtime_sec + round(five_minutes * numjobs_coef) + error;
}

static te_errno
runtime_argument(const void *value, const void *priv, te_vec *args)
{
    int seconds = *(const unsigned int *)value;
    te_errno rc;

    UNUSED(priv);

    if (seconds < 0)
        return TE_ENOENT;

    rc = te_vec_append_str_fmt(args, "%ds", seconds);
    if (rc == 0)
        rc = te_vec_append_str_fmt(args, "--time_based");

    return rc;
}

static te_errno
user_argument(const void *value, const void *priv, te_vec *args)
{
    const char *str = *(const char **)value;
    char *saveptr;
    te_errno rc;
    char *arg;
    char *dup;

    UNUSED(priv);

    if (str == NULL)
        return TE_ENOENT;

    dup = strdup(str);
    if (dup == NULL)
        return TE_ENOMEM;

    for (arg = strtok_r(dup, " ", &saveptr);
         arg != NULL;
         arg = strtok_r(NULL, " ", &saveptr))
    {
        rc = te_vec_append_str_fmt(args, "%s", arg);
        if (rc != 0)
        {
            free(dup);
            return rc;
        }
    }

    free(dup);
    return 0;
}

static te_errno
rand_generator_argument(const void *value, const void *priv, te_vec *args)
{
    static const char *generators[] = {
        "lfsr",
        "tausworthe",
        "tausworthe64",
    };
    const char *str = *(const char **)value;
    size_t i;

    UNUSED(priv);

    if (str == NULL)
        return TE_ENOENT;

    for (i = 0; i < TE_ARRAY_LEN(generators); i++)
    {
        if (strcmp(str, generators[i]) == 0)
            return te_vec_append_str_fmt(args, "%s", str);
    }

    ERROR("Random generator '%s' is not supported", str);
    return TE_EINVAL;
}

/** Mapping of possible values for fio::ioengine option. */
static const te_enum_map tapi_fio_ioengine_mapping[] = {
    {.name = "sync",        .value = TAPI_FIO_IOENGINE_SYNC},
    {.name = "psync",       .value = TAPI_FIO_IOENGINE_PSYNC},
    {.name = "vsync",       .value = TAPI_FIO_IOENGINE_VSYNC},
    {.name = "pvsync",      .value = TAPI_FIO_IOENGINE_PVSYNC},
    {.name = "pvsync2",     .value = TAPI_FIO_IOENGINE_PVSYNC2},
    {.name = "libaio",      .value = TAPI_FIO_IOENGINE_LIBAIO},
    {.name = "posixaio",    .value = TAPI_FIO_IOENGINE_POSIXAIO},
    {.name = "rbd",         .value = TAPI_FIO_IOENGINE_RBD},
    TE_ENUM_MAP_END
};

/** Mapping of possible values for fio::rwtype option. */
static const te_enum_map tapi_fio_rwtype_mapping[] = {
    {.name = "randrw",      .value = TAPI_FIO_RWTYPE_RAND},
    {.name = "rw",          .value = TAPI_FIO_RWTYPE_SEQ},
    {.name = "read",        .value = TAPI_FIO_RWTYPE_READ},
    {.name = "write",       .value = TAPI_FIO_RWTYPE_WRITE},
    {.name = "trim",        .value = TAPI_FIO_RWTYPE_TRIM},
    {.name = "randread",    .value = TAPI_FIO_RWTYPE_RANDREAD},
    {.name = "randwrite",   .value = TAPI_FIO_RWTYPE_RANDWRITE},
    {.name = "randtrim",    .value = TAPI_FIO_RWTYPE_RANDTRIM},
    {.name = "trimwrite",   .value = TAPI_FIO_RWTYPE_TRIMWRITE},
    TE_ENUM_MAP_END
};

static const tapi_job_opt_bind fio_binds[] = TAPI_JOB_OPT_SET(
    TAPI_JOB_OPT_STRING("--name=", true, tapi_fio_opts, name),
    TAPI_JOB_OPT_STRING("--filename=", true, tapi_fio_opts, filename),
    TAPI_JOB_OPT_UINT("--blocksize=", true, NULL, tapi_fio_opts, blocksize),
    TAPI_JOB_OPT_UINT("--iodepth=", true, NULL, tapi_fio_opts, iodepth),
    {
        runtime_argument, "--runtime=", true, NULL,
        TAPI_JOB_OPT_OFFSETOF_CHK_SIZE(tapi_fio_opts, runtime_sec, int), NULL
    },
    TAPI_JOB_OPT_UINT("--rwmixread=", true, NULL, tapi_fio_opts, rwmixread),
    TAPI_JOB_OPT_DUMMY("--output-format=json"),
    TAPI_JOB_OPT_DUMMY("--group_reporting"),
    TAPI_JOB_OPT_STRING("--output=", true, tapi_fio_opts, output_path.ptr),
    TAPI_JOB_OPT_BOOL("--direct=1", tapi_fio_opts, direct),
    TAPI_JOB_OPT_BOOL("--scramble_buffers=1", tapi_fio_opts, scramble_buffers),
    TAPI_JOB_OPT_BOOL("--exitall_on_error=1", tapi_fio_opts, exit_on_error),
    {
        rand_generator_argument, "--random_generator=", true, NULL,
        TAPI_JOB_OPT_OFFSETOF_CHK_SIZE(tapi_fio_opts, rand_gen, char *), NULL
    },
    TAPI_JOB_OPT_ENUM("--readwrite=", true, tapi_fio_opts, rwtype,
                      tapi_fio_rwtype_mapping),
    TAPI_JOB_OPT_ENUM("--ioengine=", true, tapi_fio_opts, ioengine,
                      tapi_fio_ioengine_mapping),
    TAPI_JOB_OPT_UINT("--numjobs=", true, NULL, tapi_fio_opts, numjobs.value),
    TAPI_JOB_OPT_DUMMY("--thread"),
    {
        user_argument, NULL, false, NULL,
        TAPI_JOB_OPT_OFFSETOF_CHK_SIZE(tapi_fio_opts, user, char *), NULL
    },
    TAPI_JOB_OPT_STRING("--rbdname=", true, tapi_fio_opts, rbdname),
    TAPI_JOB_OPT_STRING("--pool=", true, tapi_fio_opts, pool),
    TAPI_JOB_OPT_UINTMAX_T("--size=", true, NULL, tapi_fio_opts, size)
);

/* See description in tapi_internal.h */
te_errno
fio_app_start(tapi_fio_app *app)
{
    te_errno rc;

    if (app->running)
        return TE_EALREADY;

    tapi_job_destroy(app->job, -1);
    app->job = NULL;
    app->out_chs[0] = NULL;
    app->out_chs[1] = NULL;
    te_vec_deep_free(&app->args);

    rc = tapi_job_opt_build_args(app->path.ptr, fio_binds, &app->opts,
                                 &app->args);
    if (rc != 0)
        return rc;

    rc = tapi_job_simple_create(app->factory,
                          &(tapi_job_simple_desc_t){
                                .program = app->path.ptr,
                                .argv = (const char **)app->args.data.ptr,
                                .job_loc = &app->job,
                                .stdout_loc = &app->out_chs[0],
                                .stderr_loc = &app->out_chs[1],
                                .filters = TAPI_JOB_SIMPLE_FILTERS(
                                    {.use_stderr = true,
                                     .log_level = TE_LL_ERROR,
                                     .readable = true,
                                     .filter_name = "fio_stderr",
                                    },
                                    {.use_stdout = true,
                                     .log_level = TE_LL_RING,
                                     .readable = false,
                                     .filter_name = "fio_stdout",
                                    }
                                 )
                          });

    if (rc != 0)
    {
        te_vec_deep_free(&app->args);
        return rc;
    }

    rc = tapi_job_start(app->job);
    if (rc != 0)
    {
        te_vec_deep_free(&app->args);
        tapi_job_destroy(app->job, -1);
        app->job = NULL;
        return rc;
    }

    app->running = true;

    return 0;
}

/* See description in tapi_internal.h */
te_errno
fio_app_stop(tapi_fio_app *app)
{
    te_errno rc;

    if (!app->running)
        return 0;

    rc = tapi_job_stop(app->job, SIGTERM, -1);
    if (rc != 0)
        return rc;

    app->running = false;

    return 0;
}

/* See description in tapi_internal.h */
te_errno
fio_app_wait(tapi_fio_app *app, int16_t timeout_sec, tapi_job_status_t *status)
{
    te_errno rc;

    if (timeout_sec == TAPI_FIO_TIMEOUT_DEFAULT)
        timeout_sec = get_default_timeout(&app->opts);

    rc = tapi_job_wait(app->job, TE_SEC2MS(timeout_sec), status);
    if (rc != 0)
        return rc;

    app->running = false;

    return 0;
}
