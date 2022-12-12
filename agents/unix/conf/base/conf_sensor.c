/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Ltd. All rights reserved. */
/** @file
 * @brief Sensor data.
 *
 * Sensor-related functions for Configurators.
 */

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

/* libsensors */
#include <sensors/sensors.h>
#include <sensors/error.h>

#include "te_alloc.h"
#include "te_defs.h"
#include "te_errno.h"
#include "te_str.h"
#include "te_string.h"
#include "logger_api.h"

#include "rcf_ch_api.h"
#include "rcf_pch.h"

#if HAVE_CTYPE_H
#include <ctype.h>
#endif

#define SENSOR_NAME_MAX_LEN       512
#define SENSOR_SUBF_DESC_MAX_LEN  64
#define SENSOR_ARRAY_MAX_SIZE     16

/* Tuple for sensor minimum/maximum values */
typedef struct sensor_value_record {
    char chip_name[SENSOR_NAME_MAX_LEN];
    char feature_name[SENSOR_NAME_MAX_LEN];
    double value;
} sensor_value_record;

/* Description for sensor subfeature types */
typedef struct sensor_subfeature_type_desc {
    sensors_subfeature_type type;
    char description[SENSOR_SUBF_DESC_MAX_LEN];
} sensor_subfeature_type_desc;

/*
 * NULL-terminated arrays of pointers to structure with
 * minimum and maximum observed sensor values
 */
static sensor_value_record *min_values[SENSOR_ARRAY_MAX_SIZE];
static sensor_value_record *max_values[SENSOR_ARRAY_MAX_SIZE];

static const sensor_subfeature_type_desc threshold_types[] = {
    { SENSORS_SUBFEATURE_TEMP_MIN,  "low" },
    { SENSORS_SUBFEATURE_TEMP_MAX,  "high" },
    { SENSORS_SUBFEATURE_TEMP_CRIT, "crit" }
};

static te_errno sensor_list(unsigned int gid, const char *oid,
                            const char *sub_id, char **list);

static te_errno sensor_data_list(unsigned int gid, const char *oid,
                                 const char *sub_id, char **list,
                                 const char *unused1,
                                 const char *sensor_str);

static te_errno
sensor_get_subfeature(const char *sensor_str, const char *data_id_str,
                      const sensors_subfeature_type *subfeature_types,
                      size_t sf_types_len, const char *function_name,
                      char *value);

static int extract_last_int(const char* s);

static te_errno sensor_min_get(unsigned int gid, const char *oid,
                               char *value, const char *unused1,
                               const char *sensor_str,
                               const char *data_id_str);

static te_errno sensor_max_get(unsigned int gid, const char *oid,
                               char *value, const char *unused1,
                               const char *sensor_str,
                               const char *data_id_str);

static int get_value_by_name(sensor_value_record **records,
                             const char *chip_name,
                             const char *feature_name, double *value);

static void update_value_by_name(sensor_value_record **records,
                                 const char *chip_name,
                                 const char *feature_name,
                                 double new_value);

static te_errno sensor_value_get(unsigned int gid, const char *oid,
                                 char *value, const char *unused1,
                                 const char *sensor_str,
                                 const char *data_id_str);

static te_errno sensor_threshold_list(unsigned int gid, const char *oid,
                                      const char *sub_id, char **list,
                                      const char *unused1,
                                      const char *sensor_str,
                                      const char *data_id_str);

static te_errno sensor_threshold_value_get(unsigned int gid,
                                           const char *oid, char *value,
                                           const char *unused1,
                                           const char *sensor_str,
                                           const char *data_id_str,
                                           const char *thr_desc);

te_errno ta_unix_conf_sensor_init(void);

te_errno ta_unix_conf_sensor_cleanup(void);

static te_errno
sensor_list(unsigned int gid, const char *oid, const char *sub_id,
            char **list)
{
    te_string result = TE_STRING_INIT;
    te_bool first = TRUE;
    te_errno rc = 0;
    int chip_number = 0;
    int err;
    char chip_name[SENSOR_NAME_MAX_LEN];
    const sensors_chip_name *chip;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    while ((chip = sensors_get_detected_chips(NULL, &chip_number)) != NULL)
    {
        err = sensors_snprintf_chip_name(chip_name, sizeof(chip_name),
                                         chip);
        if (err < 0)
        {
            ERROR("sensor_list: sensors_snprintf_chip_name failed: %s",
                  sensors_strerror(err));
            te_string_free(&result);
            return rc;
        }
        te_string_append(&result, "%s%s", first ? "" : " ", chip_name);
        first = FALSE;
    }

    *list = result.ptr;
    return 0;
}

static te_errno
sensor_data_list(unsigned int gid, const char *oid, const char *sub_id,
                 char **list, const char *unused1, const char *sensor_str)
{
    te_string result = TE_STRING_INIT;
    te_errno rc = 0;
    int chip_nr = 0;
    int feature_nr = 0;
    int err;
    unsigned int id = 0;
    char *feature_label;
    sensors_chip_name match;
    const sensors_chip_name *chip;
    const sensors_feature *feature;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);
    UNUSED(unused1);

    err = sensors_parse_chip_name(sensor_str, &match);
    if (err != 0)
    {
        ERROR("sensor_data_list: sensors_parse_chip_name failed for "
              "chip name '%s': %s", sensor_str, sensors_strerror(err));
        te_string_free(&result);
        return rc;
    }
    chip = sensors_get_detected_chips(&match, &chip_nr);
    if (chip == NULL)
    {
        te_string_free(&result);
        sensors_free_chip_name(&match);
        return rc;
    }

    while ((feature = sensors_get_features(chip, &feature_nr)) != NULL)
    {
        feature_label = sensors_get_label(chip, feature);
        if (feature_label != NULL)
        {
            te_string_append(&result, "%s%s_%u", id == 0 ? "" : " ",
                             feature_label, id);
            free(feature_label);
        }
        else
        {
            te_string_append(&result, "%s%u", id == 0 ? "" : " ", id);
        }
        if (rc != 0)
        {
            te_string_free(&result);
            sensors_free_chip_name(&match);
            return rc;
        }
        id++;
    }

    *list = result.ptr;
    sensors_free_chip_name(&match);
    return 0;
}

static te_errno
sensor_get_subfeature(const char *sensor_str, const char *data_id_str,
                      const sensors_subfeature_type *subfeature_types,
                      size_t sf_types_len, const char *function_name,
                      char *value)
{
    int chip_nr = 0;
    int feature_nr;
    int err;
    int i;
    double subfeature_value;
    sensors_chip_name match;
    const sensors_chip_name *chip;
    const sensors_feature *feature;
    const sensors_subfeature *subfeature;

    err = sensors_parse_chip_name(sensor_str, &match);
    if (err)
    {
        ERROR("%s: sensors_parse_chip_name failed for chip name '%s': %s",
              function_name, sensor_str, sensors_strerror(err));
        return 0;
    }

    chip = sensors_get_detected_chips(&match, &chip_nr);
    if (chip == NULL)
    {
        snprintf(value, RCF_MAX_VAL, "%s", "");
        sensors_free_chip_name(&match);
        return 0;
    }

    feature_nr = extract_last_int(data_id_str);
    if (feature_nr == -1)
    {
        ERROR("%s: invalid data_id_str: %s", function_name, data_id_str);
        sensors_free_chip_name(&match);
        return 0;
    }
    feature = sensors_get_features(chip, &feature_nr);

    for (i = 0; i < sf_types_len; i++)
    {
        subfeature = sensors_get_subfeature(chip, feature,
                                            subfeature_types[i]);
        if (subfeature != NULL)
            break;
    }
    if (subfeature != NULL)
    {
        err = sensors_get_value(chip, subfeature->number,
                                &subfeature_value);
        if (err)
        {
            ERROR("%s: sensors_get_value failed: %s", function_name,
                  sensors_strerror(err));
            sensors_free_chip_name(&match);
            return 0;
        }
        snprintf(value, RCF_MAX_VAL, "%f", subfeature_value);
    }
    else
    {
        snprintf(value, RCF_MAX_VAL, "%s", "");
    }

    sensors_free_chip_name(&match);
    return 0;
}

/*
 * Extracts a non-negative integral value from the end of string s.
 * Returns:
 *   on success, the int value scanned;
 *   -1, if the string is NULL, empty, or not terminated by a number
 */
static int
extract_last_int(const char* s)
{
    int value = -1;
    const char *scanstr;

    if (s != NULL && *s != '\0')
    {
        scanstr = s + strlen(s) - 1;
        if (!isdigit(*scanstr))
        {
            return -1;
        }
        while (scanstr > s && isdigit(*(scanstr-1)))
        {
            scanstr--;
        }
        sscanf(scanstr, "%d", &value);
    }
    return value;
}

static te_errno
sensor_min_get(unsigned int gid, const char *oid, char *value,
               const char *unused1, const char *sensor_str,
               const char *data_id_str)
{
    double val;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(unused1);

    if (get_value_by_name(min_values, sensor_str, data_id_str, &val) == 0)
        snprintf(value, RCF_MAX_VAL, "%f", val);
    else
        snprintf(value, RCF_MAX_VAL, "%s", "");

    return 0;
}

static te_errno
sensor_max_get(unsigned int gid, const char *oid, char *value,
               const char *unused1, const char *sensor_str,
               const char *data_id_str)
{
    double val;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(unused1);

    if (get_value_by_name(max_values, sensor_str, data_id_str, &val) == 0)
        snprintf(value, RCF_MAX_VAL, "%f", val);
    else
        snprintf(value, RCF_MAX_VAL, "%s", "");

    return 0;
}

static int
get_value_by_name(sensor_value_record **records, const char *chip_name,
                  const char *feature_name, double *value)
{
    int i;

    for (i = 0; records[i] != NULL; i++)
    {
        if (strcmp(records[i]->chip_name, chip_name) == 0 &&
            strcmp(records[i]->feature_name, feature_name) == 0)
        {
            *value = records[i]->value;
            return 0;
        }
    }
    return -1;
}

static void
update_value_by_name(sensor_value_record **records, const char *chip_name,
                     const char *feature_name, double new_value)
{
    int i;
    sensor_value_record *new_record;

    for (i = 0; records[i] != NULL; i++)
    {
        if (strcmp(records[i]->chip_name, chip_name) == 0 &&
            strcmp(records[i]->feature_name, feature_name) == 0)
        {
            /* Update existing record */
            records[i]->value = new_value;
            return;
        }
    }

    if (i == SENSOR_ARRAY_MAX_SIZE - 1)
    {
        ERROR("Too many sensors for updating minimum/maximum values");
        return;
    }

    /* Append new record */
    new_record = TE_ALLOC(sizeof(sensor_value_record));
    te_strlcpy(new_record->chip_name, chip_name, SENSOR_NAME_MAX_LEN);
    te_strlcpy(new_record->feature_name, feature_name, SENSOR_NAME_MAX_LEN);
    new_record->value = new_value;
    records[i] = new_record;
    records[i + 1] = NULL;
}

static te_errno
sensor_value_get(unsigned int gid, const char *oid, char *value,
                 const char *unused1, const char *sensor_str,
                 const char *data_id_str)
{
    te_errno rc;
    double new_value;
    double min;
    double max;
    te_bool is_min_set;
    te_bool is_max_set;

    /* There are only some types; for more see sensors/sensors.h */
    static const sensors_subfeature_type subf_types[] = {
        SENSORS_SUBFEATURE_IN_INPUT, SENSORS_SUBFEATURE_FAN_INPUT,
        SENSORS_SUBFEATURE_TEMP_INPUT, SENSORS_SUBFEATURE_POWER_INPUT,
        SENSORS_SUBFEATURE_CURR_INPUT
    };

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(unused1);

    rc = sensor_get_subfeature(sensor_str, data_id_str, subf_types,
                               TE_ARRAY_LEN(subf_types), "sensor_value_get",
                               value);
    if (rc != 0 || strlen(value) == 0)
        return rc;

    te_strtod(value, &new_value);

    /* Update sensor/data/min */
    is_min_set = (get_value_by_name(min_values, sensor_str, data_id_str,
                                    &min) == 0);
    if (!is_min_set || new_value < min)
    {
        update_value_by_name(min_values, sensor_str, data_id_str,
                             new_value);
    }

    /* Update sensor/data/max */
    is_max_set = (get_value_by_name(max_values, sensor_str, data_id_str,
                                    &max) == 0);
    if (!is_max_set || new_value > max)
    {
        update_value_by_name(max_values, sensor_str, data_id_str,
                             new_value);
    }

    return 0;
}

static te_errno
sensor_threshold_list(unsigned int gid, const char *oid, const char *sub_id,
                      char **list, const char *unused1,
                      const char *sensor_str, const char *data_id_str)
{
    te_string result = TE_STRING_INIT;
    te_bool first = TRUE;
    te_errno rc;
    int i;
    char value[RCF_MAX_VAL];
    sensors_subfeature_type subf_types[1];

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);
    UNUSED(unused1);

    for (i = 0; i < TE_ARRAY_LEN(threshold_types); i++)
    {
        subf_types[0] = threshold_types[i].type;
        strcpy(value, "");
        rc = sensor_get_subfeature(sensor_str, data_id_str, subf_types, 1,
                                   "sensor_threshold_list", value);
        if (rc != 0)
        {
            te_string_free(&result);
            return rc;
        }
        if (strlen(value) == 0)
            continue;

        te_string_append(&result, "%s%s", first ? "" : " ",
                         threshold_types[i].description);
        first = FALSE;
    }

    *list = result.ptr;
    return 0;
}

static te_errno
sensor_threshold_value_get(unsigned int gid, const char *oid, char *value,
                           const char *unused1, const char *sensor_str,
                           const char *data_id_str, const char *thr_desc)
{
    int i;
    sensors_subfeature_type subf_types[1];

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(unused1);

    for (i = 0; i < TE_ARRAY_LEN(threshold_types); i++)
    {
        if (strcmp(threshold_types[i].description, thr_desc) == 0)
        {
            subf_types[0] = threshold_types[i].type;
            return sensor_get_subfeature(sensor_str, data_id_str,
                                         subf_types, 1,
                                         "sensor_threshold_value_get",
                                         value);
        }
    }

    snprintf(value, RCF_MAX_VAL, "%s", "");
    return 0;
}

RCF_PCH_CFG_NODE_RO(node_sensor_threshold_value, "value", NULL, NULL,
                    sensor_threshold_value_get);

RCF_PCH_CFG_NODE_RO_COLLECTION(node_sensor_threshold, "threshold",
                               &node_sensor_threshold_value, NULL, NULL,
                               sensor_threshold_list);

RCF_PCH_CFG_NODE_RO(node_sensor_value, "value", NULL,
                    &node_sensor_threshold, sensor_value_get);

RCF_PCH_CFG_NODE_RO(node_sensor_max, "max", NULL, &node_sensor_value,
                    sensor_max_get);

RCF_PCH_CFG_NODE_RO(node_sensor_min, "min", NULL, &node_sensor_max,
                    sensor_min_get);

RCF_PCH_CFG_NODE_RO_COLLECTION(node_sensor_data, "data", &node_sensor_min,
                               NULL, NULL, sensor_data_list);

RCF_PCH_CFG_NODE_RO_COLLECTION(node_sensor, "sensor", &node_sensor_data,
                               NULL, NULL, sensor_list);

te_errno
ta_unix_conf_sensor_init(void)
{
    te_errno rc = 0;
    int err;

    min_values[0] = NULL;
    max_values[0] = NULL;

    err = sensors_init(NULL);
    if (err)
    {
        ERROR("Failed lo initialize libsensors library: %s",
              sensors_strerror(err));
        return rc;
    }

    return rcf_pch_add_node("/agent/hardware", &node_sensor);
}

te_errno
ta_unix_conf_sensor_cleanup(void)
{
    int i;

    for (i = 0; min_values[i] != NULL; i++)
        free(min_values[i]);
    for (i = 0; max_values[i] != NULL; i++)
        free(max_values[i]);

    return 0;
}
