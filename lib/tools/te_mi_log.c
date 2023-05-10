/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Machine interface data logging
 *
 * Implementation of API for machine interface data logging.
 *
 * Copyright (C) 2019-2022 OKTET Labs Ltd. All rights reserved.
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
#include "tq_string.h"
#include "te_mi_log.h"
#include "te_vector.h"
#include "te_json.h"
#include "math.h"

#define TE_MI_LOG_VERSION 1

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
    char *descr;
    te_mi_meas_value_h values;
} te_mi_meas_impl;

typedef TAILQ_HEAD(te_mi_meas_impl_h, te_mi_meas_impl) te_mi_meas_impl_h;

/** Reference to measurement. */
typedef struct te_mi_meas_ref {
    te_mi_meas_impl *meas;  /**< Pointer to referenced measurement */
    te_mi_meas_aggr aggr;   /**< Type of a measurement aggregation */
} te_mi_meas_ref;

/** Data specific for line-graph view */
typedef struct te_mi_meas_view_line_graph {
    te_mi_meas_ref axis_x; /**< Measurement used as X-coordinate */
    te_bool axis_x_auto_seqno; /**< If @c TRUE, on axis X sequence numbers
                                    are used instead of a measurement */
    te_vec axis_y;        /**< Measurement(s) used as Y-coordinate
                               (each measurement will be drawn as a
                               separate line on the graph) */
} te_mi_meas_view_line_graph;

/** Data specific for point view */
typedef struct te_mi_meas_view_point {
    te_mi_meas_ref value; /**< Reference to a measurement value */
} te_mi_meas_view_point;

/** Structure describing a view (such as graph). */
typedef struct te_mi_meas_view {
    TAILQ_ENTRY(te_mi_meas_view) next;  /**< Links for TAILQ */

    te_mi_meas_view_type type;          /**< View type */
    char *name;                         /**< View name */
    char *title;                        /**< View title */

    union {
        te_mi_meas_view_line_graph line_graph; /**< Line graph */
        te_mi_meas_view_point point; /**< Point */
    } data; /**< Data specific to a view type */
} te_mi_meas_view;

/** Type of a head of a queue of views */
typedef TAILQ_HEAD(te_mi_meas_view_h, te_mi_meas_view) te_mi_meas_view_h;

struct te_mi_logger {
    char *tool;
    te_mi_type type;
    unsigned int version;
    te_mi_meas_impl_h meas_q;
    te_kvpair_h meas_keys;
    te_kvpair_h comments;
    te_mi_meas_view_h views;
    te_bool error_ignored;
};

typedef enum te_mi_meas_base_unit_type {
    TE_MI_MEAS_BASE_UNITLESS, /**< Unitless value represented as a ratio */
    TE_MI_MEAS_BASE_UNIT_PPS, /**< Packets per second */
    TE_MI_MEAS_BASE_UNIT_SECOND, /**< Seconds */
    TE_MI_MEAS_BASE_UNIT_BPS, /**< Bits per second */
    TE_MI_MEAS_BASE_UNIT_CELSIUS, /**< Degrees of celsius */
    TE_MI_MEAS_BASE_UNIT_RPS, /**< Requests per second */
    TE_MI_MEAS_BASE_UNIT_HZ, /**< Events per seconds (Hz) */
    TE_MI_MEAS_BASE_UNIT_IOPS, /**< Input/Output operations per second */
} te_mi_meas_base_unit_type;

static te_mi_meas_base_unit_type meas_base_unit_by_type_map[] = {
    [TE_MI_MEAS_PPS] = TE_MI_MEAS_BASE_UNIT_PPS,
    [TE_MI_MEAS_LATENCY] = TE_MI_MEAS_BASE_UNIT_SECOND,
    [TE_MI_MEAS_THROUGHPUT] = TE_MI_MEAS_BASE_UNIT_BPS,
    [TE_MI_MEAS_BANDWIDTH_USAGE] = TE_MI_MEAS_BASE_UNITLESS,
    [TE_MI_MEAS_TEMP] = TE_MI_MEAS_BASE_UNIT_CELSIUS,
    [TE_MI_MEAS_RPS] = TE_MI_MEAS_BASE_UNIT_RPS,
    [TE_MI_MEAS_RTT] = TE_MI_MEAS_BASE_UNIT_SECOND,
    [TE_MI_MEAS_RETRANS] = TE_MI_MEAS_BASE_UNITLESS,
    [TE_MI_MEAS_FREQ] = TE_MI_MEAS_BASE_UNIT_HZ,
    [TE_MI_MEAS_EPE] = TE_MI_MEAS_BASE_UNITLESS,
    [TE_MI_MEAS_IOPS] = TE_MI_MEAS_BASE_UNIT_IOPS,
    [TE_MI_MEAS_LOADAVG] = TE_MI_MEAS_BASE_UNITLESS,
};

static const te_enum_map meas_base_unit_names[] = {
    {.value = TE_MI_MEAS_BASE_UNITLESS, .name = ""},
    {.value = TE_MI_MEAS_BASE_UNIT_PPS, .name = "pps"},
    {.value = TE_MI_MEAS_BASE_UNIT_SECOND, .name = "second"},
    {.value = TE_MI_MEAS_BASE_UNIT_BPS, .name = "bps"},
    {.value = TE_MI_MEAS_BASE_UNIT_CELSIUS, .name = "degrees celsius"},
    {.value = TE_MI_MEAS_BASE_UNIT_RPS, .name = "rps"},
    {.value = TE_MI_MEAS_BASE_UNIT_HZ, .name = "Hz"},
    {.value = TE_MI_MEAS_BASE_UNIT_IOPS, .name = "iops"},
    TE_ENUM_MAP_END
};

static const te_enum_map mi_type_names[] = {
   {.value = TE_MI_TYPE_MEASUREMENT, .name = "measurement"},
   TE_ENUM_MAP_END
};

static const te_enum_map meas_aggr_names[] = {
    {.value = TE_MI_MEAS_AGGR_SINGLE, .name = "single"},
    {.value = TE_MI_MEAS_AGGR_MIN, .name = "min"},
    {.value = TE_MI_MEAS_AGGR_MAX, .name = "max"},
    {.value = TE_MI_MEAS_AGGR_MEAN, .name = "mean"},
    {.value = TE_MI_MEAS_AGGR_MEDIAN, .name = "median"},
    {.value = TE_MI_MEAS_AGGR_CV, .name = "cv"},
    {.value = TE_MI_MEAS_AGGR_STDEV, .name = "stdev"},
    {.value = TE_MI_MEAS_AGGR_OUT_OF_RANGE, .name = "out of range"},
    {.value = TE_MI_MEAS_AGGR_PERCENTILE, .name = "percentile"},
    {.value = TE_MI_MEAS_AGGR_SV_UNSPECIFIED, .name = "unspecified"},
    TE_ENUM_MAP_END
};

static const te_enum_map meas_type_names[] = {
    {.value = TE_MI_MEAS_PPS, .name = "pps"},
    {.value = TE_MI_MEAS_LATENCY, .name = "latency"},
    {.value = TE_MI_MEAS_THROUGHPUT, .name = "throughput"},
    {.value = TE_MI_MEAS_BANDWIDTH_USAGE, .name = "bandwidth-usage"},
    {.value = TE_MI_MEAS_TEMP, .name = "temperature"},
    {.value = TE_MI_MEAS_RPS, .name = "rps"},
    {.value = TE_MI_MEAS_RTT, .name = "rtt"},
    {.value = TE_MI_MEAS_RETRANS, .name = "TCP retransmissions"},
    {.value = TE_MI_MEAS_FREQ, .name = "events-per-second"},
    {.value = TE_MI_MEAS_EPE, .name = "events-per-event"},
    {.value = TE_MI_MEAS_IOPS, .name = "iops"},
    {.value = TE_MI_MEAS_LOADAVG, .name = "load-average"},
    TE_ENUM_MAP_END
};

static const te_enum_map meas_view_type_names[] = {
    {.value = TE_MI_MEAS_VIEW_LINE_GRAPH, .name = "line-graph"},
    {.value = TE_MI_MEAS_VIEW_POINT, .name = "point"},
    TE_ENUM_MAP_END
};

static const te_enum_map meas_multiplier_names[] = {
    {.value = TE_MI_MEAS_MULTIPLIER_NANO, .name = "1e-9"},
    {.value = TE_MI_MEAS_MULTIPLIER_MICRO, .name = "1e-6"},
    {.value = TE_MI_MEAS_MULTIPLIER_MILLI, .name = "1e-3"},
    {.value = TE_MI_MEAS_MULTIPLIER_PLAIN, .name = "1"},
    {.value = TE_MI_MEAS_MULTIPLIER_KILO, .name = "1e+3"},
    {.value = TE_MI_MEAS_MULTIPLIER_KIBI, .name = "0x1p10"},
    {.value = TE_MI_MEAS_MULTIPLIER_MEGA, .name = "1e+6"},
    {.value = TE_MI_MEAS_MULTIPLIER_MEBI, .name = "0x1p20"},
    {.value = TE_MI_MEAS_MULTIPLIER_GIGA, .name = "1e+9"},
    {.value = TE_MI_MEAS_MULTIPLIER_GIBI, .name = "0x1p30"},
    TE_ENUM_MAP_END
};

static const char *
te_mi_meas_get_base_unit_str(te_mi_meas_type type, te_mi_meas_aggr aggr)
{
    te_mi_meas_base_unit_type base;

    /*
     * Some of aggregation types are unit-independend. They override the unit
     * type of the measurement and make any measurement unitless.
     */
    if (aggr == TE_MI_MEAS_AGGR_CV || aggr == TE_MI_MEAS_AGGR_OUT_OF_RANGE)
        base = TE_MI_MEAS_BASE_UNITLESS;
    else
        base = meas_base_unit_by_type_map[type];

    return te_enum_map_from_any_value(meas_base_unit_names, base, NULL);
}

static te_bool
te_mi_meas_aggr_is_specified(te_mi_meas_aggr aggr)
{
    return aggr > TE_MI_MEAS_AGGR_START && aggr < TE_MI_MEAS_AGGR_END;
}

static te_bool
te_mi_meas_aggr_is_special_value(te_mi_meas_aggr aggr)
{
    return aggr > TE_MI_MEAS_AGGR_SV_START && aggr < TE_MI_MEAS_AGGR_SV_END;
}

static te_bool
te_mi_meas_aggr_valid(te_mi_meas_aggr aggr)
{
    return te_mi_meas_aggr_is_specified(aggr)
           || te_mi_meas_aggr_is_special_value(aggr);
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

static te_bool
te_mi_meas_view_type_valid(te_mi_meas_view_type type)
{
    return type >= 0 && type < TE_MI_MEAS_VIEW_MAX;
}

static te_bool
te_mi_graph_axis_valid(te_mi_graph_axis axis)
{
    return axis >= 0 && axis < TE_MI_GRAPH_AXIS_MAX;
}

static te_bool
te_mi_check_meas_type_name(te_mi_meas_type type, const char *name)
{
    if (type == TE_MI_MEAS_END && name == NULL)
    {
        ERROR("%s(): either measurement name or measurement type must be "
              "specified", __FUNCTION__);
        return FALSE;
    }

    if (type != TE_MI_MEAS_END && !te_mi_meas_type_valid(type))
    {
        ERROR("%s(): invalid measurement type %d", __FUNCTION__, type);
        return FALSE;
    }

    return TRUE;
}

static void
te_mi_set_logger_error(te_mi_logger *logger, te_errno *retval, te_errno val)
{
    if (retval != NULL)
        *retval = val;

    if (val != 0 && logger != NULL && retval == NULL)
        logger->error_ignored = TRUE;
}

static void
te_mi_meas_value_str_append(const te_mi_meas_value *value, te_mi_meas_type type,
                            te_json_ctx_t *ctx)
{
    te_json_start_object(ctx);
    te_json_add_key_enum(ctx, meas_aggr_names, "aggr", value->aggr);

    te_json_add_key(ctx, "value");
    te_json_add_float(ctx, value->val, 6);

    te_json_add_key_str(ctx, "base_units",
                        te_mi_meas_get_base_unit_str(type, value->aggr));

    te_json_add_key_enum(ctx, meas_multiplier_names, "multiplier",
                         value->multiplier);
    te_json_end(ctx);
}

static void
te_mi_meas_values_str_append(const te_mi_meas_value_h *values,
                             te_mi_meas_type type,
                             te_json_ctx_t *ctx)
{
    const te_mi_meas_value *value;

    te_json_add_key(ctx, "entries");

    te_json_start_array(ctx);
    TAILQ_FOREACH(value, values, next)
    {
        te_mi_meas_value_str_append(value, type, ctx);
    }
    te_json_end(ctx);
}

static void
te_mi_meas_str_append(const te_mi_meas_impl *meas,
                      te_json_ctx_t *ctx)
{
    te_json_start_object(ctx);
    te_json_add_key_enum(ctx, meas_type_names, "type", meas->type);

    te_json_add_key_str(ctx, "name", meas->name);
    te_json_add_key_str(ctx, "description", meas->descr);

    te_mi_meas_values_str_append(&meas->values, meas->type, ctx);

    te_json_end(ctx);
}

static void
te_mi_meas_q_str_append(const te_mi_meas_impl_h *meas_q,
                        te_json_ctx_t *ctx)
{
    const te_mi_meas_impl *meas;

    te_json_add_key(ctx, "results");

    te_json_start_array(ctx);
    TAILQ_FOREACH(meas, meas_q, next)
    {
        te_mi_meas_str_append(meas, ctx);
    }
    te_json_end(ctx);
}

static void
te_mi_kvpairs_str_append(const te_kvpair_h *pairs, const char *dict_name,
                         te_json_ctx_t *ctx)
{
    if (TAILQ_EMPTY(pairs))
        return;

    te_json_add_key(ctx, dict_name);
    te_json_add_kvpair(ctx, pairs);
}

static te_errno
te_mi_meas_ref_str_append(const te_mi_meas_ref *ref,
                          te_json_ctx_t *ctx)
{
    if (ref->meas == NULL)
    {
        ERROR("%s(): measurement pointer is NULL in a reference",
              __FUNCTION__);
        return TE_EINVAL;
    }

    te_json_start_object(ctx);
    te_json_add_key_enum(ctx, meas_type_names, "type",
                         ref->meas->type);
    te_json_add_key_str(ctx, "name", ref->meas->name);

    if (te_mi_meas_aggr_is_specified(ref->aggr))
    {
        te_json_add_key_enum(ctx, meas_aggr_names, "aggr",
                             ref->aggr);
    }
    te_json_end(ctx);

    return 0;
}

static te_errno
te_mi_meas_view_line_graph_str_append(const te_mi_meas_view *view,
                                      te_json_ctx_t *ctx)
{
    const te_mi_meas_view_line_graph *line_graph = NULL;
    te_errno rc = 0;
    const te_mi_meas_ref *ref;

    line_graph = &view->data.line_graph;

    te_json_add_key(ctx, "axis_x");
    if (line_graph->axis_x_auto_seqno)
    {
        te_json_start_object(ctx);
        te_json_add_key_str(ctx, "name", "auto-seqno");
        te_json_end(ctx);
    }
    else
    {
        rc = te_mi_meas_ref_str_append(&line_graph->axis_x, ctx);
        if (rc != 0)
            return rc;
    }

    if (te_vec_size(&line_graph->axis_y) > 0)
    {
        te_json_add_key(ctx, "axis_y");

        te_json_start_array(ctx);
        TE_VEC_FOREACH(&line_graph->axis_y, ref)
        {
            rc = te_mi_meas_ref_str_append(ref, ctx);
            if (rc != 0)
                return rc;
        }
        te_json_end(ctx);
    }

    return 0;
}

static te_errno
te_mi_meas_view_point_str_append(const te_mi_meas_view *view,
                                 te_json_ctx_t *ctx)
{
    te_errno rc;

    te_json_add_key(ctx, "value");
    rc = te_mi_meas_ref_str_append(&view->data.point.value, ctx);
    if (rc != 0)
        return rc;

    return 0;
}

static te_errno
te_mi_meas_view_str_append(const te_mi_meas_view *view,
                           te_json_ctx_t *ctx)
{
    te_errno rc;

    te_json_start_object(ctx);
    te_json_add_key_str(ctx, "name", view->name);
    te_json_add_key_enum(ctx, meas_view_type_names, "type",
                         view->type);
    te_json_add_key_str(ctx, "title", view->title);

    switch (view->type)
    {
        case TE_MI_MEAS_VIEW_LINE_GRAPH:
            rc = te_mi_meas_view_line_graph_str_append(view, ctx);
            if (rc != 0)
                return rc;
            break;

        case TE_MI_MEAS_VIEW_POINT:
            rc = te_mi_meas_view_point_str_append(view, ctx);
            if (rc != 0)
                return rc;
            break;

        default:
            ERROR("%s(): not supported view type %d",
                  __FUNCTION__, view->type);
            return TE_EINVAL;
    }
    te_json_end(ctx);

    return 0;
}

static te_errno
te_mi_meas_views_str_append(const te_mi_meas_view_h *views,
                            te_json_ctx_t *ctx)
{
    const te_mi_meas_view *view;
    te_errno rc;

    if (TAILQ_EMPTY(views))
        return 0;

    te_json_add_key(ctx, "views");

    te_json_start_array(ctx);
    TAILQ_FOREACH(view, views, next)
    {
        rc = te_mi_meas_view_str_append(view, ctx);
        if (rc != 0)
            return rc;
    }
    te_json_end(ctx);

    return rc;
}

static char *
te_mi_logger_data2str(const te_mi_logger *logger)
{
    te_string str = TE_STRING_INIT;
    te_json_ctx_t ctx = TE_JSON_INIT_STR(&str);

    te_json_start_object(&ctx);
    te_json_add_key_enum(&ctx, mi_type_names, "type", logger->type);

    te_json_add_key(&ctx, "version");
    te_json_add_integer(&ctx, logger->version);

    te_json_add_key_str(&ctx, "tool", logger->tool);

    te_mi_meas_q_str_append(&logger->meas_q, &ctx);

    te_mi_kvpairs_str_append(&logger->meas_keys, "keys", &ctx);

    te_mi_kvpairs_str_append(&logger->comments, "comments", &ctx);

    if (te_mi_meas_views_str_append(&logger->views, &ctx) != 0)
    {
        ERROR("Failed to convert logger data to string");
        te_string_free(&str);
        return NULL;
    }

    te_json_end(&ctx);
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

/* Search for measurement by name. Only one measurement can be found by name */
static te_mi_meas_impl *
te_mi_meas_impl_find_uniq_by_name(te_mi_meas_impl_h *meas_q, const char *name)
{
    te_mi_meas_impl *result;
    unsigned int found_count = 0;
    te_mi_meas_impl *p;

    TAILQ_FOREACH(p, meas_q, next)
    {
        if (strcmp_null(p->name, name) == 0)
        {
            result = p;
            found_count++;
        }
    }

    if (found_count == 1)
        return result;
    else
        return NULL;
}

/* Search for measurements by name or by tuple name + type. */
static te_mi_meas_impl *
te_mi_meas_impl_find_ext(te_mi_meas_impl_h *meas_q, te_mi_meas_type type,
                         const char *name)
{
    te_mi_meas_impl *result;

    if (type == TE_MI_MEAS_END)
    {
        result = te_mi_meas_impl_find_uniq_by_name(meas_q, name);
        if (result == NULL)
        {
            ERROR("%s(): Failed to found an unique measurement "
                  "with name '%s'", __FUNCTION__, name);
            return NULL;
        }
    }
    else
    {
       result = te_mi_meas_impl_find(meas_q, type, name);
       if (result == NULL)
       {
            ERROR("%s(): Failed to found a measurement with name '%s' and "
                  "type '%s'", __FUNCTION__, name,
                  te_enum_map_from_any_value(meas_type_names, type, "???"));
            return NULL;
       }
    }

    return result;
}

static te_mi_meas_value *
te_mi_meas_value_find_aggr_spec(te_mi_meas_value_h *values,
                                te_mi_meas_aggr aggr, unsigned int *count)
{
    te_mi_meas_value *v;
    te_mi_meas_value *result;

    TAILQ_FOREACH(v, values, next)
    {
        if (v->aggr == aggr)
        {
            result = v;
            (*count)++;
        }
    }

    return result;
}

static te_mi_meas_value *
te_mi_meas_value_find_aggr_unspec(te_mi_meas_value_h *values,
                                  unsigned int *count)
{
    te_mi_meas_value *v;
    te_mi_meas_value *result;

    TAILQ_FOREACH(v, values, next)
    {
        (*count)++;
    }

    result = TAILQ_LAST(values, te_mi_meas_value_h);
    return result;
}

/* Find te_mi_meas_value. If the quantity is not equal to 1 - print an error */
static te_mi_meas_value *
te_mi_meas_value_find_uniq(te_mi_meas_value_h *values, te_mi_meas_type type,
                           const char *name, te_mi_meas_aggr aggr)
{
    unsigned int count = 0;
    te_mi_meas_value *result;

    if (!te_mi_meas_aggr_valid(aggr))
    {
        ERROR("%s(): invalid aggregation type: aggr = %d",
              __FUNCTION__, aggr);
        return NULL;
    }

    if (aggr == TE_MI_MEAS_AGGR_SV_UNSPECIFIED)
    {
        result = te_mi_meas_value_find_aggr_unspec(values, &count);
    }
    else if (te_mi_meas_aggr_is_specified(aggr))
    {
        result = te_mi_meas_value_find_aggr_spec(values, aggr, &count);
    }
    else
    {
        ERROR("%s():  can't search for value by aggregation: "
              "aggr = %s", __FUNCTION__,
              te_enum_map_from_any_value(meas_aggr_names, aggr, "????"));
        return NULL;
    }

    if (count == 0)
    {
        ERROR("%s(): failed to find an aggregation '%s' for a measurement "
              "with type %s and name '%s'", __FUNCTION__,
              te_enum_map_from_any_value(meas_aggr_names, aggr, "????"),
              te_enum_map_from_any_value(meas_type_names, type, "(null)"),
              (name == NULL ? "(null)" : name));
        return NULL;
    }
    else if (count > 1)
    {
        ERROR("%s(): value found by aggregation '%s' for a measurement "
              " with type %s and name %s is not unique", __FUNCTION__,
              te_enum_map_from_any_value(meas_aggr_names, aggr, "????"),
              te_enum_map_from_any_value(meas_type_names, type, "(null)"),
              (name == NULL ? "(null)" : name));
        return NULL;
    }

    return result;
}

const char *
te_mi_meas_type2descr(te_mi_meas_type type)
{
    switch (type)
    {
        case TE_MI_MEAS_PPS:
            return "Packets per second";

        case TE_MI_MEAS_LATENCY:
            return "Latency in seconds";

        case TE_MI_MEAS_THROUGHPUT:
            return "Throughput in bits per second";

        case TE_MI_MEAS_BANDWIDTH_USAGE:
            return "Bandwidth usage ratio";

        case TE_MI_MEAS_TEMP:
            return "Temperature in degrees Celsius";

        case TE_MI_MEAS_RPS:
            return "Requests per second";

        case TE_MI_MEAS_RTT:
            return "Round trip time in seconds";

        case TE_MI_MEAS_RETRANS:
            return "TCP retransmissions";

        case TE_MI_MEAS_FREQ:
            return "Events per second";

        case TE_MI_MEAS_EPE:
            return "Events per another event";

        case TE_MI_MEAS_IOPS:
            return "Input/Output operations per second";

        case TE_MI_MEAS_LOADAVG:
            return "Load average";

        default:
            return "Unknown type";
    }
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
        result->descr = strdup(name);
        if (result->name == NULL || result->descr == NULL)
        {
            ERROR("Failed to duplicate measurement name");
            free(result->name);
            free(result->descr);
            free(result);
            return NULL;
        }
    }
    else
    {
        result->descr = strdup(te_mi_meas_type2descr(type));
        if (result->descr == NULL)
        {
            ERROR("Failed to duplicate measurement description");
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
    free(meas->descr);
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
           TAILQ_EMPTY(&logger->meas_keys) &&
           TAILQ_EMPTY(&logger->views);
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

static te_mi_meas_view *
te_mi_meas_view_find(te_mi_meas_view_h *views,
                     te_mi_meas_view_type type,
                     const char *name)
{
    te_mi_meas_view *view;

    if (name == NULL)
        return NULL;

    TAILQ_FOREACH(view, views, next)
    {
        if (view->type == type && strcmp(view->name, name) == 0)
        {
            return view;
        }
    }

    return NULL;
}

static void
te_mi_meas_view_init(te_mi_meas_view *view)
{
    switch (view->type)
    {
        case TE_MI_MEAS_VIEW_LINE_GRAPH:
            view->data.line_graph.axis_y = TE_VEC_INIT(te_mi_meas_ref);
            break;

        default:
            /* Do nothing */
            break;
    }
}

void
te_mi_logger_add_meas_view(te_mi_logger *logger, te_errno *retval,
                           te_mi_meas_view_type type, const char *name,
                           const char *title)
{
    te_mi_meas_view *view = NULL;
    te_errno rc = 0;

    if (name == NULL || title == NULL)
    {
        ERROR("Name and title of the view must not be NULL "
              "(they may be empty strings)");
        rc = TE_EINVAL;
        goto out;
    }

    if (!te_mi_meas_view_type_valid(type))
    {
        ERROR("Invalid view type %d", type);
        rc = TE_EINVAL;
        goto out;
    }

    view = te_mi_meas_view_find(&logger->views, type, name);
    if (view != NULL)
    {
        ERROR("A view with type '%s' and name '%s' is already present",
              te_enum_map_from_any_value(meas_view_type_names, type, "????"),
              name);
        rc = TE_EEXIST;
        goto out;
    }

    view = TE_ALLOC(sizeof(*view));
    if (view == NULL)
    {
        ERROR("Failed to allocate memory for a view");
        rc = TE_ENOMEM;
        goto out;
    }

    te_mi_meas_view_init(view);
    view->type = type;

    view->name = strdup(name);
    view->title = strdup(title);
    if (view->name == NULL || view->title == NULL)
    {
        ERROR("strdup() failed to copy view name or title");
        free(view->name);
        free(view->title);
        free(view);
        rc = TE_ENOMEM;
        goto out;
    }

    TAILQ_INSERT_TAIL(&logger->views, view, next);

out:
    te_mi_set_logger_error(logger, retval, rc);
}

static te_errno
meas_view_add_meas_to_axis(te_mi_meas_view *view,
                           te_mi_graph_axis axis,
                           te_mi_meas_impl *impl)
{
    te_mi_meas_view_line_graph *line_graph = NULL;
    te_mi_meas_ref ref;
    te_errno rc;

    if (view->type != TE_MI_MEAS_VIEW_LINE_GRAPH)
    {
        ERROR("%s(): only line-graph views are currently supported",
              __FUNCTION__);
        return TE_EINVAL;
    }

    line_graph = &view->data.line_graph;

    switch (axis)
    {
        case TE_MI_GRAPH_AXIS_X:
            if (line_graph->axis_x.meas != NULL)
            {
                ERROR("%s(): only one measurement name can be specified "
                      "for X axis for a line-graph", __FUNCTION__);
                return TE_EINVAL;
            }

            line_graph->axis_x.meas = impl;
            line_graph->axis_x.aggr = TE_MI_MEAS_AGGR_SV_UNSPECIFIED;

            break;

        case TE_MI_GRAPH_AXIS_Y:

            memset(&ref, 0, sizeof(ref));
            ref.meas = impl;
            ref.aggr = TE_MI_MEAS_AGGR_SV_UNSPECIFIED;

            rc = TE_VEC_APPEND(&line_graph->axis_y, ref);
            if (rc != 0)
            {
                ERROR("%s(): TE_VEC_APPEND() failed to add a "
                      "measurement for axis Y", __FUNCTION__);
                return rc;
            }

            break;

        default:
            ERROR("%s(): unsupported axis type %d", __FUNCTION__, axis);
            return TE_EINVAL;
    }

    return 0;
}

void
te_mi_logger_meas_graph_axis_add(te_mi_logger *logger,
                                 te_errno *retval,
                                 te_mi_meas_view_type view_type,
                                 const char *view_name,
                                 te_mi_graph_axis axis,
                                 te_mi_meas_type meas_type,
                                 const char *meas_name)
{
    te_mi_meas_view *view = NULL;
    te_mi_meas_impl *meas = NULL;
    te_errno rc;

    if (!te_mi_meas_view_type_valid(view_type))
    {
        ERROR("%s(): invalid view type %d", __FUNCTION__, view_type);
        te_mi_set_logger_error(logger, retval, TE_EINVAL);
        return;
    }

    if (!te_mi_graph_axis_valid(axis))
    {
        ERROR("%s(): invalid axis type %d", __FUNCTION__, axis);
        te_mi_set_logger_error(logger, retval, TE_EINVAL);
        return;
    }

    view = te_mi_meas_view_find(&logger->views, view_type, view_name);
    if (view == NULL)
    {
        ERROR("%s(): failed to find measurement view with type '%s' and "
              "name '%s'", __FUNCTION__,
              te_enum_map_from_any_value(meas_view_type_names, view_type,
                                         "????"),
              (view_name == NULL ? "(null)" : view_name));
        te_mi_set_logger_error(logger, retval, TE_ENOENT);
        return;
    }

    if (!te_mi_check_meas_type_name(meas_type, meas_name))
    {
        te_mi_set_logger_error(logger, retval, TE_EINVAL);
        return;
    }

    if (meas_name != NULL && strcmp(meas_name, "auto-seqno") == 0)
    {
        if (axis == TE_MI_GRAPH_AXIS_X)
        {
            view->data.line_graph.axis_x_auto_seqno = TRUE;
        }
        else
        {
            ERROR("%s(): auto-seqno can be specified only for axis X",
                  __FUNCTION__);
            te_mi_set_logger_error(logger, retval, TE_EINVAL);
        }

        return;
    }

    meas = te_mi_meas_impl_find_ext(&logger->meas_q, meas_type, meas_name);
    if (meas == NULL)
    {
        te_mi_set_logger_error(logger, retval, TE_ENOENT);
        return;
    }

    rc = meas_view_add_meas_to_axis(view, axis, meas);
    if (rc != 0)
        te_mi_set_logger_error(logger, retval, rc);
}

void
te_mi_meas_view_line_graph_destroy(te_mi_meas_view *view)
{
    te_vec_free(&view->data.line_graph.axis_y);
}

void
te_mi_meas_view_destroy(te_mi_meas_view *view)
{
    switch (view->type)
    {
        case TE_MI_MEAS_VIEW_LINE_GRAPH:
            te_mi_meas_view_line_graph_destroy(view);
            break;

        default:
            /* Do nothing */
            break;
    }

    free(view->name);
    free(view->title);
    free(view);
}

void
te_mi_logger_meas_point_add(te_mi_logger *logger,
                            te_errno *retval,
                            const char *view_name,
                            te_mi_meas_type meas_type,
                            const char *meas_name,
                            te_mi_meas_aggr meas_aggr)
{
    te_mi_meas_view *view = NULL;
    te_mi_meas_impl *meas = NULL;
    te_mi_meas_value *value = NULL;

    if (!te_mi_meas_aggr_valid(meas_aggr))
    {
        ERROR("%s(): invalid aggregation type: aggr = %d",
              __FUNCTION__, meas_aggr);
        te_mi_set_logger_error(logger, retval, TE_EINVAL);
        return;
    }

    view = te_mi_meas_view_find(&logger->views, TE_MI_MEAS_VIEW_POINT,
                                view_name);
    if (view == NULL)
    {
        ERROR("%s(): failed to find measurement view with type '%s' and "
              "name '%s'", __FUNCTION__,
              te_enum_map_from_value(meas_view_type_names,
                                     TE_MI_MEAS_VIEW_POINT),
              (view_name == NULL ? "(null)" : view_name));
        te_mi_set_logger_error(logger, retval, TE_ENOENT);
        return;
    }

    if (!te_mi_check_meas_type_name(meas_type, meas_name))
    {
        te_mi_set_logger_error(logger, retval, TE_EINVAL);
        return;
    }

    meas = te_mi_meas_impl_find_ext(&logger->meas_q, meas_type, meas_name);
    if (meas == NULL)
    {
        te_mi_set_logger_error(logger, retval, TE_EINVAL);
        return;
    }

    value = te_mi_meas_value_find_uniq(&meas->values, meas_type,
                                       meas_name, meas_aggr);
    if (value == NULL)
    {
        te_mi_set_logger_error(logger, retval, TE_ENOENT);
        return;
    }

    view->data.point.value.meas = meas;
    view->data.point.value.aggr = value->aggr;
}


void
te_mi_logger_reset(te_mi_logger *logger)
{
    te_mi_meas_impl *meas;
    te_mi_meas_value *meas_value;
    te_mi_meas_view *view;
    te_mi_meas_view *view_aux;

    if (logger == NULL)
        return;

    te_kvpair_fini(&logger->comments);
    te_kvpair_fini(&logger->meas_keys);
    logger->error_ignored = FALSE;

    TAILQ_FOREACH_SAFE(view, &logger->views, next, view_aux)
    {
        TAILQ_REMOVE(&logger->views, view, next);
        te_mi_meas_view_destroy(view);
    }

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
    TAILQ_INIT(&result->views);
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

    if (name != NULL && strcmp(name, TE_MI_GRAPH_AUTO_SEQNO) == 0)
    {
        ERROR("Name '%s' is reserved for MI graphs",
              TE_MI_GRAPH_AUTO_SEQNO);
        rc = TE_EINVAL;
        goto out;
    }

    if (!te_mi_meas_type_valid(type))
    {
        ERROR("Invalid measurement type");
        rc = TE_EINVAL;
        goto out;
    }

    if (!te_mi_meas_aggr_is_specified(aggr))
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
