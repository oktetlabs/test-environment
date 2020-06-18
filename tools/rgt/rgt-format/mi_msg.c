/** @file
 * @brief Implementation of API for logging MI messages
 *
 * Copyright (C) 2020 OKTET Labs. All rights reserved.
 *
 *
 * @author Dmitry Izbitsky  <Dmitry.Izbitsky@oktetlabs.ru>
 */

#include "te_config.h"

#if HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_LIBJANSSON
#include <jansson.h>
#endif

#include "te_defs.h"
#include "mi_msg.h"
#include "te_alloc.h"

/**
 * Get JSON value's type as a string.
 *
 * @param type      JSON value type
 *
 * @returns Statically allocated string with type name
 */
static const char *
json_type_string(json_type type)
{
#define JSONCASE(_type) case _type: return #_type;
    switch (type)
    {
        JSONCASE(JSON_OBJECT)
        JSONCASE(JSON_ARRAY)
        JSONCASE(JSON_STRING)
        JSONCASE(JSON_INTEGER)
        JSONCASE(JSON_REAL)
        JSONCASE(JSON_TRUE)
        JSONCASE(JSON_FALSE)
        JSONCASE(JSON_NULL)
        default: return "UNKNOWN";
    }
#undef JSONCASE
}

/**
 * Signal message parsing error.
 *
 * @param mi        MI message
 * @param fmt       Format string
 * @param ...       Values for the format string
 */
static void
te_rgt_mi_parse_error(te_rgt_mi *mi, te_errno rc, const char *fmt, ...)
{
    va_list va;

    va_start(va, fmt);

    mi->parse_failed = TRUE;
    vsnprintf(mi->parse_err, sizeof(mi->parse_err), fmt, va);
    mi->rc = rc;

    va_end(va);
    return;
}

/**
 * Release memory allocated for storing data for MI measurement view.
 *
 * @param view    Structure describing measurement view.
 */
static void
te_rgt_mi_meas_view_clean(te_rgt_mi_meas_view *view)
{
    te_rgt_mi_meas_view_line_graph *line_graph;

    if (view->type != NULL &&
        strcmp(view->type, "line-graph") == 0)
    {
        line_graph = &view->data.line_graph;
        free(line_graph->axis_y);
        line_graph->axis_y = NULL;
        line_graph->axis_y_num = 0;
    }
}

/**
 * Release memory allocated for storing data for MI measurement views.
 *
 * @param views    Array of measurement views.
 * @param num      Number of elements in the array.
 */
static void
te_rgt_mi_meas_views_clean(te_rgt_mi_meas_view *views,
                           size_t num)
{
    size_t i;

    for (i = 0; i < num; i++)
    {
        te_rgt_mi_meas_view_clean(&views[i]);
    }
}

/**
 * Release memory allocated for storing data for MI message
 * of type "measurement".
 *
 * @param meas      Pointer to te_rgt_mi_meas structure.
 */
static void
te_rgt_mi_meas_clean(te_rgt_mi_meas *meas)
{
    size_t i;

    if (meas->params != NULL)
    {
        for (i = 0; i < meas->params_num; i++)
        {
            free(meas->params[i].values);
        }

        free(meas->params);
        meas->params = NULL;
    }
    meas->params_num = 0;

    free(meas->keys);
    meas->keys = NULL;
    meas->keys_num = 0;

    free(meas->comments);
    meas->comments = NULL;
    meas->comments_num = 0;

    for (i = 0; i < meas->views_num; i++)
    {
        te_rgt_mi_meas_view_clean(&meas->views[i]);
    }

    free(meas->views);
    meas->views = NULL;
    meas->views_num = 0;
}

/**
 * Release memory allocated for storing data for MI message
 * of type "test_start".
 *
 * @param meas      Pointer to te_rgt_mi_test_start structure.
 */
static void
te_rgt_mi_test_start_clean(te_rgt_mi_test_start *data)
{
    if (data->params != NULL)
    {
        free(data->params);
        data->params = NULL;
    }
    data->params_num = 0;

    if (data->authors != NULL)
    {
        free(data->authors);
        data->authors = NULL;
    }
    data->authors_num = 0;
}

/**
 * Release memory allocated for storing data for MI message
 * of type "test_end".
 *
 * @param meas      Pointer to te_rgt_mi_test_end structure.
 */
static void
te_rgt_mi_test_end_clean(te_rgt_mi_test_end *data)
{
    size_t i;

    free(data->obtained.verdicts);
    data->obtained.verdicts = NULL;
    data->obtained.verdicts_num = 0;

    if (data->expected != NULL)
    {
        for (i = 0; i < data->expected_num; i++)
            free(data->expected[i].verdicts);

        free(data->expected);
        data->expected = NULL;
        data->expected_num = 0;
    }
}

/* See description in mi_msg.h */
void
te_rgt_mi_clean(te_rgt_mi *mi)
{
    switch (mi->type)
    {
        case TE_RGT_MI_TYPE_MEASUREMENT:
            te_rgt_mi_meas_clean(&mi->data.measurement);
            break;
        case TE_RGT_MI_TYPE_TEST_START:
            te_rgt_mi_test_start_clean(&mi->data.test_start);
            break;
        case TE_RGT_MI_TYPE_TEST_END:
            te_rgt_mi_test_end_clean(&mi->data.test_end);
            break;
        default:;
    }

#ifdef HAVE_LIBJANSSON
    if (mi->json_obj != NULL)
        json_decref((json_t *)(mi->json_obj));
#endif
}

#ifdef HAVE_LIBJANSSON

/**
 * Get string value of a key in JSON object.
 *
 * @param obj       JSON object.
 * @param key       Key name.
 *
 * @return String value on success, @c NULL otherwise.
 */
static const char *
json_object_get_string(json_t *obj, const char *key)
{
    json_t *field;

    field = json_object_get(obj, key);
    if (field == NULL)
        return NULL;
    return json_string_value(field);
}

/**
 * Get numeric value of a key in JSON object.
 *
 * @param obj       JSON object.
 * @param key       Key name.
 * @param num       Where to save obtained value.
 *
 * @return @c 0 on success, @c -1 on failure.
 */
static int
json_object_get_number(json_t *obj, const char *key, double *num)
{
    json_t *field;

    field = json_object_get(obj, key);
    if (field == NULL)
        return -1;

    *num = json_number_value(field);
    return 0;
}

/**
 * Obtain object from a given field of parent object and store
 * all its keys together with values in an array (it is assumed that all
 * the values are strings).
 *
 * @param obj     Parent object.
 * @param field   Field of the parent object containing child object.
 * @param kvs     Where to save pointer to an array.
 * @param num     Where to save number of elements in the array.
 *
 * @return Status code.
 */
static te_errno
get_child_keys(json_t *obj, const char *field,
               te_rgt_mi_kv **kvs, size_t *num)
{
    json_t *keys;
    const char *key;
    json_t *key_value;
    const char *str_val;
    size_t i = 0;

    *kvs = NULL;
    *num = 0;

    keys = json_object_get(obj, field);
    if (keys == NULL || !json_is_object(keys))
        return 0;

    *num = json_object_size(keys);
    if (*num == 0)
        return 0;

    *kvs = calloc(*num, sizeof(*kvs));
    if (*kvs == NULL)
        return TE_ENOMEM;

    json_object_foreach(keys, key, key_value)
    {
        str_val = json_string_value(key_value);

        (*kvs)[i].key = key;
        (*kvs)[i].value = str_val;
        i++;
    }

    return 0;
}

/**
 * Parse measurement reference used in graph specification.
 *
 * @param ref_obj       Object containing reference.
 * @param meas          Pointer to structure describing parsed
 *                      MI measurement attribute.
 * @param idx           Will be set to index of measured parameter
 *                      on success.
 *
 * @return Status code.
 */
static te_errno
parse_meas_ref(json_t *ref_obj, te_rgt_mi_meas *meas,
               ssize_t *idx)
{
    const char *type;
    const char *name;
    size_t i;

    if (!json_is_object(ref_obj))
        return TE_EINVAL;

    name = json_object_get_string(ref_obj, "name");
    type = json_object_get_string(ref_obj, "type");

    if (name != NULL && strcmp(name, "auto-seqno") == 0)
    {
        *idx = TE_RGT_MI_GRAPH_AXIS_AUTO_SEQNO;
        return 0;
    }

    for (i = 0; i < meas->params_num; i++)
    {
        if (strcmp_null(meas->params[i].type, type) == 0 &&
            strcmp_null(meas->params[i].name, name) == 0)
        {
            *idx = i;
            return 0;
        }
    }

    return TE_ENOENT;
}

/**
 * Parse line-graph view specification.
 *
 * @param view_obj      Description of the graph view in JSON.
 * @param mi            Structure describing MI artifact.
 * @param line_graph    Where to store data about the line-graph.
 *
 * @return Status code.
 */
static te_errno
get_line_graph(json_t *view_obj, te_rgt_mi *mi,
               te_rgt_mi_meas_view_line_graph *line_graph)
{
    json_t *axis_x;
    json_t *axis_y;
    json_t *axis_y_el;
    size_t axis_y_num = 0;
    ssize_t *axis_y_list = NULL;
    size_t i;
    ssize_t j;
    te_errno rc;

    te_rgt_mi_meas *meas = &mi->data.measurement;

    axis_x = json_object_get(view_obj, "axis_x");
    if (axis_x == NULL)
    {
        te_rgt_mi_parse_error(mi, TE_EINVAL,
                              "Failed to obtain 'axis_x' property "
                              "of a line-graph");
        return TE_EINVAL;
    }

    rc = parse_meas_ref(axis_x, meas, &line_graph->axis_x);
    if (rc != 0)
    {
        te_rgt_mi_parse_error(mi, rc,
                              "Failed to parse 'axis_x' property "
                              "of a line-graph");
        return rc;
    }
    else if (line_graph->axis_x >= 0)
    {
        meas->params[line_graph->axis_x].in_graph = TRUE;
    }

    axis_y = json_object_get(view_obj, "axis_y");
    if (axis_y != NULL)
    {
        if (!json_is_array(axis_y))
        {
            te_rgt_mi_parse_error(mi, TE_EINVAL,
                                  "'axis_y' field is not an array");
            return TE_EINVAL;
        }

        axis_y_num = json_array_size(axis_y);
        if (axis_y_num > 0)
        {
            axis_y_list = TE_ALLOC(axis_y_num * sizeof(*axis_y_list));
            if (axis_y_list == NULL)
                return TE_ENOMEM;

            for (i = 0; i < axis_y_num; i++)
            {
                axis_y_el = json_array_get(axis_y, i);
                if (axis_y_el == NULL)
                {
                    te_rgt_mi_parse_error(mi, TE_EFAIL,
                                          "Failed to obtain element %zu in "
                                          "'axis_y' array", i);
                    free(axis_y_list);
                    return TE_EFAIL;
                }

                rc = parse_meas_ref(axis_y_el, meas, &j);
                if (rc != 0)
                {
                    te_rgt_mi_parse_error(mi, rc,
                                          "Failed to parse element %zu in "
                                          "'axis_y' property of a "
                                          "line-graph", i);
                    return rc;
                }

                axis_y_list[i] = j;
                meas->params[j].in_graph = TRUE;
            }
        }
    }
    else
    {
        /*
         * Axis Y is not specified, it means that all parameters
         * except the one assigned to axis X are displayed on graph,
         * and therefore all parameters are used by graph on some axis.
         */
        for (i = 0; i < meas->params_num; i++)
        {
            meas->params[i].in_graph = TRUE;
        }
    }

    line_graph->axis_y = axis_y_list;
    line_graph->axis_y_num = axis_y_num;

    return 0;
}

/**
 * Get MI measurement views (specifying things like graphs).
 *
 * @param obj         JSON object with MI measurement.
 * @param mi          Structure describing MI artifact.
 *
 * @return Status code.
 */
static te_errno
get_views(json_t *obj, te_rgt_mi *mi)
{
    json_t *views_json;
    json_t *view_json;
    te_rgt_mi_meas *meas = &mi->data.measurement;
    te_rgt_mi_meas_view *views_aux;
    size_t num_aux;
    size_t i;
    te_errno rc;

    views_json = json_object_get(obj, "views");
    if (views_json == NULL)
        return 0;

    if (!json_is_array(views_json))
    {
        te_rgt_mi_parse_error(mi, TE_EINVAL,
                              "'views' field is not an array");
        return TE_EINVAL;
    }

    num_aux = json_array_size(views_json);
    if (num_aux == 0)
    {
        meas->views_num = 0;
        meas->views = NULL;
        return 0;
    }

    views_aux = calloc(num_aux, sizeof(*views_aux));
    if (views_aux == NULL)
        return TE_ENOMEM;

    for (i = 0; i < num_aux; i++)
    {
        view_json = json_array_get(views_json, i);
        if (view_json == NULL || !json_is_object(view_json))
        {
            te_rgt_mi_parse_error(mi, TE_EINVAL, "Cannot obtain view %d or "
                                  "it is not an object", i);
            te_rgt_mi_meas_views_clean(views_aux, i);
            free(views_aux);
            return TE_EINVAL;
        }

        views_aux[i].type = json_object_get_string(view_json, "type");
        views_aux[i].name = json_object_get_string(view_json, "name");
        views_aux[i].title = json_object_get_string(view_json, "title");

        if (views_aux[i].type == NULL)
        {
            te_rgt_mi_parse_error(mi, TE_EINVAL, "Cannot obtain view type");
            te_rgt_mi_meas_views_clean(views_aux, i);
            free(views_aux);
            return TE_EINVAL;
        }

        if (strcmp(views_aux[i].type, "line-graph") == 0)
        {
            rc = get_line_graph(view_json, mi,
                                &views_aux[i].data.line_graph);
            if (rc != 0)
            {
                te_rgt_mi_meas_views_clean(views_aux, i);
                free(views_aux);
                return TE_EINVAL;
            }
        }
    }

    meas->views = views_aux;
    meas->views_num = num_aux;

    return 0;
}

/* See description in mi_msg.h */
void
te_rgt_parse_mi_meas_message(te_rgt_mi *mi)
{
    json_t *root;
    json_t *results;
    json_t *result;

    size_t i;
    size_t param_id;
    size_t j;

    te_rgt_mi_meas *meas;

    mi->type = TE_RGT_MI_TYPE_MEASUREMENT;
    root = (json_t *)(mi->json_obj);

    meas = &mi->data.measurement;
    meas->tool = json_object_get_string(root, "tool");

    results = json_object_get(root, "results");
    if (results != NULL && json_is_array(results) &&
        json_array_size(results) > 0)
    {
        meas->params_num = json_array_size(results);
        meas->params = calloc(meas->params_num,
                              sizeof(te_rgt_mi_meas_param));
        if (meas->params == NULL)
        {
            mi->rc = TE_ENOMEM;
            goto cleanup;
        }

        param_id = 0;
        for (i = 0; i < meas->params_num; i++)
        {
            json_t *entries;
            te_bool no_stats;

            te_rgt_mi_meas_param *param = &meas->params[param_id];
            te_rgt_mi_meas_value value = TE_RGT_MI_MEAS_VALUE_INIT;
            size_t value_id = 0;

            result = json_array_get(results, i);
            if (result == NULL || !json_is_object(result))
                continue;

            param->name = json_object_get_string(result, "name");
            param->type = json_object_get_string(result, "type");
            param->descr = json_object_get_string(result, "description");
            param_id++;

            entries = json_object_get(result, "entries");
            if (entries == NULL || !json_is_array(entries) ||
                json_array_size(entries) == 0)
                continue;

            param->values_num = json_array_size(entries);
            param->values = calloc(param->values_num,
                                   sizeof(te_rgt_mi_meas_value));
            if (param->values == NULL)
            {
                mi->rc = TE_ENOMEM;
                goto cleanup;
            }

            for (j = 0; j < param->values_num; j++)
            {
                json_t *entry;
                const char *aggr;

                entry = json_array_get(entries, j);
                if (entry == NULL || !json_is_object(entry))
                    continue;

                aggr = json_object_get_string(entry, "aggr");
                if (aggr == NULL)
                    continue;

                value.defined = TRUE;
                if (json_object_get_number(entry, "value",
                                           &value.value) == 0)
                    value.specified = TRUE;

                value.multiplier = json_object_get_string(entry,
                                                          "multiplier");
                value.base_units = json_object_get_string(entry,
                                                          "base_units");

                no_stats = FALSE;
                if (strcmp(aggr, "min") == 0)
                {
                    param->min = value;
                }
                else if (strcmp(aggr, "max") == 0)
                {
                    param->max = value;
                }
                else if (strcmp(aggr, "mean") == 0)
                {
                    param->mean = value;
                }
                else if (strcmp(aggr, "median") == 0)
                {
                    param->median = value;
                }
                else if (strcmp(aggr, "stdev") == 0)
                {
                    param->stdev = value;
                }
                else if (strcmp(aggr, "cv") == 0)
                {
                    param->cv = value;
                }
                else if (strcmp(aggr, "out of range") == 0)
                {
                    param->out_of_range = value;
                }
                else if (strcmp(aggr, "percentile") == 0)
                {
                    param->percentile = value;
                }
                else
                {
                    if (strcmp(aggr, "single") == 0)
                    {
                        param->values[value_id] = value;
                        value_id++;
                    }
                    no_stats = TRUE;
                }

                if (!no_stats)
                    param->stats_present = TRUE;
            }

            param->values_num = value_id;
        }

        meas->params_num = param_id;
    }

    mi->rc = get_child_keys(root, "keys", &meas->keys,
                            &meas->keys_num);
    if (mi->rc != 0)
        goto cleanup;

    mi->rc = get_child_keys(root, "comments", &meas->comments,
                            &meas->comments_num);
    if (mi->rc != 0)
        goto cleanup;

    mi->rc = get_views(root, mi);

cleanup:
    if (mi->rc != 0)
        te_rgt_mi_clean(mi);

    return;
}

/**
 * Check if JSON object has expected type.
 *
 * The lack of value (null pointer or null json object) always passes
 * the check.
 *
 * In case of mismatch, an error will be signaled in the MI object.
 *
 * @param mi                MI object.
 * @param json              JSON object.
 * @param expected_type     The type the JSON object is expected to have.
 * @param field_name        Field name, can be used in the error message.
 *
 * @returns Boolean flag that signals successful type check.
 */
static te_bool
check_json_type(te_rgt_mi *mi, json_t **json, json_type expected_type,
                const char *field_name)
{
    json_type actual_type;

    if (*json != NULL && json_typeof(*json) == JSON_NULL)
        *json = NULL;

    if (*json == NULL)
        return TRUE;

    actual_type = json_typeof(*json);
    if (actual_type != expected_type)
    {
        te_rgt_mi_parse_error(mi, TE_EINVAL, "Unexpected type for field \"%s\":"
                              " expected %s, got %s", field_name,
                              json_type_string(expected_type),
                              json_type_string(actual_type));
        return FALSE;
    }

    return TRUE;
}

/** Parse "test_start" MI message */
static void
te_rgt_parse_mi_test_start_message(te_rgt_mi *mi)
{
    int     ret;
    size_t  i;

    json_t *root;
    json_t *name = NULL;
    json_t *params = NULL;
    json_t *authors = NULL;
    json_t *objective = NULL;
    json_t *page = NULL;
    json_t *tin = NULL;
    json_t *hash = NULL;
    json_t *item;

    json_error_t err;

    te_rgt_mi_test_start *data;

    mi->type = TE_RGT_MI_TYPE_TEST_START;
    data = &mi->data.test_start;

    root = (json_t *)(mi->json_obj);
    root = json_object_get(root, "msg");
    if (root == NULL)
    {
        te_rgt_mi_parse_error(mi, TE_EINVAL,
                              "Failed to get the \"msg\" field from the "
                              "test_start message");
        goto cleanup;
    }

    ret = json_unpack_ex(root, &err, JSON_STRICT,
                         "{s:i, s:i, s:s, s?o, s?o, s?o, s?o, s?o, s?o, s?o}",
                         "id", &data->node_id,
                         "parent", &data->parent_id,
                         "node_type", &data->node_type,
                         "name", &name,
                         "params", &params,
                         "authors", &authors,
                         "objective", &objective,
                         "page", &page,
                         "tin", &tin,
                         "hash", &hash);
    if (ret != 0)
    {
        te_rgt_mi_parse_error(mi, TE_EINVAL,
                              "Error unpacking test_start JSON log message: %s"
                              " (line %d, column %d)", err.text, err.line,
                              err.column);
        goto cleanup;
    }

    if (!check_json_type(mi, &name, JSON_STRING, "name") ||
        !check_json_type(mi, &params, JSON_ARRAY, "params") ||
        !check_json_type(mi, &authors, JSON_ARRAY, "authors") ||
        !check_json_type(mi, &objective, JSON_STRING, "objective") ||
        !check_json_type(mi, &page, JSON_STRING, "page") ||
        !check_json_type(mi, &tin, JSON_INTEGER, "tin") ||
        !check_json_type(mi, &hash, JSON_STRING, "hash"))
        goto cleanup;

    if (name != NULL)
        data->name = json_string_value(name);
    if (objective != NULL)
        data->objective = json_string_value(objective);
    if (page != NULL)
        data->page = json_string_value(page);
    if (tin != NULL)
        data->tin = json_integer_value(tin);
    else
        data->tin = -1;
    if (hash != NULL)
        data->hash = json_string_value(hash);

    if (params != NULL)
    {
        data->params_num = json_array_size(params);
        if (data->params_num == 0)
        {
            te_rgt_mi_parse_error(mi, TE_EINVAL,
                                  "test_start parameter list cannot be an "
                                  "empty array. If there are no arguments,"
                                  "this field must be omitted");
            goto cleanup;
        }
        data->params = TE_ALLOC(sizeof(te_rgt_mi_kv) * data->params_num);
        if (data->params == NULL)
        {
            te_rgt_mi_parse_error(mi, TE_ENOMEM,
                                  "Failed to allocate memory for test_start params");
            goto cleanup;
        }

        json_array_foreach(params, i, item)
        {
            ret = json_unpack_ex(item, &err, JSON_STRICT,
                                 "[ss]",
                                 &data->params[i].key,
                                 &data->params[i].value);
            if (ret != 0)
            {
                te_rgt_mi_parse_error(mi, TE_EINVAL,
                                      "Error unpacking JSON param object: %s"
                                      " (line %d, column %d)", err.text,
                                      err.line, err.column);
                goto cleanup;
            }
        }
    }

    if (authors != NULL)
    {
        data->authors_num = json_array_size(authors);
        data->authors = TE_ALLOC(sizeof(te_rgt_mi_person) * data->authors_num);
        if (data->authors == NULL)
        {
            te_rgt_mi_parse_error(mi, TE_ENOMEM,
                                  "Failed to allocate memory for authors");
            goto cleanup;
        }

        json_array_foreach(authors, i, item)
        {
            json_t *name  = NULL;
            json_t *email = NULL;

            ret = json_unpack_ex(item, &err, JSON_STRICT,
                                 "{s?o, s?o}",
                                 "name",  &name,
                                 "email", &email);
            if (ret != 0)
            {
                te_rgt_mi_parse_error(mi, TE_EINVAL,
                                      "Error unpacking JSON param object: %s"
                                      " (line %d, column %d)", err.text,
                                      err.line, err.column);
                goto cleanup;
            }

            if (!check_json_type(mi, &name, JSON_STRING, "authors.name") ||
                !check_json_type(mi, &email, JSON_STRING, "authors.email"))
                goto cleanup;

            if (name != NULL)
                data->authors[i].name = json_string_value(name);
            if (email != NULL)
                data->authors[i].email = json_string_value(email);
        }
    }

cleanup:
    if (mi->rc != 0)
        te_rgt_mi_clean(mi);
}

/**
 * Parse test result object.
 *
 * @param mi            MI object.
 * @param json          JSON result object.
 * @param result        Where to put the parsing result.
 */
static te_errno
te_rgt_parse_mi_test_result(te_rgt_mi *mi, json_t *json,
                            te_rgt_mi_test_result *result)
{
    int           ret;
    json_t       *verdicts = NULL;
    json_t       *verdict = NULL;
    json_t       *notes = NULL;
    json_t       *key = NULL;
    json_error_t  err;

    if (!check_json_type(mi, &json, JSON_OBJECT, "result") ||
        result == NULL)
        return TE_EINVAL;


    ret = json_unpack_ex(json, &err, JSON_STRICT,
                         "{s:s, s?o, s?o, s?o}",
                         "status", &result->status,
                         "verdicts", &verdicts,
                         "notes", &notes,
                         "key", &key);
    if (ret != 0)
    {
        te_rgt_mi_parse_error(mi, TE_EINVAL,
                              "Error unpacking JSON result object: %s"
                              " (line %d, column %d)", err.text, err.line,
                              err.column);
        return TE_EINVAL;
    }

    if (!check_json_type(mi, &verdicts, JSON_ARRAY, "result.verdicts") ||
        !check_json_type(mi, &notes, JSON_STRING, "result.notes") ||
        !check_json_type(mi, &key, JSON_STRING, "result.key"))
        return TE_EINVAL;

    if (notes != NULL)
        result->notes = json_string_value(notes);
    if (key != NULL)
        result->key = json_string_value(key);

    if (verdicts != NULL)
    {
        size_t i;

        result->verdicts_num = json_array_size(verdicts);
        result->verdicts = TE_ALLOC(sizeof(const char *) * result->verdicts_num);
        if (result->verdicts == NULL)
        {
            te_rgt_mi_parse_error(mi, TE_ENOMEM,
                                  "Failed to allocate memory for verdicts");
            return TE_ENOMEM;
        }

        json_array_foreach(verdicts, i, verdict)
        {
            if (!check_json_type(mi, &verdict, JSON_STRING, "verdict") ||
                verdict == NULL)
                return TE_EINVAL;

            result->verdicts[i] = json_string_value(verdict);
            if (result->verdicts[i] == NULL)
            {
                te_rgt_mi_parse_error(mi, TE_EINVAL,
                                      "Failed to extract verdict string");
                return TE_EINVAL;
            }
        }
    }

    return 0;
}

/** Parse "test_end" MI message */
static void
te_rgt_parse_mi_test_end_message(te_rgt_mi *mi)
{
    int      ret;
    te_errno rc;
    size_t   i;

    json_t *root;
    json_t *obtained = NULL;
    json_t *expected = NULL;
    json_t *result = NULL;
    json_t *tags_expr = NULL;
    json_t *error = NULL;

    json_error_t err;

    te_rgt_mi_test_end *data;

    mi->type = TE_RGT_MI_TYPE_TEST_END;
    data = &mi->data.test_end;

    root = (json_t *)(mi->json_obj);
    root = json_object_get(root, "msg");
    if (root == NULL)
    {
        te_rgt_mi_parse_error(mi, TE_EINVAL,
                              "Failed to get the \"msg\" field from the "
                              "test_end message");
        goto cleanup;
    }

    ret = json_unpack_ex(root, &err, JSON_STRICT,
                         "{s:i, s:i, s?s, s?o, s?o, s?o, s?o}",
                         "id", &data->node_id,
                         "parent", &data->parent_id,
                         "status", &data->obtained.status,
                         "obtained", &obtained,
                         "expected", &expected,
                         "tags_expr", &tags_expr,
                         "error", &error);
    if (ret != 0)
    {
        te_rgt_mi_parse_error(mi, TE_EINVAL,
                              "Error unpacking test_end JSON log message: %s"
                              " (line %d, column %d)", err.text, err.line,
                              err.column);
        goto cleanup;
    }

    if (!check_json_type(mi, &obtained, JSON_OBJECT, "obtained") ||
        !check_json_type(mi, &expected, JSON_ARRAY, "expected") ||
        !check_json_type(mi, &tags_expr, JSON_STRING, "tags_expr") ||
        !check_json_type(mi, &error, JSON_STRING, "error"))
        goto cleanup;

    if (obtained != NULL)
    {
        rc = te_rgt_parse_mi_test_result(mi, obtained, &data->obtained);
        if (rc != 0)
            goto cleanup;
    }

    if (expected != NULL)
    {
        data->expected_num = json_array_size(expected);
        data->expected = TE_ALLOC(sizeof(te_rgt_mi_test_result) *
                                  data->expected_num);
        if (data->expected == NULL)
        {
            te_rgt_mi_parse_error(mi, TE_ENOMEM,
                                  "Failed to allocate memory for expected results");
            goto cleanup;
        }

        json_array_foreach(expected, i, result)
        {
            rc = te_rgt_parse_mi_test_result(mi, result, &data->expected[i]);
            if (rc != 0)
                goto cleanup;
        }
    }

    if (tags_expr != NULL)
        data->tags_expr = json_string_value(tags_expr);
    if (error != NULL)
        data->error = json_string_value(error);

cleanup:
    if (mi->rc != 0)
        te_rgt_mi_clean(mi);
}

#endif /* HAVE_LIBJANSSON */

/* See description in mi_msg.h */
void
te_rgt_parse_mi_message(const char *json_buf, size_t buf_len,
                        te_rgt_mi *mi)
{
    memset(mi, 0, sizeof(*mi));

#ifndef HAVE_LIBJANSSON
    mi->rc = TE_EOPNOTSUPP;
    return;
#else
    json_t *root;
    const char *type;
    json_error_t error;

    mi->type = TE_RGT_MI_TYPE_UNKNOWN;

    root = json_loadb(json_buf, buf_len, 0, &error);
    if (root == NULL)
    {
        mi->parse_failed = TRUE;
        snprintf(mi->parse_err, sizeof(mi->parse_err),
                 "%s", error.text);
        mi->rc = TE_EINVAL;
        return;
    }

    mi->json_obj = root;

    type = json_object_get_string(root, "type");

    if (type == NULL)
    {
        /* No type - handle as generic JSON */
    }
    else if (strcmp(type, "measurement") == 0)
    {
        te_rgt_parse_mi_meas_message(mi);
    }
    else if (strcmp(type, "test_start") == 0)
    {
        te_rgt_parse_mi_test_start_message(mi);
    }
    else if (strcmp(type, "test_end") == 0)
    {
        te_rgt_parse_mi_test_end_message(mi);
    }
    else
    {
        /* Unknown type - handle as generic JSON */
    }
#endif /* HAVE_LIBJANSSON */
}

/* See description in mi_msg.h */
const char *
te_rgt_mi_meas_param_name(te_rgt_mi_meas_param *param)
{
    if (param->descr != NULL && *(param->descr) != '\0')
        return param->descr;

    if (param->name != NULL && *(param->name) != '\0')
        return param->name;

    return param->type;
}
