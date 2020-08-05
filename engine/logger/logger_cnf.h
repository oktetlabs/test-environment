/** @file
 * @brief TE project. Logger subsystem.
 *
 * Internal utilities for Logger configuration file parsing.
 *
 * Copyright (C) 2020 OKTET Labs. All rights reserved.
 *
 * @author Viacheslav Galaktionov <Viacheslav.Galaktionov@oktetlabs.ru>
 */

#ifndef __TE_LOGGER_CNF_H__
#define __TE_LOGGER_CNF_H__

#include <yaml.h>

#ifdef _cplusplus
extern "C" {
#endif

/** Extract the value if it's a scalar node */
static inline const char *
get_scalar_value(const yaml_node_t *node)
{
    if (node != NULL && node->type == YAML_SCALAR_NODE)
        return (const char *)node->data.scalar.value;
    return NULL;
}

/** Configuration file format */
typedef enum cfg_file_type {
    CFG_TYPE_ERROR,
    CFG_TYPE_EMPTY,
    CFG_TYPE_XML,
    CFG_TYPE_YAML,
    CFG_TYPE_OTHER,
} cfg_file_type;

/**
 * Determine config file format.
 *
 * @param filename      path to the config file
 */
extern cfg_file_type get_cfg_file_type(const char *filename);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_LOGGER_CNF_H__ */
