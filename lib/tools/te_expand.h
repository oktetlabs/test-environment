/** @file
 * @brief Parameters expansion API
 *
 * Definitions of the C API that allows to expand parameters in a string
 *
 * Copyright (C) 2018-2022 OKTET Labs. All rights reserved.
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

#include "te_kvpair.h"

/** Maximal length of parameter name */
#define TE_EXPAND_PARAM_NAME_LEN 128

/**
 * A function type for getting value by name from given data.
 */
typedef const char *(*te_param_value_getter)(const char *, const void *);

/**
 * Expands parameters in a string.
 * Parameter values will be obtained with @p get_param_value callback.
 * The parameter names must be between ${ and }.
 * Conditional expansion is supported:
 * - ${NAME:-VALUE} is expanded into VALUE if NAME variable is not set,
 * otherwise to its value
 * - ${NAME:+VALUE} is expanded into VALUE if NAME variable is set,
 * otherwise to an empty string.
 *
 * @note The length of anything between ${...} must be less than
 *       @c TE_EXPAND_PARAM_NAME_LEN
 *
 * @param src               Source string
 * @param posargs           Positional parameters (expandable via ${[0-9]})
 *                          (may be NULL)
 * @param get_param_value   Function to get param value by name
 * @param params_ctx        Context for parameters
 * @param retval            Resulting string (OUT)
 *
 * @return 0 if success, an error code otherwise
 * @retval ENOBUFS The variable name is longer than @c TE_EXPAND_PARAM_NAME_LEN
 * @retval EINVAL Unmatched ${ found
 */
extern int te_expand_parameters(const char *src, const char **posargs,
                                te_param_value_getter get_param_value,
                                const void *params_ctx, char **retval);

/**
 * Expands environment variables in a string.
 * The variable names must be between ${ and }.
 * Conditional expansion is supported:
 * - ${NAME:-VALUE} is expanded into VALUE if NAME variable is not set,
 * otherwise to its value
 * - ${NAME:+VALUE} is expanded into VALUE if NAME variable is set,
 * otherwise to an empty string.
 *
 * @note The length of anything between ${...} must be less than
 *       @c TE_EXPAND_PARAM_NAME_LEN
 *
 * @param src     Source string
 * @param posargs Positional parameters (expandable via ${[0-9]})
 *                (may be NULL)
 * @param retval  Resulting string (OUT)
 *
 * @return 0 if success, an error code otherwise
 * @retval ENOBUFS The variable name is longer than @c TE_EXPAND_PARAM_NAME_LEN
 * @retval EINVAL Unmatched ${ found
 */
extern int te_expand_env_vars(const char *src, const char **posargs,
                              char **retval);

/**
 * Expands key-value pairs in a string.
 * The variable names must be between ${ and }.
 * Conditional expansion is supported:
 * - ${NAME:-VALUE} is expanded into VALUE if NAME variable is not found,
 * otherwise to its value
 * - ${NAME:+VALUE} is expanded into VALUE if NAME variable is found,
 * otherwise to an empty string.
 *
 * @note The length of anything between ${...} must be less than
 *       @c TE_EXPAND_PARAM_NAME_LEN
 *
 * @param src     Source string
 * @param posargs Positional parameters (expandable via ${[0-9]})
 *                (may be NULL)
 * @param kvpairs Head of queue of key-value pairs
 * @param retval  Resulting string (OUT)
 *
 * @return 0 if success, an error code otherwise
 * @retval ENOBUFS The variable name is longer than @c TE_EXPAND_PARAM_NAME_LEN
 * @retval EINVAL Unmatched ${ found
 */
extern int te_expand_kvpairs(const char *src, const char **posargs,
                             const te_kvpair_h *kvpairs, char **retval);


#ifdef TE_EXPAND_XML
/**
 * A wrapper around xmlGetProp that expands custom parameters from
 * list of key-value pairs if given. Otherwise it expands environment variable
 * references.
 *
 * @param node          XML node
 * @param name          XML attribute name
 * @param expand_vars   List of key-value pairs for expansion in file,
 *                      @c NULL if environment variables are used for
 *                      substitutions
 *
 * @return The expanded attribute value or NULL if no attribute
 * or an error occured while expanding.
 *
 * @sa cfg_expand_env_vars
 */
static inline char *
xmlGetProp_exp_vars_or_env(xmlNodePtr node, const xmlChar *name,
                           const te_kvpair_h *kvpairs)
{
    xmlChar *value = xmlGetProp(node, name);

    if (value)
    {
        char *result = NULL;
        int   rc;

        if (kvpairs == NULL)
            rc = te_expand_env_vars((const char *)value, NULL, &result);
        else
            rc = te_expand_kvpairs((const char *)value, NULL, kvpairs, &result);

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

/**
 * Case of xmlGetProp_exp_vars_or_env that expands only environment
 * variables reference.
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
    return xmlGetProp_exp_vars_or_env(node, name, NULL);
}
#endif /* TE_EXPAND_XML */



#endif /* !__TE_TOOLS_EXPAND_H__ */
