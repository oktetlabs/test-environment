/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Ltd. All rights reserved. */
/** @file
 * @brief Test API to get info about sensors.
 *
 * Definition of API to get info about sensors.
 */

#ifndef __TE_TAPI_CFG_SENSOR_H__
#define __TE_TAPI_CFG_SENSOR_H__

#include "conf_api.h"
#include "te_defs.h"
#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup tapi_conf_sensor Sensor information for Test Agents
 * @ingroup tapi_conf
 * @{
 */

/**
 * Gets value of specified sensor data (feature) on a test agent.
 *
 * @param[in]  ta               test agent
 * @param[in]  sensor_name      sensor name
 * @param[in]  data_id_str      feature label + underscore + feature number
 * @param[out] value            value of sensor's feature
 *
 * @return status code
 */
extern te_errno tapi_cfg_sensor_get_value(const char *ta,
                                          const char *sensor_name,
                                          const char *data_id_str,
                                          double *value);

/**
 * Gets minimum value of specified sensor data (feature) on a test agent.
 *
 * @param[in]  ta               test agent
 * @param[in]  sensor_name      sensor name
 * @param[in]  data_id_str      feature label + underscore + feature number
 * @param[out] value            minimum value of sensor's feature
 *
 * @return status code
 */
extern te_errno tapi_cfg_sensor_get_min(const char *ta,
                                        const char *sensor_name,
                                        const char *data_id_str,
                                        double *value);

/**
 * Gets maximum value of specified sensor data (feature) on a test agent.
 *
 * @param[in]  ta               test agent
 * @param[in]  sensor_name      sensor name
 * @param[in]  data_id_str      feature label + underscore + feature number
 * @param[out] value            maximum value of sensor's feature
 *
 * @return status code
 */
extern te_errno tapi_cfg_sensor_get_max(const char *ta,
                                        const char *sensor_name,
                                        const char *data_id_str,
                                        double *value);

/**
 * Gets threshold value of specified sensor data (feature) on a test agent.
 *
 * @param[in]  ta               test agent
 * @param[in]  sensor_name      sensor name
 * @param[in]  data_id_str      feature label + underscore + feature number
 * @param[in]  threshold_id     threshold type ("low"/"high"/"crit")
 * @param[out] value            threshold value
 *
 * @return status code
 */
extern te_errno tapi_cfg_sensor_get_threshold(const char *ta,
                                              const char *sensor_name,
                                              const char *data_id_str,
                                              const char *threshold_id,
                                              double *value);

/**@} <!-- END tapi_conf_sensor --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_SENSOR_H__ */
