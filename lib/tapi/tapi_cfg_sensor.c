/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Ltd. All rights reserved. */
/** @file
 * @brief Test API to get info about sensors.
 *
 * Implementation of API to get info about sensors.
 */

#define TE_LGR_USER      "Conf sensors TAPI"

#include "te_config.h"

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif
#ifdef HAVE_MATH_H
#include <math.h>
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "te_str.h"
#include "te_string.h"

#include "conf_api.h"
#include "tapi_cfg.h"
#include "tapi_cfg_base.h"
#include "tapi_cfg_sensor.h"

static te_errno
tapi_cfg_sensor_get(const char *ta, const char *sensor_name,
                    const char *data_id_str, const char *threshold_id,
                    const char *param_name, double *value)
{
    char *value_str;
    te_errno rc;

    rc = cfg_get_instance_string_fmt(&value_str,
                                  "/agent:%s/hardware:/sensor:%s/data:"
                                  "%s%s%s/%s:", ta, sensor_name,
                                  data_id_str,
                                  threshold_id == NULL ? "" : "/threshold:",
                                  threshold_id == NULL ? "" : threshold_id,
                                  param_name);
    if (rc != 0)
    {
        ERROR("Failed to get %s property of sensor %s, data_id %s%s%s:"
              " error %r", param_name, sensor_name, data_id_str,
              threshold_id == NULL ? "" : ", threshold ",
              threshold_id == NULL ? "" : threshold_id, rc);
        return rc;
    }

    if (*value_str == '\0')
    {
        /* Value is not set */
        *value = NAN;
        return 0;
    }
    else
    {
        return te_strtod(value_str, value);
    }
}

te_errno
tapi_cfg_sensor_get_value(const char *ta, const char *sensor_name,
                          const char *data_id_str, double *value)
{
    return tapi_cfg_sensor_get(ta, sensor_name, data_id_str, NULL, "value",
                               value);
}

te_errno
tapi_cfg_sensor_get_min(const char *ta, const char *sensor_name,
                        const char *data_id_str, double *value)
{
    return tapi_cfg_sensor_get(ta, sensor_name, data_id_str, NULL, "min",
                               value);
}

te_errno
tapi_cfg_sensor_get_max(const char *ta, const char *sensor_name,
                        const char *data_id_str, double *value)
{
    return tapi_cfg_sensor_get(ta, sensor_name, data_id_str, NULL, "max",
                               value);
}

te_errno
tapi_cfg_sensor_get_threshold(const char *ta, const char *sensor_name,
                              const char *data_id_str,
                              const char *threshold_id, double *value)
{
    return tapi_cfg_sensor_get(ta, sensor_name, data_id_str, threshold_id,
                               "value", value);
}
