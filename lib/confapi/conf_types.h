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
#include "conf_val_type.h"


/** Maximum length of the instance in the message */
#define CFG_MAX_INST_VALUE      RCF_MAX_VAL

/* Forward */
struct cfg_msg;

/** Object instance value */
typedef union cfg_inst_val {
        struct sockaddr *val_addr;    /**< sockaddr value */
        bool             val_bool;    /**< bool value */
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
     * functions using TE_ALLOC().
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
    bool(* is_equal)(cfg_inst_val val1, cfg_inst_val val2);

    /** Get the size of given value */
    size_t (* value_size)(cfg_inst_val val);
} cfg_primary_type;

/** Primary types array */
extern cfg_primary_type cfg_types[CFG_PRIMARY_TYPES_NUM];

#ifdef __cplusplus
}
#endif
#endif /* __TE_CONF_TYPES_H__ */
