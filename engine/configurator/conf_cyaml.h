/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief YAML configuration types and schema
 *
 * API definitions.
 *
 * Copyright (C) 2018-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_CONF_CYAML_H__
#define __TE_CONF_CYAML_H__

#include "conf_api.h"
#include "conf_types.h"
#include "logger_api.h"

/*
 ******************************************************************************
 * C data structure for storing a backup.
 *
 * This is what we want to load the YAML into.
 ******************************************************************************
 */

/** Structure for storing depends entry */
typedef struct depends_entry {
    /** @c oid is taken from YAML key @c oid in depends entry */
    char *oid;
    /**
     * @c scope is taken from YAML key @c scope in depends entry
     * optional; 0 is default value means instance
     */
    uint8_t scope;
} depends_entry;

/**
 * Structure for storing object information. The name for the field coincides
 * with the corresponding YAML key except @ def_val for @c default,
 * @c volat for @c volatile and @ no_parent_dep for @c parent_dep. In the last
 * case there is also a logical negation.
 */
typedef struct object_type {
    /**
     * @c d is taken from YAML key @c d in object.
     * @c d means description and will be ignored.
     * CYAML doesn'know about this field.
     */
    char *d;

    /** @c oid is taken from YAML key @c oid in object */
    char *oid;
    /** @ access is taken from YAML key @c access in object */
    uint8_t access;
    /**
     * @c type is taken from YAML key @c type in object
     * optional; 0 is default value means @c NONE type
     */
    uint8_t type;
    /**
     * @c unit is taken from YAML key @c unit in object
     * optional; 0 is default value
     */
    te_bool unit;
    /**
     * @c def_val is taken from YAML key @c default in object
     * optional; NULL is default value
     */
    char *def_val;
    /**
     * @c volat is taken from YAML key @c volatile in object
     * optional; 0 is default value
     */
    te_bool volat;
    /**
     * @c substitution is taken from YAML key @c substitution in object
     * optional; 0 is default value
     */
    te_bool substitution;
    /**
     * @ no_parent_dep is taken from YAML key @c no_parent in object
     * There is a logical negation.
     * optional; 0 is default value
     */
    te_bool no_parent_dep;

    /** @c depends array is taken from YAML key @c depends in object */
    depends_entry *depends;
    /** length of @c depends array */
    unsigned int depends_count;
} object_type;

/** Structure for storing instance */
typedef struct instance_type {
    /**
     * @c if_cond is taken from YAML key @c if in instance.
     * The oid will be ignored iff the resolved value would be @c FALSE.
     * CYAML doesn'know about this field.
     */
    char *if_cond; /* cyaml doesn't know about this field */

    /** @c oid is taken from YAML key @c oid in instance */
    char *oid;
    /**
     * @c value is taken from YAML key @c value in instance
     * optional; NULL is default value
     */
    char *value;
} instance_type;

/**
 * Structure for storing backup entry
 * The only one of pointers is not @c NULL
 */
typedef struct backup_entry {
    /** object */
    object_type *object;
    /** instance */
    instance_type *instance;
} backup_entry;

/** Structure for storing the whole backup. */
typedef struct backup_seq {
    /** @c entries array is taken from the root YAML sequence */
    backup_entry *entries;
    /** length of @c entries array */
    unsigned int entries_count;
} backup_seq;

/**
 * Structure for storing @c cond node.
 * cyaml doesn't know about this structure.
 */
typedef struct cond_entry {
    /**
     * @c if_cond is taken from YAML key @c if in @c cond node
     * If the resolved value is TRUE then CS should consider @c then node.
     * Otherwise @c else (if it exists).
     */
    char *if_cond;
    /** @c then_cond is taken from YAML key @c then in @c cond node */
    struct history_seq *then_cond;
    /** @c else_cond is taken from YAML key @c else in @c cond node */
    struct history_seq *else_cond;
} cond_entry;

/* Structure for storing entry of history.
 * The only one pointer in not equal to @c NULL. */
typedef struct history_entry {
    /**
     * @c comment is taken from YAML key @c comment in root sequence.
     * It will be ignored.
     * CYAML doesn'know about this field.
     */
    char *comment;

    /**
     * @c incl array is taken from YAML key @c include in root sequence.
     * It consists of the names of configuration files that needs to include.
     * CYAML doesn't know about this field.
     */
    char **incl;
    /** length of @c incl array */
    unsigned int incl_count;

    /**
     * @c cond is taken from YAML key @c cond.
     * If resolved @c if is true then CS should consider @c then.
     * Otherwise @c else (if it exists).
     * CYAML doesn't know about this field.
     */
    cond_entry *cond;

    /** @c reg array is taken from YAML key @c register in root sequence */
    object_type *reg;
    /** length of @c reg array */
    unsigned int reg_count;

    /** @c unreg array is taken from YAML key @c unregister in root sequence */
    object_type *unreg;
    /** length of @c unreg array */
    unsigned int unreg_count;

    /** @c add array is taken from YAML key @c add in root sequence */
    instance_type *add;
    /** length of @c add array */
    unsigned int add_count;

    /** @c get array is taken from YAML key @c get in root sequence */
    instance_type *get;
    /** length of @c get array */
    unsigned int get_count;

    /** @c delete array is taken from YAML key @c delete in root sequence */
    instance_type *delete;
    /** length of @c delete array */
    unsigned int delete_count;

    /** @c copy array is taken from YAML key @c copy in root sequence */
    instance_type *copy;
    /** length of @c copy array */
    unsigned int copy_count;

    /** @c set array is taken from YAML key @c set in root sequence */
    instance_type *set;
    /** length of @c set array */
    unsigned int set_count;

    /** name of reboote Test Agent */
    char *reboot_ta; /* optional */
} history_entry;

/** Structure for storing the whole history */
typedef struct history_seq {
    /** @c entries array is taken from the root YAML sequence */
    history_entry *entries;
    /** length of @c entries array */
    unsigned int entries_count;
} history_seq;

/**
 * Free object.
 *
 * @param obj          object
 */
extern void cfg_yaml_free_obj(object_type *obj);

/**
 * Free instance.
 *
 * @param inst         instance
 */
extern void cfg_yaml_free_inst(instance_type *inst);

/**
 * Free sequense of history entries.
 *
 * @param hist_seq     sequence of history entries
 */
extern void cfg_yaml_free_hist_seq(history_seq *hist);

/**
 * Free sequense of backup entries.
 *
 * @param backup_seq     sequence of backup entries
 */
extern void cfg_yaml_free_backup_seq(backup_seq *backup);

/**
 * Save backup structure to yaml file.
 *
 * @param backup_seq     sequence of backup entries
 */
extern te_errno cfg_yaml_save_backup_file(const char *filename,
                                          backup_seq *backup);

/**
 * Parse backup YAML file to backup structure.
 *
 * @param backup_seq     sequence of backup entries
 */
extern te_errno cfg_yaml_parse_backup_file(const char *filename,
                                          backup_seq **backup);

/**
 * Save history structure known by CYAML to yaml file.
 *
 * @param hist_seq     sequence of history entries
 */
extern te_errno cfg_yaml_save_history_file(const char *filename,
                                           history_seq *history);

#endif /* __TE_CONF_CYAML_H__ */
