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
 * Get MI measurement views (specifying things like graphs).
 *
 * @param obj         JSON object with MI measurement.
 * @param views       Where to save pointer to array of parsed
 *                    views.
 * @param num         Where to save number of parsed views.
 *
 * @return Status code.
 */
static te_errno
get_views(json_t *obj, te_rgt_mi_meas_view **views, size_t *num)
{
    json_t *views_json;
    json_t *view_json;
    te_rgt_mi_meas_view *views_aux;
    size_t num_aux;
    size_t i;

    views_json = json_object_get(obj, "views");
    if (views_json == NULL)
        return 0;

    if (!json_is_array(views_json))
        return TE_EINVAL;

    num_aux = json_array_size(views_json);
    if (num_aux == 0)
    {
        *num = 0;
        *views = NULL;
        return 0;
    }

    views_aux = calloc(num_aux, sizeof(*views_aux));
    if (views_aux == NULL)
        return TE_ENOMEM;

    for (i = 0; i < num_aux; i++)
    {
        view_json = json_array_get(views_json, i);
        if (view_json == NULL)
        {
            /* This should not happen */
            free(views_aux);
            return TE_EFAIL;
        }

        if (!json_is_object(view_json))
        {
            free(views_aux);
            return TE_EINVAL;
        }

        views_aux[i].type = json_object_get_string(view_json, "type");
        views_aux[i].name = json_object_get_string(view_json, "name");
    }

    *views = views_aux;
    *num = num_aux;

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

    mi->rc = get_views(root, &meas->views, &meas->views_num);

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
                         "{s:i, s:i, s:s, s?o, s?o, s?o, s?o, s?o, s?o}",
                         "id", &data->node_id,
                         "parent", &data->parent_id,
                         "node_type", &data->node_type,
                         "name", &name,
                         "params", &params,
                         "authors", &authors,
                         "objective", &objective,
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
        !check_json_type(mi, &tin, JSON_INTEGER, "tin") ||
        !check_json_type(mi, &hash, JSON_STRING, "hash"))
        goto cleanup;

    if (name != NULL)
        data->name = json_string_value(name);
    if (objective != NULL)
        data->objective = json_string_value(objective);
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

/** Parse "test_end" MI message */
static void
te_rgt_parse_mi_test_end_message(te_rgt_mi *mi)
{
    int     ret;

    json_t *root;
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
                         "{s:i, s:i, s:s, s?o}",
                         "id", &data->node_id,
                         "parent", &data->parent_id,
                         "status", &data->obtained.status,
                         "error", &error);
    if (ret != 0)
    {
        te_rgt_mi_parse_error(mi, TE_EINVAL,
                              "Error unpacking test_end JSON log message: %s"
                              " (line %d, column %d)", err.text, err.line,
                              err.column);
        goto cleanup;
    }

    if (!check_json_type(mi, &error, JSON_STRING, "error"))
        goto cleanup;

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
    if (param->name != NULL && *(param->name) != '\0')
        return param->name;

    if (param->type == NULL || *(param->type) == '\0')
        return "[unknown]";

    if (strcmp(param->type, "pps") == 0)
        return "Packets per second";
    else if (strcmp(param->type, "latency") == 0)
        return "Latency in seconds";
    else if (strcmp(param->type, "throughput") == 0)
        return "Throughput in bits per second";
    else if (strcmp(param->type, "bandwidth-usage") == 0)
        return "Bandwidth usage ratio";
    else if (strcmp(param->type, "temperature") == 0)
        return "Temperature in degrees Celsius";
    else if (strcmp(param->type, "rps") == 0)
        return "Requests per second";
    else if (strcmp(param->type, "rtt") == 0)
        return "Round trip time in seconds";

    return param->type;
}
