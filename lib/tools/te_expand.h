/** @file
 * @brief Parameters expansion API
 *
 * Definitions of the C API that allows to expand parameters in a string
 *
 * Copyright (C) 2018 OKTET Labs. All rights reserved.
 *
 * @author Artem Andreev <Artem.Andreev@oktetlabs.ru>
 */

#ifndef __TE_TOOLS_EXPAND_H__
#define __TE_TOOLS_EXPAND_H__

#include "te_config.h"

#ifdef TE_EXPAND_XML
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#endif

/**
 * Expands environment variables in a string.
 * The variable names must be between ${ and }.
 * Conditional expansion is supported:
 * - ${NAME:-VALUE} is expanded into VALUE if NAME variable is not set,
 * otherwise to its value
 * - ${NAME:+VALUE} is expanded into VALUE if NAME variable is set,
 * otherwise to an empty string.
 *
 * @note The length of anything between ${...} must be less than 128
 *
 * @param src     Source string
 * @param posargs Positional parameters (expandable via ${[0-9]})
 *                (may be NULL)
 * @param retval  Resulting string (OUT)
 *
 * @return 0 if success, an error code otherwise
 * @retval ENOBUFS The variable name is longer than 128
 * @retval EINVAL Unmatched ${ found
 */
extern int te_expand_env_vars(const char *src, char **posargs,
                              char **retval);


#ifdef TE_EXPAND_XML
/**
 * A wrapper around xmlGetProp that expands environment variable
 * references.
 *
 * @param node    XML node
 * @param name    XML attribute name
 *
 * @return The expanded attribute value or NULL if no attribute
 * or an error occured while expanding.
 *
 * @sa cfg_expand_env_vars
 */
static inline char *
xmlGetProp_exp(xmlNodePtr node, const xmlChar *name)
{
    xmlChar *value = xmlGetProp(node, name);

    if (value)
    {
        char *result = NULL;
        int   rc;

        rc = te_expand_env_vars((const char *)value, NULL, &result);
        if (rc == 0)
        {
            xmlFree(value);
            value = (xmlChar *)result;
        }
        else
        {
            ERROR("Error substituting variables in %s '%s': %s",
                  name, value, strerror(rc));
            xmlFree(value);
            value = NULL;
        }
    }
    return (char *)value;
}
#endif /* TE_EXPAND_XML */



#endif /* !__TE_TOOLS_EXPAND_H__ */
