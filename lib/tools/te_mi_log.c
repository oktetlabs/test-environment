/** @file
 * @brief Machine interface data logging
 *
 * Implementation of API for machine interface data logging.
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 * @author Igor Romanov <Igor.Romanov@oktetlabs.ru>
 */

#define TE_LGR_USER     "TE MI LOG"

#include "te_config.h"

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif

#include "te_alloc.h"
#include "te_queue.h"
#include "te_kvpair.h"
#include "te_mi_log.h"

#define TE_MI_LOG_VERSION 1

#define TE_MI_STR_FMT "\"%s\""

typedef struct te_mi_meas_value {
    TAILQ_ENTRY(te_mi_meas_value) next;
    te_mi_meas_aggr aggr;
    double val;
    te_mi_meas_multiplier multiplier;
} te_mi_meas_value;

typedef TAILQ_HEAD(te_mi_meas_value_h, te_mi_meas_value) te_mi_meas_value_h;

typedef struct te_mi_meas_impl {
    TAILQ_ENTRY(te_mi_meas_impl) next;
    te_mi_meas_type type;
    char *name;
    te_mi_meas_value_h values;
} te_mi_meas_impl;

typedef TAILQ_HEAD(te_mi_meas_impl_h, te_mi_meas_impl) te_mi_meas_impl_h;

struct te_mi_logger {
    char *tool;
    te_mi_type type;
    unsigned int version;
    te_mi_meas_impl_h meas_q;
    te_kvpair_h meas_keys;
    te_kvpair_h comments;
    te_bool error_ignored;
};

typedef enum te_mi_meas_base_unit_type {
    TE_MI_MEAS_BASE_UNITLESS, /**< Unitless value represented as a ratio */
    TE_MI_MEAS_BASE_UNIT_PPS, /**< Packets per second */
    TE_MI_MEAS_BASE_UNIT_SECOND, /**< Seconds */
    TE_MI_MEAS_BASE_UNIT_BPS, /**< Bits per second */
    TE_MI_MEAS_BASE_UNIT_CELSIUS, /**< Degrees of celsius */
    TE_MI_MEAS_BASE_UNIT_RPS, /**< Requests per second */
} te_mi_meas_base_unit_type;

static te_mi_meas_base_unit_type meas_base_unit_by_type_map[] = {
    [TE_MI_MEAS_PPS] = TE_MI_MEAS_BASE_UNIT_PPS,
    [TE_MI_MEAS_LATENCY] = TE_MI_MEAS_BASE_UNIT_SECOND,
    [TE_MI_MEAS_THROUGHPUT] = TE_MI_MEAS_BASE_UNIT_BPS,
    [TE_MI_MEAS_BANDWIDTH_USAGE] = TE_MI_MEAS_BASE_UNITLESS,
    [TE_MI_MEAS_TEMP] = TE_MI_MEAS_BASE_UNIT_CELSIUS,
    [TE_MI_MEAS_RPS] = TE_MI_MEAS_BASE_UNIT_RPS,
};

static const char *meas_base_unit_names[] = {
    [TE_MI_MEAS_BASE_UNITLESS] = "",
    [TE_MI_MEAS_BASE_UNIT_PPS] = "pps",
    [TE_MI_MEAS_BASE_UNIT_SECOND] = "second",
    [TE_MI_MEAS_BASE_UNIT_BPS] =  "bps",
    [TE_MI_MEAS_BASE_UNIT_CELSIUS] = "degrees celsius",
    [TE_MI_MEAS_BASE_UNIT_RPS] = "rps",
};

static const char *mi_type_names[] = {
   [TE_MI_TYPE_MEASUREMENT] = "measurement",
};

static const char *meas_aggr_names[] = {
    [TE_MI_MEAS_AGGR_SINGLE] = "single",
    [TE_MI_MEAS_AGGR_MIN] = "min",
    [TE_MI_MEAS_AGGR_MAX] = "max",
    [TE_MI_MEAS_AGGR_MEAN] = "mean",
    [TE_MI_MEAS_AGGR_MEDIAN] = "median",
    [TE_MI_MEAS_AGGR_CV] = "cv",
    [TE_MI_MEAS_AGGR_STDEV] = "stdev",
};

static const char *meas_type_names[] = {
    [TE_MI_MEAS_PPS] = "pps",
    [TE_MI_MEAS_LATENCY] = "latency",
    [TE_MI_MEAS_THROUGHPUT] = "throughput",
    [TE_MI_MEAS_BANDWIDTH_USAGE] = "bandwidth-usage",
    [TE_MI_MEAS_TEMP] = "temperature",
    [TE_MI_MEAS_RPS] = "rps",
};

static const char *meas_multiplier_names[] = {
    [TE_MI_MEAS_MULTIPLIER_NANO] = "1e-9",
    [TE_MI_MEAS_MULTIPLIER_MICRO] = "1e-6",
    [TE_MI_MEAS_MULTIPLIER_MILLI] = "1e-3",
    [TE_MI_MEAS_MULTIPLIER_PLAIN] = "1",
    [TE_MI_MEAS_MULTIPLIER_KILO] = "1e+3",
    [TE_MI_MEAS_MULTIPLIER_KIBI] = "0x1p10",
    [TE_MI_MEAS_MULTIPLIER_MEGA] = "1e+6",
    [TE_MI_MEAS_MULTIPLIER_MEBI] = "0x1p20",
    [TE_MI_MEAS_MULTIPLIER_GIGA] = "1e+9",
    [TE_MI_MEAS_MULTIPLIER_GIBI] = "0x1p30",
};

static const char *
te_mi_meas_get_base_unit_str(te_mi_meas_type type, te_mi_meas_aggr aggr)
{
    te_mi_meas_base_unit_type base;

    if (aggr == TE_MI_MEAS_AGGR_CV)
        base = TE_MI_MEAS_BASE_UNITLESS;
    else
        base = meas_base_unit_by_type_map[type];

    return meas_base_unit_names[base];
}

static te_bool
te_mi_meas_aggr_valid(te_mi_meas_aggr aggr)
{
    return aggr >= 0 && aggr < TE_MI_MEAS_AGGR_END;
}

static te_bool
te_mi_meas_type_valid(te_mi_meas_type type)
{
    return type >= 0 && type < TE_MI_MEAS_END;
}

static te_bool
te_mi_meas_multiplier_valid(te_mi_meas_multiplier multiplier)
{
    return multiplier >= 0 && multiplier < TE_MI_MEAS_MULTIPLIER_END;
}

static const char *
te_mi_type2str(te_mi_type type)
{
    return mi_type_names[type];
}

static const char *
te_mi_meas_aggr2str(te_mi_meas_aggr aggr)
{
    return meas_aggr_names[aggr];
}

static const char *
te_mi_meas_type2str(te_mi_meas_type meas_type)
{
    return meas_type_names[meas_type];
}

static const char *
te_mi_meas_multiplier2str(te_mi_meas_multiplier multiplier)
{
    return meas_multiplier_names[multiplier];
}

static void
te_mi_set_logger_error(te_mi_logger *logger, te_errno *retval, te_errno val)
{
    if (retval != NULL)
        *retval = val;

    if (val != 0 && logger != NULL && retval == NULL)
        logger->error_ignored = TRUE;
}

static te_errno
te_mi_meas_value_str_append(const te_mi_meas_value *value, te_mi_meas_type type,
                            te_string *str)
{
    te_errno rc;

    rc = te_string_append(str,
            "{\"aggr\":"TE_MI_STR_FMT",\"value\":%.3f,",
            te_mi_meas_aggr2str(value->aggr), value->val);

    if (rc == 0)
    {
        rc = te_string_append(str,
            "\"base_units\":"TE_MI_STR_FMT",\"multiplier\":"TE_MI_STR_FMT"},",
            te_mi_meas_get_base_unit_str(type, value->aggr),
            te_mi_meas_multiplier2str(value->multiplier));
    }

    if (rc != 0)
        ERROR("Failed to append measurement value");

    return rc;
}

static te_errno
te_mi_meas_values_str_append(const te_mi_meas_value_h *values,
                             te_mi_meas_type type, te_string *str)
{
    const te_mi_meas_value *value;
    te_errno rc;

    rc = te_string_append(str, "\"entries\":[");

    TAILQ_FOREACH(value, values, next)
    {
        if (rc == 0)
            rc = te_mi_meas_value_str_append(value, type, str);
    }

    if (!TAILQ_EMPTY(values))
        te_string_cut(str, 1);

    if (rc == 0)
        rc = te_string_append(str, "]");

    if (rc != 0)
        ERROR("Failed to append measurement values");

    return rc;
}

static te_errno
te_mi_meas_str_append(const te_mi_meas_impl *meas, te_string *str)
{
    te_errno rc;

    rc = te_string_append(str, "{\"type\":"TE_MI_STR_FMT",",
                          te_mi_meas_type2str(meas->type));
    if (rc == 0 && meas->name != NULL)
        rc = te_string_append(str, "\"name\":"TE_MI_STR_FMT",", meas->name);

    if (rc == 0)
        rc = te_mi_meas_values_str_append(&meas->values, meas->type, str);

    if (rc == 0)
        rc = te_string_append(str, "},");

    if (rc != 0)
        ERROR("Failed to append a measurement");

    return rc;
}

static te_errno
te_mi_meas_q_str_append(const te_mi_meas_impl_h *meas_q, te_string *str)
{
    const te_mi_meas_impl *meas;
    te_errno rc;

    rc = te_string_append(str, "\"results\":[");

    TAILQ_FOREACH(meas, meas_q, next)
    {
        if (rc == 0)
            rc = te_mi_meas_str_append(meas, str);
    }
    if (!TAILQ_EMPTY(meas_q))
        te_string_cut(str, 1);

    if (rc == 0)
        rc = te_string_append(str, "],");

    if (rc != 0)
        ERROR("Failed to append measurement array");

    return rc;
}

static te_errno
te_mi_kvpairs_str_append(const te_kvpair_h *pairs, const char *dict_name,
                         te_string *str)
{
    const te_kvpair *pair;
    te_errno rc;

    if (TAILQ_EMPTY(pairs))
        return 0;

    rc = te_string_append(str, TE_MI_STR_FMT":{", dict_name);

    TAILQ_FOREACH(pair, pairs, links)
    {
        if (rc == 0)
            rc = te_string_append(str, TE_MI_STR_FMT":"TE_MI_STR_FMT",",
                                  pair->key, pair->value);
    }

    te_string_cut(str, 1);

    if (rc == 0)
        rc = te_string_append(str, "},");

    if (rc != 0)
        ERROR("Failed to append key-value pairs '%s'", dict_name);

    return rc;
}

static char *
te_mi_logger_data2str(const te_mi_logger *logger)
{
    te_string str = TE_STRING_INIT;
    te_errno rc;

    rc = te_string_append(&str, "{\"type\":"TE_MI_STR_FMT",",
                          te_mi_type2str(logger->type));

    if (rc == 0)
        rc = te_string_append(&str, "\"version\":%u,", logger->version);

    if (rc == 0)
        rc = te_string_append(&str, "\"tool\":"TE_MI_STR_FMT",", logger->tool);

    if (rc == 0)
        rc = te_mi_meas_q_str_append(&logger->meas_q, &str);

    if (rc == 0)
        rc = te_mi_kvpairs_str_append(&logger->meas_keys, "keys", &str);

    if (rc == 0)
        rc = te_mi_kvpairs_str_append(&logger->comments, "comments", &str);

    if (rc == 0)
    {
        te_string_cut(&str, 1);
        rc = te_string_append(&str, "}");
    }

    if (rc != 0)
    {
        ERROR("Failed to convert logger data to string");
        te_string_free(&str);
        return NULL;
    }

    return str.ptr;
}

static te_mi_meas_value *
te_mi_meas_value_add(te_mi_meas_value_h *values, te_mi_meas_aggr aggr,
                     double val, te_mi_meas_multiplier multiplier)
{
    te_mi_meas_value *result;

    result = TE_ALLOC(sizeof(*result));
    if (result == NULL)
        return NULL;

    result->aggr = aggr;
    result->val = val;
    result->multiplier = multiplier;
    TAILQ_INSERT_TAIL(values, result, next);

    return result;
}

static te_mi_meas_impl *
te_mi_meas_impl_find(te_mi_meas_impl_h *meas_q, te_mi_meas_type type,
                     const char *name)
{
    te_mi_meas_impl *result;

    TAILQ_FOREACH(result, meas_q, next)
    {
        if (result->type == type && strcmp_null(result->name, name) == 0)
            return result;
    }

    return NULL;
}

static te_mi_meas_impl *
te_mi_meas_impl_add(te_mi_meas_impl_h *meas_q, te_mi_meas_type type,
                    const char *name)
{
    te_mi_meas_impl *result;

    result = TE_ALLOC(sizeof(*result));
    if (result == NULL)
        return NULL;

    if (name != NULL)
    {
        result->name = strdup(name);
        if (result->name == NULL)
        {
            ERROR("Failed to duplicate measurement name");
            free(result);
            return NULL;
        }
    }

    result->type = type;
    TAILQ_INIT(&result->values);
    TAILQ_INSERT_TAIL(meas_q, result, next);

    return result;
}

static void
te_mi_meas_impl_destroy(te_mi_meas_impl *meas)
{
    if (meas == NULL)
        return;

    free(meas->name);
    free(meas);
}

static void
te_mi_meas_impl_remove(te_mi_meas_impl_h *meas_q, te_mi_meas_impl *meas)
{
    if (meas_q != NULL)
        TAILQ_REMOVE(meas_q, meas, next);

    te_mi_meas_impl_destroy(meas);
}

static te_bool
te_mi_logger_is_empty(const te_mi_logger *logger)
{
    return TAILQ_EMPTY(&logger->comments) &&
           TAILQ_EMPTY(&logger->meas_q) &&
           TAILQ_EMPTY(&logger->meas_keys);
}

void
te_mi_logger_add_comment(te_mi_logger *logger, te_errno *retval,
                         const char *name, const char *value_fmt, ...)
{
    va_list  ap;
    te_errno rc;

    if (logger == NULL || name == NULL || value_fmt == NULL)
    {
        ERROR("Failed to add comment with invalid args");
        rc = TE_EINVAL;
        goto out;
    }

    va_start(ap, value_fmt);
    rc = te_kvpair_add_va(&logger->comments, name, value_fmt, ap);
    va_end(ap);

    if (rc != 0 && retval == NULL)
        ERROR("Failed to add a comment to MI logger: %r", rc);

out:
    te_mi_set_logger_error(logger, retval, rc);
}

void
te_mi_logger_reset(te_mi_logger *logger)
{
    te_mi_meas_impl *meas;
    te_mi_meas_value *meas_value;

    if (logger == NULL)
        return;

    te_kvpair_fini(&logger->comments);
    te_kvpair_fini(&logger->meas_keys);
    logger->error_ignored = FALSE;

    while ((meas = TAILQ_FIRST(&logger->meas_q)) != NULL)
    {
        while ((meas_value = TAILQ_FIRST(&meas->values)) != NULL)
        {
            TAILQ_REMOVE(&meas->values, meas_value, next);
            free(meas_value);
        }

        te_mi_meas_impl_remove(&logger->meas_q, meas);
    }
}

te_errno
te_mi_logger_flush(te_mi_logger *logger)
{
    char *data;

    if (logger == NULL)
    {
        ERROR("Failed to flush a NULL logger");
        return TE_EINVAL;
    }

    if (logger->error_ignored)
    {
        ERROR("Previous failures in MI logger were ignored, flush is aborted");
        return TE_EFAIL;
    }

    if (te_mi_logger_is_empty(logger))
        return 0;

    data = te_mi_logger_data2str(logger);
    if (data == NULL)
        return TE_ENOMEM;

    LGR_MESSAGE(TE_LL_MI | TE_LL_CONTROL, TE_LOG_ARTIFACT_USER, "%s", data);
    free(data);
    te_mi_logger_reset(logger);

    return 0;
}

void
te_mi_logger_destroy(te_mi_logger *logger)
{
    te_errno rc;

    if (logger == NULL)
        return;

    rc = te_mi_logger_flush(logger);
    if (rc != 0)
    {
        ERROR("MI logger flush error on destroy: %r, logger is destroyed anyway",
              rc);
        te_mi_logger_reset(logger);
    }

    free(logger->tool);
    free(logger);
}

te_errno
te_mi_logger_meas_create(const char *tool, te_mi_logger **logger)
{
    te_mi_logger *result;

    if (logger == NULL)
    {
        ERROR("Failed to create logger: invalid logger pointer location");
        return TE_EINVAL;
    }

    if (tool == NULL)
    {
        ERROR("Failed to create logger: tool is not specified");
        return TE_EINVAL;
    }

    result = TE_ALLOC(sizeof(*result));
    if (result == NULL)
        return TE_ENOMEM;

    result->tool = strdup(tool);
    if (result->tool == NULL)
    {
        free(result);
        return TE_ENOMEM;
    }

    TAILQ_INIT(&result->meas_q);
    te_kvpair_init(&result->meas_keys);
    te_kvpair_init(&result->comments);
    result->version = TE_MI_LOG_VERSION;
    result->type = TE_MI_TYPE_MEASUREMENT;
    result->error_ignored = FALSE;

    *logger = result;

    return 0;
}

te_errno
te_mi_log_meas(const char *tool, const te_mi_meas *measurements,
               const te_mi_log_kvpair *keys, const te_mi_log_kvpair *comments)
{
    te_mi_logger *logger = NULL;
    const te_mi_log_kvpair *c;
    const te_mi_log_kvpair *k;
    te_errno rc;

    if (measurements == NULL)
    {
        ERROR("Failed to log empty measurements");
        rc = TE_EINVAL;
        goto out;
    }

    rc = te_mi_logger_meas_create(tool, &logger);
    if (rc != 0)
        goto out;

    te_mi_logger_add_meas_vec(logger, &rc, measurements);
    if (rc != 0)
        goto out;

    for (k = keys; k != NULL && k->key != NULL; k++)
    {
        te_mi_logger_add_meas_key(logger, &rc, k->key, "%s",
                                  (k->value == NULL) ? "" : k->value);
        if (rc != 0)
            goto out;
    }

    for (c = comments; c != NULL && c->key != NULL; c++)
    {
        te_mi_logger_add_comment(logger, &rc, c->key, "%s",
                                 (c->value == NULL) ? "" : c->value);
        if (rc != 0)
            goto out;
    }

    rc = te_mi_logger_flush(logger);

out:
    te_mi_logger_reset(logger);
    te_mi_logger_destroy(logger);

    if (rc != 0)
        ERROR("Failed to log MI data: %r", rc);

    return rc;
}

void
te_mi_logger_add_meas(te_mi_logger *logger, te_errno *retval,
                      te_mi_meas_type type, const char *name,
                      te_mi_meas_aggr aggr, double val,
                      te_mi_meas_multiplier multiplier)
{
    te_mi_meas_impl *meas_new = NULL;
    te_mi_meas_impl *meas;
    te_errno rc = 0;

    if (logger == NULL)
    {
        ERROR("Failed to add measurement with invalid args");
        rc = TE_EINVAL;
        goto out;
    }

    if (!te_mi_meas_type_valid(type))
    {
        ERROR("Invalid measurement type");
        rc = TE_EINVAL;
        goto out;
    }

    if (!te_mi_meas_aggr_valid(aggr))
    {
        ERROR("Invalid measurement aggregation");
        rc = TE_EINVAL;
        goto out;
    }

    if (!te_mi_meas_multiplier_valid(multiplier))
    {
        ERROR("Invalid measurement multiplier");
        rc = TE_EINVAL;
        goto out;
    }

    meas = te_mi_meas_impl_find(&logger->meas_q, type, name);
    if (meas == NULL)
    {
        meas_new = meas = te_mi_meas_impl_add(&logger->meas_q, type, name);
        if (meas == NULL)
        {
            rc = TE_ENOMEM;
            goto out;
        }
    }

    if (te_mi_meas_value_add(&meas->values, aggr, val, multiplier) == NULL)
    {
        if (meas_new != NULL)
            te_mi_meas_impl_remove(&logger->meas_q, meas_new);

        rc = TE_ENOMEM;
        goto out;
    }

out:
    te_mi_set_logger_error(logger, retval, rc);
}

void
te_mi_logger_add_meas_obj(te_mi_logger *logger, te_errno *retval,
                          const te_mi_meas *meas)
{
    if (logger == NULL || meas == NULL)
    {
        ERROR("Failed to add measurement object with invalid args");
        te_mi_set_logger_error(logger, retval, TE_EINVAL);

        return;
    }

   te_mi_logger_add_meas(logger, retval, meas->type, meas->name, meas->aggr,
                          meas->val, meas->multiplier);
}

void
te_mi_logger_add_meas_vec(te_mi_logger *logger, te_errno *retval,
                          const te_mi_meas *measurements)
{
    const te_mi_meas *m;
    te_errno rc = 0;

    for (m = measurements; m != NULL && m->type != TE_MI_MEAS_END; m++)
    {
        te_mi_logger_add_meas_obj(logger, &rc, m);

        if (rc != 0)
            break;
    }

    te_mi_set_logger_error(logger, retval, rc);
}

void
te_mi_logger_add_meas_key(te_mi_logger *logger, te_errno *retval,
                          const char *key, const char *value_fmt, ...)
{
    va_list  ap;
    te_errno rc;

    if (logger == NULL || key == NULL || value_fmt == NULL)
    {
        ERROR("Failed to add measurement key with invalid args");
        rc = TE_EINVAL;
        goto out;
    }

    va_start(ap, value_fmt);
    rc = te_kvpair_add_va(&logger->meas_keys, key, value_fmt, ap);
    va_end(ap);

    if (rc != 0 && retval == NULL)
        ERROR("Failed to add a measurement key to MI logger: %r", rc);

out:
    te_mi_set_logger_error(logger, retval, rc);
}
