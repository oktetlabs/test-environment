/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 OKTET Ltd. All rights reserved. */
/** @file
 * @brief Configurator primary value types
 *
 * The set of types an object instance value can have.  Shared by the
 * engine and the Test Agents so that both sides describe a value with
 * the same vocabulary.
 */

#ifndef __TE_CONF_VAL_TYPE_H__
#define __TE_CONF_VAL_TYPE_H__

#include "te_enum.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Constants for primary types */
typedef enum {
    /**
     * The object instance has no value. It is a default type for
     * Configurator. It is set if the type is missed in a configuration yaml
     * file. Therefore it should be set as a zero.
     */
    CVT_NONE = 0,
    CVT_BOOL,        /**< Value of the type 'bool' */
    CVT_INT8,        /**< Value of the type 'int8_t' */
    CVT_UINT8,       /**< Value of the type 'uint8_t' */
    CVT_INT16,       /**< Value of the type 'int16_t' */
    CVT_UINT16,      /**< Value of the type 'uint16_t' */
    CVT_INT32,       /**< Value of the type 'int32_t' */
    CVT_UINT32,      /**< Value of the type 'uint32_t' */
    CVT_INT64,       /**< Value of the type 'int64_t' */
    CVT_UINT64,      /**< Value of the type 'uint64_t' */
    CVT_STRING,      /**< Value of the type 'char *' */
    CVT_ADDRESS,     /**< Value of the type 'sockaddr *' */
    CVT_DOUBLE,      /**< Value of the type 'double' */
    CVT_UNSPECIFIED  /**< The type is unknown. It'd be the last enum member */
} cfg_val_type;

/** CVT_INTEGER = CVT_INT32 fallback for test suites */
#define CVT_INTEGER CVT_INT32

/** Number of configurator primary types */
#define CFG_PRIMARY_TYPES_NUM   CVT_UNSPECIFIED

/** Array to convert cfg_val_type to string and vice versa using te_enum.h */
extern const te_enum_map cfg_cvt_mapping[];

#ifdef __cplusplus
}
#endif
#endif /* __TE_CONF_VAL_TYPE_H__ */
