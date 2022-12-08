/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Numeric operations.
 *
 * @defgroup te_tools_te_numeric Numeric operations.
 * @ingroup te_tools
 * @{
 *
 * Numeric operations.
 *
 */

#ifndef __TE_NUMERIC_H__
#define __TE_NUMERIC_H__

#include <inttypes.h>

#include "te_defs.h"
#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Safely converts a double @p val to an integer.
 *
 * The value is truncated towards zero, as with the regular conversion,
 * but non-finite values and overflows are taken care of.
 *
 * @param[in]  val    input value
 * @param[in]  lim    limit value:
 *                    the absolute of @p val shall not be greater than @p lim
 * @param[out] result output value
 *
 * @return status code
 * @retval 0          Conversion is successful.
 * @retval TE_ERANGE  The absolute value of @p val is greater than @p lim
 * @retval TE_EDOM    @p val is non-zero and is less than @c 1.0.
 * @retval TE_EINVAL  @p val is not finite.
 */
extern te_errno te_double2int_safe(long double val, intmax_t lim,
                                   intmax_t *result);

/*
 * Same as te_double2int_safe(), but the result is unsigned.
 *
 * All negative non-zero values of @p val are rejected by the function.
 *
 * @param[in]  val    input value
 * @param[in]  max    maximum value: @p val shall not be greater than @p max
 * @param[out] result output value
 *
 * @return status code
 * @retval 0          Conversion is successful.
 * @retval TE_ERANGE  @p val is greater than @p lim
 * @retval TE_EDOM    @p val is non-zero and is less than @c 1.0.
 * @retval TE_EINVAL  @p val is not finite.
 */
extern te_errno te_double2uint_safe(long double val, uintmax_t max,
                                    uintmax_t *result);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* ndef __TE_NUMERIC_H__ */
/**@} <!-- END te_tools_te_numeric --> */
