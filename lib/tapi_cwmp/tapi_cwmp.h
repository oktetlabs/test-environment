/** @file
 * @brief Test API for CWMP usage
 *
 * Declarations of Test API to CWMP (TR-069)
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 */

#ifndef __TE_TAPI_CWMP_H__
#define __TE_TAPI_CWMP_H__

#include "te_cwmp.h"
#include "tapi_test.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The list of values allowed for parameter of type @ref te_cwmp_datamodel_t.
 */
#define TAPI_CWMP_DM_MAPPING_LIST   \
    { "tr098",  TE_CWMP_DM_TR098 }, \
    { "tr181",  TE_CWMP_DM_TR181 }

/**
 * The macro to return parameters of type @ref te_cwmp_datamodel_t
 *
 * @param var_name_  Variable whose name is the same as the name of
 *                   parameter we get the value
 * @return @ref te_cwmp_datamodel_t value
 */
#define TEST_CWMP_DM_PARAM(var_name_) \
    TEST_ENUM_PARAM(var_name_, TAPI_CWMP_DM_MAPPING_LIST)

/**
 * The macro to get parameters of type @ref te_cwmp_datamodel_t
 *
 * @param var_name_     Variable whose name is the same as the name of
 *                      parameter we get the value
 */
#define TEST_GET_CWMP_DM_PARAM(var_name_) \
    (var_name_) = TEST_CWMP_DM_PARAM(var_name_)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_TAPI_CWMP_H__ */
