/** @file
 * @brief Utility functions for YAML processing
 *
 * @defgroup te_tools_te_yaml YAML processing
 * @ingroup te_tools
 * @{
 *
 * YAML processing
 *
 * Copyright (C) 2020-2022 OKTET Labs. All rights reserved.
 *
 */

#ifndef __TE_YAML_H__
#define __TE_YAML_H__

#include <strings.h>
#include <yaml.h>

#include "te_defs.h"

#ifdef _cplusplus
extern "C" {
#endif

/** Extract the value if it's a scalar node */
static inline const char *
te_yaml_scalar_value(const yaml_node_t *node)
{
    if (node != NULL && node->type == YAML_SCALAR_NODE)
        return (const char *)node->data.scalar.value;
    return NULL;
}

/** Check if a value is defined and is a possible YAML value for "true" */
static inline te_bool
te_yaml_value_is_true(const char *value)
{
    return value != NULL &&
           strcasecmp(value, "false") != 0 &&
           strcasecmp(value, "off") != 0 &&
           strcasecmp(value, "no") != 0 &&
           strcasecmp(value, "0") != 0 &&
           strcasecmp(value, "") != 0;
}

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_YAML_H__ */
/**@} <!-- END te_tools_te_yaml --> */
