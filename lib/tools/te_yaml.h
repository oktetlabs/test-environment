/** @file
 * @brief Utility functions for YAML processing
 *
 * @defgroup te_tools_te_yaml YAML processing
 * @ingroup te_tools
 * @{
 *
 * YAML processing
 *
 * Copyright (C) 2020 OKTET Labs. All rights reserved.
 *
 * @author Viacheslav Galaktionov <Viacheslav.Galaktionov@oktetlabs.ru>
 */

#ifndef __TE_YAML_H__
#define __TE_YAML_H__

#include <yaml.h>

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

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_YAML_H__ */
/**@} <!-- END te_tools_te_yaml --> */
