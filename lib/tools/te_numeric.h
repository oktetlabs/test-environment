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
#include "te_enum.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name Scalar type dynamic cast API.
 * @{
 */
/**
 * List of scalar types and their properties.
 *
 * Each member of the list specifies one of the scalar types and describes its
 * properties:
 *  1. name of the scalar type: named constants are declared in the enum of
 *     scalar types;
 *  2. corresponding type in C language;
 *  3. minimum value of the scalar type (zero for unsigned types);
 *  4. maximum value of the scalar type.
 *
 * The properties are processed by a user-defined @p HANDLER_ which normally
 * should be a local macro accepting at least four arguments, e.g.:
 * @code
 * #define PRINT(scalar_type_, c_type_, min_, max_) \
 *     printf("- %s %jd %ju", #c_type_, (min_), (max_));
 * TE_SCALAR_TYPE_LIST(PRINT)
 * #undef PRINT
 * @endcode
 *
 * Refer https://en.wikipedia.org/wiki/X_macro for a description of the trick.
 *
 * When adding new types to the list two constraints must be satisfied for
 * te_scalar_type_fit_size() to work properly:
 *  - fixed-size types must precede semantic types;
 *  - fixed-size types must be ordered by their size.
 *
 * @param HANDLER_ Name of the user-defined handler for members of the list
 */
#define TE_SCALAR_TYPE_LIST(HANDLER_) \
    /* Types of fixed size below */                                     \
    HANDLER_(TE_SCALAR_TYPE_INT8_T, int8_t, INT8_MIN, INT8_MAX)         \
    HANDLER_(TE_SCALAR_TYPE_UINT8_T, uint8_t, 0, UINT8_MAX)             \
    HANDLER_(TE_SCALAR_TYPE_INT16_T, int16_t, INT16_MIN, INT16_MAX)     \
    HANDLER_(TE_SCALAR_TYPE_UINT16_T, uint16_t, 0, UINT16_MAX)          \
    HANDLER_(TE_SCALAR_TYPE_INT32_T, int32_t, INT32_MIN, INT32_MAX)     \
    HANDLER_(TE_SCALAR_TYPE_UINT32_T, uint32_t, 0, UINT32_MAX)          \
    HANDLER_(TE_SCALAR_TYPE_INT64_T, int64_t, INT64_MIN, INT64_MAX)     \
    HANDLER_(TE_SCALAR_TYPE_UINT64_T, uint64_t, 0, UINT64_MAX)          \
    /* Types of non-fixed size below */                                 \
    HANDLER_(TE_SCALAR_TYPE_SHORT, short, SHRT_MIN, SHRT_MAX)           \
    HANDLER_(TE_SCALAR_TYPE_USHORT, unsigned short, 0, USHRT_MAX)       \
    HANDLER_(TE_SCALAR_TYPE_INT, int, INT_MIN, INT_MAX)                 \
    HANDLER_(TE_SCALAR_TYPE_UINT, unsigned int, 0, UINT_MAX)            \
    HANDLER_(TE_SCALAR_TYPE_LONG, long, LONG_MIN, LONG_MAX)             \
    HANDLER_(TE_SCALAR_TYPE_ULONG, unsigned long, 0, ULONG_MAX)         \
    HANDLER_(TE_SCALAR_TYPE_SIZE_T, size_t, 0, SIZE_MAX)                \
    HANDLER_(TE_SCALAR_TYPE_INTPTR_T, intptr_t, INTPTR_MIN, INTPTR_MAX) \
    HANDLER_(TE_SCALAR_TYPE_UINTPTR_T, uintptr_t, 0, UINTPTR_MAX)       \
    HANDLER_(TE_SCALAR_TYPE_INTMAX_T, intmax_t, INTMAX_MIN, INTMAX_MAX) \
    HANDLER_(TE_SCALAR_TYPE_UINTMAX_T, uintmax_t, 0, UINTMAX_MAX)       \
    HANDLER_(TE_SCALAR_TYPE_TE_BOOL, te_bool, FALSE, TRUE)

/** Scalar types enumeration */
typedef enum te_scalar_type {
#define MEMBER(scalar_type_, ...) scalar_type_,
    TE_SCALAR_TYPE_LIST(MEMBER)
#undef MEMBER
} te_scalar_type;

/** Maps te_scalar_type into a string name of corresponding C type */
extern const te_enum_map te_scalar_type_names[];

/**
 * Get the maximum value of the given scalar type.
 *
 * @param type  Scalar type.
 *
 * @return Maximum value of the scalar @p type.
 */
extern uintmax_t te_scalar_type_max(te_scalar_type type);

/**
 * Get the minimum value of the given scalar type.
 *
 * @param type  Scalar type.
 *
 * @return Minimum value of the scalar @p type.
 */
extern intmax_t te_scalar_type_min(te_scalar_type type);

/**
 * Return the size of storage for values of given type (akin to `sizeof`).
 *
 * @param type  Scalar type.
 *
 * @return Size of type.
 */
extern size_t te_scalar_type_sizeof(te_scalar_type type);

/**
 * Checks whether a type is signed or unsigned.
 *
 * @param type  Scalar type.
 *
 * @return TRUE if the @p type is signed.
 */
extern te_bool te_scalar_type_is_signed(te_scalar_type type);

/**
 * Return a type spec than can hold values of given size.
 *
 * @param is_signed Sign of value type.
 * @param size      Size of value type (the same as size of some scalar C type).
 * @param type[out] Location for the scalar type.
 *
 * @return Status code.
 * @retval TE_ERANGE    There is no scalar type of the requested size and sign.
 */
extern te_errno te_scalar_type_fit_size(te_bool is_signed, size_t size,
                                        te_scalar_type *type);

/**
 * Cast the source value to the destination type.
 *
 * If the @p src value cannot be represented in the @p dst_type exactly an error
 * is returned. However, if the @p dst is not @c NULL, it is always overwritten
 * by the casted @p src even if @p src value does not fit the range of
 * @p dst_type.
 *
 * @param       src_type Type of source value.
 * @param       src      Pointer to memory holding the source value.
 * @param       dst_type Type of destination value.
 * @param[out]  dst      Location for the casted value (may be @c NULL).
 *
 * @return Status code.
 * @retval TE_EOVERFLOW Value does not fit the destination type.
 */
extern te_errno te_scalar_dynamic_cast(te_scalar_type src_type, const void *src,
                                       te_scalar_type dst_type, void *dst);
/**@}*/

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
