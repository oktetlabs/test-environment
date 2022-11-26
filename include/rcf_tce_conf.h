/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief TE RCF Engine - TCE configuration
 *
 * Internal definitions to process the TCE configuration.
 *
 * The TCE information is generated on TA agents within the same directories
 * the TA agents have been built. To fetch this information the RCF controller
 * has to be provided with paths to this directories and to the directory,
 * the TCE workspace directory, to save this information into.
 *
 * The TCE configuration provides:
 * - the information local for the TE engine,
 * - the information for types of the TA agents.
 * The later provides:
 * - the directory where TE engine executables are located,
 * - the directory where to save the TCE information.
 * The former provides:
 * - the name of a TA type,
 * - the base of build directories of TA components,
 * - the information on TA components of the TA type, names and build
 *   directories.
 */

#ifndef __TE_RCF_TCE_CONF_H__
#define __TE_RCF_TCE_CONF_H__

#include "te_queue.h"

/** The TCE configuration of a TA component. */
typedef struct rcf_tce_comp_conf {
    SLIST_ENTRY(rcf_tce_comp_conf) next;

    char *name;  /**< The name of the TA component. */
    char *build; /**< The build directory. */
} rcf_tce_comp_conf_t;

/** The TCE configuration of a TA type. */
typedef struct rcf_tce_type_conf {
    SLIST_ENTRY(rcf_tce_type_conf) next;

    char *name; /**< The name of the TA type. */
    char *base; /**< The base of build directories. */

    /** TCE configurations of TA components. */
    SLIST_HEAD(, rcf_tce_comp_conf) comp;
} rcf_tce_type_conf_t;

/** The TCE configuration of the TE engine. */
typedef struct rcf_tce_local_conf {
    char *tebin; /**< The TE bin directory. */
    char *tcews; /**< The TCE workspace directory. */
} rcf_tce_local_conf_t;

/** The TCE configuration. */
typedef struct rcf_tce_conf {
    /** The configuration of the TA engine. */
    rcf_tce_local_conf_t local;
    /** Configurations of TA types. */
    SLIST_HEAD(, rcf_tce_type_conf) types;
} rcf_tce_conf_t;

/**
 * Get the TCE configuration of a TA component.
 *
 * @param type The TA type configuration.
 * @param comp The configuration of the previous TA component or NULL to get
 *             the configuration of the first TA component.
 */
extern const rcf_tce_comp_conf_t *
rcf_tce_get_next_comp_conf(const rcf_tce_type_conf_t *type,
                           const rcf_tce_comp_conf_t *comp);

/**
 * Get the TCE configuration of a TA type.
 *
 * @param conf The TCE configuration.
 * @param type The TA type.
 */
extern const rcf_tce_type_conf_t *
rcf_tce_get_type_conf(const rcf_tce_conf_t *conf, const char *type);

#endif /* __TE_RCF_TCE_CONF_H__ */
