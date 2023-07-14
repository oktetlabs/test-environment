/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Configurator
 *
 * Configurator primary types definitions
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_CONF_TYPES_H__
#define __TE_CONF_TYPES_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "te_stdint.h"
#include "te_defs.h"
#include "te_errno.h"
#include "te_enum.h"


/** Maximum length of the instance in the message */
#define CFG_MAX_INST_VALUE      RCF_MAX_VAL

/** CVT_INTEGER = CVT_INT32 fallback for test suites */
#define CVT_INTEGER CVT_INT32

/** Constants for primary types */
typedef enum {
    /**
     * The object instance has no value. It is a default type for
     * Configurator. It is set if the type is missed in a configuration yaml
     * file. Therefore it should be set as a zero.
     */
    CVT_NONE = 0,
    CVT_BOOL,        /**< Value of the type 'te_bool' */
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

/** Number of configurator primary types */
#define CFG_PRIMARY_TYPES_NUM   CVT_UNSPECIFIED

/** Array to convert cfg_val_type to string and vice versa using te_enum.h */
extern const te_enum_map cfg_cvt_mapping[];

/* Forward */
struct cfg_msg;

/** Object instance value */
typedef union cfg_inst_val {
        struct sockaddr *val_addr;    /**< sockaddr value */
        te_bool          val_bool;    /**< te_bool value */
        int8_t           val_int8;    /**< int8_t value */
        uint8_t          val_uint8;   /**< uint8_t value */
        int16_t          val_int16;   /**< int16_t value */
        uint16_t         val_uint16;  /**< uint16_t value */
        int32_t          val_int32;   /**< int32_t value */
        uint32_t         val_uint32;  /**< uint32_t value */
        int64_t          val_int64;   /**< int64_t value */
        uint64_t         val_uint64;  /**< uint64_t value */
        double           val_double;  /**< double value */
        char            *val_str;     /**< string value */
} cfg_inst_val;

/** Primary type structure */
typedef struct cfg_primary_type {
    /*
     * Conversion functions return errno from te_errno.h
     * Memory for complex types (struct sockaddr and char *) as well
     * as for value in string representation is allocated by these
     * functions using malloc().
     * Functions return void or status code (see te_errno.h).
     */

    /** Convert value from string representation to cfg_inst_val */
    te_errno (* str2val)(char *val_str, cfg_inst_val *val);


    /** Convert value from cfg_inst_val to string representation */
    te_errno (* val2str)(cfg_inst_val val, char **val_str);

    /** Put default value of the type to cfg_inst_val */
    te_errno (* def_val)(cfg_inst_val *val);

    /** Free memory allocated for the value (dummy for integer types) */
    void (* free)(cfg_inst_val val);

    /** Copy the value (allocating memory, if necessary). */
    te_errno (* copy)(cfg_inst_val val, cfg_inst_val *var);

    /** Obtain value from the message */
    te_errno (* get_from_msg)(struct cfg_msg *msg, cfg_inst_val *val);

    /**
     * Put the value to the message; the message length should be
     * updated.
     */
    void (* put_to_msg)(cfg_inst_val val, struct cfg_msg *msg);

    /** Compare two values */
    te_bool (* is_equal)(cfg_inst_val val1, cfg_inst_val val2);

    /** Get the size of given value */
    size_t (* value_size)(cfg_inst_val val);
} cfg_primary_type;

/** Primary types array */
extern cfg_primary_type cfg_types[CFG_PRIMARY_TYPES_NUM];

#ifdef __cplusplus
}
#endif
#endif /* __TE_CONF_TYPES_H__ */
