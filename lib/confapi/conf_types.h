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

/** Maximum length of the instance in the message */
#define CFG_MAX_INST_VALUE      RCF_MAX_VAL

/** CVT_INTEGER = CVT_INT32 fallback for test suites */
#define CVT_INTEGER CVT_INT32

/** Constants for primary types */
typedef enum {
    CVT_INT32 = 0,       /**< Value of the type 'int32_t' */
    CVT_UINT64,      /**< Value of the type 'uint64_t' */
    CVT_STRING,      /**< Value of the type 'char *' */
    CVT_ADDRESS,     /**< Value of the type 'sockaddr *' */
    CVT_NONE,        /**< The object instance has no value */
    CVT_UNSPECIFIED  /**< The type is unknown. It'd be the last enum member */
} cfg_val_type;

/** Number of configurator primary types */
#define CFG_PRIMARY_TYPES_NUM   CVT_UNSPECIFIED

/* Forward */
struct cfg_msg;

/** Object instance value */
typedef union cfg_inst_val {
        struct sockaddr *val_addr;    /**< sockaddr value */
        int32_t          val_int32;   /**< int32_t value */
        uint64_t         val_uint64;  /**< uint64_t value */
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
    int (* str2val)(char *val_str, cfg_inst_val *val);


    /** Convert value from cfg_inst_val to string representation */
    int (* val2str)(cfg_inst_val val, char **val_str);

    /** Put default value of the type to cfg_inst_val */
    int (* def_val)(cfg_inst_val *val);

    /** Free memory allocated for the value (dummy for int type) */
    void (* free)(cfg_inst_val val);

    /** Copy the value (allocating memory, if necessary). */
    int (* copy)(cfg_inst_val val, cfg_inst_val *var);

    /** Obtain value from the message */
    int (* get_from_msg)(struct cfg_msg *msg, cfg_inst_val *val);

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
