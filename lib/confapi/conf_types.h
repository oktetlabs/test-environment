/** @file
 * @brief Configurator
 *
 * Configurator primary types definitions
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
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

/** Number of configurator primary types */
#define CFG_PRIMARY_TYPES_NUM   4

/* Forward */
struct cfg_msg;

/** Object instance value */
typedef union cfg_inst_val {
        struct sockaddr *val_addr;  /**< sockaddr value */
        int              val_int;   /**< int value */
        char            *val_str;   /**< string value */
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
} cfg_primary_type;

/** Primary types array */
extern cfg_primary_type cfg_types[CFG_PRIMARY_TYPES_NUM];

#ifdef __cplusplus
}
#endif
#endif /* __TE_CONF_TYPES_H__ */
